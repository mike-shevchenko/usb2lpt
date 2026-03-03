Imports System.Runtime.InteropServices
'********************************************
'* Declares InpOut32.dll entries for VB.NET *
'********************************************
'haftmann#software, 130326
'change all "inpout32.dll" to "inpoutx64.dll", and a couple of UInteger to ULong for VB 64bit!

Module InpOut32
    '*****************
    '** General I/O **
    '*****************

    ' Basic BYTE input/output
    ' Access to old-style LPT addresses &H378 (LPT1), &H278 (LPT2), and &H3BC (LPT3)
    ' are automatically redirected to PCI and PCIexpress cards.
    ' If no true parallel port available for those addresses, redirection applies
    ' to USB2LPT (native or HID) and USB->ParallelPrinter adapter
    ' You can use space-saving numerical inports like GetProcAddress(h,MAKEINTRESOURCE(1))
    ' safely, as these numbers never changed


    Declare Function Inp32 Lib "inpout32.dll" (ByVal addr As UShort) As Byte
    Declare Sub Out32 Lib "inpout32.dll" (ByVal addr As UShort, ByVal value As Byte)
    Declare Function IsInpOutDriverOpen Lib "inpout32.dll" () As Boolean

    ' DlPortIo.dll compatible entries.
    ' BYTE access is redirected too (as above), but WORD/DWORD not.
    ' Not recommended for new software.
    ' Import by number is not recommended from here and below.

    Declare Function DlPortReadPortUchar Lib "inpout32.dll" (ByVal addr As UShort) As Byte
    Declare Sub DlPortWritePortUchar Lib "inpout32.dll" (ByVal addr As UShort, ByVal value As Byte)
    Declare Function DlPortReadPortUshort Lib "inpout32.dll" (ByVal addr As UShort) As UShort
    Declare Sub DlPortWritePortUshort Lib "inpout32.dll" (ByVal addr As UShort, ByVal value As UShort)
    Declare Function DlPortReadPortUlong Lib "inpout32.dll" (ByVal addr As UShort) As UInteger
    Declare Sub DlPortWritePortUlong Lib "inpout32.dll" (ByVal addr As UShort, ByVal value As UInteger)

    Declare Function MapPhysToLin Lib "inpout32.dll" (ByVal PhysAddr As UInteger, ByVal PhysSize As UInteger, ByRef MemoryHandle As UInteger) As UInteger
    Declare Function UnmapPhysicalMemory Lib "inpout32.dll" (ByVal MemoryHandle As UInteger, ByVal LinAddr As UInteger) As Boolean
    Declare Function GetPhysLong Lib "inpout32.dll" (ByVal PhysAddr As UInteger, ByRef PhysVal As UInteger) As Boolean
    Declare Function SetPhysLong Lib "inpout32.dll" (ByVal PhysAddr As UInteger, ByVal PhysVal As UInteger) As Boolean

    ' System query and some useful file system fiddling, only useful for VB.NET 32bit executeables
    Declare Function IsXP64Bit Lib "inpout32.dll" () As Boolean
    Declare Function DisableWOW64 Lib "inpout32.dll" (ByRef oldValue As Object) As Boolean
    Declare Function RevertWOW64 Lib "inpout32.dll" (ByVal oldValue As Object) As Boolean

    '**********************
    '** LPT-specific I/O **
    '**********************

    ' Because inpout32.dll most-used purpose is to access the parallel port,
    ' routines are added 2013 to accomodate the fading but still useful port.
    ' Most important are the built-in redirection features.
    ' But it's much better do avoid direct access to ports either, and to move
    ' time-critical code to a USB (or Ethernet, etc.) microcontroller.
    ' As a good intermediate step, sufficient for a bunch of applications,
    ' my so-called USB2LPT device is able to process microcode (ucode)
    ' as a concatenation of OUT and IN instruction into one USB packet.
    ' This requires rewriting of applications due to logical restructuring
    ' but will help making any redirection much faster than ever before.
    ' (Because it's not possible to concatenate IN instructions automatically,
    '  a random application relies to get the return value immediately.)
    ' Good candidates for a rewrite are applications accessing synchron-serial
    ' connected devices, as SPI, JTAG, and IIC, mostly, chip programmers.


    ' This function is for debugging and possibly for some interactive setup.
    ' It's called by invoking "rundll32 inpout32.dll,Info"
    ' and pops up a Messagebox showing computer's hardware and redirection features
    ' concerning parallel ports. Arguments, except Wnd, are currently not used.

    Declare Ansi Sub Info Lib "inpout32.dll" (ByVal Wnd As UInteger, ByVal hInstance As UInteger, ByVal CmdLine As String, ByVal nCmdShow As Integer)
    Declare Unicode Sub InfoW Lib "inpout32.dll" (ByVal Wnd As UInteger, ByVal hInstance As UInteger, ByVal CmdLine As String, ByVal nCmdShow As Integer)

    ' This new function returns the address of a true parallel port, via PnP service.
    ' LOWORD = SPP base address, you can assume 8 contiguous addresses: 4 SPP, 4 EPP
    ' HIWORD = ECP base address, you can assume 4 contiguous addresses
    ' Returns zero when no address was found (no or no true parallel port).
    ' <n> is the zero-based(!) number of parallel port, as for
    ' \devices\parallelX (NT kernel namespace) or /dev/lpX (Linux)

    Declare Function LptGetAddr Lib "inpout32.dll" (<MarshalAs(UnmanagedType.LPArray, SizeParamIndex:=1)> ByVal result() As UInteger, ByVal n As Integer) As UInteger

    ' These functions work similar to Inp32() and Out32(), but with one difference:
    ' LptQueueIn() doesn't return a useful value!
    ' As a result, all I/O can be collected and executed in one transaction
    ' and can be much faster than single transfers, especially for USB devices.
    ' You get the result for all IN transactions on QueueFlush().
    ' These routines go to standard LPT ports if available (i.e. don't enqueue),
    ' and to USB devices, like USB2LPT, USBLotIo or USB->ParallelPrinter etc.
    ' For USB devices, no administrative privileges are necessary.
    ' For new software, programmers should use this interface.
    ' For USB->ParallelPrinter adapters, a latch and some connections are necessary!
    ' Read http://www.tu-chemnitz.de/~heha/usb2lpt/faq#DIY

    ' <n> is zero-based (LPT1 = 0), the current working maximum is 8 (for LPT9)
    ' <flags> is currently unused and should be zero
    ' LptOpen() returns hQueue needed for all other functions, zero on failure.
    ' <offset> is 0..7 for SPP and EPP port range, and 8..11 for ECP port range
    '		Example: QueueOut(hQueue,10,&H20) enqueues a switch to BiDi mode
    ' <us> is a delay in microseconds. Maximum value is 10 ms, i.e. 10000 us
    ' <buf> is a buffer for the results of all enqueued IN instructions.
    ' <buflen> is the size of that buffer, for a sanity check.
    '		Must be >= count of enqueued IN instructions.
    ' LptQueueFlush() returns the number of IN results made, and -1 on failure.
    '		For OUT-only programs, you SHOULD call QueueFlush() regularly,
    '		as this DLL doesn't create a time-out thread for you.
    ' All other functions return TRUE on success and FALSE otherwise.
    ' LptInOut() is a real workhorse, processes microcode containing IN and OUT
    '		instructions and returns the IN results in another buffer.
    '		The microcode is the same as for USB2LPT device, see below.
    '		This is an alternate access method, possibly bypassing the queue.
    '		New applications are strongly encouraged to use that function!
    '		You should not intermix LptInOut() and LptQueueXxx() functions,
    '		except you have ensured LptQueueFlush() before LptInOut().
    ' Don't use CloseHandle() to close the handle returned by LptOpen().

    Declare Function LptOpen Lib "inpout32.dll" (ByVal n As Integer, Optional ByVal flags As UInteger = 0) As UInteger
    Declare Function LptInOut Lib "inpout32.dll" (ByVal hQueue As UInteger, ByVal ucode() As Byte, ByVal ulen As Integer, <MarshalAs(UnmanagedType.LPArray, SizeParamIndex:=4)> ByVal result() As Byte, ByVal rlen As Integer) As Integer
    Declare Function LptQueueOut Lib "inpout32.dll" (ByVal hQueue As UInteger, ByVal offset As Byte, ByVal value As Byte) As Boolean
    Declare Function LptQueueIn Lib "inpout32.dll" (ByVal hQueue As UInteger, ByVal offset As Byte) As Boolean
    Declare Function LptQueueDelay Lib "inpout32.dll" (ByVal hQueue As UInteger, ByVal us As UInteger) As Boolean
    Declare Function LptQueueFlush Lib "inpout32.dll" (ByVal hQueue As UInteger, <MarshalAs(UnmanagedType.LPArray, SizeParamIndex:=2)> ByVal result() As Byte, ByVal rlen As Integer) As Integer
    Declare Function LptClose Lib "inpout32.dll" (ByVal hQueue As UInteger) As Boolean

    ' Short-form description of microcode (ucode)
    ' 1 BYTE opcode = (mostly) LPT port address offset + IN bit (bit 4), e.g.
    '   &H00 = write to data port
    '   &H11 = read from status port
    '   &H0A = write to ECP control register (ECR)
    '   &H20 = delay for n x 4 us
    ' For OUT and DELAY opcode, the data byte follows the opcode.
    ' For each IN instruction, no byte follows, but one byte space in result buffer is needed.
    ' Multiple opcodes and data bytes are contiguous and not aligned somehow.
    ' General USB implementations allow maximum ucode length (as EP0 transfer) of 4096 bytes.
    ' Further limits may apply to specific USB2LPT devices.
    ' Opcodes > &H20 are currently disallowed.

End Module

'Lightweight wrapper class for DLL transition (inside inpout32.dll, there is also a class structure)
NotInheritable Class Lpt
    Public Shared Function LptGetAddr(ByVal n As Integer) As UInteger
	Return InpOut32.LptGetAddr(Nothing, n)
    End Function
    Private hQueue As UInteger
    Public Sub New(ByVal n As Integer)
        hQueue = LptOpen(n)
    End Sub
    Public Function InOut(ByVal ucode() As Byte, ByVal result() As Byte) As Integer
        Return LptInOut(hQueue, ucode, ucode.Length, result, If(result Is Nothing, 0, result.Length))
    End Function
    Public Function QueueOut(ByVal offset As Byte, ByVal value As Byte) As Boolean
        Return LptQueueOut(hQueue, offset, value)
    End Function
    Public Function QueueIn(ByVal offset As Byte) As Boolean
        Return LptQueueIn(hQueue, offset)
    End Function
    Public Function QueueDelay(ByVal us As UInteger) As Boolean
        Return LptQueueDelay(hQueue, us)
    End Function
    Public Function QueueFlush(ByVal result() As Byte) As Integer
	Return LptQueueFlush(hQueue, result, If(result Is Nothing, 0, result.Length))
    End Function
    Protected Overrides Sub finalize()
        LptClose(hQueue)
    End Sub
End Class
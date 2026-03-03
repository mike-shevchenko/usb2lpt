/**********************************************************************************\
 * Defines InpOut32.dll and InpOutX64.dll for direct and GetProcAddress() linking *
\**********************************************************************************/
/* Change log:
 1304xx	created
+130819	Additional DlPortIo.DLL entries (stubs) for compatibility to PCS500
+130819	IO.DLL entries (stubs)
-190515	_declspec -> __declspec
*/

#pragma once

#include <windows.h>
#ifdef INPOUT_EXPORTS
# define FUNC(ret,name,args) ret WINAPI name args
  typedef struct QUEUE *HQUEUE;		// virtual class pointer
// __declspec(dllexport) would do nothing here, as exports are all done by .def file
// for undecorated names and stable export ordinals
#else
/* FUNC() will generate import function prototypes AND function pointer types
 * for the application programmer.
 * Function prototypes are useful for importing this library using inpout32.lib.
 * __declspec(dllimport) functions are called by "call [__imp__funcname]".
 * Forgetting __declspec(dllimport) lead to code-bloating stubs in your executable.
 * Drawback: The inpout32.dll is required, otherwise, the application will not start.
 *
 * Function pointer types are suffixed with "_t". These are useful for defining
 * pointers to funtions and to cast the result of GetProcAddress().
 * Drawback: This requires more coding effort for the application programmer.
 *
 * I have never used the "delayed import" feature (available since Visual C++ 2005).
 */
# define FUNC(ret,name,args) __declspec(dllimport) ret WINAPI name args;\
   typedef ret(WINAPI*name##_t)args
  DECLARE_HANDLE(HQUEUE);		// opaque to user of inpout.h
#endif

/***************\
|** Basic I/O **|
\***************/

/* Basic BYTE input/output
 * Access to old-style LPT addresses 0x378 (LPT1), 0x278 (LPT2), and 0x3BC (LPT3)
 * are automatically redirected to PCI and PCIexpress cards.
 * If no true parallel port available for those addresses, redirection applies
 * to USB2LPT (native or HID API) and USB->ParallelPrinter adapter
 * You can use space-saving numerical imports like GetProcAddress(h,MAKEINTRESOURCE(1))
 * safely, as these numbers will never change
 */
FUNC(BYTE, Inp32, (WORD addr));			// @1
FUNC(void, Out32, (WORD addr, BYTE value));	// @2
FUNC(BOOL, IsInpOutDriverOpen, (void));		// @3

/********************************\
|** DlPortIo.DLL compatibility **|
\********************************/

/* BYTE access is redirected too (as above), but WORD/DWORD not.
 * Not recommended for new software.
 * Import by number is not recommended from here and below.
 */
FUNC(BYTE, DlPortReadPortUchar,		(WORD addr));			// @4
FUNC(DWORD,DlPortReadPortUlong,		(WORD addr));			// @5
FUNC(WORD, DlPortReadPortUshort,	(WORD addr));			// @6
FUNC(void, DlPortWritePortBufferUchar,	(WORD, const UCHAR*, ULONG));	// @13
FUNC(void, DlPortWritePortBufferUlong,	(WORD, const ULONG*, ULONG));	// @14
FUNC(void, DlPortWritePortBufferUshort,	(WORD, const USHORT*, ULONG));	// @15
FUNC(void, DlPortWritePortUchar,	(WORD addr, BYTE value));	// @10
FUNC(void, DlPortWritePortUshort,	(WORD addr, WORD value));	// @11
FUNC(void, DlPortWritePortUlong,	(WORD addr, DWORD value));	// @12
FUNC(void, DlPortReadPortBufferUchar,	(WORD, PUCHAR, ULONG));		// @13
FUNC(void, DlPortReadPortBufferUlong,	(WORD, PULONG, ULONG));		// @14
FUNC(void, DlPortReadPortBufferUshort,	(WORD, PUSHORT, ULONG));	// @15

FUNC(BYTE, Pass,			(BYTE pass));			// @16

/**********************\
|** LPT-specific I/O **|
\**********************/

/* Because inpout32.dll most-used purpose is to access the parallel port,
 * routines are added 2013 to accomodate the fading but still useful port.
 * Most important are the built-in redirection features.
 * But it's much better do avoid direct access to ports either, and to move
 * time-critical code to a USB (or Ethernet, etc.) microcontroller.
 * As a good intermediate step, sufficient for a bunch of applications,
 * my so-called USB2LPT device is able to process microcode (ucode)
 * as a concatenation of OUT and IN instruction into one USB packet.
 * This requires rewriting of applications due to logical restructuring
 * but will help making any redirection much faster than ever before.
 * (Because it's not possible to concatenate IN instructions automatically;
 *  a random application relies to get the return value immediately.)
 * Good candidates for a rewrite are applications accessing synchron-serial
 * connected devices, as SPI, JTAG, and IIC, mostly, chip programmers.
 */

/* This function is for debugging and possibly for some interactive setup.
 * It's called by invoking "rundll32 inpout32.dll,Info"
 * and pops up a MessageBox showing computer's hardware and redirection features
 * concerning parallel ports. Arguments, except HWND, are currently not used.
 * The LPTSTR is the command line, possibly used later.
 */
FUNC(void, Info,  (HWND, HINSTANCE, PCSTR, int));	// @18
FUNC(void, InfoW, (HWND, HINSTANCE, PCWSTR, int));	// @19

/* This new function returns the address of parallel port(s), via PnP service.
 * 1. Mode <a> is zero: Returns
 * LOWORD = SPP base address, you can assume 8 contiguous addresses: 4 SPP, 4 EPP
 * HIWORD = ECP base address, you can assume 3 contiguous addresses
 * Returns zero when no address was found (no or no true parallel port).
 * <n> is the zero-based(!) number of parallel port, as for
 * \devices\parallelX (NT kernel namespace) or /dev/lpX (Linux)
 * 2. Mode <a> is nonzero:
 * This function will find all LPT port addresses to array <a>,
 * whereas n is the size of the array.
 * Unused table entries are left unchanged, so the caller should zero-out
 * this array on entry. The return value is the count of found ports,
 * include those that wouldn't fit into the table (i.e. LPT21 when n==10).
 */
FUNC(DWORD, LptGetAddr, (DWORD a[], int n));		// @20

/* These functions work similar to Inp32() and Out32(), but with one difference:
 * LptQueueIn() doesn't return a useful value!
 * As a result, all I/O can be collected and executed in one transaction
 * and can be much faster than single transfers, especially for USB devices.
 * You get the result for all IN transactions on QueueFlush().
 * These routines go to standard LPT ports if available (i.e. don't enqueue),
 * and to USB devices, like USB2LPT, USBLotIo or USB->ParallelPrinter etc.
 * For USB devices, no administrative privileges are necessary.
 * For new software, programmers should use this interface.
 * For USB->ParallelPrinter adapters, a latch and some connections are necessary!
 * Read http://www.tu-chemnitz.de/~heha/usb2lpt/faq#DIY
 *
 * <n> is zero-based (LPT1 = 0), the current working maximum is 8 (for LPT9)
 * <flags> is currently unused and should be zero
 * LptOpen() returns NULL on failure.
 * <offset> is 0..7 for SPP and EPP port range, and 8..11 for ECP port range
 *		Example: QueueOut(10,0x20) enqueues a switch to BiDi mode
 * <us> is a delay in microseconds. Maximum value is 10 ms, i.e. 10000 us
 * <buf> is a buffer for the results of all enqueued IN instructions.
 * <buflen> is the size of that buffer, for a sanity check.
 *		Must be >= count of enqueued IN instructions.
 * LptQueueFlush() returns the number of IN results made, and -1 on failure.
 *		For OUT-only programs, you SHOULD call QueueFlush() regularly,
 *		as this DLL doesn't create a time-out thread for you.
 * All other functions return TRUE on success and FALSE otherwise.
 * LptInOut() is a real workhorse, processes microcode containing IN and OUT
 *		instructions and returns the IN results in another buffer.
 *		The microcode is the same as for USB2LPT device, see below.
 *		This is an alternate access method, possibly bypassing the queue.
 *		New applications are strongly encouraged to use that function!
 *		You should not intermix LptInOut() and LptQueueXxx() functions,
 *		except you have ensured LptQueueFlush() before LptInOut().
 * Don't use CloseHandle() to close the handle returned by LptOpen().
 */
 
FUNC(HQUEUE,LptOpen,	(int n, DWORD flags));			// @21
FUNC(int, LptInOut,	(HQUEUE, const BYTE ucode[], int ulen, BYTE result[], int rlen)); // @22
FUNC(void,LptOut,	(HQUEUE, BYTE offset, BYTE value));	// @23
FUNC(BYTE,LptIn,	(HQUEUE, BYTE offset));			// @24
FUNC(void,LptDelay,	(HQUEUE, DWORD us));			// @25
FUNC(int, LptFlush,	(HQUEUE, BYTE result[], int rlen));	// @26
FUNC(BOOL,LptClose,	(HQUEUE));				// @27
FUNC(BYTE,LptPass,	(HQUEUE, BYTE pass));			// @28

/* Short-form description of microcode (ucode)
 * 1 BYTE opcode = (mostly) LPT port address offset + IN bit (bit 4), e.g.
 *   0x00 = write to data port
 *   0x11 = read from status port
 *   0x0A = write to ECP control register (ECR)
 *   0x20 = delay for n x 4 us
 * For OUT and DELAY opcode, the data byte follows the opcode.
 * For each IN instruction, no byte follows, but one byte space in result buffer is needed.
 * Multiple opcodes and data bytes are contiguous and not aligned somehow.
 * General USB implementations allow maximum ucode length (as EP0 transfer) of 4096 bytes.
 * Further limits may apply to specific USB2LPT devices.
 * Opcodes > 0x20 are currently disallowed.
 */
 
/**************************\
|** IO.DLL compatibility **|
\**************************/

FUNC(BOOL, IsDriverInstalled,	(void));			// @40
FUNC(void, ReleasePort,		(void));			// @41
FUNC(BOOL, LeftPortShift,	(WORD,BYTE));			// @42
FUNC(BOOL, RightPortShift,	(WORD,BYTE));			// @43
FUNC(BOOL, GetPortBit,		(WORD,BYTE));			// @44
FUNC(void, NotPortBit,		(WORD,BYTE));			// @45
FUNC(void, ClrPortBit,		(WORD,BYTE));			// @46
FUNC(void, SetPortBit,		(WORD,BYTE));			// @47
FUNC(DWORD,PortDWordIn,		(WORD));			// @48
FUNC(WORD,PortWordIn,		(WORD));			// @49
FUNC(BYTE, PortIn,		(WORD));			// @50
FUNC(void, PortDWordOut,	(WORD,DWORD));			// @51
FUNC(void, PortWordOut,		(WORD,WORD));			// @52
FUNC(void, PortOut,		(WORD,BYTE));			// @53

/**************************\
|** Mapped memory access **|
\**************************/
 
FUNC(void*,MapPhysToLin,(void*PhysAddr, DWORD PhysSize, HANDLE*MemoryHandle));// @56
FUNC(BOOL, UnmapPhysicalMemory, (HANDLE MemoryHandle, void*LinAddr));	// @57
FUNC(BOOL, GetPhysLong,	(void*PhysAddr, DWORD*PhysVal));		// @58
FUNC(BOOL, SetPhysLong,	(void*PhysAddr, DWORD PhysVal));		// @59

/* System query and some useful file system fiddling, only useful for X86 executeables
 */
FUNC(BOOL, IsXP64Bit, (void));			// @60
FUNC(BOOL, DisableWOW64, (PVOID* oldValue));	// @61
FUNC(BOOL, RevertWOW64, (PVOID oldValue));	// @62

#undef FUNC

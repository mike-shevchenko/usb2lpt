This is my fork of the well-known inpout32.dll / inpoutx64.dll.
With many advantages beyond Logix4U while not touching
the certified kernel-mode drivers and the basic working principle.

[2011]
The inpout32.dll supports Win32 programs on X86 and AMD64 targets.
The inpoutx64.dll supports Win32/64-bit programs on AMD64 targets.
Automatic built-in certified driver installation and start procedure.
Runs on Windows NT4 upto Windows 10, on Windows 95 upto Windows Me.
The file sizes are kept to minimum.

* No special runtime library is required, no "side-by-side" errors
  with this version of inpout32.dll.

[2013]
These totally revised DLLs are enhanced with the following features:

* Built-in automatic address remap, for emulating PCI/PCIexpress
  LPT cards on standard parallel port addresses, i.e. 0x378, 0x278, 0x3BC
  with almost no speed loss.

* Built-in Java support, replaces jnpout32.dll and jnpoutx64.dll.
  Multiple Java package paths are supported here,
  replacing the different versions ever spreaded with this one DLL.

* Simple-to-use and versatile API for LPT address detection,
  successfully tested on all operating systems.
   The only troubling system is Windows 8 where I revealed a bug:
   For on-board LPT ports, it works, but for PCI/PCIexpress, sometimes.
   The next Windows 8 Service Pack fixed — the documentation.

* New API for LPT access with redirection to:
  - True LPT ports
  - USB2LPT in native mode (requires Driver Sign Enforcement Override [DSEO])
  - USB-Printer converter with additional outbyte-catching hardware attached
  - Further devices can be easily added, as FT232 Bit-Bang mode, UsbLotIo, etc. 
  all with microcode support.
  This API should be used by new programs.

* Built-in LPT port access ==> Non-LPT access virtualization

* Built-in DOS box port I/O redirection for the three standard port addresses,
  also available for Windows 16-bit programs.
  Unfortunately, an unknown component steals some LPT port accesses anyways.

* Built-in miniature debug monitor for twiddeling with I/O by hand,
  merely as a DOS DEBUG replacement.
  Get it invoking "rundll32 inpout32,Info" resp. "rundll32 inpoutx64,Info"

* Built-in deep-level exception handler for catching hard-coded I/O
  and redirection to kernel-mode driver. (Not for InpOutX64.dll.)
  Perfect applicable for 32 bit processes running in a Win64 environment.

* Built-in DLL injector for Win32 processes made for Win9x/Me.
  In conjunction with deep-level exception handler, no exceptions are thrown
  anymore, and the app will run (a bit slower: 150 k I/O per second).
  Invoke the program in question: "rundll32 inpout32,CatchIo my_program.exe".
  Moreover, the program is faked that it runs on Win9x/Me, so
  NT-aware programs won't install&load GiveIo.sys or DlPortIo.sys.
  
  [[Note that for programs containing hard-coded IN and OUT instructions,
  it's preferrable to load and use (my) giveio.sys instead which exists both
  as 32-bit and 64-bit driver, which enables port access instructions
  for user mode applications. It's much faster than using that exception
  handler but any redirecting feature will not be available anymore.]]

* Built-in X86 instruction decoder for skipping otherwise faulting operations
  reading the BIOS data area (0040:0008) for getting a parallel port address.

* For command-line usage, a replacement rundll32.exe (which creates a new
  console window for the new process) is crundll.exe.

[April 2014]

* An alias port address range 0x100..0x10A for DOS and Win16 programs
  fakes the printer redirection "feature" of ntvdm.exe.
  (applies to inpout32.dll; not useable for X64 systems)

* The newly constructed port address search algorithm now finds more
  addresses for a PC running Windows 8 / 8.1 64bit for a 32-bit process:
  It doesn't rely on pre-known service names like Parport and NmPar anymore.

* Via "crundll inpout32,Info", the user can choose when to load the driver.
  Either at system startup or on demand.
  The first option is good for using InpOut32.dll as unprivileged user later.
  However, that opens a backdoor to computer's hardware for everyone.
  The second option (default) requires that InpOut32.dll users are
  administrators or have the privilege to load device drivers.

The file sizes are kept to minimum.
The minimum required operating systems are kept to NT4 and Win95.

NOTE: USB2LPT and USB-Printer redirection is currently NOT THROROUGLY TESTED.
USB2LPT and USB-Printer redirection doesn't require administrative privileges.

[March 2025]

* Added interprocess communication feature to let 32-bit DLL get
  real port addresses from temporary 64-bit process.
  Needed for Windows >=8. Thank to Microsoft to make it so cumbersome!
  In such a case, the 64-bit DLL serves as proxy and cannot be deleted,
  even when your executable in question is 32-bit.

WHERE TO INSTALL
================

If you have only one application that uses inpout32.dll or inpoutx64.dll,
it's probably best to let this DLL in the same directory as the application.

If you want to make inpout32.dll globally available, you should put these DLLs
to the Windows directory, and remove all other copies:
* For Windows 32 bit, put inpout32.dll to C:\Windows\System32\.
* For Windows 64 bit, put inpout32.dll to C:\Windows\SysWOW64\,
		     and inpoutx64.dll to C:\Windows\System32\,
  using a 64-bit file manager. Really: The 32-bit DLL has to be in a folder
  with a "64" in its name, and vice versa!
  If you use a 32-bit file manager, both system directories are mapped to:
  System32 → SysNative (Vista and above, otherwise invisible)
  SysWOW64 → System32
* For inpoutx64.dll, despite it's a descriptive name, it doesn't match
  to Windows naming convention to name it inpout32.dll too.
  Meanwhile I recommend to make a hard or symbolic link to it,
  best inside c:\windows\sysnative directory.
  Renaming breaks the port address detection feature
  for Windows >=8 for the 32-bit DLL.

If you have multiple Windows installations, you probably have a common
folder for executables for all Windows versions, e.g. C:\Programs\bin,
with hand-edited path environment variable pointing to it.
Simply put both DLLs to that directory, and you are done.

Henrik Haftmann, March 2025

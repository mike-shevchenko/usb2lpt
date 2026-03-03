This release of giveio.sys or dlportio.sys for AMD64 architectures
(i.e. 64 bit Windows) allows running of old software that needs one
of the two drivers to enable port access in user mode.

USE AT YOUR OWN RISK!

	When using a hypervisor (known for: Android virtual device),
	this driver produces a bug check (aka bluescreen).
	I'm reseaching on that issue.

USELESS ON ANY 32-BIT WINDOWS!

__COMPATIBILITY__

Compatible and tested under Windows XP, Windows Vista, Windows 7,
Windows 8, Windows 8.1, and Windows 10 (64 bit versions only).

2019-04-23: As tested with Windows 10 today, loading giveio.sys
produces a bluescreen (IRQL_NOT_LESS_OR_EQUAL).
That is a remotely administrated computer "zfmpc22".
However, on another machine with local administration "ewasc025",
giveio.sys loads with no trouble.
Therefore, some unknown component is responsible for that behaviour.
Never give up, as programs accessing parallel port can still be run
in a systemwide exception handler environment using my inpout32.dll:
Prepend "crundll inpout32,CatchIo " in front of your program!

__INSTALLATION__

Replace any 32-bit giveio.sys or dlportio.sys
distributed by your application by this giveio.sys file.
You may need administrative privileges for this task,
depending on your file system structure.
I case of using Windows Explorer, it's a good idea to have
file name extensions (here: .sys) set to be visible.

In case of needing dlportio.sys, simply rename this giveio.sys file.
Internally, the driver creates two so-called DOS aliases, accessible as
"\\.\giveio" and "\\.\dlportio", to be compatible to both.

Usually, the driver will be loaded on application startup
by a built-in routine communicating with Windows' Service Manager.
For manual (i.e. fresh) installation see below.

Don't use a compatibility mode for your application in question.

__KNOWN_APPLICATIONS__

Velleman PCS500, avrdude

__TROUBLESHOOTING__

The driver is digitally signed. That signature is valid forever.
(Note that its certificate ended May 2016
 but that will not affect the valid signature once set in June 2015.)

	On some computers, the driver's certificate is not accepted.
	A set of cross-certifactes must be installed by a Windows update.
	For Windows 7, this is the update KB3033929.
	In such a case, keep the computer online for several days(!) —
	around the clock, with automatic update install enabled.
	Meanwhile, Windows will find missing updates,
	until the update above will be found and installed.
	(Information by Comodo.com and J. Strunk.)

__MANUAL_DRIVER_LOAD__

In case you have a very old program intended for Windows 9x/Me only,
you can manually "install" the giveio.sys file by invoking
"sc create giveio type= kernel binpath= <full_path_to_giveio.sys>"
and start the service (i.e. load the driver, "modprobe") by invoking
"sc start giveio"
Have a look for other "sc" options!
There is no need for reboot when you use this utility
instead of some old guides recommending registry hacks.

I/O is then opened for all processes.

__MORE_INFO__

Normally, this driver has to modify the Global Descriptor Table (GDT),
a vital kernel-mode processor structure.
But later, a Windows built-in PatchGuard
routine would fire bluescreen 0x109 after some minutes.

Therefore, this driver does a trick: The GDT is patched temporary,
for some nanoseconds, with interrupts disabled.
While patched, the ltr instruction loads the CPU-internal cache of GDT entry.
The PatchGuard does not check or reset this cache,
and nobody changes the Task Register anymore. I'm lucky!
Except for a kernel debugger breakpoint.
In such a case, PatchGuard is disabled anyways,
and the trick is not applied.

Attention: ALL port addresses will be enabled.

In opposite to 32-bit Windows versions, the Task-State Segment (TSS)
is no more thread specific. Therefore, all running programs,
independently of their bitness (16/32/64) will gain I/O access.


Note that I found out the Win64 behaviour using Windows Kernel debugger
and simply executed:
 eb @gdtr+@tr+1 20	//enlarge processor0's TSS length by 8 KByte
 $1s			//switch to processor 1
 eb @gdtr+@tr+1 20	//repeat for each processor you have
Surprisingly, Windows seems to preserve the extra 8 KB memory per processor
in advance, so no memory allocation was necessary.
Furthermore, that extra (normally dead) memory is preset with zeroes
which will automatically enable access for all ports from user mode.

See source code for more comments how it works (partially in German language).


At the end, this driver is intended to become a part of my
inpout32.dll package which should solve all I/O related hassle
in one project.

Henrik Haftmann, 150421 - 150514 - 180919

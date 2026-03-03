/***********************************************************************\
*									*
* InpOut32drv.cpp							*
*									*
* The entry point for the InpOut DLL					*
* Provides the 32 and 64bit implementation of InpOut32 DLL to install	*
* and call the appropriate driver and write directly to hardware ports. *
*									*
* Written by Phillip Gibbons (Highrez.co.uk)				*
* Based on orriginal, written by Logix4U (www.logix4u.net)		*
* Removed excess of fat haftmann#software (heha@hrz.tu-chemnitz.de)	*
* Added automatic port address redirector for LPTs			*
\***********************************************************************/

#include <windows.h>
#include "hwinterfacedrv.h"
#include "Redir.h"
#include "inpout32.h"
#include "ModRM.h"

HANDLE hdriver;
HINSTANCE hInstance;
#ifdef _M_IX86
long sysver;	// negative for Win9x, WinMe, and Win32s, else positive
BOOL wow64;	// TRUE when running on Windows-on-Win64 or as native 64-bit DLL
#else
# define wow64 TRUE
#endif
BYTE pass;
bool started;	// this process had invoked StartService() successfully

/******************************
 ** Native I/O (X86-32 only) **
 ******************************/
#ifdef _M_IX86
static void _declspec(naked) _fastcall _outportb(BYTE cl, WORD dx) { _asm{
	mov	eax,ecx
	out	dx,al
	ret
}}
static BYTE _declspec(naked) _fastcall _inportb(WORD cx) { _asm{
	mov	edx,ecx
	in	al,dx
	ret
}}
static void _declspec(naked) _fastcall _outportw(WORD cx, WORD dx) { _asm{
	mov	eax,ecx
	out	dx,ax
	ret
}}
static WORD _declspec(naked) _fastcall _inportw(WORD cx) { _asm{
	mov	edx,ecx
	in	ax,dx
	ret
}}
static void _declspec(naked) _fastcall _outportd(DWORD ecx, WORD dx) { _asm{
	mov	eax,ecx
	out	dx,eax
	ret
}}
static DWORD _declspec(naked) _fastcall _inportd(WORD cx) { _asm{
	mov	edx,ecx
	in	eax,dx
	ret
}}
#else
# define _outportb(a,b)
# define _inportb(a) 0xFFU
# define _outportw(a,b)
# define _inportw(a) 0xFFFFU
# define _outportd(a,b)
# define _inportd(a) 0xFFFFFFFFUL
#endif

/************************
 ** Exported functions **
 ************************/

BOOL WINAPI IsInpOutDriverOpen() {
 if (sysver<0) return TRUE;
 if (hdriver!=INVALID_HANDLE_VALUE && hdriver) return TRUE;
 return FALSE;
}

void WINAPI Out32(WORD addr, BYTE data) {
 patch(addr);
 if (RedirOut(addr,data)) return;
 if (sysver<0) _outportb(data,addr);
 else{
  DWORD br;
  ((PBYTE)&addr)[2]=data;
  DeviceIoControl(hdriver,IOCTL_WRITE_PORT_UCHAR,&addr,3,NULL,0,&br,NULL);
 }
}

BYTE WINAPI Inp32(WORD addr) {
 patch(addr);
 BYTE b;
 if (RedirIn(addr,b)) return b;
 if (sysver<0) return _inportb(addr);
 DWORD br;
 DeviceIoControl(hdriver,IOCTL_READ_PORT_UCHAR,&addr,2,&addr,1,&br,NULL);
 return ((PBYTE)&addr)[0];
}

WORD WINAPI DlPortReadPortUshort(WORD addr) {
 patch(addr);
 if (sysver<0) return _inportw(addr);
 DWORD br;
 DeviceIoControl(hdriver,IOCTL_READ_PORT_USHORT,&addr,2,&addr,2,&br,NULL);
 return addr;
}

void WINAPI DlPortWritePortUshort(WORD addr, WORD data) {
 patch(addr);
 if (sysver<0) _outportw(data,addr);
 else{
  DWORD br;
  (&addr)[1]=data;
  DeviceIoControl(hdriver,IOCTL_WRITE_PORT_USHORT,&addr,4,NULL,0,&br,NULL);
 }
}

DWORD WINAPI DlPortReadPortUlong(WORD addr) {
 patch(addr);
 if (sysver<0) return _inportd(addr);
 DWORD br;
 DeviceIoControl(hdriver,IOCTL_READ_PORT_ULONG,&addr,2,&addr,4,&br,NULL);
 return *(DWORD*)&addr;
}

void WINAPI DlPortWritePortUlong(WORD addr, DWORD data) {
 patch(addr);
 if (sysver<0) _outportd(data,addr);
 else{
  DWORD br;
  DeviceIoControl(hdriver,IOCTL_WRITE_PORT_ULONG,&addr,8,NULL,0,&br,NULL);
 }
}

void WINAPI DlPortReadPortBufferUchar(WORD addr, PUCHAR buf, ULONG len) {
 if (len) do *buf++=Inp32(addr); while(--len);
}

void WINAPI DlPortReadPortBufferUshort(WORD addr, PUSHORT buf, ULONG len) {
 if (len) do *buf++=DlPortReadPortUshort(addr); while(--len);
}

void WINAPI DlPortReadPortBufferUlong(WORD addr, PULONG buf, ULONG len) {
 if (len) do *buf++=DlPortReadPortUlong(addr); while(--len);
}

void WINAPI DlPortWritePortBufferUchar(WORD addr, const UCHAR *buf, ULONG len) {
 if (len) do Out32(addr,*buf++); while(--len);
}

void WINAPI DlPortWritePortBufferUshort(WORD addr, const USHORT *buf, ULONG len) {
 if (len) do DlPortWritePortUshort(addr,*buf++); while(--len);
}

void WINAPI DlPortWritePortBufferUlong(WORD addr, const ULONG *buf, ULONG len) {
 if (len) do DlPortWritePortUlong(addr,*buf++); while(--len);
}

BYTE WINAPI Pass(BYTE b) {
 BYTE ret=pass;
 if (b<=2) pass=b;
 return ret;
}

// TODO: Send opcodes to USB2LPT (if ever needed)
void WINAPI ClrPortBit(WORD addr, BYTE bitno) {
 Out32(addr,Inp32(addr)&~(1<<bitno));
}

void WINAPI SetPortBit(WORD addr, BYTE bitno) {
 Out32(addr,Inp32(addr)|1<<bitno);
}

void WINAPI NotPortBit(WORD addr, BYTE bitno) {
 Out32(addr,Inp32(addr)^1<<bitno);
}

BOOL WINAPI GetPortBit(WORD addr, BYTE bitno) {
 return (Inp32(addr)>>bitno)&1;
}

// TODO: http://www.geekhideout.com/iodll.shtml
BOOL WINAPI LeftPortShift(WORD addr, BYTE count) {
 BYTE b=Inp32(addr);
 Out32(addr,b<<count);
 return b>>7;	// shifted-out bit (if count==1)
}

BOOL WINAPI RightPortShift(WORD addr, BYTE count) {
 BYTE b=Inp32(addr);
 Out32(addr,b>>count);
 return b&1;	// shifted-out bit (if count==1)
}

// Support functions for WinIO
void*WINAPI MapPhysToLin(void*pbPhysAddr, DWORD dwPhysSize, HANDLE*pPhysicalMemoryHandle) {
#ifdef _M_IX86
 if (sysver<0) {
  if ((DWORD)pbPhysAddr>=0x400 && (DWORD)pbPhysAddr+dwPhysSize<=0x500) {	// 256 bytes BIOS data area
   BYTE*p = new BYTE[dwPhysSize+8];
   if (p) {
    _asm{
	mov	edi,p			// This code is optimized for size, not for Pentium parallelism.
	mov	eax,pbPhysAddr
	sub	ah,4
	stosd				// prepend source address
	xchg	esi,eax	
	mov	eax,dwPhysSize
	stosd				// prepend length
	xchg	ecx,eax
	push	ds
	 push	0x40
	 pop	ds
	 rep	movsb			// get data
	pop	ds
    }
    *pPhysicalMemoryHandle=(HANDLE)p;
    return p+8;
   }
  }
  return NULL;
 }
#endif
	
 tagPhys32Struct Phys32Struct;
 DWORD dwBytesReturned;

 Phys32Struct.dwPhysMemSizeInBytes = dwPhysSize;
 Phys32Struct.pvPhysAddress = pbPhysAddr;

 if (!DeviceIoControl(hdriver, IOCTL_WINIO_MAPPHYSTOLIN, &Phys32Struct,
   sizeof(tagPhys32Struct), &Phys32Struct, sizeof(tagPhys32Struct),
   &dwBytesReturned, NULL))
  return NULL;
 *pPhysicalMemoryHandle = Phys32Struct.PhysicalMemoryHandle;
#ifdef _M_IX86
 return (void*)((LONGLONG)Phys32Struct.pvPhysMemLin + (LONGLONG)pbPhysAddr - (LONGLONG)Phys32Struct.pvPhysAddress);
#else
 return (void*)((DWORD)Phys32Struct.pvPhysMemLin + (DWORD)pbPhysAddr - (DWORD)Phys32Struct.pvPhysAddress);
#endif
}


BOOL WINAPI UnmapPhysicalMemory(HANDLE PhysicalMemoryHandle, void*pbLinAddr) {
#ifdef _M_IX86
 if (sysver<0) {
  if ((BYTE*)PhysicalMemoryHandle+8==(BYTE*)pbLinAddr) {
   _asm{mov	esi,PhysicalMemoryHandle
	lodsd
	xchg	edi,eax		// retrieve address of origin
	lodsd
	xchg	ecx,eax		// retrieve length
	push	es
	 push	0x40
	 pop	es
	 rep	movsb		// put data back
	pop	es
   }
   delete[] (BYTE*)PhysicalMemoryHandle;
   return TRUE;
  }
 }return FALSE;
#endif
 tagPhys32Struct Phys32Struct;
 DWORD dwBytesReturned;

 Phys32Struct.PhysicalMemoryHandle = PhysicalMemoryHandle;
 Phys32Struct.pvPhysMemLin = pbLinAddr;

 return DeviceIoControl(hdriver, IOCTL_WINIO_UNMAPPHYSADDR, &Phys32Struct,
   sizeof(tagPhys32Struct), NULL, 0, &dwBytesReturned, NULL);
}

BOOL WINAPI GetPhysLong(PBYTE pbPhysAddr, PDWORD pdwPhysVal) {
 if (sysver<0) return FALSE;
 PDWORD pdwLinAddr;
 HANDLE PhysicalMemoryHandle;

 pdwLinAddr = (PDWORD)MapPhysToLin(pbPhysAddr, 4, &PhysicalMemoryHandle);
 if (!pdwLinAddr) return FALSE;

 *pdwPhysVal = *pdwLinAddr;
 return UnmapPhysicalMemory(PhysicalMemoryHandle, (PBYTE)pdwLinAddr);
}

BOOL WINAPI SetPhysLong(PBYTE pbPhysAddr, DWORD dwPhysVal) {
 if (sysver<0) return FALSE;
 PDWORD pdwLinAddr;
 HANDLE PhysicalMemoryHandle;

 pdwLinAddr = (PDWORD)MapPhysToLin(pbPhysAddr, 4, &PhysicalMemoryHandle);
 if (!pdwLinAddr) return FALSE;

 *pdwLinAddr = dwPhysVal;
 return UnmapPhysicalMemory(PhysicalMemoryHandle, (PBYTE)pdwLinAddr);
}

/**********************************************************************
 ** Install / load / unload / uninstall certified(!) driver silently **
 **********************************************************************/

// Help the compiler to combine non-identical strings but with same endings
#ifdef _M_X64
static const WCHAR FileTail[] = L"System32\\Drivers\\inpoutx64.sys";
static const WCHAR SlnkTail[] = L"\\\\.\\inpoutx64";
#else
static const WCHAR FileTail32[] = L"System32\\Drivers\\inpout32.sys";
static const WCHAR SlnkTail32[] = L"\\\\.\\inpout32";

static const WCHAR FileTail64[] = L"System32\\Drivers\\inpoutx64.sys";
static const WCHAR SlnkTail64[] = L"\\\\.\\inpoutx64";

LPCWSTR FileTail;
LPCWSTR SlnkTail;
#endif


/***********************************************************************/
static int inst() {
// 1: Extract the right .sys file to %windir%/system32/drivers
 int ret=0;
 HRSRC hResource = FindResource(hInstance, MAKEINTRESOURCE(wow64?104:101), RT_RCDATA);
 if (hResource) {
  HGLOBAL binGlob = LoadResource(hInstance, hResource);
  if (binGlob) {
   void *binData = LockResource(binGlob);
   if (binData) {
    WCHAR path[MAX_PATH];
    HANDLE file;
    UINT len = GetSystemDirectoryW(path,elemof(path));
    lstrcpynW(path+len,FileTail+8,elemof(path)-len);
#ifdef _M_IX86
    PVOID OldWOW;
    if (wow64) DisableWOW64(&OldWOW);
#endif
    file = CreateFileW(path,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);
    if (file!=INVALID_HANDLE_VALUE) {
     DWORD size, written;
     size = SizeofResource(hInstance, hResource);
     WriteFile(file, binData, size, &written, NULL);
     CloseHandle(file);
    }else ret=-5;		// code: cannot extract .sys file
#ifdef _M_IX86
    if (wow64) RevertWOW64(OldWOW);
#endif
   }
  }
 }
// 2: Install service (possibly without extracting .sys file first)
 SC_HANDLE Mgr=OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
 if (Mgr) {
  SC_HANDLE Ser = CreateServiceW(Mgr,
    SlnkTail+4,
    SlnkTail+4,
    SERVICE_ALL_ACCESS,
    SERVICE_KERNEL_DRIVER,
    SERVICE_DEMAND_START,
    SERVICE_ERROR_NORMAL,
    FileTail,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL);
  if (Ser) CloseServiceHandle(Ser);
  else ret=-4;			// code: CreateService() failed
  CloseServiceHandle(Mgr);
 }else ret=-1;			// code: OpenSCManager() failed
 return ret;
}

static int querystarttype(SC_HANDLE Ser) {
 int ret;
 DWORD bufsize;
 if (!QueryServiceConfig(Ser,NULL,0,&bufsize) && GetLastError()==ERROR_INSUFFICIENT_BUFFER) {
  QUERY_SERVICE_CONFIG*qsc=(QUERY_SERVICE_CONFIG*)new BYTE[bufsize];
  if (qsc) {
   if (QueryServiceConfig(Ser,qsc,bufsize,&bufsize)) ret=qsc->dwStartType;
   else ret=-9;
   delete[] (BYTE*)qsc;
  }else ret=-8;
 }else ret=-7;
 return ret;
}

/**************************************************************************/
static int start(bool start=true) {
 int ret=-1;		// code: OpenSCManager() failed
 SC_HANDLE Mgr = OpenSCManager(NULL, NULL,SC_MANAGER_ALL_ACCESS);
 if (!Mgr && GetLastError()==ERROR_ACCESS_DENIED) {
  Mgr = OpenSCManager(NULL, NULL, GENERIC_READ);
  if (!Mgr && GetLastError()==ERROR_ACCESS_DENIED) {
   Mgr = OpenSCManager(NULL, NULL, GENERIC_EXECUTE);
  }
 }
 if (Mgr) {
  ret=-2;		// code: OpenService(EXECUTE) failed
  SC_HANDLE Ser = OpenServiceW(Mgr,SlnkTail+4,start?GENERIC_EXECUTE:GENERIC_EXECUTE|SERVICE_QUERY_CONFIG);
  if (Ser) {
   ret=-3;		// code: unable to start/stop
   if (start) {
    if (StartService(Ser,0,NULL)) {started=true; ret=0;}
    else if (GetLastError()==ERROR_SERVICE_ALREADY_RUNNING) ret=0;
   }else{
    ret=querystarttype(Ser);
    if (ret==SERVICE_DEMAND_START) {	// leave "ret" for other codes than DEMAND_START
     SERVICE_STATUS ss;
     if (ControlService(Ser,SERVICE_CONTROL_STOP,&ss)) ret=0;
    }
   }
   CloseServiceHandle(Ser);
  }
  CloseServiceHandle(Mgr);
 }
 return ret;
}

// sets or queries the start type of inpout32.sys / inpoutx64.sys service
int setstarttype(int starttype) {
 int ret;
 SC_HANDLE Mgr=OpenSCManager(NULL,NULL,starttype>=0?SC_MANAGER_ALL_ACCESS:GENERIC_READ);
 if (Mgr) {
  SC_HANDLE Ser=OpenServiceW(Mgr,SlnkTail+4,starttype>=0?SERVICE_CHANGE_CONFIG|SERVICE_QUERY_CONFIG:SERVICE_QUERY_CONFIG);
  if (Ser) {
   if (starttype>=0
   && !ChangeServiceConfig(Ser,SERVICE_NO_CHANGE,starttype,SERVICE_NO_CHANGE,0,0,0,0,0,0,0)) ret=-6;
   else ret=querystarttype(Ser);
   CloseServiceHandle(Ser);
  }else ret=-2;
  CloseServiceHandle(Mgr);
 }else ret=-1;
 return ret;
}

/*********************************************************************/

static void openhdriver() {
 hdriver = CreateFileW(SlnkTail, 
   GENERIC_READ|GENERIC_WRITE,
   0,
   NULL,
   OPEN_EXISTING, 
   FILE_ATTRIBUTE_NORMAL, 
   NULL);
}

static void Opendriver() {
 if (!wow64) InstallVdmHook();		// no VDM hook where no ntvdm.exe exists
 openhdriver();
 if (hdriver==INVALID_HANDLE_VALUE) {
  if (start()) inst(), start();
  openhdriver();
 }
// DefineDosDeviceW(DDD_RAW_TARGET_PATH,L"\\\\.\\DLPORTIO",SlnkTail);
// doesn't work, sorry!
}

/************************
 ** Exception catching **
 ************************/
#ifdef _M_IX86

static void _cdecl MBox(HWND w, const char*t, ...) {
 char s[1024];
 wvsprintf(s,t,va_list(&t+1));
 MessageBox(w,s,NULL,0);
}

static DWORD ExceptionHandler(EXCEPTION_RECORD*ex, CONTEXT*ctx, VARPTR&code) {
 UINT prefixes=Prefixes(code);

 switch (ex->ExceptionCode) {
  case EXCEPTION_ACCESS_VIOLATION: switch (*code.b++) {	// access to BIOS data area?
   case 0x8E: {	// mov es,r/m16, faults on 64 bit OS, i.e. WOW32 (was 0xCx8E66)
    ModRM(prefixes,code,ctx,NULL);	// skip r/m16 opcode trail
   }return EXCEPTION_CONTINUE_EXECUTION;

   case 0x8B: goto getlpt;	// mov r,r/m, seen as 0x168B2666 == mov dx,es:[esi] with es==0x40, in WGif12NT.exe

   case 0x0F: switch (*code.b++) {
    case 0xB7:			// movzx r,r/m
    case 0xBF: goto getlpt;	// movsx r,r/m
   }break;
  }break;

  case EXCEPTION_PRIV_INSTRUCTION: switch (*code.b++) {
   case 0xEF:		// OUT dx,eax
   if (prefixes&0x200) DlPortWritePortUshort((WORD)ctx->Edx,(WORD)ctx->Eax);
   else DlPortWritePortUlong((WORD)ctx->Edx,ctx->Eax);
   return EXCEPTION_CONTINUE_EXECUTION;

   case 0xEE:		// OUT dx,al
   Out32((WORD)ctx->Edx,(BYTE)ctx->Eax);
   while (*code.b==0xEE) code.b++;	// skip I/O instructions (improve speed for dumb LPT access)
   return EXCEPTION_CONTINUE_EXECUTION;

   case 0xED:		// IN eax,dx
   if (prefixes&0x200) *(WORD*)&ctx->Eax=DlPortReadPortUshort((WORD)ctx->Edx);
   ctx->Eax=DlPortReadPortUlong((WORD)ctx->Edx);
   return EXCEPTION_CONTINUE_EXECUTION;

   case 0xEC:		// IN al,dx
   *(BYTE*)&ctx->Eax=Inp32((WORD)ctx->Edx);
   while (*code.b==0xEC) code.b++;	// skip I/O instructions (improve speed for dumb LPT access)
   return EXCEPTION_CONTINUE_EXECUTION;
  }break;
 }
 return EXCEPTION_CONTINUE_SEARCH;

getlpt:
 VARPTR destreg;
 long srcaddr=ModRM(prefixes,code,ctx,&destreg);	// and skip r/m16 opcode trail
	// here, srcaddr should result in either 0x08, 0x0A, or 0x0C.
	// Possibly, 0x0E to get LPT4, or 0x10, to get the number of LPT ports
	// For simplicity, that's not checked here for now.
 srcaddr=SppDef[(srcaddr>>1)&3];
 if (prefixes&0x200) *destreg.w=(WORD)srcaddr;	// with 0x66 prefix, set WORD register
 else *destreg.d=srcaddr;			// otherwise, set DWORD register
 return EXCEPTION_CONTINUE_EXECUTION;
}

static LONG WINAPI vectoredHandler(EXCEPTION_POINTERS*ep) {
 CONTEXT*ctx=ep->ContextRecord;
 VARPTR code={(void*)ctx->Eip};
 DWORD ret=ExceptionHandler(ep->ExceptionRecord,ctx,code);
 if (ret==EXCEPTION_CONTINUE_EXECUTION) {
  ctx->Eip=(DWORD)code.v;
 }
 return ret;
}

static void exceptInit() {
 if (!krnl.AddVectoredExceptionHandler) return;
 krnl.AddVectoredExceptionHandler(1,vectoredHandler);
}

static void exceptDone() {
 if (!krnl.RemoveVectoredExceptionHandler) return;
 krnl.RemoveVectoredExceptionHandler(vectoredHandler);
}

static BOOL CALLBACK MyGetVersionEx(OSVERSIONINFO*vi) {
 BOOL ret=GetVersionEx(vi);
 if (ret) vi->dwPlatformId=VER_PLATFORM_WIN32_WINDOWS;	// Windows 9x/Me
 return ret;
}

#ifdef HOOK_GETVERSION
static DWORD CALLBACK MyGetVersion() {
 return GetVersion()|0x80000000;
}
#endif

// Hooks GetVersionEx() to return Win9x, for the .EXE image.
// This prevents executables to load drivers like GiveIo [avrdude] or DlPortIo [pcs500]
// or both GiveIo and MapMem [WGif12NT.exe].
// No program in question seems to use GetVersion() for the NT detection
// The function signature is equal to ThreadProc(), but arguments are not used.
// As return value, this function reports whether it is a console process.
static BOOL CALLBACK HookGetVersion(PVOID) {
 BYTE*ExeBase=(BYTE*)GetModuleHandle(NULL);
 IMAGE_NT_HEADERS*NtHeader=(IMAGE_NT_HEADERS*)(ExeBase + ((IMAGE_DOS_HEADER*)ExeBase)->e_lfanew);
 IMAGE_IMPORT_DESCRIPTOR*ImportDesc=(IMAGE_IMPORT_DESCRIPTOR*)(ExeBase
   + NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
 bool fRestoreProtection=false;
 DWORD OldProtection;
 BYTE*FuncTab=ExeBase+NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress;
 DWORD FuncTabSize=NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size;
 if (FuncTabSize) {
// The import table may be write protected
  MEMORY_BASIC_INFORMATION MI;
  VirtualQuery(FuncTab,&MI,sizeof MI);
  if (MI.Protect==PAGE_READONLY) {
   VirtualProtect(FuncTab,FuncTabSize,PAGE_READWRITE,&OldProtection);
   fRestoreProtection=true;
  }
 }
 for (;ImportDesc->Name;++ImportDesc) {
  for (IMAGE_THUNK_DATA*Thunk = (IMAGE_THUNK_DATA*)(ExeBase + ImportDesc->FirstThunk);Thunk->u1.Function;++Thunk) {
   if (Thunk->u1.Function==(DWORD)GetVersionEx) Thunk->u1.Function=(DWORD)MyGetVersionEx;
#ifdef HOOK_GETVERSION
   if (Thunk->u1.Function==(DWORD)GetVersion) Thunk->u1.Function=(DWORD)MyGetVersion;
#endif
  }
 }
 if (fRestoreProtection) VirtualProtect(FuncTab,FuncTabSize,OldProtection,&OldProtection);
 return NtHeader->OptionalHeader.Subsystem==IMAGE_SUBSYSTEM_WINDOWS_CUI;
}

#else
# define exceptInit()
# define exceptDone()
#endif

void CALLBACK CatchIoW(HWND w, HINSTANCE, PWSTR arg, int) {	// export #66
#ifdef _M_IX86
 if (sysver<0) {
  MBox(w,"CatchIo requires NT based systems");
  return;
 }
 STARTUPINFOW si;
 GetStartupInfoW(&si);
 PROCESS_INFORMATION pi;
 int HookMode=0;		// Bit 0: suppress HookGetVersion, Bit 1: run as debuggee
 while (*arg==' ') arg++;	// skip leading spaces
 if (*arg=='-') {
  HookMode=arg[1];
  arg+=2;
  while (*arg==' ') arg++;
 }
 if (!krnl.AddVectoredExceptionHandler) HookMode|=2;
 if (CreateProcessW(NULL,arg,NULL,NULL,TRUE,CREATE_SUSPENDED,NULL,NULL,&si,&pi)) {
  DWORD bConsole=FALSE;
  if (~HookMode&3) {	// no DLL injection when both flags are set
   DWORD hLib=0;
   WCHAR self[MAX_PATH];
   int len=(GetModuleFileNameW(hInstance,self,elemof(self))+1)*2;
   void*addr=VirtualAllocEx(pi.hProcess,0,len,MEM_COMMIT,PAGE_READWRITE);
   if (addr) {
    WriteProcessMemory(pi.hProcess,addr,self,len,NULL);
    HANDLE hThread=krnl.CreateRemoteThread(pi.hProcess,NULL,0,
      (LPTHREAD_START_ROUTINE)LoadLibraryW,addr,0,NULL);
    if (hThread) {
     WaitForSingleObject(hThread,INFINITE);	// execute _DllMainCRTStartup() there
     GetExitCodeThread(hThread,&hLib);		// returns whether DLL is loaded
     CloseHandle(hThread);
    }
    VirtualFreeEx(pi.hProcess,addr,0,MEM_RELEASE);
    if (hLib) {
     if (!(HookMode&1)) {
      hThread=krnl.CreateRemoteThread(pi.hProcess,NULL,0,
        (LPTHREAD_START_ROUTINE)((DWORD)HookGetVersion-(DWORD)hInstance+hLib),0,0,NULL);
      if (hThread) {
       WaitForSingleObject(hThread,INFINITE);	// execute HookGetVersion() there
       GetExitCodeThread(hThread,&bConsole);	// returns type of process image
       CloseHandle(hThread);
      }
     }else MBox(w,"Could not inject %S into process %S",self,arg);
    }
   } 
  }
  if (HookMode&2 && DebugActiveProcess(pi.dwProcessId)) {
   ResumeThread(pi.hThread);
   DEBUG_EVENT de;
   while (WaitForDebugEvent(&de,INFINITE)) {
    switch (de.dwDebugEventCode) {
     case EXCEPTION_DEBUG_EVENT: if (de.u.Exception.dwFirstChance) {
// Seltsam! Fängt nicht "mov ds,es:[esi]"!
      HANDLE hThread=krnl.OpenThread(THREAD_ALL_ACCESS,FALSE,de.dwThreadId);
      CONTEXT ctx;
      ctx.ContextFlags=CONTEXT_FULL;
      GetThreadContext(hThread,&ctx);
      BYTE bytes[16];				// should be long enough
      VARPTR code={(void*)bytes};
      if (ReadProcessMemory(pi.hProcess,(BYTE*)ctx.Eip,bytes,sizeof bytes,NULL)
      && ExceptionHandler(&de.u.Exception.ExceptionRecord,&ctx,code)==EXCEPTION_CONTINUE_EXECUTION) {
       ctx.Eip+=code.b-bytes;			// skip parsed opcode piece
       SetThreadContext(hThread,&ctx);
      }
     }break;
     case EXIT_PROCESS_DEBUG_EVENT: goto exi;
    }
    ContinueDebugEvent(de.dwProcessId,de.dwThreadId,DBG_CONTINUE);
   }
exi:;
  }else{
   ResumeThread(pi.hThread);
   if (bConsole) WaitForSingleObject(pi.hProcess,INFINITE);	// otherwise, command prompt occurs in-between
  }
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
 }else MBox(w,"Could not create process »%S«",arg);
#endif
}

/*************************************
 ** DLL entry point: Initialization **
 *************************************/

EXTERN_C BOOL CALLBACK _DllMainCRTStartup(
  HINSTANCE hModule,DWORD callReason,LPVOID) {
 hInstance = hModule;
 switch (callReason) {
  case DLL_PROCESS_ATTACH: {
   DisableThreadLibraryCalls(hModule);
// Detect environment and set 4 global variables
#ifdef _M_IX86
   sysver=GetVersion();
   wow64=IsXP64Bit();
   FileTail=wow64?FileTail64:FileTail32;
   SlnkTail=wow64?SlnkTail64:SlnkTail32;
#endif
   if (sysver>=0) Opendriver();	// also installs VDM redirection
   InitRedirector();
   if (sysver>=0) {
    if (!wow64) VddInit();
    exceptInit();
   }
  }break;
  case DLL_PROCESS_DETACH: {
   if (sysver>=0) {
    exceptDone();
    VddDone();
    CloseHandle(hdriver);
    if (started) start(false);		// can interfere other inpout32.dll applications if not started+ended in nested order!
   }
  }break;
 }
 return TRUE;
}

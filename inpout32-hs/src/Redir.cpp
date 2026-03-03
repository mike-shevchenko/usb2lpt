#include <windows.h>
#include <windowsx.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>	// strtoul()
#include <tchar.h>	// _tcsdup()
EXTERN_C{
#include <hidsdi.h>
#include <hidpi.h>
}
#include <setupapi.h>
#include "usb2lpt.h"
#include "Redir.h"
#include "inpout32.h"


// This source code relies on tabsize == 8

/*****************************
 * True dynamic entry points *
 *****************************/

#ifdef _M_IX86
// This header seems to require _stdcall (/Gz) compiler option!
#include <nt_vdd.h>

// Dynamic entries ensure compatibility to Windows NT 4.0 and possibly NT 3.51
struct _setupapi setupapi;
const char setupapi_names[]=
 "setupapi\0"
 "SetupDiEnumDeviceInterfaces\0"	// Win2k++, Win98++
 "SetupDiGetDeviceInterfaceDetailA\0";

struct _hid hid;
const char hid_names[]=
 "hid\0"				// Win2k++, Win98++
 "HidD_GetHidGuid\0"
 "HidD_GetPreparsedData\0"
 "HidD_GetFeature\0"
 "HidD_SetFeature\0"
 "HidD_GetAttributes\0"
 "HidP_GetCaps\0";
 
struct _krnl krnl;
const char krnl_names[]=
 "kernel32\0"
 "CreateRemoteThread\0"
 "OpenThread\0"
 "CancelIo\0"				// Win2k++
 "AddVectoredExceptionHandler\0"
 "RemoveVectoredExceptionHandler\0"
 "IsWow64Process\0"			// WinXP++ 64bit
 "Wow64DisableWow64FsRedirection\0"
 "Wow64RevertWow64FsRedirection\0";

static struct _vdm{
 HINSTANCE hLib;
 BOOL (WINAPI*InstallIOHook)(HANDLE,WORD,PVDD_IO_PORTRANGE,PVDD_IO_HANDLERS);
 VOID (WINAPI*DeInstallIOHook)(HANDLE,WORD,PVDD_IO_PORTRANGE);
}vdm;
static const char vdm_names[]=
 "\0"
 "VDDInstallIOHook\0"
 "VDDDeInstallIOHook\0";

bool _fastcall dynaload(HMODULE &hLib,const char*e) {
 if (hLib==INVALID_HANDLE_VALUE) return false;	// already failed
 if (!hLib && !(hLib=LoadLibraryA(e))) {
  hLib--;					// don't try again
  return false;					// failed
 }
 FARPROC *proc=(FARPROC*)&hLib;
 bool ret=true;
 while (++proc,*(e+=strlen(e)+1)) {
  if (!*proc && !(*proc=GetProcAddress(hLib,e))) ret=false;
 }
 return ret;					// true when all entry points are OK
}

static bool wdmHooked;
#endif

/*****************
 * Debug Console *
 *****************/

REDIR RedirInfo[9];	// 0..2: Automatic, for standard addresses, 3..9: Via OpenLpt()
struct _con{
 HANDLE hOut;
 CONSOLE_SCREEN_BUFFER_INFO csbi;
 void Init();
 void SetColor(BYTE c) const;
 void SetGreenOrRed(BOOL) const;
 void SetNormal() const;
}con;

void _con::Init() {
 AllocConsole();
 hOut=GetStdHandle(STD_OUTPUT_HANDLE);
 GetConsoleScreenBufferInfo(hOut,&csbi);
}

// Sets given foreground color for lightgray text color, with high intensity.
// For other text colors, distinct foregrounds are produced.
void _con::SetColor(BYTE c) const{
 SetConsoleTextAttribute(hOut,csbi.wAttributes
 ^(FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_INTENSITY)
 ^c);
}

void _con::SetGreenOrRed(BOOL green) const{
 SetColor(green?FOREGROUND_GREEN:FOREGROUND_RED);
}

void _con::SetNormal() const{
 SetConsoleTextAttribute(hOut,csbi.wAttributes);
}

// returns a bit array, each of three bits for a changed LPT entry in BIOS data area 40:08 .. 40:10
// See Ralf Brown Interrupt List : Memory for meanings.
// Free slots (i.e. a non-available LPT2) are not checked to be free (zero).
// The BIOS data area is used by WGif12NT.exe, at least.
static char EnsureBiosDataArea() {
 HANDLE h;
 WORD*bios=(WORD*)MapPhysToLin((void*)0x408,0x0A,&h);
 if (!bios) return -1;
 char ret=0;
 for (int i=0; i<3; i++) {
  if (RedirInfo[i].where==1) {
   if (bios[i]!=LOWORD(RedirInfo[i].addr)) {
    bios[i]=LOWORD(RedirInfo[i].addr);
    ret|=1<<i;
   }
   if (bios[4]>>14<i+1) {	// Ensure that "Installed hardware" reflects the maximum parallel port number
    bios[4]=bios[4]&0x3FFF|(i+1)<<14;
    ret|=1<<i;
   }
  }
 }
 UnmapPhysicalMemory(h,bios);
 return ret;
}

static void StartType(int typ) {
 typ=setstarttype(typ);
 static const char *starttypes[]={"BOOT","SYSTEM","AUTO","DEMAND","DISABLED"};
 if ((unsigned)typ>SERVICE_DISABLED) {
  con.SetColor(FOREGROUND_RED);
  _cprintf("Error code %d getting/setting StartType value!",typ);
 }else{
  con.SetColor(FOREGROUND_GREEN|FOREGROUND_BLUE);
  _cprintf("StartType = SERVICE_%s%s",starttypes[typ],typ!=SERVICE_DISABLED?"_START":"");
 }
 con.SetNormal();
 _cprintf("\n\n");
}

static void ShowAssignment() {
 int e=elemof(RedirInfo)-1;
 for (;e>2;e--) if (RedirInfo[e].where) break;
 for (int i=0; i<=e; i++) {
  _cprintf("LPT%d: ",i+1);
  switch (RedirInfo[i].where) {
   case 0: _cprintf("free"); break;
   case 1: _cprintf("SPP=0x%X, ECP=0x%X",LOWORD(RedirInfo[i].addr),HIWORD(RedirInfo[i].addr)); break;
   case 2: _cprintf("USB2LPT (native, \\\\.\\%" TS ")",RedirInfo[i].name); break;
   case 3: _cprintf("USB2LPT (HID, %.50" TS "...)",RedirInfo[i].name); break;
   case 4: _cprintf("USB->Prn (%.50" TS "...)",RedirInfo[i].name); break;
//   case 5: _cprintf("FT232 BitBang mode",RedirInfo[i].name); break;
//   case 6: _cprintf("V-USB PowerSwitch",RedirInfo[i].name); break;
//   case 7: _cprintf("UsbLotIo",RedirInfo[i].name); break;
   default: _cprintf("unknown (%d)",RedirInfo[i].where);
  }
  _cprintf("\n");
 }
 BOOL ok=IsInpOutDriverOpen();
 con.SetGreenOrRed(ok);
 _cprintf("\nKernel-mode driver is %s\n\n",sysver<0?"not necessary.":ok?"loaded.":"NOT loaded!");
#ifdef _M_IX86
 BOOL w64=IsXP64Bit();
 if (sysver>=0 && !w64) {
  con.SetGreenOrRed(wdmHooked);
  _cprintf("DOS box & Win16 LPT redirection is %s\n\n",wdmHooked?"active.":"NOT active!");
 }
#endif
 if (ok) {			// requires Win9x or loaded driver
  char result=EnsureBiosDataArea();
  con.SetGreenOrRed(!result);
  if (!result) {
   _cprintf("BIOS data area is OK.");
#ifdef _M_IX86
   if (!w64) _cprintf(" (Not visible to DOS boxes.)");
#endif
   _cprintf("\n");
  }else if (result<0) _cprintf("BIOS data area NOT accessible!\n");
  else for(int i=0; i<3; i++) {
   if (result&1<<i) _cprintf("BIOS data area corrected for LPT%d, set to 0x%X.\n",i+1,LOWORD(RedirInfo[i].addr));
  }
  _cprintf("\n");
 }
 con.SetNormal();
 if (sysver>=0) StartType(-1);	// here: GetStartType
}

static bool VAL(char*&s, int&v) {
 char*e;
 while (*s==' ') s++;		// skip leading spaces
 v=strtoul(s,&e,16);		// try to convert
 if (e==s) return false;	// Not a valid number (e.g. ';')
 s=e;				// Next start
 if (*s==',') s++;		// Skip one comma
 return true;
}

static bool ProcessLine(HQUEUE &q, char *s) {
 LARGE_INTEGER pf,tic,toc;
 QueryPerformanceFrequency(&pf);
 for (char c;c=*s++;) {
  if (c<'?') continue;
  int a=0,b=0;
  if (isupper(c)) c=tolower(c);
  switch (c) {	// one-parameter commands
   case 'o':
   case 'i':
   case 'l':
   case 'd':
   case 's': if (!VAL(s,a)) goto noparam;
  }
  switch (c) {	// two-parameter commands
   case 'o': if (!VAL(s,b)) goto noparam;
  }
  int i,l;
  BYTE buf[256];
  switch (c) {
   case 'o': if (q) LptOut(q,a,b); else Out32((WORD)a,(BYTE)b); break;
   case 'i': if (q) {if (!LptIn(q,a)) goto failed;} else {b=Inp32((WORD)a); _cprintf("%X\n",b);} break;
   case 'l': if (q) _cprintf("Queue already open!\n"); else q=LptOpen(a,0); if (!q) _cprintf("Open failed!\n"); break;
   case 'c': if (q) {LptClose(q); q=0;} else goto notopen; break;
   case 'd': if (q) LptDelay(q,a); else goto notopen; break;
   case 'u':
   case 'e': if (q) {
    for(i=0; i<sizeof buf; i++) {
     if (!VAL(s,b)) break;
     buf[i]=b;
    }
    QueryPerformanceCounter(&tic);
    l=LptInOut(q,buf,i,buf,sizeof buf);
    goto outbuf;
   }goto notopen;
   case 'f': if (q) {
    QueryPerformanceCounter(&tic);
    l=LptFlush(q,buf,sizeof buf);
    outbuf:
    QueryPerformanceCounter(&toc);
    if (l<0) failed: _cprintf("Command `%c' failed!\n",c);
    else if (l) {for (i=0; i<l; i++) _cprintf("%X ",buf[i]);_cprintf("\n");}
    _cprintf("Execution time: %u us\n",MulDiv(toc.LowPart-tic.LowPart,1000000,pf.LowPart));
   }else notopen: _cprintf("Not in queue mode!\n");
   break;
   case 'a': ShowAssignment(); break;
   case 'q':
   case 'x': return false;	// let exit command-line processor
   case 's': StartType(a); break;
   case '?':
   case 'h':
   _cprintf(
       "### InpOut32 debug console - all parameters are hexadecimal\n"
       "	## Direct mode ##		## Queue mode ##\n"
       "o a b	OUT byte to address		Enqueue OUT byte to offset\n"
       "i a	IN from address, emit byte	Enqueue IN from offset\n"
       "l n	Open Queue for LPT n+1		-\n"
       "d us	-				Enqueue delay in microseconds\n"
       "e ...	-				Execute microcode, emit IN results\n"
       "f	-				Flush Queue, emit IN results\n"
       "c	-				Close Queue\n"
       "a	Show LPT assignment\n"
       "%s"
       "h,?	This help\n"
       "x,q	Exit debug console\n\n",
       sysver>=0?"s n	Set service start type, n=2: AUTO_START, n=3: DEMAND_START\n":""); break;
   case 0: noparam: _cprintf("Missing parameter for `%c'!\n",c); break;
   default: _cprintf("Unknown command `%c'!\n",c);
  }
 }
 return true;	// let continue command-line processing
}

void InfoA (HWND Wnd, HINSTANCE, char *cmdLine, int) {
 HQUEUE q=0;
 con.Init();
 ShowAssignment();
 char buf[128];
 buf[0]=126;
 buf[1]=(char)lstrlenA(cmdLine);	// safe for cmdLine==nullptr (2025)
 if (buf[1]) {
  lstrcpynA(buf+2,cmdLine,126);	// prepare for editing
  con.SetColor(FOREGROUND_RED|FOREGROUND_GREEN);
  _cprintf("-%s\n",cmdLine);
  con.SetNormal();
 }else{
  buf[2]=0;
  _cprintf("Enter `q' for exiting.\n");
 }
 while (ProcessLine(q, buf+2)) {
  con.SetColor(FOREGROUND_RED|FOREGROUND_GREEN);
//  FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE)); doesn't help for the two-"-"-problem!
  _cprintf("-");	// prompt (two loops for Win7/8-64, unknown reason)
  _cgets(buf);		// get line-editable command
  con.SetNormal();
 }
 if (q) {_cprintf("Queue cleanup.\n"); LptClose(q);}
}

void WINAPI InfoW(HWND Wnd, HINSTANCE hInst, PCWSTR cmdLine, int nCmdShow) {
 char buf[128];
 WideCharToMultiByte(CP_ACP,0,cmdLine,-1,buf,elemof(buf),NULL,NULL);
 InfoA(Wnd,hInst,buf,nCmdShow);
}

static UINT wmLptGetAddr;
static WNDPROC oMsgProc;
static INT_PTR CALLBACK MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
 switch (msg) {
  case WM_CLOSE: DestroyWindow(hWnd); break;
  case WM_DESTROY: PostQuitMessage(0); break;
  default: if (msg==wmLptGetAddr) {
   DWORD LptGetAddrPnp(DWORD a[], int n);
   return LptGetAddrPnp(0,int(wParam));	// Query just ONE port address to avoid using shared memory
  }
 }
 return CallWindowProc(oMsgProc,hWnd,msg,wParam,lParam);
}

void WINAPI RunQueryWnd(HWND,HINSTANCE,void*,int) {
#ifdef _M_X64
 HWND hWnd=CreateWindowEx(0,TEXT("Message"),TEXT("LptGetAddr"),0,0,0,0,0,HWND_MESSAGE,0,0,0);
 wmLptGetAddr=RegisterWindowMessage(TEXT("LptGetAddr"));
 oMsgProc=SubclassWindow(hWnd,MsgProc);
 MSG Msg;
 while (GetMessage(&Msg,0,0,0)) {
  DispatchMessage(&Msg);
 }
#endif
}

/***************************************************************
 ** LPT address redirector, fine for PCI and PCIexpress cards **
 ***************************************************************/

WORD SppDef[3]={0x378,0x278,0x3BC};

// Change 0x378 to LOWORD(RedirInfo[0].addr), 0x778 to HIWORD(...), etc.
// 0x3BC (LPT3) is not exprected to have EPP/ECP registers, as this would break
// compatibility to some software accessing 0x3C0 (graphics card) or 0x7BC (unknown)
void patch(WORD &addr) {
 for (int i=0;; i++) {
  REDIR *info=RedirInfo+i;
  if ((WORD)(addr-SppDef[i])<(i<2?8:4) && info->where==1 /*&& LOWORD(info->addr)*/) {
   addr+=LOWORD(info->addr)-SppDef[i];
   break;
  }
  if (i==2) break;	// No ECP for 0x3BC, abort loop here
  if ((WORD)(addr-SppDef[i]-0x400)<4 && info->where==1 && HIWORD(info->addr)) {
   addr+=HIWORD(info->addr)-SppDef[i]-0x400;
   break;
  }
 }
}

#ifdef _M_IX86
/**************************************************************
 ** Redirection for DOS programs (i.e. NTVDM: DOS and Win16) **
 **************************************************************/

/* DOS+Win16 programs will also get redirected port addresses, therefore,
 * a DOS program doesn't need to know the PCI card's base address.
 * Simply use 0x378 (378h, &H378, $378) for LPT1, etc.
 * Redirection also occurs to USB->ParallelPrinter and USB2LPT adapters!
 * Moreover, USB ports doesn't need administrative privilege for access.
 * Note that ntvdm.exe only exists for x86 platform, not for amd64.
 * If a 64-bit DosBox emulator behaves like ntvdm.exe (i.e. has same
 * entry points), the #ifdef above should be removed.
 *
 * BUGBUG: ntvdm.exe seems to _always_ redirect access to the well-known
 * LPT addresses (378, 278, 3BC) to printer somehow WITHOUT ANY WORKAROUND.
 * (Except patching ntvdm.exe, of-course.)
 * Therefore, this DLL creates an alias address range which is commonly
 * free. This feature is only useful for programs where you can
 * select a nonstandard port base address.
 * To assist DOS+Win16 programs to fetch that aliases,
 * this DLL patches the well-known BIOS data area for the ntvdm process.
 */
VDD_IO_PORTRANGE portranges[6];
WORD nRanges;

static int setup_portranges() {
 portranges[0].First=0x100;		// Additional feature 141012:
 portranges[0].Last=0x10A;		// Alias port adresses for LPT1..LPT3
 int k=1;
 for(int i=0;; i++) {
  REDIR *info=RedirInfo+i;
  if (info->where) {
   portranges[k].First=SppDef[i]-1;
   portranges[k].Last=SppDef[i]+(i<2?7:3);// with / without EPP data
   k++;
  }
  if (i==2) break;
  if (info->where) {
   portranges[k].First=SppDef[i]+0x400;
   portranges[k].Last=SppDef[i]+0x407;	// with ECP (and USB2LPT direction registers)
   k++;
  }
 }
 nRanges=(WORD)k;
 return k;
}

static void VddInbHandler(WORD addr,BYTE*data) {
 if ((addr&~0xF)==0x100) {	// ersatzweises LPT1-LPT3 für DOS-Box auf 100 (LPT1), 104 (LPT2), 108 (LPT3)
  addr=(addr&3)+SppDef[addr>>2&3];
 }
 *data=Inp32(addr);
}

static void VddOutbHandler(WORD addr, BYTE data) {
 if ((addr&~0xF)==0x100) {	// als Workaround für fix gecapture Druckerportadressen
  addr=(addr&3)+SppDef[addr>>2&3];// (DataIO 2700 Programmiergerät)
 }
 Out32(addr,data);
}

void VddInit() {
 static const VDD_IO_HANDLERS IOHandler={
  VddInbHandler,NULL,NULL,NULL,
  VddOutbHandler,NULL,NULL,NULL
 };
 vdm.hLib=GetModuleHandle(NULL);	// get calling process (ntvdm.exe, or any third-party program that exports VDDInstallIoHook() etc.)
 if (dynaload(vdm.hLib,vdm_names)	// if not ntvdm.exe, this will fail because entries won't exist
 && setup_portranges()
 && vdm.InstallIOHook(hInstance,nRanges,portranges,(PVDD_IO_HANDLERS)&IOHandler)) {
  _asm{
	push	es
	push	edi
	 mov	di,0x40
	 mov	es,di		// access BIOS data area (of current process??) - still valid in Win32
	 mov	edi,8
	 push	3
	 pop	ecx
	 xor	eax,eax
	 inc	ah		// 0x100
l1:	 stosw			// patch 3 words
	 add	eax,4
	 loop	l1
	pop	edi
	pop	es
  }
#ifdef _DEBUG
  MessageBeep(MB_OK);
 }else{
  MessageBox(0,T("inpout32.dll: Failed to install IO Hook!"),NULL,MB_OK|MB_ICONEXCLAMATION|MB_APPLMODAL);
#endif
 }
}

void VddDone() {
 if (nRanges) vdm.DeInstallIOHook(hInstance,nRanges,portranges);
//   MessageBeep(MB_ICONSTOP);
}

// Installs globally for all DOS programs, which is mostly intended
// This procedure requires administrative privileges on Vista or newer,
// otherwise, the registry access is redirected to a wrong place.
// (And RegOpenKeyEx() should fail silently in this case.)
void InstallVdmHook() {
 TCHAR self[MAX_PATH],c;
 GetModuleFileName(hInstance,self,elemof(self));
 c=self[3];
 self[3]=0;
 if (GetDriveType(self)!=DRIVE_FIXED) return;	// ab XP?
 self[3]=c;
 HKEY hKey;
 if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE,T("System\\CurrentControlSet\\Control\\VirtualDeviceDrivers"),0,
   KEY_QUERY_VALUE|KEY_SET_VALUE,&hKey)) {
  TCHAR *selfname=_tcsrchr(self,'\\')+1;	// "inpout32.dll" or "inpoutx64.dll" - or somehow renamed
  TCHAR buf[1500],*p=buf,*q,*r,*e;
  DWORD len=sizeof(buf);
  if (!RegQueryValueEx(hKey,T("VDD"),NULL,NULL,(PBYTE)buf,&len)) {
   e=buf+len/sizeof(TCHAR);	// behind double-zero
   for (p=buf; *p; p=q) {	// traverse each entry
    if (!lstrcmpi(p,self)) {wdmHooked=true; goto raus;}	// already installed, do nothing
    q=p+_tcslen(p)+1;		// next entry
    r=_tcsrchr(p,'\\');		// filename portion
    if (!r) r=_tcsrchr(p,'/');	// possibly UNIX style here
    if (r) r++; else r=p;	// with no path, take entire entry as file name
    if (!lstrcmpi(r,selfname)) {	// this DLL on another place?
     memmove(p,q,(BYTE*)e-(BYTE*)q);	// Remove this entry! (Move trailing data to lower addresses)
     e-=q-p;			// Adjust end address
     q=p;			// Adjust next-entry address
     continue;
    }
   }
  }
  lstrcpyn(p,self,buf+elemof(buf)-p-1);	// Append
  e=p+_tcslen(p)+1;
  *e++=0;	// now double-terminated
  if (!RegSetValueEx(hKey,T("VDD"),0,REG_MULTI_SZ,(PBYTE)buf,(e-buf)*sizeof(TCHAR))) wdmHooked=true;
raus:
  RegCloseKey(hKey);
 }
}
#endif

// Dummy entry! DLL initialization is the workhorse.
BOOL WINAPI VDDInitialize(HANDLE DllHandle,ULONG Reason,PCONTEXT Context) {
 return TRUE;
}

/*****************************************************
 ** Automated full redirector for Inp32() / Out32() **
 *****************************************************/

// Fill residual free slots of RedirInfo[]
static bool InsertDevice(BYTE where, PTSTR name) {
 for (int i=0; i<elemof(RedirInfo); i++) {
  if (!RedirInfo[i].where) {	// found free slot
   RedirInfo[i].where=where;
   RedirInfo[i].name=_tcsdup(name);
   return true;
  }
 }
 return false;			// couldn't insert
}

static bool AddUsb2Lpt(PTSTR name) {
// As FindDevices will enumerate all HID devices here
// (at least non-mice, non-keyboards, and non-joysticks)
// filter for USB2LPT devices here
 bool ret=true;	// let continue enumeration
 HANDLE h=CreateFile(name,0,0,NULL,OPEN_EXISTING,0,NULL);
 if (h!=INVALID_HANDLE_VALUE) {
  HIDD_ATTRIBUTES attr;
  if (HidD_(GetAttributes(h,&attr))) {
   if (attr.VendorID==0x16C0		// VOTI (5824)
   && (WORD)(attr.ProductID-1715)<5) {	// siphec -> h#s
    ret=InsertDevice(3,name);
   }
  }
  CloseHandle(h);
 }
 return ret;
}

static bool AddUsbPrn(PTSTR name) {
// Take any USB->ParallelPrinter adapter
 return InsertDevice(4,name);
}
 
static void FindDevices(const GUID*guid, bool(*FilterProc)(PTSTR)) {
 HDEVINFO devs;
 devs=SetupDiGetClassDevs(guid,0,0,DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);
 if (devs!=INVALID_HANDLE_VALUE) {
  SP_DEVICE_INTERFACE_DATA devinterface;
  devinterface.cbSize=sizeof(SP_DEVICE_INTERFACE_DATA);
  for (int i=0; SetupDi(EnumDeviceInterfaces(devs,NULL,guid,i,&devinterface)); i++) {
   SP_DEVINFO_DATA devinfo;
   struct{
    SP_DEVICE_INTERFACE_DETAIL_DATA id;
    TCHAR space[MAX_PATH];
   }id;
   devinfo.cbSize = sizeof(SP_DEVINFO_DATA);
   id.id.cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
   if (SetupDi(GetDeviceInterfaceDetail(devs, &devinterface, &id.id, sizeof(id), NULL, &devinfo))) {
    if (!FilterProc(id.id.DevicePath)) break;
   }
  }
  SetupDiDestroyDeviceInfoList(devs);
 }
}
 
void InitRedirector() {
 // Traverse true parallel ports and USB2LPT devices first
 DWORD a[9];
 ZeroMemory(a,sizeof(a));
 LptGetAddr(a,elemof(a));	// Collect true LPT ports
 for (int n=0; n<9; n++) {
  if (a[n]) {
   RedirInfo[n].where=1;
   RedirInfo[n].addr=a[n];
   continue;
  }
  TCHAR s[16];
  _sntprintf(s,elemof(s),T("LPT%d"),n+1);
  HANDLE hDev=CreateFile(s,0,0,NULL,OPEN_EXISTING,0,NULL);
  if (hDev!=INVALID_HANDLE_VALUE) {
   DWORD e,bw;			// Check whether it's a USB2LPT
   if (DeviceIoControl(hDev,IOCTL_VLPT_GetLastError,NULL,0,&e,sizeof(e),&bw,NULL)) {
    RedirInfo[n].where=2;
    RedirInfo[n].name=_tcsdup(s);
   }
   CloseHandle(hDev);
  }
 }
 // Fill gaps with available USB2LPT (HID) and USB->PRN adapters
 if (!dynaload(setupapi.hLib,setupapi_names)) return;
 if (!dynaload(hid.hLib,hid_names)) return;
 dynaload(krnl.hLib,krnl_names);		// load CancelIo() - not available on Win95
 // Typically, a PCI card defaults to LPT3, so a USB->PRN adapter can be used as LPT2 here
 GUID guid;
 HidD_(GetHidGuid(&guid));
 FindDevices(&guid,AddUsb2Lpt);
 static const GUID GUID_DEVINTERFACE_USBPRINT = {
   0x28d78fad,0x5a12,0x11D1,0xae,0x5b,0x00,0x00,0xf8,0x03,0xa8,0xc2};
 FindDevices(&GUID_DEVINTERFACE_USBPRINT,AddUsbPrn);
}

static QUEUE* RedirQueue[3];

static QUEUE* RedirCandidate(WORD addr, BYTE &offset) {
 for (int i=0; i<3; i++) {
  if (RedirInfo[i].where>=2) {
   WORD diff=addr-SppDef[i];
   if ((WORD)(diff&~0x400)<8) {
    if (!RedirQueue[i]) RedirQueue[i]=LptOpen(i,0);
    if (!RedirQueue[i]) return false;
    offset=(BYTE)diff | diff>>7&8;
    return RedirQueue[i];
   }
  }
 }
 return NULL;
}

bool RedirOut(WORD addr, BYTE data) {
 BYTE ucode[2];
 QUEUE *q=RedirCandidate(addr,ucode[0]);
 if (!q) return false;
 ucode[1]=data;
 LptInOut(q,ucode,2,NULL,0);	// no result checking here
 return true;
}

bool RedirIn(WORD addr, BYTE &data) {
 QUEUE *q=RedirCandidate(addr,data);
 if (!q) return false;
 data|=0x10;			// IN instruction
 LptInOut(q,&data,1,&data,1);
 return true;
}

BOOL ExitPortTalk() {
 for (int i=0; i<elemof(RedirQueue); i++) {
  LptClose(RedirQueue[i]);
  RedirQueue[i]=0;
 }
 return TRUE;
}

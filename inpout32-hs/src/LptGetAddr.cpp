#include <windows.h>
#include <shlwapi.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <devguid.h>
#include <stdio.h>
#include <tchar.h>
#include "redir.h"

// Gets base address from PnP service, LPT1 -> n=0, etc.
// LOWORD = SPP address, HIWORD = ECP address
// Into an array of size n if a given
// Surprisingly, it works for NT4 too!
// It should work on Win95 too, but SetupDiEnumDeviceInfo() returns FALSE
// and GetLastError() returns 0x103 == ERROR_NO_MORE_ITEMS. (Win95C)
DWORD LptGetAddrPnp(DWORD a[], int n) {
 DWORD Count=0;
 HDEVINFO Devs=SetupDiGetClassDevs(&GUID_DEVCLASS_PORTS,NULL,0,DIGCF_PRESENT);
 if (Devs!=INVALID_HANDLE_VALUE) {
  SP_DEVINFO_DATA devInfo;
  int Index,j;
  devInfo.cbSize=sizeof(devInfo);
  for (j=0; SetupDiEnumDeviceInfo(Devs,j,&devInfo); j++) {
   HKEY hKey;
   TCHAR val[16];
   DWORD valsize=sizeof(val);
   RES_DES resDes=0;
   IO_RESOURCE ior;
   DWORD addr=0;
   if (CM_Open_DevNode_Key(devInfo.DevInst,KEY_QUERY_VALUE,0,RegDisposition_OpenExisting,&hKey,CM_REGISTRY_HARDWARE)) continue;
   if (!RegQueryValueEx(hKey,T("PortName"),NULL,NULL,(LPBYTE)val,&valsize)
   && (_stscanf(val,T("LPT%d"),&Index)==1)
   && (unsigned)--Index<256
   && (a || Index==n)) {
    LOG_CONF Config;
    if (!CM_Get_First_Log_Conf(&Config,devInfo.DevInst,ALLOC_LOG_CONF)) {	// Returns 0x34 on Win8
     while (!CM_Get_Next_Res_Des(&resDes,Config,ResType_IO,NULL,0)) {
      CM_Get_Res_Des_Data(resDes,&ior,sizeof(ior),0);
      if (addr) {
       addr |= (WORD)ior.IO_Header.IOD_Alloc_Base<<16;	// ECP address (second entry)
       break;
      }else addr = (WORD)ior.IO_Header.IOD_Alloc_Base;	// EPP address (first entry)
      Config = resDes;
     }
     CM_Free_Res_Des_Handle(resDes);
    }
   }
   RegCloseKey(hKey);
   if (addr) {		// found an address?
    if (a) {		// array mode?
     if (Index<n) a[Index]=addr;	// write to array
     Count++;		// count LPT ports
    }else{		// single mode
     Count=addr;	// return address
     break;		// exit loop
    }
   }
  }/*for*/
  SetupDiDestroyDeviceInfoList(Devs);
 }
 return Count;
}

// This is merely a workaround for the Windows 8 bug stated above.
// In newer documentation, Microsoft says this were a feature:
// CM_Get_First_Log_Conf() is supported for 64 bit processes only.
// This routine will work for all Windows from 2000 and newer.
// As you can see, I have a decent reason to indent by only one space.
// -141022: This new implementation doesn't rely on known service names anymore
//	As a disadvantage, it contains three nested loops for GUID+PortName scan
static DWORD LptGetAddrNT(DWORD a[], int n) {
 DWORD Count=0;		// possible return value (can be greater than n)
 TCHAR buf[MAX_PATH];	// handy stack-allocated multi-purpose buffer
 
 HKEY hEnum;		// Open the registry (first stage)
 if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE,T("SYSTEM\\CurrentControlSet\\Enum"),0,
   KEY_QUERY_VALUE|KEY_ENUMERATE_SUB_KEYS,&hEnum)) {
// first-level enumeration
  DWORD size;
  for (DWORD i1=0; size=elemof(buf),
    !RegEnumKeyEx(hEnum,i1,buf,&size,NULL,NULL,NULL,NULL); i1++) {
   HKEY hK1;
   if (!RegOpenKeyEx(hEnum,buf,0,KEY_QUERY_VALUE|KEY_ENUMERATE_SUB_KEYS,&hK1)) {
// second-level enumeration
    for (DWORD i2=0; size=elemof(buf),
      !RegEnumKeyEx(hK1,i2,buf,&size,NULL,NULL,NULL,NULL); i2++) {
     HKEY hK2;
     if (!RegOpenKeyEx(hK1,buf,0,KEY_QUERY_VALUE|KEY_ENUMERATE_SUB_KEYS,&hK2)) {
// third-level enumeration
      for (DWORD i3=0; size=elemof(buf),
        !RegEnumKeyEx(hK2,i3,buf,&size,NULL,NULL,NULL,NULL); i3++) {
       DWORD addr=0;
       int Index;
       HKEY hPath;
       if (!RegOpenKeyEx(hK2,buf,0,KEY_QUERY_VALUE,&hPath)) {
// check for right ClassGUID
        size=sizeof buf;
#ifdef CHECKGUID
        if (!RegQueryValueEx(hPath,T("ClassGUID"),NULL,NULL,(PBYTE)buf,&size)
        && !_tcsicmp(buf,T("{4d36e978-e325-11ce-bfc1-08002be10318}")))
#else
        if (!RegQueryValueEx(hPath,T("Class"),NULL,NULL,(PBYTE)buf,&size)
        && !_tcsicmp(buf,T("Ports")))
#endif
	{
         HKEY hParams;
         if (!RegOpenKeyEx(hPath,T("Device Parameters"),0,KEY_QUERY_VALUE,&hParams)) {
          HKEY hControl;
          size=sizeof buf;
          if (!RegQueryValueEx(hParams,T("PortName"),NULL,NULL,(PBYTE)buf,&size)
          && _stscanf(buf,T("LPT%d"),&Index)==1
          && (unsigned)--Index<256	// Use zero-based index from here, avoid negative values
          && (a || Index==n)
          && !RegOpenKeyEx(hPath,T("Control"),0,KEY_QUERY_VALUE,&hControl)) {
           size=sizeof buf;
           if (!RegQueryValueEx(hControl,T("AllocConfig"),NULL,NULL,(PBYTE)buf,&size)) {
// This undocumented AllocConfig structure is checked against Win2k and Win8/64.
// In both cases, the first ResType entry was at byte offset 16.
            DWORD *p=(DWORD*)buf;
            DWORD marker=0;
            int k;
            for (k=0; k<(int)size/4-5; k++,p++) {
             if (marker) {
              if (p[0]==marker
              && !HIWORD(p[1])		// port address less than 64K
              && !p[2]			// no high DWORD part
              && p[3]<16) {		// length limited to 16
               if (addr) {
                addr|=p[1]<<16;		// ECP address
                break;
               }else addr=p[1];		// SPP address
               p+=3;
               k+=3;			// eat DWORDs
              }
             }else if (p[0]==0x00010001	// seems like ResType_IO
             && !HIWORD(p[1])		// some small number
             && p[2]			// marker DWORD available
             && !HIWORD(p[3])) {	// port address less than 64K
              marker=p[2];
             }
            }
           }
           RegCloseKey(hControl);
          }
          RegCloseKey(hParams);
         }
        }
        RegCloseKey(hPath);
       }
       if (addr) {
        if (a) {
         if (Index<n) a[Index]=addr;
         Count++;
        }else{
         RegCloseKey(hK2);
         RegCloseKey(hK1);
         RegCloseKey(hEnum);
         return addr;
        }
       }
      }
      RegCloseKey(hK2);
     }
    }
    RegCloseKey(hK1);
   }
  }
  RegCloseKey(hEnum);
 }
 return Count;
}

#ifdef _M_IX86

#ifdef UNICODE
#define LptGetAddr95(a,n) 0
#else
// This routine will work for Windows 95, 98, and Me,
// but is used only for Windows 95 here, as Windows 98+ can use SetupDi functions
static DWORD LptGetAddr95(DWORD a[], int n) {
 char buf[MAX_PATH];		// multi-purpose buffer
 HKEY hEnumDD,hEnumLM;
 DWORD Count=0;
   // Open the registry
 if (!RegOpenKeyEx(HKEY_DYN_DATA,"Config Manager\\Enum",0,KEY_ENUMERATE_SUB_KEYS,&hEnumDD)) {
  if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE,"Enum",0,KEY_QUERY_VALUE,&hEnumLM)) {
   FILETIME ft;
   DWORD Length;
   DWORD i;
   for (i=0; Length=elemof(buf),!RegEnumKeyEx(hEnumDD,i,buf,&Length,NULL,NULL,NULL,&ft); i++) {
    int Index;
    DWORD addr=0;
    HKEY hEntry;
    if (!RegOpenKeyEx(hEnumDD,buf,0,KEY_QUERY_VALUE,&hEntry)) {
     DWORD Problem=0;
     Length=sizeof Problem;
     RegQueryValueEx(hEntry,"Problem",NULL,NULL,(PBYTE)&Problem,&Length);
     if (!Problem) {
      HKEY hDevInfo;
      Length=sizeof buf;
      if (!RegQueryValueEx(hEntry,"HardwareKey", NULL,NULL,(PBYTE)buf,&Length)
      && !RegOpenKeyEx(hEnumLM,buf,0,KEY_QUERY_VALUE,&hDevInfo)) {
       Length=sizeof buf;
       if (!RegQueryValueEx(hDevInfo,"PortName",NULL,NULL,(PBYTE)buf,&Length)
       && sscanf(buf,"LPT%d",&Index)==1
       && (unsigned)--Index<256
       && (a || Index==n)
       && !(Length=sizeof(buf),RegQueryValueEx(hEntry,"Allocation",NULL,NULL,(PBYTE)buf,&Length))) {
        WORD *p=(WORD*)buf;	// Decode the Allocation data: the port address is present
        int k;			// directly after a 0x000C entry (which doesn't have 0x0000 after it).
        for (k=0; k<(int)Length/2-2; k++,p++) {
         if (p[0]==0x000C && p[1]) {	// there is an I/O address
          if (addr) {			// (which is possibly NOT correct because 0x0C may describe ISA!!)
           addr|=p[1]<<16;	// assume ECP address
           break;
          }else addr=p[1];	// SPP address
          p+=2;			// skip start and end address
          k+=2;
         }
        }
       }
       RegCloseKey(hDevInfo);
      }
     }
     RegCloseKey(hEntry);
    }
    if (addr) {
     if (a) {
      if (Index<n) a[Index]=addr;
      Count++;
     }else{
      Count=addr;
      break;
     }
    }
   }/*for*/
   RegCloseKey(hEnumLM);
  }
  RegCloseKey(hEnumDD);
 }
 return Count;
}
#endif

// This is the routine for NT4 and possibly NT3.51
static DWORD LptGetAddrNT4(DWORD a[], int n) {
 DWORD Count=0;
 HKEY hHdw;
 if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE,T("HARDWARE"),0,KEY_ENUMERATE_SUB_KEYS|KEY_QUERY_VALUE,&hHdw)) {
  HKEY hPP;
  TCHAR key[128],buf[128];
  DWORD kLength,Length;
  if (!RegOpenKeyEx(hHdw,T("DEVICEMAP\\PARALLEL PORTS"),
    0,KEY_ENUMERATE_SUB_KEYS|KEY_QUERY_VALUE,&hPP)) {
   int i;
   for (i=0;
     kLength=elemof(key), Length=sizeof buf,
     !RegEnumValue(hPP,i,key,&kLength,NULL,NULL,(PBYTE)buf,&Length);
     i++) {
    int Index,i0;
    DWORD addr=0;
    if (_stscanf(buf,T("\\DosDevices\\LPT%d"),&Index)==1
    && (unsigned)--Index<256
    && _stscanf(key,T("\\Device\\Parallel%d"),&i0)) {
     HKEY hRes;
     if (!RegOpenKeyEx(hHdw,T("RESOURCEMAP\\LOADED PARALLEL DRIVER RESOURCES\\Parport"),
       0,KEY_QUERY_VALUE,&hRes)) {
      _sntprintf(key,elemof(key),T("\\Device\\ParallelPort%d.Raw"),i0);
      Length=sizeof buf;
      if (!RegQueryValueEx(hRes,key,NULL,NULL,(PBYTE)buf,&Length)
      && Length>=26) addr=((WORD*)buf)[12];
      RegCloseKey(hRes);
     }
    }
    if (addr) {
     if (a) {
      if (Index<n) a[Index]=addr;
      Count++;
     }else{
      Count=addr;
      break;
     }
    }
   }/*for*/
   RegCloseKey(hPP);
  }
  RegCloseKey(hHdw);
 }
 return Count;
}   

// Get redirection data from 64-bit subprocess
static DWORD LptGetAddrIpc(DWORD a[], int n) {
 PROCESS_INFORMATION pi;
 ZeroMemory(&pi,sizeof pi);
 HWND hWin64=FindWindowEx(HWND_MESSAGE,0,TEXT("Message"),TEXT("LptGetAddr"));
 if (!hWin64) {		// vor allem zu Debuggen
  TCHAR echse[MAX_PATH],cmdline[MAX_PATH*2],s0[MAX_PATH],s1[MAX_PATH];
  const TCHAR*extrasearch[3]={s0,s1};
  GetCurrentDirectory(elemof(s0),s0);
  PathAppend(s0,TEXT("..\\x64"));
  GetWindowsDirectory(s1,elemof(s1));
  PathAppend(s1,TEXT("sysnative"));	// „secret“ alias to get x64 version of System32 directory
  lstrcpy(echse,TEXT("crundll.exe"));	// DON'T SEARCH crundll.exe in current path because there is likely the 32-bit version!
  if (!PathFindOnPath(echse,extrasearch) && !PathFindOnPath(echse,0)) {
   lstrcpy(echse,TEXT("rundll32.exe"));
   if (!PathFileExists(echse) && !PathFindOnPath(echse,extrasearch) && !PathFindOnPath(echse,0)) {
    _cprintf("%"TS" not found",echse);
    return 0;
   }
  }
  lstrcpy(cmdline,PathFindFileName(echse));
  int k=lstrlen(cmdline);
  cmdline[k++]=' ';
  lstrcpy(cmdline+k,TEXT("inpoutx64.dll"));
  if (!PathFileExists(cmdline+k) && !PathFindOnPath(cmdline+k,extrasearch) && !PathFindOnPath(cmdline+k,0)) return 0;
  StrCatBuff(cmdline,TEXT(",RunQueryWnd"),elemof(cmdline));
  STARTUPINFO si;
  ZeroMemory(&si,sizeof si);
  si.cb=sizeof si;
  if (!CreateProcess(echse,cmdline,0,0,0,0,0,0,&si,&pi)) return 0;
  while (!(hWin64=FindWindowEx(HWND_MESSAGE,0,TEXT("Message"),TEXT("LptGetAddr")))) {
   if (!WaitForSingleObject(pi.hProcess,100)) {
    _cprintf("premature end of process %"TS,cmdline);
    CloseHandle(pi.hProcess);
    return 0;
   }
// Das passiert wenn crundll.exe bzw. rundll32.exe als 32-Bit-Version gestartet wurde
// oder (bei einer alten Version von inpoutx64.dll) der Einsprungpunkt nicht gefunden wurde.
  }
 }
 UINT wmLptGetAddr=RegisterWindowMessage(TEXT("LptGetAddr"));
 int ret=0;
 for(;ret<n;ret++) {
  DWORD addr=SendMessage(hWin64,wmLptGetAddr,ret,0);
  if (!addr || !DWORD(~addr)) {
   if (ret>=3) break;			// Mindestens LPT1..LPT3 versuchen, alles andere muss zusammenhängend sein
  }else a[ret]=addr;
 }
 if (pi.hProcess) {
  PostMessage(hWin64,WM_CLOSE,0,0);	// Messagefenster beenden
  if (WaitForSingleObject(pi.hProcess,200)) TerminateProcess(pi.hProcess,0);
  CloseHandle(pi.hProcess);
 }
 return ret;
}

#endif

// This single entry tries to detect LPT port addresses
// at first using documented PnP dervices.
// On failure, it will use old-style, undocumented registry parsing.
// All four detechtion methods above share the same signature.
DWORD WINAPI LptGetAddr(DWORD a[], int n) {
 DWORD ret=LptGetAddrPnp(a,n);	// Works for Win98,Me,2k,XP,Vista,7,surely 2k3, 2k8
 if (!ret) {
#ifdef _M_IX86
  if (sysver<0) ret=LptGetAddr95(a,n);	// Windows 95 only (not suitable for Win32s)
  else if ((BYTE)sysver<5) ret=LptGetAddrNT4(a,n);	// (NT4), NT3.51
  else if (wow64 && MAKEWORD(HIBYTE(sysver),LOBYTE(sysver))>=0x602) ret=LptGetAddrIpc(a,n);
  else
#endif
       ret=LptGetAddrNT(a,n);		// Here: for Windows 8
 }
 return ret;
}

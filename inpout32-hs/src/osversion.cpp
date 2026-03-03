#include "inpout32.h"
#include "redir.h"

#ifdef _M_IX86
#include "hwinterfacedrv.h"

//Purpose: Return TRUE if we are running in WOW64 (i.e. a 32bit process on XP x64 edition)
//Must be called first!
BOOL WINAPI IsXP64Bit() {
 BOOL bIsWow64 = FALSE;
 if (dynaload(krnl.hLib,krnl_names))
   krnl.IsWow64Process(GetCurrentProcess(),&bIsWow64);
 return bIsWow64;
}

BOOL WINAPI DisableWOW64(PVOID* oldValue) {
 return krnl.Wow64Disable(oldValue);
}

BOOL WINAPI RevertWOW64(PVOID oldValue) {
 return krnl.Wow64Revert(oldValue);
}

#else
BOOL WINAPI IsXP64Bit() {
 return TRUE;
}
BOOL WINAPI DisableWOW64(PVOID*) {
 return TRUE;
}
BOOL WINAPI RevertWOW64(PVOID) {
 return TRUE;
}
#endif//_M_IX86

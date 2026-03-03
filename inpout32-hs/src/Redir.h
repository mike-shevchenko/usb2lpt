#pragma once
#include <conio.h>
#ifdef UNICODE
# define TS "S"
#else
# define TS "s"
#endif

#ifdef _M_IX86
#include <setupapi.h>
#include <cfgmgr32.h>
EXTERN_C{
#include <hidsdi.h>
}

extern struct _setupapi{
 HMODULE hLib;
 BOOL (WINAPI*EnumDeviceInterfaces)(HDEVINFO,PSP_DEVINFO_DATA,LPCGUID,DWORD,PSP_DEVICE_INTERFACE_DATA);
 BOOL (WINAPI*GetDeviceInterfaceDetail)(HDEVINFO,PSP_DEVICE_INTERFACE_DATA,PSP_DEVICE_INTERFACE_DETAIL_DATA,DWORD,PDWORD,PSP_DEVINFO_DATA);
}setupapi;
extern const char setupapi_names[];

extern struct _hid{
 HMODULE hLib;
 void (WINAPI*GetHidGuid)(LPGUID);
 BOOLEAN (WINAPI*GetPreparsedData)(HANDLE,PHIDP_PREPARSED_DATA*);
 BOOLEAN (WINAPI*GetFeature)(HANDLE,PVOID,ULONG);
 BOOLEAN (WINAPI*SetFeature)(HANDLE,PCVOID,ULONG);
 BOOLEAN (WINAPI*GetAttributes)(HANDLE,PHIDD_ATTRIBUTES);
 NTSTATUS (WINAPI*GetCaps)(PHIDP_PREPARSED_DATA,PHIDP_CAPS);
}hid;
extern const char hid_names[];

extern struct _krnl{
 HINSTANCE hLib;
 HANDLE (WINAPI*CreateRemoteThread)(HANDLE,LPSECURITY_ATTRIBUTES,SIZE_T,LPTHREAD_START_ROUTINE,LPVOID,DWORD,LPDWORD);
 HANDLE (WINAPI*OpenThread)(DWORD,BOOL,DWORD);
 BOOL (WINAPI*CancelIo)(HANDLE);
 PVOID (WINAPI*AddVectoredExceptionHandler)(BOOL,LONG(WINAPI*)(PEXCEPTION_POINTERS));
 ULONG (WINAPI*RemoveVectoredExceptionHandler)(LONG(WINAPI*)(PEXCEPTION_POINTERS));
 BOOL (WINAPI*IsWow64Process)(HANDLE,PBOOL);
 BOOL (WINAPI*Wow64Disable)(PVOID*);
 BOOL (WINAPI*Wow64Revert)(PVOID);
}krnl;
extern const char krnl_names[];

#define SetupDi(x) setupapi.x
#define HidD_(x) hid.x
#define HidP_(x) hid.x
#define KRNL(x) krnl.x

/* The first parameter gets an instance handle of the DLL of the first string,
 * followed by more or less function pointers to functions inside this DLL,
 * defined by the strings that follow the first string.
 * The second IN parameter is a double-zero-terminated ASCII string list.
 * The first is the DLL name (.dll can be omitted),
 * all others are import names.
 * On call for given HMODULE, returns false if one of entries is not available
 * HMODULE gets -1 on LoadLibraryA() failure.
 * The caller may set HMODULE manually, and then load the function pointers only.
 * In this case, the first string of the second parameter will be ignored.
 */
bool _fastcall dynaload(HMODULE&,const char*);
extern long sysver;
extern BOOL wow64;
extern HINSTANCE hInstance;

void InstallVdmHook();
void VddInit();
void VddDone();

#else

#define SetupDi(x) SetupDi##x
#define HidD_(x) HidD_##x
#define HidP_(x) HidP_##x
#define KRNL(x) x
#define dynaload(a,b) TRUE
#define sysver 5
#define InstallVdmHook() (void)0
#define VddInit() (void)0
#define VddDone() (void)0

#endif

void InitRedirector();
void patch(WORD &addr);
int setstarttype(int starttype);

/*****************************************************
 ** LPT access redirector, for USB->PRN and USB2LPT **
 *****************************************************/

bool RedirOut(WORD addr, BYTE data);
bool RedirIn(WORD addr, BYTE &data);

#define elemof(x) (sizeof(x)/sizeof(*(x)))
#define T(x) TEXT(x)

struct REDIR{
 BYTE where;	// 0 = free, 1 = true LPT port, 2 = USB2LPT native, 3 = USB2LPT HID, 4 = USB->PRN
 union{
  DWORD addr;	// address (ECP/EPP)
  LPTSTR name;	// USB device name (crypitc, strdup() generated)
 };
};

extern REDIR RedirInfo[9];	// 0..2: Automatic, for standard addresses, 3..9: Via LptOpen()
extern WORD SppDef[3];

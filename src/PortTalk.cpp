#include <windows.h>
#include <winioctl.h>
#include <shlwapi.h>
#include <setupapi.h>
extern "C"{
#include <hidsdi.h>
#include <hidpi.h>
}
#define T(x) TEXT(x)

// Opens the n'th (zero-based) HID device that has VID=16C0, PID=06B3 (h#s USB2LPT)
// Use CloseHandle() to free the handle returned.
// Returns 0 when failing.
static HANDLE OpenUsbHid(int n) {
 HANDLE h=0;
 DWORD i;
 HDEVINFO devs;
 GUID hidGuid;		// GUID for HID driver

 HidD_GetHidGuid(&hidGuid);
 devs = SetupDiGetClassDevs(&hidGuid, NULL, 0, DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);
 if (devs==INVALID_HANDLE_VALUE) return h;

 for (i=0;;i++) {
  DWORD size = 0;
  PSP_DEVICE_INTERFACE_DETAIL_DATA interface_detail;
  union{
   struct{
    SP_DEVICE_INTERFACE_DATA devinterface;
    SP_DEVINFO_DATA devinfo;
   };
   HIDD_ATTRIBUTES deviceAttributes;
   WCHAR productName[32];
  }s;	// some shared buffers not used the same time (does the optimizer see that?)

  if (h) CloseHandle(h), h=0;
  s.devinterface.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
  if (!SetupDiEnumDeviceInterfaces(devs, 0, &hidGuid, i, &s.devinterface)) break;
	// See how large a buffer we require for the device interface details
  SetupDiGetDeviceInterfaceDetail(devs, &s.devinterface, NULL, 0, &size, 0);
  s.devinfo.cbSize = sizeof(SP_DEVINFO_DATA);
  interface_detail = (PSP_DEVICE_INTERFACE_DETAIL_DATA)LocalAlloc(LPTR,size);
  if (!interface_detail) continue;
  interface_detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
  s.devinfo.cbSize = sizeof(SP_DEVINFO_DATA);
  if (!SetupDiGetDeviceInterfaceDetail(devs, &s.devinterface, interface_detail, size, 0, &s.devinfo)) {
   LocalFree(interface_detail);	// ignore this entry in case of error
   continue;
  }
  h = CreateFile(interface_detail->DevicePath, GENERIC_READ|GENERIC_WRITE,
    FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
  LocalFree(interface_detail);
  if (h == INVALID_HANDLE_VALUE) h = 0;
  if (!h) continue;
  if (!HidD_GetAttributes(h, &s.deviceAttributes)) continue;
  if (*(DWORD*)&s.deviceAttributes.VendorID != 0x06B316C0) continue;	// not mine *)
  if (!n) break;	// found my device
  n--;			// iterate to my next device
 }
 SetupDiDestroyDeviceInfoList(devs);
 return h;
}

// Annahme über USB2LPT: Es ist LPT2 oder LPT1
// Bei PowerSDR muss als Adresse 378h eingestellt sein.
// Es findet kein Cacheing der OUT-Befehle statt.
// TODO: Ein HID-kompatibles Zusatzinterface entschärft das
// Treibersignierungsproblem unter x64-Systemen

HANDLE hAccess;
bool HidMode;
HIDP_CAPS HidCaps;

EXTERN_C _declspec(dllexport) void InitPortTalk(void) {
 hAccess=OpenUsbHid(0);
 if (hAccess) {
  HidMode=true;
  PHIDP_PREPARSED_DATA pd;
  HidD_GetPreparsedData(hAccess,&pd);
  HidP_GetCaps(pd,&HidCaps);
  HidD_FreePreparsedData(pd);
  return;
 }
 hAccess=CreateFile(T("LPT2"),GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,0);
 if (hAccess==INVALID_HANDLE_VALUE)
   hAccess=CreateFile(T("LPT1"),GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,0);
 if (hAccess==INVALID_HANDLE_VALUE) MessageBox(0,T("No access to USB2LPT!"),NULL,0);
}

EXTERN_C _declspec(dllexport) void ExitPortTalk(void) {
 CloseHandle(hAccess); HidMode=false;
}

EXTERN_C _declspec(dllexport) BYTE inport(WORD a) {
 BYTE IoData[8];
 IoData[0]=1;
 IoData[1]=(a-0x378)|0x10;	// Lese-Bit
 if (HidMode) {
  HidD_SetFeature(hAccess,IoData,HidCaps.FeatureReportByteLength);
  HidD_GetFeature(hAccess,IoData,HidCaps.FeatureReportByteLength);
 }else{
  DWORD BytesRet;
  DeviceIoControl(hAccess,
    CTL_CODE(FILE_DEVICE_UNKNOWN,0x804,METHOD_BUFFERED,FILE_ANY_ACCESS),
    IoData+1,1,IoData+1,1,&BytesRet,NULL);
 }
 return IoData[1];
}

EXTERN_C _declspec(dllexport) void outport(WORD a, BYTE b) {
 BYTE IoData[8];
 IoData[0]=2;
 IoData[1]=a-0x378;
 IoData[2]=b;
 if (HidMode) {
  HidD_SetFeature(hAccess,IoData,HidCaps.FeatureReportByteLength);
 }else{
  DWORD BytesRet;
  DeviceIoControl(hAccess,
    CTL_CODE(FILE_DEVICE_UNKNOWN,0x804,METHOD_BUFFERED,FILE_ANY_ACCESS),
    IoData+1,2,NULL,0,&BytesRet,NULL);
 }
}

EXTERN_C BOOL WINAPI _DllMainCRTStartup(HINSTANCE hDll,DWORD dwReason,LPVOID) {
 switch (dwReason) {
  case DLL_PROCESS_ATTACH: DisableThreadLibraryCalls(hDll); break;
 }
 return TRUE;
}

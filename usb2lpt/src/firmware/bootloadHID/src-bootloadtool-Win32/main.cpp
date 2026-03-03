/* Name: main.c
 * Project: AVR bootloader HID
 * Author: Christian Starkjohann
 * Creation Date: 2007-03-19
 * Tabsize: 8
 * Copyright: (c) 2007 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: Proprietary, free under certain conditions. See Documentation.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <setupapi.h>
extern "C" {
#include <hidsdi.h>
#include <hidpi.h>
}
#include <shlwapi.h>

HANDLE OpenHidBoot(int n, DWORD flags) {
 GUID		hidGuid;        /* GUID for HID driver */
 HDEVINFO	devs;
 DWORD		size;
 int		i;
 HANDLE		h=0;
 SP_DEVICE_INTERFACE_DATA		devInfo;
 union {	// save stack space
  char space[4+MAX_PATH];
  SP_DEVICE_INTERFACE_DETAIL_DATA	devDetails;
  HIDD_ATTRIBUTES			devAttributes;
 }u;

 HidD_GetHidGuid(&hidGuid);
 devs = SetupDiGetClassDevs(&hidGuid,NULL,NULL,DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);
 n++;
 if (devs) for (i=0; ; i++) {
  if (h) {CloseHandle(h); h=0;}
  devInfo.cbSize = sizeof(devInfo);
  if (!SetupDiEnumDeviceInterfaces(devs,0,&hidGuid,i,&devInfo)) break;	/* no more entries */
  size=sizeof(u);
  u.devDetails.cbSize = sizeof(u.devDetails);
  if (!SetupDiGetDeviceInterfaceDetail(devs, &devInfo, &u.devDetails, size, &size, NULL)) continue;
  h = CreateFile(u.devDetails.DevicePath, GENERIC_READ|GENERIC_WRITE,
    FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, flags, NULL);
  if (h == INVALID_HANDLE_VALUE) h=0;
  if (!h) continue;
  u.devAttributes.Size = sizeof(u.devAttributes);
  HidD_GetAttributes(h,&u.devAttributes);
  if (*(long*)&u.devAttributes.VendorID!=0x05DF16c0) continue;		/* ignore this device */
  if (!HidD_GetManufacturerString(h,u.space,sizeof(u))) continue;
  if (StrCmpW(L"obdev.at",(LPCWSTR)u.space)) continue;
  if (!HidD_GetProductString(h,u.space,sizeof(u))) continue;
  if (StrCmpW(L"HIDBoot",(LPCWSTR)u.space)) continue;
  if (!--n) break;	/* we have found the device we are looking for! */
 }
 SetupDiDestroyDeviceInfoList(devs);
 return h;
}

/* ------------------------------------------------------------------------- */

static char dataBuffer[65536 + 256];    /* buffer for file data */
static int  startAddress, endAddress;

/* ------------------------------------------------------------------------- */

static int parseUntilColon(FILE *fp) {
 int c;

 do c = getc(fp);
 while (c != ':' && c != EOF);
 return c;
}

static int parseHex(FILE *fp, int numDigits) {
 int i;
 char temp[9];

 for(i = 0; i < numDigits; i++) temp[i] = getc(fp);
 temp[i] = 0;
 return strtol(temp, NULL, 16);
}

/* ------------------------------------------------------------------------- */

static int parseIntelHex(char *hexfile, char buffer[], int &startAddr, int &endAddr) {
 int address, base, d, segment, i, lineLen, sum;
 FILE *input;

 input = fopen(hexfile, "r");
 if(input == NULL){
  char *p;
  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
    NULL,GetLastError(),0,(LPTSTR)&p,0,&hexfile);
  printf("error opening %s: %s", hexfile, p);
  return 1;
 }
 while(parseUntilColon(input) == ':'){
  sum = 0;
  sum += lineLen = parseHex(input, 2);
  base = address = parseHex(input, 4);
  sum += address >> 8;
  sum += address;
  sum += segment = parseHex(input, 2);  /* segment value? */
  if(segment != 0)    /* ignore lines where this byte is not 0 */
    continue;
  for(i = 0; i < lineLen ; i++){
   d = parseHex(input, 2);
   buffer[address++] = d;
   sum += d;
  }
  sum += parseHex(input, 2);
  if ((sum & 0xff) != 0) {
   printf("Warning: Checksum error between address 0x%x and 0x%x\n", base, address);
  }
  if (startAddr > base) startAddr = base;
  if (endAddr < address) endAddr = address;
 }
 fclose(input);
 return 0;
}

bool reboot;	// reboot switch

/* ------------------------------------------------------------------------- */
// Normally, both structures should be described by HID descriptors.
// But for saving flash space, HID reports are fixed.
#pragma pack(1)
typedef struct deviceInfo{
 char reportId;
 short pageSize;
 long flashSize;
}deviceInfo_t;

typedef struct deviceData{
 long reportId_address;	// 3 byte address
 char data[128];
}deviceData_t;
#pragma pack()

static bool uploadData(char *dataBuffer, int &startAddr, int &endAddr) {
 HANDLE h = 0;
 bool ok = false;
 int mask, pageSize, deviceSize;
 union{
  char         bytes[1];
  deviceInfo_t info;
  deviceData_t data;
 }u;
 static bool subsequentopen;
 int i=0; repeat:
 h = OpenHidBoot(0,0);
 if (!h) {
  printf("Error opening HIDBoot device: The specified device was not found\n");
  if (!subsequentopen || ++i==2) goto errorOccurred;
  Sleep(2000); goto repeat;
 }
 subsequentopen=true;
 u.info.reportId = 1;
 if (!HidD_GetFeature(h,u.bytes,sizeof(u))) {
  printf("Error reading page size: Communication error with device\n");
  goto errorOccurred;
 }
 pageSize = u.info.pageSize;
 deviceSize = u.info.flashSize;
 static bool alreadyshown;
 if (!alreadyshown) {
  printf("Page size of Flash   =%5d (0x%04X)\n", pageSize, pageSize);
  printf("Remaining Flash size =%5d (0x%04X)\n", deviceSize, deviceSize);
 }
 if (endAddr > deviceSize){
  printf("Data exceeds remaining flash size!\n");
  goto errorOccurred;
 }
 if (pageSize < 128) mask = 127;
 else mask = pageSize - 1;
 startAddr &= ~mask;                  /* round down */
 endAddr = (endAddr + mask) & ~mask;  /* round up */
 if (!alreadyshown) {
  printf("Uploading %d (0x%x) bytes starting at %d (0x%x)\n",
    endAddr - startAddr, endAddr - startAddr, startAddr, startAddr);
  alreadyshown=true;
 }
 while (startAddr < endAddr){
  u.data.reportId_address = startAddr<<8 | 2;
  memcpy(u.data.data, dataBuffer + startAddr, 128);
  printf("\r0x%05x ... 0x%05x", startAddr, startAddr + 128);
  if (!HidD_SetFeature(h, u.bytes, sizeof(u))) {
   printf(" Error uploading data block: Communication error with device\n");
   Sleep(100);
   goto errorOccurred;
  }
  Sleep(10);
  startAddr += sizeof(u.data.data);
 }
 printf("\n");
 ok = true;
 if (reboot) {
/* and now leave boot loader: */
  u.info.reportId = 1;
  HidD_SetFeature(h, u.bytes, sizeof(u));
/* Ignore errors here. If the device reboots before we poll the response,
 * this request fails.
 */
 }
errorOccurred:
 if (h) CloseHandle(h);
 return ok;
}

/* ------------------------------------------------------------------------- */
static void printUsage() {
 printf("usage: bootloadHID [-r] <intel-hexfile>\n");
}

int WINAPI mainCRTStartup() {
 char *file = NULL;

 char *arg,*next;
// iterate over command-line arguments
 for (arg=PathGetArgs(GetCommandLine()); *arg; arg=next) {
  next = PathGetArgs(arg);
  if (*next) *(next-1)=0;
  PathUnquoteSpaces(arg);

  switch (arg[0]) {
   case '-':
   case '/': switch (arg[1]) {
    case 'r': reboot = true; break;
    default: printUsage(); return 1;
   }break;
   default: {
    if (file) {printUsage(); return 1;}
    file = arg;
   }
  }
 }
 if (!file) {printUsage(); return 1;}
 startAddress = sizeof(dataBuffer);
 endAddress = 0;
 memset(dataBuffer, -1, sizeof(dataBuffer));
 if (parseIntelHex(file, dataBuffer, startAddress, endAddress)) return 1;
 if(startAddress >= endAddress){
  printf("No data in input file, exiting.\n");
  return 0;
 }
 for (int i=0; i<5; i++) {
  if (uploadData(dataBuffer, startAddress, endAddress)) {
   printf("The firmware is written successfully, %i retries.\n",i);
   return 0;
  }
 }
 return 1;
}

// Firmware and serial number burner
#define WIN32_LEAN_AND_MEAN
#define STRICT
//#define WINVER 0x0500
#include <windows.h>
#include <winioctl.h>
#include <shlwapi.h>
#include "C:/cypress/usb/Drivers/ezusbdrv/ezusbsys.h"

#define elemof(x) (sizeof(x)/sizeof(*(x)))
#define T(x) TEXT(x)
#define nobreak
typedef enum {false,true} bool;
typedef const TCHAR *PCTSTR,FAR*LPCTSTR,NEAR*NPCTSTR;

HANDLE hStdIn, hStdOut, hStdErr;
HANDLE hAccess;
DWORD DownloadIoctl;	// depending upon driver (EZUSB.SYS oder USB2LPT.SYS)

void _cdecl printf(const char*t,...){
 TCHAR buf[1024];
 DWORD len;
 len=wvsprintf(buf,t,(va_list)(&t+1));
 CharToOemBuff(buf,buf,len);
 WriteConsole(hStdOut,buf,len,&len,NULL);
}

int JaNein(void){
 char c;
 DWORD len,OldMode;
 GetConsoleMode(hStdIn,&OldMode);
 SetConsoleMode(hStdIn,0);
 ReadConsole(hStdIn,&c,1,&len,NULL);
 printf("%c\n",c);
 SetConsoleMode(hStdIn,OldMode);
 switch (c){
  case 'J':
  case 'j':
  case 'Y':
  case 'y': return IDYES;
  case 'N':
  case 'n': return IDNO;
 }
 return 0;
}

// about like memory insertion of the IIC file here:
#define IICLEN 0x2000
BYTE IicData[IICLEN];
DWORD IicLen;
BYTE DataStart;	// FX2: 8, AN2131: 7, by LoadIic one specifies
WORD ResetAddr;	// FX2: E600, AN2131: 7F92, by LoadIic one specifies

//IIC file read in
bool LoadIic(PCTSTR IicName) {
 HANDLE f;
 bool ret=false;
 f=CreateFile(IicName,GENERIC_READ,FILE_SHARE_READ,
   NULL,OPEN_EXISTING,0,0);
 if (f==INVALID_HANDLE_VALUE) {
  printf("IIC file could not open!\n");
 }else{
  if (!ReadFile(f,IicData,IICLEN,&IicLen,NULL)) {
   printf("IIC file could not read!\n");
  }else switch (IicData[0]) {
   case 0xB2: {	//AN2131
    DataStart=7;
    ResetAddr=0x7F92;
    ret=true;
   }break;
   case 0xC2: {	//FX2
    DataStart=8;
    ResetAddr=0xE600;
    ret=true;
   }break;
   default: printf("File is not a valid IIC file!\n");
  }
 }
 CloseHandle(f);
 return ret;
}

// Data in the 8051-XRAM load, via ezusb.sys or usb2lpt.sys
bool WriteRam(WORD adr, BYTE*data, DWORD len) {
 if (!DeviceIoControl(hAccess,
   DownloadIoctl,&adr,sizeof(adr),data,len,&len,NULL)) return false;
 return true;
}

// IIC data into RAM load
bool LoadRam(void) {
 bool ret=false;
 BYTE Reset8051=1;
 BYTE*p=IicData+DataStart;
 if (!WriteRam(ResetAddr,&Reset8051,1)) goto ende;
 for (;;) {
  WORD len=MAKEWORD(p[1],p[0]);
  WORD adr=MAKEWORD(p[3],p[2]);
  if (adr==ResetAddr) break;
  p+=4;
  if (!WriteRam(adr,p,len)) goto ende;
  p+=len;
 }
 Reset8051=0;
 if (!WriteRam(ResetAddr,&Reset8051,1)) goto ende;
 ret=true;
ende:
 if (!ret) printf("Error during the RAM-writing!\n");
 return ret;
}

// Serial number of console enter and then burn
bool NeueSeriennummer(void) {
 static DWORD auto_sn;
 DWORD sn,len;
 WORD adr=0xFFFC;
 TCHAR Number[12];
 if (auto_sn) printf(T("(%u) "),auto_sn);
 ReadConsole(hStdIn,Number,elemof(Number)-1,&len,NULL);
 Number[len]=0;
 if (!StrToIntEx(Number,STIF_SUPPORT_HEX,&sn) || !sn) sn=auto_sn;
 if (!sn) return false;
 if (!DeviceIoControl(hAccess,
   CTL_CODE(FILE_DEVICE_UNKNOWN,0x08A2,METHOD_IN_DIRECT,FILE_ANY_ACCESS),
   &adr,sizeof(adr),
   &sn,sizeof(sn),&len,NULL)) return false;
// if (len!=sizeof(sn)) return false;
 printf("Serial number %u burned.\n",sn);
 auto_sn=sn+1;
 return true;
}

void TryOpen(LPTSTR TemplateName) {
 int i;
 for (i=9; i>=0; i--){
  TCHAR DevName[12];
  wsprintf(DevName,TemplateName,i);
  hAccess=CreateFile(DevName,
    GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,0);
  if (hAccess!=INVALID_HANDLE_VALUE) break;
 }
}

bool ReadSerialNumber(DWORD *psn) {
 DWORD sn,br;
 struct {
  WORD adr;
  BYTE i2ca;	// 0xA2 für 8-K-EEPROM
  BYTE i2cs;	// 0x02 für 8-K-EEPROM
 }adr={0xFFFC,0xA2,0x02};
 if (!DeviceIoControl(hAccess,
  CTL_CODE(FILE_DEVICE_UNKNOWN,0x08A2,METHOD_OUT_DIRECT,FILE_ANY_ACCESS),
  &adr,sizeof(adr),
  &sn,sizeof(sn),&br,NULL)) return false;
 if (~sn && !(~sn&0xFFFFFF)) sn>>=24;	// Seriennummer im MSB (altes Format)
 if (psn) *psn=sn;
 return true;
}

void _cdecl mainCRTStartup(){
 int j;
 DWORD sn,br;
 LPTSTR IicName;
 hStdIn =GetStdHandle(STD_INPUT_HANDLE);
 hStdOut=GetStdHandle(STD_OUTPUT_HANDLE);
 hStdErr=GetStdHandle(STD_ERROR_HANDLE);
 IicName=PathGetArgs(GetCommandLine());
 if (!IicName || !*IicName) {
  printf("The .iic file which can be burned must be indicated!\n");
  printf("\nPress any key to end the program...");
  JaNein();
  ExitProcess(1);
 }
 printf("IIC file reads...\n");
 if (!LoadIic(IicName)) ExitProcess(2);
 do{
  DownloadIoctl=IOCTL_EZUSB_ANCHOR_DOWNLOAD;	// via ezusb.sys
  printf("Verbinde zu <ezusb.sys>...\n");
// Search from the rear, in order to omit possible driver corpses
  TryOpen("\\\\.\\EZUSB-%d");
  if (hAccess==INVALID_HANDLE_VALUE) {
   printf("No EZUSB equipment found.\n");
// Search alternatively a USB2LPT with firmware...
   TryOpen("\\\\.\\LPT%d");
   if (hAccess==INVALID_HANDLE_VALUE) goto ende;
   if (!ReadSerialNumber(&sn)) goto ende;
   printf("Instead (programmed) a USB2LPT-Equipment found, firmware updates?");
   if (JaNein()!=IDYES) goto ende;
   DownloadIoctl=CTL_CODE(FILE_DEVICE_UNKNOWN,0x08A0,METHOD_IN_DIRECT,FILE_ANY_ACCESS);	// via usb2lpt.sys
  }

  printf("Loads IIC data into RAM and start again...\n");
  if (!LoadRam()) goto ende;
  if (DownloadIoctl==IOCTL_EZUSB_ANCHOR_DOWNLOAD) {
   CloseHandle(hAccess);
   printf("Wait for Renumeration, connect to <usb2lpt.sys>...\n");
   for (j=12; j; j--){
    Sleep(500);		// 6 Sekunden warten (nicht suchen; Absturz W2K!!)
    printf(".");	// Programm arbeitet noch:-)
   }
   printf("\n");
// usb2lpt.sys produces LPTx devices, which "behind; normalen" LPT numbers
// lie. Therefore from the rear ago look for, over none as "as possible; normales"
// To get LPT (or driver corpses).
   TryOpen("\\\\.\\LPT%d");
   if (hAccess==INVALID_HANDLE_VALUE) {
    printf("Kein (USB2)LPT-Gerät gefunden.\n");
    goto ende;
   }
// Highest possible LPTx opened, is it also a USB2LPT?
// Read in addition serial number. If does not fold, continue to wait.
   if (!ReadSerialNumber(&sn)) {
    printf("Problem: No USB2LPT found!\n");
    goto ende;
   }
  }
  if (sn==(DWORD)-1) {
   printf("No serial number in the EEPROM, again enter: ");
   if (!NeueSeriennummer()) goto ende;
  }else{
   printf("Serial number: enter %lu, again?",sn);
   if (JaNein()==IDYES && !NeueSeriennummer()) goto ende;
  }
  printf("Burn Firmware...\n");
  {
   struct {
    WORD adr;	// Position (however it is sufficient only, if EEPROM were present when starting)
    BYTE i2ca;	// 0xA2 für 8-K-EEPROM
    BYTE i2cs;	// 0x02 für 8-K-EEPROM
   }adr={0x0000,0xA2,0x02};
   if (!DeviceIoControl(hAccess,
     CTL_CODE(FILE_DEVICE_UNKNOWN,0x08A2,METHOD_IN_DIRECT,FILE_ANY_ACCESS),
     &adr,sizeof(adr),
     IicData,IicLen,&br,NULL)) {
    printf("Error!\n");
    goto ende;
   }
  }
  printf("Finished.\n");
ende:
  if (hAccess!=INVALID_HANDLE_VALUE) CloseHandle(hAccess);
  printf("\nPress arbitrary key to the program end (Y for repetition)...");
 }while (JaNein()==IDYES);
 ExitProcess(0);
}

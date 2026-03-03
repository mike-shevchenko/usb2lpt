#include <windows.h>
#include <winioctl.h>
HANDLE hStdIn, hStdOut, hStdErr;
HANDLE hAccess;

void _cdecl printf(const char*t,...){
 TCHAR buf[256];
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

void _cdecl mainCRTStartup(){
 BYTE data=0;
 WORD adr=0;
 DWORD dw;
 hStdIn =GetStdHandle(STD_INPUT_HANDLE);
 hStdOut=GetStdHandle(STD_OUTPUT_HANDLE);
 hStdErr=GetStdHandle(STD_ERROR_HANDLE);

 printf("Aid programme for the deletion (deactivate) of USB2LPT firmware\n");
 printf("Versions 1.0 to 1.4, April 2007\n\n");
 for (dw=8;dw;dw--){	// von hinten probieren
  TCHAR DevName[16];
  wsprintf(DevName,"\\\\.\\LPT%u",dw);
  hAccess=CreateFile(DevName,
   GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,0);
  if (hAccess!=INVALID_HANDLE_VALUE) break;
 }
 if (hAccess==INVALID_HANDLE_VALUE){
  printf("USB2LPT sticks on, must be LPTx!\n");
  goto ende;
 }
// C2-Byte zur Kontrolle lesen (B2 beim AN2131)
 if (!DeviceIoControl(hAccess,
   CTL_CODE(FILE_DEVICE_UNKNOWN,0x08A2,METHOD_OUT_DIRECT,FILE_ANY_ACCESS),
   &adr,2,&data,1,&dw,NULL) /*|| dw!=1*/) {
  printf("Errors with the device control drive, USB2LPT not attached?\n");
  goto ende;
 }
 if (data!=0xC2 && data!=0xB2) {
  printf("Firmware is already deleted. USB2LPT unplug and reattach!\n");
  goto ende;
 }
// C2-Byte delete (bridge in Rev. 2 and 3 does not function!)
 printf("Firmware in the EEPROM of the USB2LPT now delete?\n"
  "[Writing EEPROM needs the free development program EzMr.exe from www.cypress.com]\n"
  "First byte (0x%02X) delete (overwrite with 0xFF)? Y/N: ",data);
 if (JaNein()!=IDYES) goto ende;
 data=0xFF;
 if (!DeviceIoControl(hAccess,
   CTL_CODE(FILE_DEVICE_UNKNOWN,0x08A2,METHOD_IN_DIRECT,FILE_ANY_ACCESS),
   &adr,2,&data,1,&dw,NULL) /*|| dw!=1*/) {
  printf("Errors with the device control drive, USBLPT not stuck on?\n");
  goto ende;
 }
 printf("Firmware was deleted by overwriting of the first byte.");

ende:
 CloseHandle(hAccess);
 printf("\nPress any key to end the program...");
 JaNein();
 ExitProcess(0);
}

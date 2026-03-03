/******************************
 * Hilfsprogramm zum Firmware-Deaktivieren auf AN2131 / CY7C68013A
 * Erfordert laufende USB2LPT-Firmware (Quelle USB2LPT.A51 oder USB2LPT2.A51)
 * (muss nicht notwendigerweise im EEPROM sein; BRENNER.EXE lädt diese auch
 * in den RAM) sowie einen laufenden USB2LPT-Treiber USB2LPT.SYS
 * USB2LPT-Projekt, haftmann#software, 2007-2012, Public Domain.
 * tabsize=8, lineends=CRLF, codepage=CP1252, indent=1, changes:
 070400	Erstausgabe
-120100	Umbau auf Zwang zu 2-Byte-EEPROM (sonst klappt der Brückentrick nicht)
+120307	I18N (deutsch+englisch), Löschen mit nur 2 Bit (B2->BE, C2->CE)
	Der neue Gerätemanager-Eigenschaftsseiten-Lieferant hat (neu)
	eine Hintertür zum Firmware-Löschen und macht dieses Programm unnötig.
 ******************************/

#include <windows.h>
#include <winioctl.h>
HANDLE hStdIn, hStdOut;
HANDLE hAccess;

void _cdecl printf(const char*t,...){
 TCHAR buf[256],b2[256];
 DWORD len;
 if (!HIWORD(t)) {
  LoadString(0,(UINT)t,b2,256);
  t=b2;
 }
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
 DWORD adr=0x02A20000;	// Ansprechen eines 2-Adressbyte-EEPROMs mit I²C-Adresse 0xA2
 DWORD dw;
 hStdIn =GetStdHandle(STD_INPUT_HANDLE);
 hStdOut=GetStdHandle(STD_OUTPUT_HANDLE);

 printf(MAKEINTRESOURCE(1));	//"Hilfsprogramm zum Löschen (Deaktivieren) der Firmware vom h#s USB2LPT-Gerät.\n"
 printf(MAKEINTRESOURCE(2));	//"Versionen 1.0 bis 1.4, 1.7 (Controllerchips AN2131 oder CY7C68013A) März 2012\n\n"
 for (dw=8;dw;dw--){	// von hinten probieren
  TCHAR DevName[16];
  wsprintf(DevName,"\\\\.\\LPT%u",dw);
  hAccess=CreateFile(DevName,
   GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,0);
  if (hAccess!=INVALID_HANDLE_VALUE) break;
 }
 if (hAccess==INVALID_HANDLE_VALUE){
  printf(MAKEINTRESOURCE(3));	//"USB2LPT anstecken, muss LPTx sein!\n"
  goto ende;
 }
// C2-Byte zur Kontrolle lesen (B2 beim AN2131, C0/B0 bei 8-Bit-EEPROM)
 if (!DeviceIoControl(hAccess,
   CTL_CODE(FILE_DEVICE_UNKNOWN,0x08A2,METHOD_OUT_DIRECT,FILE_ANY_ACCESS),
   &adr,4,&data,1,&dw,NULL) || dw!=1) {
  printf(MAKEINTRESOURCE(4));	//"Fehler beim Ausführen der Gerätesteuerung, USB2LPT weg? Oder EEPROM 24C64 weg?\n"
  goto ende;
 }
 if ((data+0x10&~0x12)!=0xC0) {
  printf(MAKEINTRESOURCE(5),data);	//"Firmware ist bereits gelöscht, erstes Byte = 0x%02X. USB2LPT abziehen und wieder anstecken!\n"
  goto ende;
 }
// C2-Byte löschen (Brücke in Rev. 2 und 3 funktioniert nicht!)
 printf(MAKEINTRESOURCE(6));	//"Firmware im EEPROM des USB2LPT jetzt löschen?\n"
 printf(MAKEINTRESOURCE(7));	//"[Zum Beschreiben des EEPROM ...
 printf(MAKEINTRESOURCE(8),data,data|0x0C);	//"Erstes Byte (0x%02X) löschen (überschreiben mit 0x%02X)? J/N: "
 if (JaNein()!=IDYES) goto ende;
 data|=0x0C;	// so kann nachfolgende Software noch zwischen AN2131 und CY7C68013A unterscheiden
 if (!DeviceIoControl(hAccess,
   CTL_CODE(FILE_DEVICE_UNKNOWN,0x08A2,METHOD_IN_DIRECT,FILE_ANY_ACCESS),
   &adr,4,&data,1,&dw,NULL) || dw!=1) {
  printf(MAKEINTRESOURCE(9));	//"Fehler beim Ausführen der Gerätesteuerung, USBLPT nicht angesteckt?\n"
  goto ende;
 }
 printf(MAKEINTRESOURCE(10));	//"Firmware wurde durch Überschreiben des ersten Bytes gelöscht."

ende:
 CloseHandle(hAccess);
 printf(MAKEINTRESOURCE(11));	//"\nBeliebige Taste zum Programmende drücken ..."
 JaNein();
 ExitProcess(0);
}

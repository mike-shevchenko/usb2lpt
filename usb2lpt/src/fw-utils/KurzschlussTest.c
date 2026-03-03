#include <windows.h>
#include <winioctl.h>
HANDLE hStdIn, hStdOut, hStdErr;
HANDLE hAccess;
BYTE gError;
#define DDR_OUT 0xFF	// Firmware ab 28.1.2007

void outb(BYTE a, BYTE b) {
 BYTE IoData[2];
 DWORD BytesRet;
 IoData[0]=a;
 IoData[1]=b;
 DeviceIoControl(hAccess,
   CTL_CODE(FILE_DEVICE_UNKNOWN,0x804,METHOD_BUFFERED,FILE_ANY_ACCESS),
   IoData,sizeof(IoData),NULL,0,&BytesRet,NULL);
}

BYTE inb(BYTE a) {
 BYTE IoData[1];
 DWORD BytesRet;
 IoData[0]=a|0x10;	// Lese-Bit
 DeviceIoControl(hAccess,
   CTL_CODE(FILE_DEVICE_UNKNOWN,0x804,METHOD_BUFFERED,FILE_ANY_ACCESS),
   IoData,sizeof(IoData),IoData,sizeof(IoData),&BytesRet,NULL);
 return IoData[0];
}

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

void Check(BYTE bData, BYTE bStatus, BYTE bControl, BYTE bDoOut){
 if (bDoOut){
  if ((inb(0x0D)^DDR_OUT)&0xC0) bStatus|=0xC0;	// kann nicht ausgeben!
  outb(0,bData);
  outb(1,(bStatus^0x80)|0x07);
  outb(2,(bControl^0x0B)&0x0F);
 }
 bData^=inb(0);
 bStatus^=inb(1)^0x80; bStatus&=0xF8;
 bControl^=inb(2)^0x0B; bControl&=0x0F;
 if (bData) printf("Datenfehler: %02X\n",bData);
 if (bStatus) printf("Statusfehler: %02X\n",bStatus);
 if (bControl) printf("Steuerfehler: %02X\n",bControl);
 gError|=bData|bStatus|bControl;
}

int _cdecl mainCRTStartup(){
 BYTE b,MerkFeature;
 WORD adr,date;
 DWORD sn,br;
 hStdIn =GetStdHandle(STD_INPUT_HANDLE);
 hStdOut=GetStdHandle(STD_OUTPUT_HANDLE);
 hStdErr=GetStdHandle(STD_ERROR_HANDLE);
rept:
 for (sn=8;sn;sn--){	// von hinten probieren
  TCHAR DevName[12];
  wsprintf(DevName,"\\\\.\\LPT%u",sn);
  hAccess=CreateFile(DevName,
   GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,0);
  if (hAccess!=INVALID_HANDLE_VALUE) break;
 }
 if (hAccess==INVALID_HANDLE_VALUE){
  printf("USB2LPT anstecken, muss LPTx sein!\n");
  goto ende;
 }
 MerkFeature=inb(0x0F);
 outb(0x0F,0x00);	// Normalbetrieb ohne Features
 outb(0x0A,0x00);	// SPP-Modus
 outb(0x0D,DDR_OUT);	// Statusleitungen Ausgänge
 if ((inb(0x0D)^DDR_OUT)&0x38){
  printf("Dieser Test erfordert Firmware 060629 oder neuer!\n");
  goto ende;
 }
#if 0	// C2-Byte löschen (Brücke funktioniert nicht!)
 adr=0;
 sn=(DWORD)-1;
 DeviceIoControl(hAccess,
   CTL_CODE(FILE_DEVICE_UNKNOWN,0x08A2,METHOD_IN_DIRECT,FILE_ANY_ACCESS),
   &adr,sizeof(adr),
   &sn,sizeof(sn),&br,NULL);
#endif
 adr=0xFFFC;	// Seriennummer-Position
 DeviceIoControl(hAccess,
   CTL_CODE(FILE_DEVICE_UNKNOWN,0x08A2,METHOD_OUT_DIRECT,FILE_ANY_ACCESS),
   &adr,sizeof(adr),
   &sn,sizeof(sn),&br,NULL);
 if (!(~sn&0xFFFFFF)) sn>>=24;	// Seriennummer im MSB (altes Format)
 printf("Seriennummer: %u\n",sn);
 adr=0x0006;	// Datums-Position
 DeviceIoControl(hAccess,
   CTL_CODE(FILE_DEVICE_UNKNOWN,0x08A3,METHOD_OUT_DIRECT,FILE_ANY_ACCESS),
   &adr,sizeof(adr),
   &date,sizeof(date),&br,NULL);
 printf("Firmware-Erstellungsdatum: %u-%02u-%02u\n",
   (date>>9)+1980,(date>>5)&0x0F,date&0x1F);
 gError=FALSE;
// Prüfen auf Kurz- und Masseschlüsse
 printf("Automatische Kurzschluss- und Masseschluss-Prüfung...\n");
 for(b=1;b;b<<=1) {	// Daten testen
  Check(b,0,0,TRUE);
  Check(~b,0xFF,0xFF,TRUE);
 }
 for(b=8;b;b<<=1) {	// Statusleitungen
  Check(0,b,0,TRUE);
  Check(0xFF,~b,0xFF,TRUE);
 }
 for(b=1;b<0x10;b<<=1) {// Steuerleitungen
  Check(0,0,b,TRUE);
  Check(0xFF,0xFF,~b,TRUE);
 }
 outb(2,0xFF^0x0B);
// Prüfen auf Vorhandensein der Pullups
 outb(0x0C,(BYTE)~DDR_OUT);	// Alles Eingänge
 outb(0x0D,(BYTE)~DDR_OUT);
 if (!(inb(0x0F)&0x02)) outb(0x0E,(BYTE)~DDR_OUT);	// Steuerport nur, wenn Pull-Up vorhanden
 Check(0xFF,0xFF,0xFF,FALSE);	// muss alles High sein
// Gegenprobe funktioniert nur mit Rev.3
	// Irgendwo Fehler!!! Firmware? Logik?
printf("Steuerport  VOR Pullup-Ausschalten: %02X %02X\n",inb(2),inb(0x0E));
 outb(0x0F,0x80);	// Pullups AUS (Feature)
printf("Steuerport NACH Pullup-Ausschalten: %02X %02X\n",inb(2),inb(0x0E));
 if (inb(0x0F)&0x80){	// falls Pullups ausgeschaltet werden konnten
  printf("Rev.3: Sind die LEDs (vom LptChk) jetzt sehr dunkel? ");
  if (JaNein()==IDNO) gError|=1;
  outb(0x0F,0x00);	// Pullups EIN
 }
 outb(0x0C,DDR_OUT);
 outb(0x0D,DDR_OUT);
 outb(0x0E,DDR_OUT);
 printf("Leuchten alle LEDs hell? ");
 if (JaNein()==IDNO) gError|=1;
// Prüfung, ob die Pins angelötet(!) sind
 Check(0x00,0x00,0x00,TRUE);
 printf("Sind alle LEDs aus? ");
 if (JaNein()==IDNO) gError|=1;
 if (gError){
  SetConsoleTextAttribute(hStdOut,FOREGROUND_RED|FOREGROUND_INTENSITY);
  printf("!!! FEHLER !!! Lötarbeit!\n");
  SetConsoleTextAttribute(hStdOut,FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE);
 }else{
  SetConsoleTextAttribute(hStdOut,FOREGROUND_GREEN|FOREGROUND_INTENSITY);
  printf("Keine Kurz- und Masseschlüsse. Alles OK.\n");
  SetConsoleTextAttribute(hStdOut,FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE);
 }
 outb(0x0D,(BYTE)~DDR_OUT);	// Eingänge
 Check(0xFF,0xFF,0x07,TRUE);	// etwaiger Ausgangszustand
 Check(0x00,0xFF,0x07,TRUE);
//eigentlich nur bei Firmware ab 9.2.2007
 printf("Helligkeit der blauen LED umschalten?");
 if (JaNein()==IDYES) MerkFeature^=0x20;
ende:
 outb(0x0F,MerkFeature);
 CloseHandle(hAccess);
 printf("\nBeliebige Taste zum Programmende drücken (J für Wiederholung)...");
 if (JaNein()==IDYES) goto rept;
 return 0;
}

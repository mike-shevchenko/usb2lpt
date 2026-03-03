// Der Parallelport-Monitor für den Gerätemanager
// liefert die dritte Eigenschaftsseite „Monitor“ mit den bunten SubD-Buchsen
// Das Hauptprogramm mit den beiden anderen Eigenschaftsseiten ist PROP.C
// Wie bei mir üblich verwende ich passend ausgewählte numerische Ressourcen-IDs ohne
// symbolische Bezeichner (also keine „resource.h“).
// Solcherart Symbolik macht Quelltext nicht wirklich leicher lesbar, nur aufgeblasener.
// Vergleichbare Programme haben die zehnfache Quelltextmenge (in Bytes),
// hier ist die Funktionalität ziemlich dicht gepackt!
// Tabweite = 8, Einrücktiefe = 1, Zeichenkodierung = CP1252, Zeilenende = CRLF
#include "prop.h"

/*** Es sollte nichts statisches geben! Alles hier verwendete ist in einer struct (class) ***/
typedef struct CMonWnd{
 HWND Wnd;	// Fenster-Handle
 PSetup S;	// Zeiger zu Setup-Daten
 BYTE LptRegsRd[16];	// Cache der Lesedaten
 BYTE LptRegsWr[16];	// Spiegel der Schreibdaten (ab Index 10 stets gleich LptRegsRd)
 WORD valid;		// Ob die Lesedaten (byteweise) gültig sind
 HWND hwndTT;		// ToolTip-Handle
 POINT SubDPos;		// Position linke obere Ecke (am wirklichen Pin 13...
 POINT GroupboxCenter[4]; // Mittenpositionen für Bitfelder (variable Dialoggrößen beachtend)
 struct Cgdi {
  HPEN WidePen;		// Dicker Stift für SubD-Umrandung
  HPEN AirwirePen;	// Luftlinien zur Verbindung von Bits und Pins
  HPEN NullPen;		// "Kein Stift" für dreieckige Polygone
  HFONT SmallFont;	// für Pin-Nummern (1..25)
  HFONT DescFont;	// für Bit-Namen (D0..D7 usw.)
  HPEN ColorPen[3];	// Farbige Stifte für Daten, Steuer, Status
  HBRUSH ColorBrush[3];	// Farbige Pinsel für Daten, Steuer, Status
  HBRUSH DarkBrush[3];	// Farbige Pinsel für Daten, Steuer, Status „ausgegraut“
  void CreateGdiResources();
  void DeleteGdiResources() const;
 }gdi;

 void outb(BYTE,BYTE);
 BYTE inb(BYTE) const;
// Die Zeigerspezifizierer _ds und _ss ersparen bei BorlandC 3.1 (16 bit)
// die Übergabe von 32-bit-far-Zeigern. Irrelevant unter Win32 und Win64.
// Wichtig für 16-bit-DLLs, weil DS (Datensegment) != SS (Stacksegment).
 int inbytes(const BYTE _ds*,int,BYTE _ss*) const;
// Die grafische Darstellung von Bytes und der SubD-Buchse haben kein
// Tastaturinterface! Dafür wären echte Kindfenster (Controls) besser geeignet.
// Ersatzhalber kann man die Bits als Hexzahlen in den Editfenstern eingeben.
 void GetPinPos(BYTE,POINT _ss*) const;
 void GetPinRect(BYTE,RECT _ss*) const;
 BYTE PinAssign(BYTE) const;
 bool BitKnown(BYTE) const;
 bool BitHasInversion(BYTE) const;
 void DrawPin(HDC,BYTE) const;
 void DrawPinNumber(HDC,BYTE) const;
 void DrawSubD(HDC) const;		// SubD-Buchse malen (nur Schale)
 void DrawPins(HDC) const;		// SubD-Pins malen
 bool BitExists(BYTE) const;
 void GetBitPos(BYTE,POINT _ss*) const;
 void GetBitRect(BYTE,RECT _ss*) const;
 bool FindPin(BYTE,BYTE _ss*) const;
 void DrawBit(HDC,BYTE) const;		// Bit-Zelle malen, HDC optional
 void DrawByte(HDC,BYTE) const;		// Bit-Felder eines Bytes malen, HDC optional
 void DrawAirwires(HDC) const;
 void SetEcrMode(int Mode);
 void OnOutputButton(void);
 void OnInputButton(void);
 void update(int,BYTE);
 void ToggleBit(BYTE);
 BYTE BitFromMessagePos(void) const;
 void GetBubblehelpText(BYTE,UINT,LPTSTR,UINT) const;
#ifdef WIN32
 void AddTools(BYTE) const;		// Alle 6*8 + 17(20) Tools mitteilen
#else
 void AddTools() const;		// Alle 6*8 + 17(20) Tools mitteilen
#endif
 void TimerUpdate(void);
}TMonData,_ds*PMonData;

// <a> im Bereich 0x00..0x0F
void CMonWnd::outb(BYTE a, BYTE b) {
 LptRegsWr[a]=b;	// vermerken
 BYTE IoctlData[2];
 IoctlData[0]=a;
 IoctlData[1]=b;
 DevIoctl(S,IOCTL_VLPT_OutIn,&IoctlData,2,&IoctlData,0);
}

// <a> im Bereich 0x10..0x1F
BYTE CMonWnd::inb(BYTE a) const{
 DevIoctl(S,IOCTL_VLPT_OutIn,&a,1,&a,1);
 return a;
}

// Mehrere Bytes mit einem Rutsch lesen, Bits 4 müssen gesetzt sein!
// <a> und <b> dürfen übereinander liegen, müssen aber nicht.
int CMonWnd::inbytes(const BYTE _ds*a, int len, BYTE _ss*b) const{
 return DevIoctl(S,IOCTL_VLPT_OutIn,a,len,b,len);
}

// Das freie Bit 3 bleibt frei und ist stets 0
static const BYTE DefPinAssign[25]={	// Bit-Kode: High-Nibble=Adresse, Low-Nibble=BitNr
 0x20,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x16,0x17,0x15,0x14,
   0x21,0x13,0x22,0x23,0xFF,0xFF,0xFF,0x10,0x11,0x12,0xFF,0xFF};
static const COLORREF PinColor[3]={0x00FF00L,0x0000FFL,0x00E0E0L};
static const COLORREF PinDark [3]={0x80C080L,0x8080C0L,0x80C0C0L};	// Nur für Pinsel - kein Paletten-Management!
// Was für ein Zufall! WinDriver verwendet die gleichen Farben wie LptChk!

// Pin-Bezeichnungen mit Invertierungsschrägstrich - aus Sicht der Software! (Also Bit-Bezeichner)
// Invertierungsschrägstriche werden am Pin ggf. "zu Fuß" hinzugefügt oder entfernt.
// normale Bedeutung
static const TCHAR SppS[]=T("E\0") T("-\0") T("/IRQ\0") T("/ERR\0") T("ONL\0") T("PE\0") T("/ACK\0") T("/BSY");
static const TCHAR SppC[]=T("STB\0") T("AF\0") T("/INI\0") T("SEL\0") T("IEN\0") T("DIR\0") T("-");	// 7 Bits
// Nibble-Modus
static const TCHAR NibS[]=T("\0") T("\0") T("\0") T("R0\0") T("R1\0") T("R2\0") T("PtrClk\0") T("/R3");
static const TCHAR NibC[]=T("\0") T("/NibAck\0") T("\0") T("/1284Active");			// ab hier 4 Bits
// Byte-Modus
static const TCHAR BytS[]=T("\0") T("\0") T("\0") T("/DataAvail\0") T("Xflag\0") T("AckDataReq\0") T("PtrClk\0") T("/PtrBusy");
static const TCHAR BytC[]=T("/HostClk\0") T("/HostBusy\0") T("/Init\0") T("/1284Active");
// ECP-Modus
static const TCHAR EcpS[]=T("\0") T("\0") T("\0") T("/PeriphRequest\0") T("Xflag\0") T("/AckReverse\0") T("PeriphClk\0") T("/PeriphAck");
static const TCHAR EcpC[]=T("/HostClk\0") T("/HostAck\0") T("/ReverseRequest\0") T("/1284Active");
// EPP-Modus
static const TCHAR EppS[]=T("Timeout\0") T("\0") T("\0") T("Spare\0") T("Spare\0") T("Spare\0") T("/Intr\0") T("Wait");
static const TCHAR EppC[]=T("Write\0") T("DataStb\0") T("/Reset\0") T("AddrStb");

//ECR-Modes für Kombinationsfenster

enum{
 SubDSpaceX=16,	// Pin-Abstand (in Wahrheit 53 mil), muss hier gerade sein
 SubDSpaceY=16,	// Reihenabstand
 SubDPinD  =10,	// Durchmesser, muss gerade sein
 SubDRand  =12,	// Abstand zum Rand (von den Mittelpunkten der Pins)
 SubDRandW =3	// Strichbreite für Umrandung
};

void CMonWnd::Cgdi::CreateGdiResources() {
 int i;
 for (i=0; i<3; i++) {
  ColorPen[i]=CreatePen(PS_SOLID,0,PinColor[i]);
  ColorBrush[i]=CreateSolidBrush(PinColor[i]);
  DarkBrush[i]=CreateSolidBrush(PinDark[i]);
 }
 WidePen=CreatePen(PS_SOLID,SubDRandW,0);
 AirwirePen=CreatePen(PS_SOLID,0,0xFF0000L);
 NullPen=CreatePen(PS_NULL,0,0);
 SmallFont=CreateFont(-7,0,0,0,0,0,0,0,0,0,0,0,0,T("Small Fonts"));
 DescFont=CreateFont(-10,0,0,0,0,0,0,0,0,0,0,0,0,T("Helv"));
}

void CMonWnd::Cgdi::DeleteGdiResources() const {
 int i;
 for (i=0; i<sizeof(this)/sizeof(HGDIOBJ); i++) {
  DeleteObject(((HGDIOBJ*)this)[i]);
 }
}

static void SetDlgItemHex(HWND w, UINT id, UINT v) {
 TCHAR s[12];
 wsprintf(s,T("%02X"),v);
 SetDlgItemText(w,id,s);	// Fehler in DDK windows.h: kein BOOL!
}

// macht aus einem Punkt ein Rechteck
static void InflatePoint(const POINT _ss* pt, RECT _ss* rc, int dx, int dy) {
 rc->left=rc->right=pt->x;
 rc->top=rc->bottom=pt->y;
 InflateRect(rc,dx,dy);
}

/*================ SubD-Buchse ================*/
// Alle Zeichenoperationen in Client-Koordinaten!
#define DIRECTIO (LptRegsRd[15]&0x40)
#define ECR      (LptRegsRd[10])
#define ECRMODE  (ECR>>5)

// Liefert Position des SubD-Pins
void CMonWnd::GetPinPos(BYTE pin, POINT _ss* pt) const{	// Pins sind 0-basiert
 pt->x=SubDPos.x+SubDRand+SubDSpaceX*(12-pin);
 pt->y=SubDPos.y+SubDRand;
 if (pin>=13) {
  pt->x+=SubDSpaceX*25/2;
  pt->y+=SubDSpaceY;
 }
}

// Liefert Umfassungsrechteck des SubD-Pins
void CMonWnd::GetPinRect(BYTE pin, RECT _ss* rc) const{	// gut für Treffertest
 POINT pt;
 GetPinPos(pin,&pt);
 InflatePoint(&pt,rc,SubDPinD/2,SubDPinD/2);
}

// liefert Bit-Kode für 0-basierte SubD-Pin-Nummer
BYTE CMonWnd::PinAssign(BYTE pin) const{
 if (pin>=17 && !DIRECTIO) return 0xFF;		// Ohne DirectIO keine drei Extra-Pins
 return DefPinAssign[pin];
}

// liefert <false> wenn der Zustand der Leitung(!) unbekannt ist
// Führt zur Ausgabe von grauen Pins
bool CMonWnd::BitKnown(BYTE bit) const{
 if (bit&0xF0) return true;	// nur das Datenport ist hier von Interesse! Alle anderen Ports sind stets lesbar.
 int Mode=ECRMODE;
 if (Mode<2 || Mode==4) return true;	// Datenport kann gelesen werden
 return false;			// Datenport kann nicht (ohne Seiteneffekte) gelesen werden!
}

bool CMonWnd::BitHasInversion(BYTE bit) const{
 switch (bit) {
  case 0x12: if (DIRECTIO) return false;// IRQ
  case 0x17: 				// BSY
  case 0x20:				// STB
  case 0x21:				// AF
  case 0x23: return true;		// ONL
 }
 return false;
}

// pin=SubD-Pin-Nummer! BorlandC verlangt ein paar überflüssige Klammern
void CMonWnd::DrawPin(HDC dc, BYTE pin) const{
 RECT rc;
 HPEN hOldPen=0;
 HBRUSH hOldBrush=0;
 BYTE bit=PinAssign(pin);
 BYTE PinHigh=0;
 int byt=(signed char)(bit&0xF0)>>4;

 GetPinRect(pin,&rc);
 if (byt>=0) {
  hOldPen=SelectPen(dc,gdi.ColorPen[byt]);
  if (BitKnown(bit)) {
   PinHigh=LptRegsRd[byt]&(1<<(bit&7));
   if (!DIRECTIO && BitHasInversion(bit)) PinHigh=!PinHigh;
   if (PinHigh) hOldBrush=SelectBrush(dc,gdi.ColorBrush[byt]);
  }else hOldBrush=SelectBrush(dc,GetStockBrush(LTGRAY_BRUSH));
 }
 Ellipse(dc,rc.left,rc.top,rc.right,rc.bottom);
 if (hOldPen) SelectPen(dc,hOldPen);
 if (hOldBrush) SelectBrush(dc,hOldBrush);
}

void CMonWnd::DrawPinNumber(HDC dc, BYTE pin) const{
 POINT pt;
 TCHAR nr[3];
 GetPinPos(pin,&pt);
 TextOut(dc,pt.x+2,pt.y-SubDRand+1,nr,wsprintf(nr,T("%i"),pin+1));
}

void CMonWnd::DrawSubD(HDC dc) const{	// SubD-Buchse malen (nur Schale)
 static const POINT Poly[6]={
  {SubDSpaceX*12+SubDRand*2,	0},
  {SubDSpaceX*12+SubDRand*2,	SubDRand+SubDSpaceY/2},
  {SubDSpaceX*23/2+SubDRand*2,	SubDRand*2+SubDSpaceY},
  {SubDSpaceX/2,		SubDRand*2+SubDSpaceY},
  {0,				SubDRand+SubDSpaceY/2},
  {0,				0}};
 
 HPEN hOld=SelectPen(dc,gdi.WidePen);
 POINT P;
 SetViewportOrgEx(dc,SubDPos.x,SubDPos.y,&P);
 Polygon(dc,Poly,elemof(Poly));
 SetViewportOrgEx(dc,P.x,P.y,NULL);
 SelectPen(dc,hOld);
}

void CMonWnd::DrawPins(HDC dc) const{	// SubD-Pins malen
 HFONT hOld=SelectFont(dc,gdi.SmallFont);
 SetBkMode(dc,TRANSPARENT);
 BYTE pin=0; do{
  DrawPin(dc,pin);
  DrawPinNumber(dc,pin);
 }while (++pin<25);
 SetBkMode(dc,OPAQUE);
 SelectFont(dc,hOld);
}

/*================ Bit-Kästchen ================*/

// liefert <false> wenn das Bit nicht physikalisch vorhanden ist,
// bspw. das Steuerport-Ausgaberegister hat nur 6 Bits,
// das Steuerport-Richtungsregister 4 Bits usw.
// Führt zur Ausgabe ausgegrauer Bitfelder
bool CMonWnd::BitExists(BYTE bit) const{
 if ((bit&0xF6)==0x26) return false;	// die oberen 2 Bits des Control-Registers
 if ((bit&0xF4)==0xE4) return false;	// die oberen 4 Bits des Control-Richtungsregisters
 if (!DIRECTIO && 0xD0<=bit && bit<0xD3) return false;	// die unteren 3 Bits des Status-Richtungsregisters
 if (bit==0x25) {
  if (DIRECTIO) return false;		// Bit wirkungslos bei DirectIO
  switch (ECRMODE) {
   case 0:
   case 2: return false;		// kein Richtungsbit (immer 0) bei SPP und AutoStrobe
  }
 }
 if (!DIRECTIO && (bit&0xF0)==0x10) {	// kein DirectIO und Statusport?
  bit&=7;				// Bitnummer
  if (bit>=3) return true;
  if (ECRMODE==4 && bit==0) return true;	// ECR: Mode = EPP und Bit 0 (TimeOut)?
  return false;				// Bits (0,) 1 und 2 existieren nicht
 }
 return true;
}

/************************************************************************
 * Mittelpunkt eines der 48 Bit-Kästchen beschaffen
 * bit = Bitadresse: aaaaibbb (aaaa=Portadresse, 0, bbb=Bitnummer)
 ************************************************************************/
void CMonWnd::GetBitPos(BYTE bit, POINT _ss* ppt) const{
 static const int bit7x[6]={	// X-Positionen des Bit 7 (der 6 Bitketten)
  -SubDSpaceX*2,
  -SubDSpaceX*5,
  SubDRand-SubDSpaceX*3/2,
  -SubDSpaceX*4,
  -SubDSpaceX*9,
  SubDSpaceX};

 int b=bit>>4;		// Byte-Adresse (0,1,2,12,13,14)
 if (b<3) {
  *ppt=GroupboxCenter[b];
 }else{
  *ppt=GroupboxCenter[3];
  if (b>12) ppt->y+=(SubDSpaceY>>1)+2;
  else ppt->y-=(SubDSpaceY>>1)+2;
  b-=12-3;	// für Richtungsregister (jetzt 3,4,5)
 }
 ppt->x+=bit7x[b];	// links/rechts verschieben
 ppt->x+=(~bit&7)*SubDSpaceX;
}

/************************************************************************
 * Zeichenrechteck eines der 48 Bit-Kästchen beschaffen
 * bit = Bitadresse: aaaaibbb (aaaa=Portadresse, 0, bbb=Bitnummer)
 ************************************************************************/
void CMonWnd::GetBitRect(BYTE bit, RECT _ss* rc) const{
 POINT pt;
 GetBitPos(bit,&pt);
 InflatePoint(&pt,rc,SubDSpaceX/2,SubDSpaceY/2);
}

/************************************************************************
 * Bitadresse in SubD-Pinnummer (0-basiert!) umrechnen
 * bit = Bitadresse: aaaaibbb (aaaa=Portadresse, 0, bbb=Bitnummer)
 * Liefert <false> wenn Bitadresse keiner SubD-Pinnummer entspricht
 ************************************************************************/
bool CMonWnd::FindPin(BYTE bit, BYTE _ss* ppin) const{
 BYTE pin;
 for (pin=0; pin<25; pin++) {
  if (bit==PinAssign(pin)) {	// Wenn sich aaaa und bbb gleichen
   if (ppin) *ppin=pin;
   return true;
  }
 }
 return false;
}

/************************************************************************
 * Zeichne ein Bit-Kästchen
 * bit = Bitadresse: aaaaibbb (aaaa=Portadresse, 0, bbb=Bitnummer)
 * LptRegsRd und LptRegsWr müssen die richtigen Werte bereits enthalten.
 ************************************************************************/
void CMonWnd::DrawBit(HDC dc, BYTE bit) const{
 int byt=bit>>4;	// <int> adressiert Arrays besser als <BYTE>
 BYTE Mask=1<<(bit&7);	// Bitmaske des Bytes
 BYTE BitHighRd,BitHighWr;
 RECT rc;
 HBRUSH hOldBrush=0;
 BYTE exist=BitExists(bit);
 const HBRUSH _ds*brushes = (const HBRUSH _ds*)(exist ? gdi.ColorBrush : gdi.DarkBrush);
 bool ldc=false;	// DC lokal beschafft (nicht übergeben)

 if (!dc) {
  dc=GetDC(Wnd);
  ldc=true;
 }
 BitHighRd=LptRegsRd[byt]&Mask;
 BitHighWr=LptRegsWr[byt]&Mask;
// Um den unkundigen Anwender nicht allzu sehr zu verwirren,
// bei auf EINGANG geschalteten Ports keine halbierten Quadrate zeichnen.
 if (!DIRECTIO && byt<3 && !(LptRegsRd[12+byt]&Mask))
   BitHighWr=BitHighRd;	// bei Eingängen angleichen (PullUp-Funktion unsichtbar)
 byt&=3;		// Ab sofort sind Daten- und Richtungsregister gleich behandelt
 GetBitRect(bit,&rc);
 if (BitHighRd&BitHighWr) hOldBrush=SelectBrush(dc,brushes[byt]);
 else if (!exist) hOldBrush=SelectBrush(dc,GetStockBrush(LTGRAY_BRUSH));
			// farbiges oder weißes (ggf. graues) Rechteck mit schwarzem Rand
 Rectangle(dc,rc.left,rc.top,rc.right,rc.bottom);
 if (hOldBrush) SelectBrush(dc,hOldBrush);
 if (BitHighRd^BitHighWr) {	// Dreiecke einzeichnen, wenn Schreib- und Lesewert verschieden
  HPEN hOld=SelectPen(dc,gdi.NullPen);
  POINT pt[3];
  hOldBrush=SelectBrush(dc,brushes[byt]);
  CopyRect((PRECT)pt,&rc);
  InflateRect((PRECT)pt,-1,-1);
  pt[2].x=pt[1].x; pt[2].y=pt[0].y;
  if (BitHighRd) pt[0].y=pt[1].y; else pt[1].x=pt[0].x;
  Polygon(dc,pt,3);	// farbiges Dreieck einzeichnen
  SelectBrush(dc,hOldBrush);
  SelectPen(dc,hOld);
 }

 HFONT hOld=SelectFont(dc,gdi.DescFont);
 SetBkMode(dc,TRANSPARENT);
 TCHAR Buf[3];
// Position der Ziffer (2. Zeichen), sonst zu weit rechts
// ExtTextOut verlangt im Ernst eine gültige Positionsangabe fürs Stringende!
// Sonst Probleme beim Zeichnen unter Mauspfeil, unter Vista gar keine Funktion
 static const int Dx[]={SubDSpaceX/2-1,SubDSpaceX-2};
 ExtTextOut(dc,rc.left+1,rc.top+2,ETO_CLIPPED,&rc,Buf,
   wsprintf(Buf,T("%c%d"),T("DSC")[byt],bit&7),(int far*)Dx);
 SetBkMode(dc,OPAQUE);
 SelectFont(dc,hOld);

 if (!DIRECTIO && BitHasInversion(bit)) {	// Invertierung?
  POINT pt[2];			// BorlandC verträgt keine nicht-konstanten
   pt[0].x=rc.left+2; pt[1].x=rc.right-2; // Initialisierer (C-konform)
   pt[0].y=pt[1].y=rc.top+2;	// und ruft bei C++ den Copy-Konstruktor
  Polyline(dc,pt,2);		// Überstrich zeichnen
 }
 if (ldc) ReleaseDC(Wnd,dc);
}

/************************************************************************
 * Zeichne alle 8 Bit-Kästchen eines Bytes
 * bit = Bitadresse: aaaa0--- (aaaa=Portadresse)
 ************************************************************************/
void CMonWnd::DrawByte(HDC dc, BYTE bit) const{	// Bit-Felder malen
 bool ldc=false;	// DC lokal beschafft (nicht übergeben)
 if (!dc) {
  dc=GetDC(Wnd);
  ldc=true;
 }
 for (; !(bit&8); bit++) {
  DrawBit(dc,bit);
 }
 if (ldc) ReleaseDC(Wnd,dc);
}

/************************************************************************
 * Zeichne alle (17..20) Luftlinien
 ************************************************************************/
void CMonWnd::DrawAirwires(HDC dc) const{
 HPEN hOld;
 BYTE pin,bit;
 hOld=SelectPen(dc,gdi.AirwirePen);
 for (pin=0; pin<25; pin++) {
  POINT pt[2];
  bit=PinAssign(pin);
  if (bit<0x80) {
   GetPinPos(pin,pt+0);
   GetBitPos(bit,pt+1);
   pt[1].y-=SubDSpaceY/2;	// oben ansetzen
   Polyline(dc,pt,2);
  }
 }
 SelectPen(dc,hOld);
}


/************************************************************************
 * Knöpfe zur Datenein/Ausgabe aktivieren/deaktivieren
 * Dialog umgestalten, Dialogelemente füllen
 ************************************************************************/
void CMonWnd::SetEcrMode(int Mode){
 static const BYTE Info[8]={0x52,0x52,0x52,0xDF,0x5F,0x02,0x57,0x11};
 BYTE Inf=Info[Mode];
 HWND w;
 TCHAR CmdAdr[32];
 LoadString(hInst,51/*"Kommando\0Adresse"*/,CmdAdr,elemof(CmdAdr));
// Info-Bits: Bit0: Eingabe-Byte sichtbar
//		1: Eingabe-Byte readonly, sonst "cfgA" und "cfgB" sichbar
//		2: Eingabe-Knopf sichtbar
//		3: Radiobuttons sichtbar
//		4: Ausgabe-Byte sichtbar
//		5: Ausgabe-Byte readonly
//		6: Ausgabe-Knopf sichtbar
//		7: Radiobutton-Ersatztext "Komm&ando" statt "&Adresse"
 SendDlgItemMessage(Wnd,102,CB_SETCURSEL,Mode,0);
 w=GetDlgItem(Wnd,168);	// Eingabe-Byte
 ShowWindow(w,Inf&0x01?SW_SHOW:SW_HIDE);
 (void)Edit_SetReadOnly(w,Inf&0x02);
 w=GetDlgItem(Wnd,131);	// Eingabe-Knopf
 ShowWindow(w,Inf&0x04?SW_SHOW:SW_HIDE);
 w=GetDlgItem(Wnd,122);	// Radioknopf "Adresse"
 ShowWindow(w,Inf&0x08?SW_SHOW:SW_HIDE);
 SetWindowText(w,CmdAdr
   +(Inf&0x80?0/*"Kommando*/:lstrlen(CmdAdr)+1/*"Adresse*/));
 w=GetWindow(w,GW_HWNDNEXT);
 ShowWindow(w,Inf&0x08?SW_SHOW:SW_HIDE);
 w=GetDlgItem(Wnd,130);	// Ausgabe-Knopf
 ShowWindow(w,Inf&0x40?SW_SHOW:SW_HIDE);
 w=GetDlgItem(Wnd,169);	// Ausgabe-Byte
 ShowWindow(w,Inf&0x10?SW_SHOW:SW_HIDE);
 (void)Edit_SetReadOnly(w,Inf&0x20);
 w=GetDlgItem(Wnd,118);	// Text "CfgA"
 ShowWindow(w,Inf&0x02?SW_HIDE:SW_SHOW);
 w=GetDlgItem(Wnd,119);	// Text "CfgB"
 ShowWindow(w,Inf&0x02?SW_HIDE:SW_SHOW);
 switch (Mode){
  case 7:{	// Konfigurationsmodus: cfgA und cfgB lesen
   LptRegsRd[8]=inb(0x18);
   SetDlgItemHex(Wnd,168,LptRegsRd[8]);
   KillTimer(Wnd,168);
   LptRegsRd[9]=inb(0x19);
   SetDlgItemHex(Wnd,169,LptRegsRd[9]);
   KillTimer(Wnd,169);
  }break;
 }
 w=GetDlgItem(Wnd,117);		// Knopf "lesen" (für Datenport +0)
 ShowWindow(w,Mode<2||Mode==4?SW_HIDE:SW_SHOW);
 valid&=~1;		// Datenregister einlesen lassen
 InvalidateRect(Wnd,NULL,FALSE); // ganze Client Area neu zeichnen
#ifdef WIN32
 if (hwndTT) DestroyWindow(hwndTT);
// TOPMOST ist unter Win98 notwendig, sonst ist der Tooltip hinter dem Fenster,
// sobald Hilfe-Knopf gedrückt und Fehlermeldung erschien
 hwndTT=CreateWindowEx(WS_EX_TOPMOST,TOOLTIPS_CLASS,NULL,
   TTS_NOPREFIX|TTS_ALWAYSTIP,0,0,0,0,0,0,hInst,NULL);
 AddTools(Mode);	// neue Beschriftungen
#endif
}

void CMonWnd::OnOutputButton(void){
 int Mode=(int)SendDlgItemMessage(Wnd,102,CB_GETCURSEL,0,0);
 UINT u;
 BYTE b;
 if (GetDlgItemHex(Wnd,169,&u) && u<256){
  b=(BYTE)u;
  switch (Mode){
   case 0:
   case 1:{
    outb(0,b);			// Datenbyte anlegen
    outb(2,(BYTE)(LptRegsWr[2]|0x01));	// Strobe generieren
    outb(2,(BYTE)(LptRegsWr[2]&~0x01));
   }goto xxx;
   case 2:{
    outb(0,b);
    SetDlgItemHex(Wnd,160,b);
    KillTimer(Wnd,160);
   }break;	//SPP-FIFO
   case 3:	// ECP
    outb(IsDlgButtonChecked(Wnd,122)?0:8,b); break; // Kommando:0, Daten:400h
   case 4:{	// EPP
    outb(IsDlgButtonChecked(Wnd,122)?3:4,b); // Adresse:3, Daten:4
    LptRegsWr[0]=b;	// automatisch
xxx:
    DrawByte(0,0x00);
    SetDlgItemHex(Wnd,160,b);
    KillTimer(Wnd,160);
   }break;
   case 6: outb(8,b); break;	// Test: 400h
  }
  b++;	// ein anderes Datenbyte anbieten (ist zweckmäßig!)
  SetDlgItemHex(Wnd,169,b);
  KillTimer(Wnd,169);
 }else{
  MessageBeep(MB_ICONEXCLAMATION);
  SetFocus(GetDlgItem(Wnd,169));
 }
 Edit_SetSel(GetDlgItem(Wnd,169),0,-1);
}

void CMonWnd::OnInputButton(void){
 int Mode=(int)SendDlgItemMessage(Wnd,102,CB_GETCURSEL,0,0);
 BYTE b;
 switch (Mode){
  case 3: b=inb(IsDlgButtonChecked(Wnd,122)?0x10:0x18); break;	// ECP
  case 4: b=inb(IsDlgButtonChecked(Wnd,122)?0x13:0x14); break;	// EPP
  case 6: b=inb(0x18); break;
  default: return;
 }
 SetDlgItemHex(Wnd,168,b);
 KillTimer(Wnd,168);
}

// Gelesenes Byte von Portadresse zur Darstellung aktualisieren
void CMonWnd::update(int a, BYTE b){
 WORD mask=1<<a;
 if (valid&mask) {
  if (b==LptRegsRd[a]) return;
 }else LptRegsWr[a]=b;		// beim ersten Start gleich annehmen

 switch (a) {
  case 12:
  case 13:
  case 14: LptRegsWr[a]=b; nobreak; // diese Register sind voll rücklesbar
  case 0:			// sichtbare Kästchen bzw. Pins
  case 1:
  case 2: {
   BYTE m,bit=(BYTE)(a<<4),pin;
   HDC dc=GetDC(Wnd);
   if (valid&mask) for (m=1; m; m<<=1,bit++) {	// über die Bits eines Bytes iterieren
    if ((LptRegsRd[a]^b)&m) {	// geändertes Bit?
     LptRegsRd[a]^=m;		// Bit kippen
     goto drw;
    }else if (!(valid&mask)) {
drw: DrawBit(dc,bit);		// Bit-Kästchen aktualisieren
     if (FindPin(bit,&pin)) DrawPin(dc,pin);	// Pin-Kreis aktualisieren
    }
   }
   ReleaseDC(Wnd,dc);
   if (a<3) SetDlgItemHex(Wnd,176+a,b);	// Hexadezimalanzeige aktualisieren
  }break;

  case 8:	// cfgA
  case 9:{	// cfgB
   SetDlgItemHex(Wnd,160+a,b);	// Hexadezimalanzeige aktualisieren
   KillTimer(Wnd,160+a);
  }break;

  case 10:{	// ECR (+402)
   CheckDlgButton(Wnd,121,b&0x01);
   CheckDlgButton(Wnd,120,(b>>1)&0x01);
   if (!(valid&mask) || (LptRegsRd[10]^b)&0xE0) SetEcrMode(b>>5);
   SetDlgItemHex(Wnd,176+a,b);	// Hexadezimalanzeige aktualisieren
  }break;
 }
 valid|=mask;
 LptRegsRd[a]=b;
}

// Ein zu schreibendes Bit ("Bitnummer" b) kippen
// Sollte nicht fürs Datenport aufgerufen werden, wenn Seiteneffekte zu erwarten sind!
void CMonWnd::ToggleBit(BYTE bit) {
 int i=bit>>4;
 outb(i,(BYTE)(LptRegsWr[i]^(1<<(bit&7))));	// neues Byte ausgeben
 if (i<3) {
  SetDlgItemHex(Wnd,160+i,LptRegsWr[i]);	// anzeigen
  KillTimer(Wnd,160+i);				// EN_CHANGE abwürgen
 }
 DrawBit(0,bit);	// anzeigen
 update(i,inb(i|0x10));	// gleiche Adresse einlesen
}

/**************
 * Bubblehelp *
 **************/

// liefert 0xFF wenn kein Treffer
BYTE CMonWnd::BitFromMessagePos(void) const{
 TTHITTESTINFO hti;
 hti.hwnd=Wnd;
 DWORD dw=GetMessagePos();
 hti.pt.x=GET_X_LPARAM(dw);
 hti.pt.y=GET_Y_LPARAM(dw);
 ScreenToClient(Wnd,&hti.pt);
 hti.ti.cbSize=CCSIZEOF_STRUCT(TOOLINFO,lpszText);
 if (SendMessage(hwndTT,TTM_HITTEST,0,(LPARAM)(LPTTHITTESTINFO)&hti))
   return LOBYTE(hti.ti.uId);
 return 0xFF;
}

static LPCTSTR Index2String(LPCTSTR p, int i) {
// Liefert String zum Index, arbeitet mit FAR-Zeigern
 if (p) for (;i;i--) p+=lstrlen(p)+1;
 return p;
}

// Invertierungsstrich '/' entfernen oder hinzufügen, String-Rest schieben, benötigt reichlich Platz
// Liefert Verschiebung (in Zeichen) zur Korrektur nachfolgender Zeiger
static int SwapInv(TCHAR _ss*p) {
 int l=lstrlen(p)*sizeof(TCHAR);	// Länge in Bytes ohne \0
 if (*p=='/') {
  RtlMoveMemory(p,p+1,l);		// heranziehen (mitsamt \0)
  return -1;
 }
 RtlMoveMemory(p+1,p,l+sizeof(TCHAR));	// wegschieben (mitsamt \0)
 *p='/';
 return 1;
}

void CMonWnd::GetBubblehelpText(BYTE Mode, UINT code, LPTSTR buf, UINT buflen) const{
 BYTE bit=LOBYTE(code),byt;
 TCHAR s[256];		// Zusammensetzungs-Puffer (reichlich dimensioniert)
 TCHAR _ss*sigp1=NULL, _ss*sigp2=NULL;	// Signalnamen-Zeiger (wegen Invertierungsstriche)
 byt=bit>>4;
 int bnr=bit&0x07;
 GUARD(s[elemof(s)-1]=(TCHAR)0xCC);	// Überlaufwächter
#ifdef WIN32
# define NEWLINE T("\r\n")
#else
# define NEWLINE " -- "		// Win98 unterstützt keine mehrzeiligen Tooltips
#endif 
 switch (byt) {
  case 0:{			// Datenport
   wsprintf(s,T("Data %u"),bnr);
  }break;

  case 1:{			// Statusport
   const TCHAR _ds*a=NULL;	// alternativer Signalname (initialisiertes Datensegment)
   TCHAR b1[128];		// lokalisierte Signal/Anschlussnamen aus Ressource
   TCHAR b2[128];		// lokalisierte Modus-Bezeichner: normal,Nibble,Byte,ECP,EPP
   int i=0;
   LoadString(hInst,49/*8 Status-Bitnamen*/,b1,elemof(b1));
   LoadString(hInst,52/*Modus-Bezeichner*/,b2,elemof(b2));
// Alternative ermitteln
   switch (Mode){
    case 0:
    case 1: a=NibS; break;	// Nibble Mode
    case 2: a=BytS; break;	// Autostrobe: Signalnamen des Byte-Mode anzeigen
    case 3: a=EcpS; break;	// ECP
    case 4: a=EppS; break;	// EPP
   }
   if (a) {
    a=(const TCHAR _ds*)Index2String(a,bnr);
    if (!*a) a=NULL;		// keine Alternative
   }
   if (DIRECTIO) {
    if (bnr<3) bnr=0;		// "Extra-Bit" ausgeben lassen
   }else{
    if (!bnr) bnr++;		// "reserviert" ausgeben lassen
   }
   if (a) i=wsprintf(s,T("%s: "),(LPTSTR)b2);
   sigp1=s+i;			// hier später Invertierung patchen
   i+=wsprintf(sigp1,T("%s (%s)"),Index2String(SppS,bnr),Index2String(b1,bnr));
   if (a) {
    i+=wsprintf(s+i,NEWLINE T("%s: "),Index2String(b2,Mode?Mode:1));
    sigp2=s+i;			// hier später Invertierung patchen
    lstrcpy(sigp2,a);
   }
  }break;

  case 2:{
   const TCHAR _ds*a=NULL;	// alternativer Signalname
   TCHAR b1[128];		// lokalisierte Signal/Anschlussnamen aus Ressource
   TCHAR b2[128];		// lokalisierte Modus-Bezeichner: normal,Nibble,Byte,ECP,EPP
   int i=0;
   LoadString(hInst,50/*6 Steuer-Bitnamen*/,b1,elemof(b1));
   LoadString(hInst,52/*Modus-Bezeichner*/,b2,elemof(b2));
// Alternative ermitteln
   if (bnr<4) switch (Mode){
    case 0:
    case 1: a=NibC; break;	// Nibble Mode
    case 2: a=BytC; break;	// Autostrobe: Signalnamen des Byte-Mode anzeigen
    case 3: a=EcpC; break;	// ECP
    case 4: a=EppC; break;	// EPP
   }
   if (a) {
    a=(const TCHAR _ds*)Index2String(a,bnr);
    if (!*a) a=NULL;		// keine Alternative
   }
   if (bnr>6) bnr=6;		// beide nicht vorhandenen Bits gleich behandeln
   if (a) i=wsprintf(s,T("%s: "),(LPTSTR)b2);	// "normal: "
   sigp1=s+i;			// hier später Invertierung patchen
   i+=wsprintf(sigp1,T("%s (%s)"),Index2String(SppC,bnr),Index2String(b1,bnr));
   if (a) {
    i+=wsprintf(s+i,NEWLINE T("%s: "),Index2String(b2,Mode?Mode:1));
    sigp2=s+i;			// hier später Invertierung patchen
    lstrcpy(sigp2,a);
   }
  }break;

  case 12:{
   wsprintf(s,T("DirD %d"),bnr);
  }break;
  case 13:{
   wsprintf(s,T("DirS %d"),bnr);
  }break;
  case 14:{
   if (bnr<4) wsprintf(s,T("DirC %d"),bnr);
   else{
    LoadString(hInst,50,s,elemof(s));
    lstrcpy(s,Index2String(s,6));	// "nicht vorhanden"
   }
  }break;

  default: s[0]=0;
 }
// Signal-Invertierungen nur am Pin umkehren.
// Bei DIRECTIO auch am Bit umkehren (Signalbezeichnungen aus Hardware-Sicht)
 if ((HIBYTE(code) || DIRECTIO) && BitHasInversion(bit)) {
  int dist=0;
  if (sigp1) dist=SwapInv(sigp1);
  if (sigp2) SwapInv(sigp2+dist);
 }
 ASSERT(s[255]==(TCHAR)0xCC);
 lstrcpyn(buf,s,buflen);
#undef NEWLINE
}

#ifdef WIN32
void CMonWnd::AddTools(BYTE Mode) const {	// Alle 6*8 + 17(20) Tools mitteilen
#else
void CMonWnd::AddTools() const {	// Alle 6*8 + 17(20) Tools mitteilen
#endif
 TOOLINFO ti;
 ti.cbSize=CCSIZEOF_STRUCT(TOOLINFO,lpszText);
// Keine ordentliche Funktion von TTF_SUBCLASS unter Win98:
// Sobald das Fenster einmal den Fokus verlor, kommt kein Tooltip mehr
// Keine Funktion von LPSTR_TEXTCALLBACK unter Win2k und Unicode;
// es werden ohnehin viel zu viele Callbacks ausgelöst
// Dagegen keine Funktion von regulären Strings unter Win98
 ti.uFlags=0;
 ti.hwnd=Wnd;
#ifdef WIN32
 TCHAR buf[80];
 ti.lpszText=buf;
#else
 ti.lpszText=LPSTR_TEXTCALLBACK;
#endif
 for (BYTE bit=0; bit<0xF0; ) {
  GetBitRect(bit,&ti.rect);
  ti.uId=bit;
#ifdef WIN32
  GetBubblehelpText(Mode,UINT(ti.uId),buf,elemof(buf));
#endif
  SendMessage(hwndTT,TTM_ADDTOOL,0,(LPARAM)(LPTOOLINFO)&ti);
  bit++; if (bit&8) bit+=8;		// Bit 3 (ehem. Negativ-Bit) überspringen
  if (bit==0x30) bit+=0xC0-0x30;	// von 3 auf 12 springen
 }
 for (BYTE pin=0; pin<25; pin++) {
  BYTE bit=PinAssign(pin);
  if (bit!=0xFF) {
   GetPinRect(pin,&ti.rect);
   ti.uId=bit|0x100;
#ifdef WIN32
   GetBubblehelpText(Mode,UINT(ti.uId),buf,elemof(buf));
#endif
   SendMessage(hwndTT,TTM_ADDTOOL,0,(LPARAM)(LPTOOLINFO)&ti);
  }
 }
 SendMessage(hwndTT,TTM_SETMAXTIPWIDTH,0,256);	// aktiviert mehrzeilige Tooltipps
}

void CMonWnd::TimerUpdate(void) {
 int Mode;
 static const BYTE SafeRegs[]={0x1F,0x1A,0x11,0x12,0x1C,0x1D,0x1E};
// Reihenfolge: Feature, ECR, Status, Control, 3 Richtungsregister
 BYTE b[elemof(SafeRegs)];
 if (inbytes(SafeRegs,sizeof(SafeRegs),b)==sizeof(SafeRegs)) {
  for (int i=0; i<elemof(SafeRegs); i++) {
   update(SafeRegs[i]&0x0F,b[i]);
  }
  Mode=b[1]>>5;
  if (Mode<2 || Mode==4) update(0,inb(0x10));	// im ECP-Modus kein Auslesen! (Test?)
// Im Autostrobe-Modus und im Testmodus bewirkt das Lesen von Adresse+0
// das Leeren der FIFO (am Rechner "Mixer"): undokumentiert!
// Bei Autostrobe wartet die FIFO auf BUSY=LOW
  if (Mode==7) {	// Nur im Testmodus automatisch auslesen
   static const BYTE TestmodeRegs[]={0x18,0x19};
   BYTE b[sizeof(TestmodeRegs)];
   inbytes(TestmodeRegs,sizeof(TestmodeRegs),b);
   update(8,b[0]);	// ändert sich eigentlich nie (sind zurzeit Konstanten im USB2LPT-Gerät)
   update(9,b[1]);
  }
 }else ASSERT(false);
}

// gemeinsam genutzt mit Extras-Dialog
void PopulateEcrComboBox(HWND w) {
 TCHAR buf[256], _ss* p=buf;
 int i=8;	// Bei Win32 sind doppelt nullterminierte Strings nicht zu laden
 LoadString(hInst,48/*ECR-Modes*/,buf,elemof(buf));
 do{
  (void)ComboBox_AddString(w,p);
  p+=lstrlen(p)+1;	// Hingegen sind Nullen dazwischen kein Problem
 }while (--i);
}

/*******************
 * Dialog-Prozedur *
 *******************/

// Während die anderen beiden PropertySheetPages mit den wenigen "shared"-Fensterdaten
// (Struktur "TSetup") auskommen, wird hier das komplexere TMonData bemüht,
// welches einen Zeiger nach TSetup beinhaltet.
INT_PTR CALLBACK _loadds MonDlgProc(HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
 PMonData MD=(PMonData)GetWindowPtr(Wnd,DWLP_USER);
 if (!MD && Msg!=WM_INITDIALOG) return FALSE;

// RelayEvent zum Tooltip laut SDK
 if (MD && MD->hwndTT && Msg>=WM_MOUSEFIRST && Msg<=WM_MOUSELAST) {
  MSG msg;
  msg.hwnd=Wnd;
  msg.message=Msg;
  msg.wParam=wParam;
  msg.lParam=lParam;
  msg.time=GetMessageTime();
  DWORD pos=GetMessagePos();
  msg.pt.x=GET_X_LPARAM(pos);
  msg.pt.y=GET_Y_LPARAM(pos);
  SendMessage(MD->hwndTT,TTM_RELAYEVENT,0,(LPARAM)(LPMSG)&msg);
 }

 switch (Msg) {
  case WM_INITDIALOG:{
   PopulateEcrComboBox(GetDlgItem(Wnd,102));
   MD=(PMonData)LocalAlloc(LPTR,sizeof(TMonData));
   if (!MD) return TRUE;
   SetWindowPtr(Wnd,DWLP_USER,MD);
   MD->Wnd=Wnd;
   MD->S=(PSetup)((LPPROPSHEETPAGE)lParam)->lParam;
   ChangeFonts(Wnd,MD->S);
   AddContextHelpButton(Wnd);
   for (int i=0; i<elemof(MD->GroupboxCenter); i++) {
    RECT R;
    GetWindowRect(GetDlgItem(Wnd,16+i),&R);	// Groupboxen-ID = 16..19
    MD->GroupboxCenter[i].x=(R.left+R.right)>>1;
    MD->GroupboxCenter[i].y=((R.top+R.bottom)>>1)+4;
   }
   MapWindowPoints(0,Wnd,MD->GroupboxCenter,elemof(MD->GroupboxCenter));
   MD->SubDPos.x=MD->GroupboxCenter[0].x-SubDSpaceX*6-SubDRand;
   MD->SubDPos.y=((MD->GroupboxCenter[0].y+MD->GroupboxCenter[1].y)>>1)-SubDSpaceY-SubDRand;
   MD->gdi.CreateGdiResources();
   CheckDlgButton(Wnd,122,TRUE);	//"Adresse" auswählen
  }return TRUE;

  case WM_TIMER: switch (wParam){
   case 1: MD->TimerUpdate(); break;	// zyklisches Update
   case 160:
   case 161:
   case 162:
   case 168:
   case 169:
   case 170:{	// Editfeld geändert: automatische Byte-Ausgabe
    UINT v;
    KillTimer(Wnd,wParam);	// Diese Timer sollen nicht-zyklisch sein
#ifdef WIN32
    SendDlgItemMessage(Wnd,(int)wParam,EM_SETSEL,0,(LPARAM)-1);	// alles markieren
#else
    SendDlgItemMessage(Wnd,(int)wParam,EM_SETSEL,0,MAKELONG(0,-1));
#endif
    if (wParam==169 && (MD->LptRegsWr[10]&0xE0)!=0xE0) {
     MD->OnOutputButton();	// Abzweigen zum Äquivalent des Knopfdrückens
     break;
    }
    if (GetDlgItemHex(Wnd,(UINT)wParam,&v) && v<256){
     wParam-=160;
     if (MD->LptRegsWr[wParam]!=v){
      MD->outb((BYTE)wParam,(BYTE)v);	// setzt LptRegsWr
      if (wParam<3) MD->DrawByte(0,(BYTE)(wParam<<4));
     }
    }else MessageBeep(MB_ICONEXCLAMATION);
   }break;
  }break;

  case WM_PAINT:{
// Normalerweise sollten die Sub-Buchse und die Bitfelder eigenständige Controls darstellen.
// Das würde die Implementierung eines (zz. nicht vorhandenen) Tastaturinterfaces erleichtern.
// Für die (sehr hilfreichen!) Luftlinien wird dennoch der Zugriff auf die Dialog-Zeichenfläche gebraucht.
   PAINTSTRUCT PS;
   BeginPaint(Wnd,&PS);
   MD->DrawSubD(PS.hdc);
   MD->DrawAirwires(PS.hdc);
   MD->DrawPins(PS.hdc);
   MD->DrawByte(PS.hdc,0x00);	// Datenregister
   MD->DrawByte(PS.hdc,0x10);
   MD->DrawByte(PS.hdc,0x20);
   MD->DrawByte(PS.hdc,0xC0);	// Richtungsregister
   MD->DrawByte(PS.hdc,0xD0);
   MD->DrawByte(PS.hdc,0xE0);
   EndPaint(Wnd,&PS);
  }return TRUE;

  case WM_SETCURSOR:{
   if (MD->BitFromMessagePos()!=0xFF) {
    SetCursor(LoadCursor(0,IDC_HAND));
    return TRUE;
   }
  }break;

  case WM_LBUTTONDOWN:
  case WM_LBUTTONDBLCLK:{
   BYTE bit=MD->BitFromMessagePos();
   if (bit!=0xFF) MD->ToggleBit(bit);
  }break;

  case WM_NOTIFY:
   switch (((LPNMHDR)lParam)->code){
   case PSN_SETACTIVE: {
    OpenDev(MD->S);
    MD->valid=0;			// dies lässt Tooltips erzeugen
#ifndef WIN32
// TOPMOST ist unter Win98 notwendig, sonst ist der Tooltip hinter dem Fenster,
// sobald Hilfe-Knopf gedrückt und Fehlermeldung erschien
    MD->hwndTT=CreateWindowEx(WS_EX_TOPMOST,TOOLTIPS_CLASS,NULL,
      TTS_NOPREFIX|TTS_ALWAYSTIP,0,0,0,0,0,0,hInst,NULL);
    MD->AddTools();
#endif
    SendMessage(Wnd,WM_TIMER,1,0);	// sofort aktualisieren
    SetTimer(Wnd,1,200,NULL);		// zyklisch aktualisieren
   }break;
   case PSN_KILLACTIVE: {
    KillTimer(Wnd,1);
    if (MD->hwndTT) DestroyWindow(MD->hwndTT);	// Tooltips unnötig
    MD->hwndTT=0;
    CloseDev(MD->S);
   }break;
#ifndef WIN32
   case TTN_NEEDTEXT: {
#define ttt ((LPTOOLTIPTEXT)lParam)
    MD->GetBubblehelpText(MD->LptRegsRd[10]>>5,ttt->hdr.idFrom,ttt->szText,elemof(ttt->szText));
#undef ttt
   }return TRUE;
#endif
   case PSN_HELP: {
    WinHelp(Wnd,HelpFileName,HELP_CONTEXT,MAKELONG(0,105));
   }break;
  }break;

  case WM_CONTEXTMENU: WM_ContextMenu_to_WM_Help(Wnd,wParam,lParam,true); break;

  case WM_HELP: {
   short id=short(((LPHELPINFO)lParam)->iCtrlId);
   if (id && id!=-1) {
// Die Hilfe für die Bits erledigt sich durch die 4 GroupBoxes von allein
// Die Hilfe zu den SubD-Pins wird per IconTitle-Dummyfenster erschlagen.
    WinHelp(Wnd,HelpFileName,HELP_CONTEXTPOPUP,MAKELONG(id,105));
    SetWindowLongPtr(Wnd,DWLP_MSGRESULT,1);
    return TRUE;
   }
  }break;
    	
  case WM_COMMAND: switch (LOWORD(wParam)) {
   case 102: switch (GET_WM_COMMAND_CMD(wParam,lParam)) { // SPP, ECP usw.
    case CBN_SELCHANGE: {
     int Mode=ComboBox_GetCurSel((HWND)lParam);
     MD->outb(10,(BYTE)((MD->LptRegsWr[10]&0x1F)|(Mode<<5)));
     SetDlgItemHex(Wnd,170,MD->LptRegsWr[10]);
     KillTimer(Wnd,170);
    }break;
   }break;

   case 117: MD->update(0,MD->inb(0x10)); break;	// Knopf "Lesen" für Datenport (+0)

   case 160:
   case 161:
   case 162:
   case 168:	// Eingabe-Byte: Nur editierbar im Konfigurationsmodus (cfgA)
   case 169:	// Ausgabe-Byte oder cfgB
   case 170: switch (GET_WM_COMMAND_CMD(wParam,lParam)) {
    case EN_CHANGE: SetTimer(Wnd,LOWORD(wParam),500,NULL); break;
   }break;

   case 131: MD->OnInputButton(); break;

   case 130: MD->OnOutputButton(); break;
  }break;

  case WM_DESTROY:{
   MD->gdi.DeleteGdiResources();
   LocalFree(MD);
   SetWindowPtr(Wnd,DWLP_USER,NULL);
  }break;
 }

 return FALSE;
}

/*
TODO:
* Warum kommt unter W2k kein CoInstaller? (Unter Win98 gibt's eh' keinen!)
*/

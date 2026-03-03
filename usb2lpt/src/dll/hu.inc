// Output of Google Translation
// DIALOG(EX)
#define S10000	"Emuláció"
#define S10001	"Emulált port &címe:"
#define S10002	"LPT tartozékhoz &módja:"
#define S10003	"Csapdázási módszer és kijelentkezés utasítások"
#define S10004	"&Debug regisztráció csapda"
#define S10005	"(fogások mind az ízek és kijelentkezés utasítások)"
#define S10006	"&Force elosztása, akkor is, ha korábban előirányzott"
#define S10007	"&READ/WRITE_PORT_UCHAR/USHORT/ULONG átirányítástól"
#define S10008	"(gyorsabb, stabilabb)"
#define S10009	"Teljesítmény lehetőségek"
#define S10010	"&Használja írásvédő vissza OUT utasítások"
#define S10011	"(Összekapcsol több OUT utasításokat egy USB-csomag)"
#define S10012	"&Küldje csomag után időtúllépési érték"
#define S10014	"Írásvédett &vissza a helyi másolatot a kimeneti regiszterek"
#define S10015	"&Pontosabban …"
#define S10016	"(oda-vissza az utolsó írásbeli érték az oktatást, nem működik, ha eredményeket hajtott [Low] külső hardver)"

#define S10100	"Statisztika"
#define S10101	"Debug regisztráció csapda"
#define S10102	"&Nullára"
#define S10103	"OUT utasítások:"
#define S10104	"IN utasítások:"
#define S10105	"Emuláció hibák:"
#define S10106	"(nem támogatott IN vagy OUT oktatás, például OUTSB)"
#define S10107	"Lopott debug regisztrációk:"
#define S10108	"(Ismeretlen járművezető írta debug regisztrációkot)"
#define S10109	"Felülírassanak debug regisztrációk észlel egy 100 ms kernel időmérőt."
#define S10110	"Tényleg hozzá:"
#define S10114	"READ_PORT_UCHAR / WRITE_PORT_UCHAR átirányítástól"
#define S10115	"N&ullára"
#define S10116	"Túlméretezett:"
#define S10117	"USB2LPT &speciális …"
#define S10118	"Firmware &frissítés …"
#define S10119	"haftmann#software, Műszaki Egyetem Chemnitz, Kara elektronika és információs technológia, Professzori mérő és érzékelőtechnika, 2006-2011"

#define S10200	"Írásvédett vissza másolatát kimeneti regiszterek"
#define S10201	"&Adat port (bázis cím + 0)"
#define S10202	"Ellenőrzési &kikötő (bázis cím + 2)"
#define S10203	"&Egyéb port (ECP, EPP)"
#define S10204	"OK"
#define S10205	"Mégsem"

#define S10302	"A rossz beállítás crash a számítógépet, vagy edd meg a tartalmát a merevlemezre! Használata az egyik a három előre megadott címekre kell biztonságos."
#define S10304	"Az LPT tartozékhoz mód lehetővé teszi a további csapdázási port címeket. Ezek nagyon ritkán használják, így az SPP beállítás OK gombra."
#define S10305	"Ezek és a további beállításokat is, hogy később a Windows Eszközkezelőből. Pont a USB2LPT eszközzel, és nyitott a Tulajdonságokra."

#define S10400	"USB2LPT speciális"
#define S10401	"Feature Regisztráció"
#define S10402	"(A Feature Regisztráció fog tartani, a tartós USB2LPT EEPROM a készüléket. Keresztül hozzáférést DeviceIoControl)"
#define S10403	"Nyitott kollektorosok &adatközponti Port (+0)"
#define S10404	"Nyitott &kollektorosok Ellenőrzési Port (+2)"
#define S10405	"kapcsoljuk ki &pullup-ok (és 5V a Pin25 ha igen vezetékes)"
#define S10406	"Közvetlen be-/kimenet &mód (USB2LPT átalakítja az I/O-eszköz)"
#define S10407	"(Gyenge 5V helyett erős 3.3V magas szintű outputs)"
#define S10408	"(A pipa szürkítve jelenti: Nyitott kollektorosok a SPP módban, Push-Pull egyéb)"
#define S10409	"(csak az energia megtakarítás)"
#define S10410	"(Nincs több inverted állapotának ellenőrzése és vonalak! Ezért nem hasznos LPT vetélkedés, a hozzáférési DeviceIoControl keresztül. Rev.4 20 I / O lines, 17 egyéb. Irányba megjelenítése bites és egy pin-választható.)"
#define S10411	"&Elhelyez && megismétlés"
#define S10412	"&Beolvasás a rövidzárlat"
#define S10413	"Sorozatszám"
#define S10414	"&USB-leíróban"
#define S10415	"Firmware dátuma"
#define S10416	"Tompított kék &LED"
#define S10417	"Indítási értéket EC&R (üzemmód):"

#define S10500	"Monitor"
#define S10501	"Data &port (+0)"
#define S10502	"Olvas"
#define S10503	"&Status port (+1)"
#define S10504	"Co&ntrol port (+2)"
#define S10506	"&Olvas Byte"
#define S10507	"&Cím"
#define S10508	"&Data"
#define S10509	"&Írj Byte"
#define S10512	"FIFO teljes"
#define S10513	"FIFO üres"
#define S10514	"Irány nyilvántartás (USB2LPT egyedi)"

// STRINGTABLE
#define S16 "Te nagyon, nagyon szeretnénk használni ezt a nem szabványos port címe? Lehet crash számítógépét!"
#define S17 "Ez a port címe nem érvényes hexadecimális szám!"
#define S18 "Ezt a port címet a baj: túl kevesebb, túl nagy, vagy nem divisable a 4!"
#define S19 "A port címe nem osztott, 8, EPP emulációt nem lehetséges!"
#define S20 "Rossz számot Timeout érték! (Nem lehet nagyobb, mint 1000 ms)."
#define S21 "Hozzárendelésére emulált port címe"
#define S22 "Ez a kikötő címet meg kell egyezniük, hogy a címet az Ön által használt szoftver."
#define S23 "Ez a firmware nem támogatja a USB2LPT sajátos jellemzője regisztráljon!"
#define S24 "semmi"	// Seriennummer
#define S25 "Közvetlen &segítség"
#define S26 "Bul&k-Transfer (schneller, bis Windows XP nutzbar)"
#define S27 "Sonst Interrupt-Transfer"
#define S28 "Das Aktivieren von Bulk-Transfer macht das USB2LPT komplett unbrauchbar unter Windows Vista und neuer!\nDenken Sie daran, dieses Bit zurückzustellen, bevor Sie damit umziehen.\n\nFortfahren?"
#define S29 "Der Debugregister-Trap beißt sich mit PatchGuard, der nach paar Minuten mit Bluescreen 0x109 aufwartet, wenn keine Vorkehrungen unternommen wurden.\n\nFortfahren?"

#define S32 "A firmware van írva, hogy az EEPROM sikeresen. Cycle hogy készüléket, hogy az új firmware."
#define S33 "Ismeretlen hiba a firmware írás a EEPROM. Kérem, ne húzza eszközt és próbálja újra. USB2LPT A készülék nem működik többé!"
#define S34 "File „%s“ nem fogadható el, hogy a firmware USB2LPT készüléket!"
#define S35 "Dátuma aktuális (futó) firmware: %s\n\nDátum firmware az írásbeli: %s\n\nFolytassam?"
#define S36 "ismeretlen"		// Firmware-Datum (frühe Ausgaben enthielten kein Datum)
#define S37 "Firmware\0*.iic"	// Filter für Datei-öffnen-Standarddialog
#define S38 "Firmware tökéletes funkciót."
#define S39 "Nem megfelelő funkció észlelt! Run Kurzschlusstest.EXE a részletekért."
#define S40 "Ugye magas szintű intézkedés minden 17 sorokat?"
#define S41 "Die Firmware der Low-Speed-USB2LPT mit ATmega-Controller kann hiermit nicht aktualisiert werden, aber es kann der Bootloader aktiviert werden, um anschließend mittels »bootloadHID.EXE« zu brennen.\n\nFolytassam?"
#define S42 "Dieses USB2LPT unterstützt kein Firmware-Update via USB.\n\n(Első EEPROM-Byte: 0x%02X)"

#define S48 "SPP (Nibble)\0Kétirányú\0AutoStrobe\0ECP\0EPP\0fenntartva\0vizsgálati mód\0Konfiguráció"
#define S49 "kiegészítő\0fenntartva\0Interrupt kérés\0Hiba\0Online\0Papír vége\0Visszaigazolás\0Elfoglalt"
#define S50 "Strobe\0Automatikus feed\0Inicializálási\0Válasszon Bemeneti\0Interrupt felhatalmaz\0Az adatok az irányba, 1 = bemenet\0végre nem hajtott"
#define S51 "&Parancs\0&Cím"		// Alternative Beschriftung für Wellenschalter (Radiobutton) 105/122
#define S52 "általában\0Nibble mód\0Byte mód\0ECP mód\0EPP mód"

// VERSIONINFO
#define I002	14,1
#define S003	"Tulajdonságlapját Szolgáltató"	// FileDescription
#define S005	"Közkinccsé"			// LegalCopyright
#define S006	"h#s USB-LPT átalakító"		// ProductName
#define S007	"Kernel módú illesztőprogram"	// FileDescription für .SYS-Treiber

/* INF file strings (processed by makefile)
InstDisk="Driver lemez h#s USB-LPT átalakítóért"
DevDesc ="h#s USB-LPT átalakító"
SvcDesc ="h#s driver USB-LPT átalakítóért (usb2lpt.sys)"
*/

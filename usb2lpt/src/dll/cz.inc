// Translated by Alena Šebková

// DIALOG(EX)
#define S10000	"Emulace"
#define S10001	"emulační port&adresa:"
#define S10002	"LPT-&rozšíření:"
#define S10003	"metoda odběru pro IN- a OUT-příkazy"
#define S10004	"pomocí ladicího rejstříku"
#define S10005	"(zahrnuje všechny tipy IN- und OUT-příkazů)"
#define S10006	"&provést obsazení, i když se zdají být obsazené"
#define S10007	"přesměrování &READ_PORT_UCHAR / WRITE_PORT_UCHAR"
#define S10008	"(rychlejší a stabilnější)"
#define S10009	"Performance-Optionen"
#define S10010	"&cache zápisu pro OUT-příkazy"
#define S10011	"(sdružení více OUT-příkazů do jedné USB-složky)"
#define S10012	"odeslání složky provést podle"
#define S10014	"zpětné čtení kopie z výstupního registru"
#define S10015	"přesně&ji…"
#define S10016	"(dodává naposledy zapsanou hodnotu IN příkazem; nefunguje, pokud jsou výstupy řízeny externím hardwarem)"

#define S10100	"Statistika"
#define S10101	"ladicí rejstřík-odběr"
#define S10102	"uvést &nulu"
#define S10103	"OUT-příkazy:"
#define S10104	"IN-příkazy:"
#define S10105	"emulační chyba:"
#define S10106	"(nepodporovaný IN- nebo OUT-příkaz, např. OUTSB)"
#define S10107	"ukradený ladící rejstřík:"
#define S10108	"(neznámý ovladač přepsal ladící rejstřík)"
#define S10109	"Přepsané ladící rejstříky jsou detekovány 100-ms-Timerem."
#define S10110	"Opravdu přiřadit k:"
#define S10114	"Přesměrování READ_PORT_UCHAR / WRITE_PORT_UCHAR"
#define S10115	"u&vést nulu"
#define S10116	"nadměrná délka:"
#define S10117	"USB2LPT-n&adstandartní…"
#define S10118	"&Update Firmware…"
#define S10119	"haftmann#software, Technická univerzita Chemnitz,\nFakulta elektrotechniky a informační techniky,\nProfesura měřící a sensorové techniky, 2006-2009"


#define S10200	"Kopie zpětného snímání výstupního registru"
#define S10201	"&datový port (základní adresa + 0)"
#define S10202	"&řídící port (základní adresa + 2)"
#define S10203	"&speciální porty (ECP, EPP)"
#define S10204	"OK"
#define S10205	"ukončit"

// Poslední strana pro asistenta hardwaru (CoInstaller; jen Win32)
#define S10302	"Chybně zadaná adresa může „zmrazit“ váš počítač nebo způsobit ještě něco horšího! Použití jedné ze tří nabízených adres je bezpečné."
#define S10304	"LPT-rozšíření způsobí emulaci dalších adres portu.\nJe nutná jen zřídka a SPP-nastavení je OK."
#define S10305	"Tato a následující nastavení mohou být aktivována přes vlastnosti v správě zařízení. "

#define S10400	"USB2LPT-nadstandartní vybavení"
#define S10401	"Rejstřík feature"
#define S10402	"(Rejstřík Feature je trvale uložen v EEPROM USB2LPT. přístup přes DeviceIoControl)"
#define S10403	"otevřené kolektory pro &datový port (+0)"
#define S10404	"otevřené kolektory pro &řídící port (+2)"
#define S10405	"&Pullups (a 5V na Pin25, pokud byl přidán dodatečně) odpojit"
#define S10406	"přímý &vstupní/výstupní režim (USB2LPT se stane I/O-…)"
#define S10407	"(způsobí slabé výchozí napětí 5V místo silného 3,3V)"
#define S10408	"(Šedé pole znamená: Otevřené kolektory v SPP-režimu, Push-Pull ve všech ostatních režimech)"
#define S10409	"(Opatření k úspoře energie)"
#define S10410	"(…zařízením - Invertace odpadají. Proto není žádná správná simulace paralelního portu, pro přístup s DeviceIoControl. U Rev.4 je 20 místo 17 vstupních/výstupních vedení. Směr je možné jednotlivě nastavit, pro každý pin.)"
#define S10411	"Stanovení && &zpětné snímání"
#define S10412	"zkratový &test (bez periferie!)"
#define S10413	"sériové číslo"
#define S10414	"v &USB deskriptoru"
#define S10415	"datum firmware"
#define S10416	"Šedě modrá &LED"
#define S10417	"Spuštění hodnota &ECR (provozní režim):"

#define S10500	"Monitor"
#define S10501	"&datový port (+0)"
#define S10502	"Čtení"
#define S10503	"&statusový port (+1)"
#define S10504	"&řídící port (+2)"
#define S10506	"načtení byte"
#define S10507	"&adresa"
#define S10508	"da&ta"
#define S10509	"vydat byte"
#define S10512	"FIFO plný"
#define S10513	"FIFO prázdný"
#define S10514	"směrový rejstřík (USB2LPT-specifický)"

// STRINGTABLE 	// Emulace+Statistika
#define S16 "Chcete opravdu použít tuto nestandartní adresu portu?\n\nJste tím na nejlepší cestě poslat váš počítač na onen svět!"
#define S17 "Uvedená adresa portu není platné šestnáctkové číslo!"
#define S18 "Adresa portu je nespolehlivá: příliš malá, velká nebo nedělitelná čtyřmi!"
#define S19 "U adres portu, které nejsou dělitelné 8, není možná EPP emulace !"
#define S20 "Neplatný údaj při překročení časového limitu! (Nesmí být vyšší než 1000 ms.)"
#define S21 "Stanovení emulační adresy portu"
#define S22 "Adresa portu musí být uvedena v soulad s uživatelským software."
#define S23 "Mikroprogramové vybavení (Software v USB2LPT-zařízení) nepodporuje Feature rejstřík!"
#define S24 "žádné"	// sériové číslo
#define S25 "kontextová nápověda"
#define S26 "&Bulk transfer (faster; good for XP and earlier)"
#define S27 "Interrupt transfer otherwise"
#define S28 "Activating bulk transfer renders USB2LPT defective under Windows Vista and newer!\nRemember to reset this bit before upgrading your system.\n\npokračování?"
#define S29 "The debug register trap hook will be detected by PatchGuard some minutes later, which results in BSoD 0x109 when no countermeasure was taken.\n\npokračování?"

// Update mikroprogramového vybavení
#define S32 "Mikroprogramové vybavení může být zapsáno do EEPROM.\nZařízení odpojit a znovu připojit, aby se spustilo nové mikroprogramové vybavení."
#define S33 "Nejasná chyba při zapisování mikroprogramového vybavení. Neodpojujte a znovu vyzkoušejte. Zařízení již není funkční!"
#define S34 "Soubor „%s“ není určen jako mikroprogramové vybavení pro toto zařízení USB2LPT!"
#define S35 "Datum aktuálního mikroprogramového vybavení: %s\n\nDatum mikroprogramového vybavení k vypálení: %s\n\npokračování?"
#define S36 "neznámý"		// datum mikroprogramového vybavení(dřívější vydání neměla datum)
#define S37 "Firmware\0*.iic"	// Filtr pro standartní dialog otevírání souborů
#define S38 "Funguje bez závad."
#define S39 "Bylo odhaleno selhání! Zkratový test.EXE poskytne detailní údaje."
#define S40 "Je na všech 17 připojeních vysoký potenciál?"
#define S41 "Firmware pro Low-Speed USB2LPT nemohou být aktualizovány, ale může být aktivován Bootovací program tak, aby bylo možno programovat pomocí „bootloadHID.EXE“.\n\npokračování?"
#define S42 "This USB2LPT doesn't support firmware update via USB.\n\n(First EEPROM byte = 0x%02X)"

// Monitor
#define S48 "SPP (půlbyte)\0obousměrný\0pravidelně se opakující snímání\0ECP\0EPP\0obsazený\0test\0Konfigurace"
#define S49 "další\0vyhrazena\0požadavek na přerušení\0chyba\0zapnutý\0konec papíru\0potvrzení\0zaměstnaný"
#define S50 "pravidelně se opakující snímání\0AutoFeed\0vrácení\0volba\0povolení přerušení\0směr informací: 1=zadání\0není prováděna"
#define S51 "přík&az\0&adresa"	// Popis pro vlnový přepínač (Radiobutton) 105/122
#define S52 "normálně\0Půlbyte režim\0Byte režim\0ECP režim\0EPP režim"

// VERSIONINFO
#define I002	5,1
#define S003	"Poskytovatel postavené stránky" // FileDescription
#define S005	"Freeware"			// LegalCopyright
#define S006	"h#s převodník z USB na LPT"	// ProductName
#define S007	"Ovladač režimu jádra"		// FileDescription for .SYS driver

/* INF file strings (processed by makefile)
InstDisk="h#s Driver Disk pro převodník z USB na LPT"
DevDesc ="h#s převodník z USB na LPT"
SvcDesc ="h#s Ovladače pro převodník z USB na LPT (usb2lpt.sys)"
*/

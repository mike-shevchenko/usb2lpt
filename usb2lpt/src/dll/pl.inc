// Translated by Marcin Cichy & Wojciech Cichy

// DIALOG(EX)
#define S10000	"Emulacja"
#define S10001	"&Adres emul. portu:"
#define S10002	"Rozsz&erzenie portu LPT:"
#define S10003	"Wyjątki dla instrukcji WE i WY"
#define S10004	"&Debuguj rejestr wyjątków"
#define S10005	"(przechwytuje wszystkie instrukcje WE/WY)"
#define S10006	"&Wymuś, nawet jeżeli zostało już przydzielone"
#define S10007	"Przekierowanie &READ/WRITE_PORT_UCHAR/USHORT/ULONG"
#define S10008	"(szybciej, bardziej stabilnie)"
#define S10009	"Opcje wydajności"
#define S10010	"Użyj wr&ite-back dla instrukcji WY"
#define S10011	"(Łączy wiele instrukcji WY w jeden pakiet USB)"
#define S10012	"Wyślij pakiety z opóźnienie&m:"
#define S10014	"Odczytaj z lokalnej kopii rejestrów WY"
#define S10015	"&Dokładniej..."
#define S10016	"(zwraca ostatnią zapisaną wartość dla instrukcji WE; nie działa gdy urządzenia zewnątrzne sterują wyjściami)"

#define S10100	"Statystyki"
#define S10101	"Debuguj rejestr wyjątków"
#define S10102	"Wyze&ruj"
#define S10103	"Instrukcje WY:"
#define S10104	"Instrukcje WE:"
#define S10105	"Błędy emulacji:"
#define S10106	"(niewspierane instrukcje WE lub WY, np. OUTSB)"
#define S10107	"Skradzione rejestry debugowania:"
#define S10108	"(Nieznany sterownik zapisał do rejestrów debugowania)"
#define S10109	"Nadpisane rejestry debugowania wykrywane są co 100 ms."
#define S10110	"Rzeczywiste przypisanie do:"
#define S10114	"Przekierowanie READ_PORT_UCHAR / WRITE_PORT_UCHAR"
#define S10115	"Wy&zeruj"
#define S10116	"Nadmiarowość:"
#define S10117	"USB2LPT &Dodatki..."
#define S10118	"Akt&ualizacja firmware'u..."
#define S10119	"haftmann#software, Politechnika Chemnitz,\nWydział Elektroniki i Informatyki,\nKatedra Technologii Pomiaru i Czujników, 2006-2009"

#define S10200	"Odczytaj z kopii rejestrów WY"
#define S10201	"Port &danych (adres bazowy + 0)"
#define S10202	"Port &kontrolny (adres bazowy + 2)"
#define S10203	"&Inne p&orty (ECP, EPP)"
#define S10204	"Zatwierdź"
#define S10205	"Anuluj"

#define S10302	"Nieprawidłowe ustawienia mogą uszkodzić komputer lub wykasować dane na dysku! Używanie jednego z trzech zdefiniowanych adresów jest bezpieczne."
#define S10304	"Tryb rozszerzony LPT umożliwia emulację kolejnych adresów portów.\nJest to rzadko używane, więc ustawienie SPP jest właściwe."
#define S10305	"Te i inne ustawienia można zmienić w Menedżerze Urządzeń Windows. Wybierz USB2LPT i otwórz Właściwości."

#define S10400	"USB2LPT Dodatki"
#define S10401	"Opcje Rejestru"
#define S10402	"(Opcje rejestru zostaną zachowane w pamięci EEPROM. Dostęp możliwy przez DeviceIoControl)"
#define S10403	"Otwórz kolektory dla portu &danych (+0)"
#define S10404	"Otwórz kolektory dla portu &kontrolnego (+2)"
#define S10405	"wyłącz &podciąganie (i 5V na Pin25 jeżeli podłączone)"
#define S10406	"Bezpośredn&ie WE/WY (USB2LPT jako urządzenie WE/WY)"
#define S10407	"(Słabe 5V zamiast silnego 3.3V na wyjściu)" 
#define S10408	"(Szare pola oznaczają: otwarte kolektory w trybie SPP, Push-Pull w pozostałych trybach)"
#define S10409	"(Oszczędzanie energii)"
#define S10410	"(Bez zamiany linii kontroli i stanu! Nie przydatne przy emulacji portu LPT, dla dostępu przez DeviceIoControl.\nRev.4 ma 20 linii WE/WY lines, zamiast 17.\nKierunek można indywidualnie wybierać dla każdego pinu.)"
#define S10411	"U&staw i odczytaj"
#define S10412	"Szuka&j zwarć (bez peryferii!)"
#define S10413	"Numer seryjny"
#define S10414	"w deskryptorze &USB"
#define S10415	"Data firmware'u"
#define S10416	"przyciemniony &niebieski LED"
#define S10417	"Wartość początkowa dla &ECR (tryb pracy):"

#define S10500	"Monitor"
#define S10501	"&Port danych (+0)"
#define S10502	"Odczyt"
#define S10503	"Port &stanu (+1)"
#define S10504	"Port &kontrolny (+2)"
#define S10506	"&Czytaj bajt"
#define S10507	"&Adres"
#define S10508	"&Dane"
#define S10509	"&Zapisz bajt"
#define S10512	"Pełne FIFO"
#define S10513	"Puste FIFO"
#define S10514	"Rejestr kierunku (specyficzne dla USB2LPT)"


// STRINGTABLE
#define S16 "Czy na pewno, chcesz użyć tego niestandardowego adresu portu?\n\nMożesz uszkodzić Twój komputer!"
#define S17 "Adres portu jest nieprawidłową wartością hexadecymalną!"
#define S18 "Adres portu jest nieprawidłowy: za krótki, za długi, lub niepodzielny przez 4!"
#define S19 "Na adresach portów niepodzielnych przez 8, emulacja EPP jest niemożliwa!"
#define S20 "Nieprawidłowa wartość limitu czasu! (Nie może być większa od 1000 ms.)"
#define S21 "Przypisanie emulowanego adresu portu"
#define S22 "Adres portu musi być zgodny z adresem używanym przez Twoją aplikację."
#define S23 "Ten firmware nie wspiera specyficznych funkcji rejestru USB2LPT!"
#define S24 "brak"	// numer seryjny
#define S25 "&Pomoc kontekstowa"
#define S26 "&Bulk transfer (faster; good for XP and earlier)"
#define S27 "Interrupt transfer otherwise"
#define S28 "Activating bulk transfer renders USB2LPT defective under Windows Vista and newer!\nRemember to reset this bit before upgrading your system.\n\nKontynuować?"
#define S29 "The debug register trap hook will be detected by PatchGuard some minutes later, which results in BSoD 0x109 when no countermeasure was taken.\n\nKontynuować?"

#define S32 "Firmware został poprawnie zapisany w pamięci EEPROM.\nOdłącz i podłącz urządzenie, aby zakończyć aktualizację."
#define S33 "Nieznany błąd podczas zapisu firmwaru do pamięci EEPROM. Nie rozłączaj urządzenia i spróbuj ponownie. USB2LPT może być uszkodzone!"
#define S34 "Plik „%s” nie zawiera firmwaru dla tego urządzenia USB2LPT!"
#define S35 "Data aktualnego firmwaru: %s\n\nData firmwaru, który zostanie wgrany: %s\n\nKontynuować?"
#define S36 "nieznane"		// data oprogramowania układowego (wcześniejsze wersje nie posiadają zapisanej daty)
#define S37 "Firmware\0*.iic"	// filtr dla standartowego dialogu otwarcia pliku
#define S38 "Prawidłowe działanie."
#define S39 "Nieprawidłowe działanie! Uruchom KurzschlussTest.EXE po więcej szczegółów."
#define S40 "Czy na wszystkich 17 liniach jest wysoki poziom?"
#define S41 "Firmware dla USB2LPT Low-Speed nie może być zaktualizowane w tym miejscu, ale można ustawić flagę bootloadera. Po ponownym podłączeniu użyj do aktualizacji „bootloadHID.EXE”.\n\nKontynuować?"
#define S42 "This USB2LPT doesn't support firmware update via USB.\n\n(First EEPROM byte = 0x%02X)"

#define S48 "SPP (Półbajt)\0Dwukierunkowy\0AutoStrobe\0ECP\0EPP\0Zarezerwowany\0Tryb testowy\0Konfiguracja"
#define S49 "Dodatkowy\0Zarezerwowany\0Przerwanie\0Błąd\0Podłączony\0Koniec papieru\0Potwierdzenie odbioru\0Zajęty"
#define S50 "Strobe\0Automatyczny wysuw\0Inicjacja\0Wybierz wejście\0Przerwanie włączone\0Kierunek przesyłu danych, 1=wejście\0nie wdrożone"
#define S51 "P&olecenie\0&Adres"		// Alternatywny opis przełącznika (radio button) 105/122
#define S52 "Normalny\0Tryb Półbajt\0Tryb Bajt\0Tryb ECP\0Tryb EPP"

// VERSIONINFO
#define I002	21,1				// LANGUAGE
#define S003	"Zakładka właściwości dostawcy"	// FileDescription
#define S005	"Freeware"			// LegalCopyright
#define S006	"h#s konwerter USB na LPT"	// ProductName
#define S007	"Sterownik trybu jądra"		// FileDescription for .SYS driver

/* INF file strings (processed by makefile)
InstDisk="Dysk ze sterownikami h#s konwertera USB do LPT"
DevDesc ="h#s Konwerter USB do LPT"
SvcDesc ="sterownik dla h#s konwertera USB do LPT (usb2lpt.sys)"
*/

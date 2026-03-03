//Finnish language, Suomi texts 24.3.2012 by Seppo Rantapuska

// DIALOG(EX)
#define S10000	"Emulointi"
#define S10001	"Emuloitu portti &osoite:"
#define S10002	"LPT &laajennusmoodi:"
#define S10003	"IN- ja OUT- käskyjen käsittelymenetelmät"
#define S10004	"&Rekisterien debuggaus"
#define S10005	"(tarkistaa kaikki IN- ja OUT- käskyjen tyypit)"
#define S10006	"&Pakotetaan varaus, vaikka jo aiemmin varattu"
#define S10007	"&READ/WRITE_PORT_UCHAR/USHORT/ULONG uudelleenohjaus"
#define S10008	"(nopeampi, stabiilimpi)"
#define S10009	"Suorituskykyvaihtoehdot"
#define S10010	"&Cache muisti OUT- käskyille"
#define S10011	"(yhdistä useita OUT- käskyjä yhteen USB- pakettiin)"
#define S10012	"Lähetä paketti &timeout-ajan jälkeen:"
#define S10014	"Read-&back paikallinen kopio käskyrekistereistä"
#define S10015	"&Tarkemmin..."
#define S10016	"(palauttaa viimeksi kirjoitetun arvon IN- käskyn mukana; ei toimi, jos lähtöjä ohjataan ulkoisella laiteohjauksella)"

#define S10100	"Statistiikka"
#define S10101	"Rekisterien debuggaus"
#define S10102	"&Resetoi nollaan"
#define S10103	"OUT käskyt:"
#define S10104	"IN käskyt:"
#define S10105	"Emulointivirheet:"
#define S10106	"(ei tuettu IN- tai OUT- käsky, esim. OUTSB)"
#define S10107	"Kaapatut debugrekisterit:"
#define S10108	"(Tuntematon ajuri kirjoitti debug rekistereihin)"
#define S10109	"Ylikirjoitetut debug- rekisterit havaitaan 100 ms välein."
#define S10110	"Todellisuudessa osoitettu:"
#define S10114	"READ_PORT_UCHAR / WRITE_PORT_UCHAR uudelleenohjaus"
#define S10115	"Resetoi &nollaan"
#define S10116	"Ylisuuri:"
#define S10117	"USB2LPT-&Ekstrat..."
#define S10118	"Ohjelmisto &Päivitys..."
#define S10119	"haftmann#software, Tekninen Yliopisto Chemnitz,\nElektroniikan ja Tietotekniikan Tiedekunta,\nMittaus- ja anturitekniikan professuuri, 2006-2009"

#define S10200	"Read-back kopio output-rekistereistä"
#define S10201	"&Data- portti (base address + 0)"
#define S10202	"&Control- portti (base address + 2)"
#define S10203	"&Muut portit (ECP, EPP)"
#define S10204	"OK"
#define S10205	"Peruuta"

#define S10302	"Väärä asetus voi aiheuttaa järjestelmän kaatumisen tai muuta vakavaa! Turvallista on käyttää kolmea ennalta määriteltyä osoitetta ."
#define S10304	"LPT-laajennusmoodit tekevät lisäporttiosoitteet mahdollisiksi.\nNäitä käytetään harvoin;joten SPP valinta on OK."
#define S10305	"Nämä ja muut asetukset voit tehdä myöhemmin Windows' Device Manager:ssa. Valitse hiiren oikealla USB2LPT adapteri, ja avaa Properties."

#define S10400	"USB2LPT- Ekstrat"
#define S10401	"Feature-Rekisteri"
#define S10402	"(Feature- Rekisteri pidetään varmennettuna USB2LPT:n EEPROM-muistissa . Pääsy  DeviceIoControl- kautta)"
#define S10403	"Open Collector-lähdöt &Data Port (+0)"
#define S10404	"Open Collector-lähdöt &Control Port (+2)"
#define S10405	"&Pullups pois päältä(ja 5V Pin25, jos näin langoitettu)"
#define S10406	"&Suora In/Out moodi (USB2LPT invertoi I/O -laitteelle)"
#define S10407	"(Heikko 5V vahvan 3.3V sijaan output-lähdöille)"
#define S10408	"(Harmaa valinta: Open Collector-lähdöt SPP-moodissa, Push-Pull muuten)"
#define S10409	"(vain energian säästämiseksi)"
#define S10410	"(Ei enää invertoituja Control-lähtöjä ja Status-tuloja! Ei siis oikeaa LPT emulointia, käytetään DeviceIoControl kautta.\nRev.4:ssa on 20 I/O linjaa, 17 muutoin.\nSuunta on yksittäin per pin valittavissa.)"
#define S10411	"&Aseta && Lue takaisin"
#define S10412	"Tarkista oikosulut"
#define S10413	"Sarjanumero"
#define S10414	"&USB- descriptorissa"
#define S10415	"Ohjelmistoversion pvm"
#define S10416	"Himmeä &sininen LED"
#define S10417	"Alkuarvo &ECR(operointimoodi):"

#define S10500	"Monitori"
#define S10501	"Data- &portti (+0)"
#define S10502	"Lue"
#define S10503	"&Status- portti (+1)"
#define S10504	"&Control- portti (+2)"
#define S10506	"&Lue Byte"
#define S10507	"&Osoite"
#define S10508	"&Data"
#define S10509	"&Lähetä Byte"
#define S10512	"FIFO täynnä"
#define S10513	"FIFO tyhjä"
#define S10514	"Suuntarekisteri (USB2LPT ekstra)"


// STRINGTABLE
#define S16 "Haluatko aivan varmasti käyttää tätä ei-standardia porttiosoitetta?\n\nJärjestelmä voi kaatua!"
#define S17 "Tämä porttiosoite ei ole sallittu hexadesimaaliluku!"
#define S18 "Tämä porttiosoite on väärä: liian pieni, liian suuri, tai ei ole jaollinen 4:llä !"
#define S19 "Jos porttiosoite ei ole jaollinen 8:lla, EPP emulointi ei ole mahdollista!"
#define S20 "Väärä luku Timeout- arvona! (Ei saa olla yli 1000 ms.)"
#define S21 "Emuloitavan porttiosoitteen valinta"
#define S22 "Tämän porttiosoitteen pitää sopia yhteen sovellusohjelmasi käyttämän osoitteen kanssa."
#define S23 "Tämä ohjelmistoversio ei tue USB2LPT:n ekstra feature rekisteriä!"
#define S24 "tyhjä"	// sarjanumero
#define S25 "Kohdennettu &help"
#define S26 "&Bulk transfer (faster; good for XP and earlier)"
#define S27 "Interrupt transfer otherwise"
#define S28 "Activating bulk transfer renders USB2LPT defective under Windows Vista and newer!\nRemember to reset this bit before upgrading your system.\n\nJatketaan?"
#define S29 "The debug register trap hook will be detected by PatchGuard some minutes later, which results in BSoD 0x109 when no countermeasure was taken.\n\nJatketaan?"

#define S32 "Ohjelmisto kirjoitettu EEPROM-muistiin onnistuneesti.\nKäytä sähköt pois/päälle uuden version käynnistämiseksi."
#define S33 "Tuntematon virhe ohjelman kirjoittamisessa EEPROM:lle. Älä irroita laitetta, vaan yritä uudelleen.  USB2LPT ei ehkä enää toimi!"
#define S34 "Tiedosto ”%s” ei ole hyväksyttävä tälle USB2LPT-laitteelle!"
#define S35 "Tämän (ajossa olevan) firmwaren päiväys: %s\n\nFirmwaren päiväys kirjoitetaan: %s\n\nJatketaan?"
#define S36 "tuntematon"		// firmware päiväys(vanhoissa versioissa ei ole päiväystä)
#define S37 "Firmware\0*.iic"	// vakiodialogin avaaminen 
#define S38 "Kaikki toimii OK."
#define S39 "Virheellinen toiminta havaittu ! Aja Kurzschlusstest.EXE tarkemman diagnoosin saamiseksi."
#define S40 "Onvatko kaikki 17 linjaa tilassa HIGH?"
#define S41 "Low-Speed-USB2LPT:n firmwaren päivitys ATmega-Controller:illa ei onnistunut , mutta voit asettaa flagin sisäänrakennetun bootloaderin käynnistämiseksi. Uudelleen käynnistyksen jälkeen päivitä käyttäen ”bootloadHID.EXE”.\n\nJatketaan?"
#define S42 "This USB2LPT doesn't support firmware update via USB.\n\n(First EEPROM byte = 0x%02X)"

#define S48 "SPP (Nibble)\0Bidirectional\0AutoStrobe\0ECP\0EPP\0varattu\0testimoodi\0Konfiguraatio"
#define S49 "lisäksi\0varattu\0Keskeytyspyyntö\0Virhe\0Linjalla\0Paperi loppu\0Hyväksy\0Busy"
#define S50 "Strobe\0AutoFeed\0Init\0Valitse Input\0Keskeytys sallittu\0Datan suunta, 1=input\0ei käytössä"
#define S51 "K&äsky\0&Osoite"		// Vaihtuva teksti radio-tyyppiselle painikkeelle 105/122
#define S52 "normaalisti\0Nibble Moodi\0Byte Moodi\0ECP Moodi\0EPP Moodi"

// VERSIOINFO
#define I002	11,1				// KIELI
#define S003	"Property Sheet Provider"	// Tiedostokuvaus
#define S005	"Public Domain"			// lainmukainen Copyright
#define S006	"h#s USB to LPT muunnin"	// Tuotenimi
#define S007	"Kernel-moodin ajuri"		// Tiedostokuvaus .SYS ajurille

/* INF tiedoston tekstit (makefile prosessoi tekstit)
InstDisk="h#s USB-LPT- muuntimen ajurin levyke"
DevDesc ="h#s USB to LPT muuntimelle"
SvcDesc ="h#s ajuri USB - LPT muuntimelle (usb2lpt.sys)"
*/

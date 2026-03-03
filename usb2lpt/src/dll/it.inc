// Translated using Google Translation by h#s 090223

// DIALOG(EX)
#define S10000	"Emulazione"
#define S10001	"Emulato porta &indirizzo:"
#define S10002	"LPT &valorizzazione modalità:"
#define S10003	"Metodo di cattura per IN e OUT istruzioni"
#define S10004	"Trappola registri di &debug"
#define S10005	"(tutte le catture di sapori istruzioni IN e OUT)"
#define S10006	"&Forze di assegnazione, anche se già assegnato"
#define S10007	"&Redirezione READ/WRITE_PORT_UCHAR/USHORT/ULONG"
#define S10008	"(più veloce, più stabile)"
#define S10009	"opzioni prestazioni"
#define S10010	"Usa &write-back per le istruzioni"
#define S10011	"(Concatenare più istruzioni per uno OUT USB pacchetto)"
#define S10012	"Invia pacchetto dopo che il valore di &timeout:"
#define S10014	"Read-&back copia locale della registri uscita"
#define S10015	"&Più precisamente…"
#define S10016	"(ritorno ultimo scritto sul valore IN istruzione; non funziona quando le uscite sono spinti [basso] con l'hardware esterno)"

#define S10100	"Statistiche"
#define S10101	"Trappola registri di debug"
#define S10102	"&Reset a zero"
#define S10103	"OUT istruzioni:"
#define S10104	"IN istruzioni:"
#define S10105	"Emulazione errori:"
#define S10106	"(non supportato IN o OUT istruzione, a.e. OUTSB)"
#define S10107	"Rubati i registri di debug:"
#define S10108	"(Uno sconosciuto ha scritto il driver per i registri di debug)"
#define S10109	"Sopra i registri di debug vengono rilevati da un kernel timer 100 ms."
#define S10110	"davvero assegnato a:"
#define S10114	"Redirezione READ_PORT_UCHAR / WRITE_PORT_UCHAR"
#define S10115	"Reset a &zero"
#define S10116	"più grande:"
#define S10117	"USB2LPT apposta…"
#define S10118	"a&ggiornamento firmware…"
#define S10119	"haftmann#software, Technische Universität Chemnitz,\nFakultät für Elektrotechnik und Informationstechnik,\nProfessur für Mess- und Sensortechnik, 2006-2009"


#define S10200	"Read-back copia locale della registri uscita"
#define S10201	"&Porta dati (indirizzo di base + 0)"
#define S10202	"&Porta di controllo (indirizzo di base + 2)"
#define S10203	"&Altri porti (ECP, EPP)"
#define S10204	"OK"
#define S10205	"Annulla"

#define S10302	"Una errata impostazione può crash il computer o mangiare il contenuto del disco rigido! Utilizzando uno dei tre indirizzi predefiniti devono essere sicuri."
#define S10304	"Le modalità di valorizzazione LPT consentire la cattura di altri indirizzi.\nQuesti sono molto raramente utilizzata; così l'impostazione SPP è OK."
#define S10305	"Queste e altre impostazioni che si possono poi fare in Windows Gestione Periferiche. Punto di USB2LPT il dispositivo, e aprire Proprietà."

#define S10400	"USB2LPT apposta"
#define S10401	"Registro apposta"
#define S10402	"(La funzione di registro apposta tenuto persistente in EEPROM di USB2LPT dispositivo. Accesso via DeviceIoControl)"
#define S10403	"Collettore aperto per porta &dati (+0)"
#define S10404	"Collettore aperto per porta di &controllo (+2)"
#define S10405	"spegnere &Pullups (e 5V a Pin25 in caso affermativo cablata)"
#define S10406	"Direct In/Out &modalità (USB2LPT convertiti al di I/O del dispositivo)"
#define S10407	"(Debole 5V invece di forte 3,3 V per realizzazioni di alto livello)"
#define S10408	"(Un segno di spunta grigio significa: Collettore aperto in modalità SPP, Push-Pull altrimenti)"
#define S10409	"(solo il risparmio energetico)"
#define S10410	"(Non è più stato invertito il controllo e le linee! Pertanto, non utile LPT emulazione, per l'accesso tramite DeviceIoControl. Rev.4 dispone di 20 linee I/O, altrimenti 17. Direzione è per bit e per pin selezionabile.)"
#define S10411	"&Insieme && Rilettura"
#define S10412	"Analisi per cortocircuiti"
#define S10413	"Numero di serie"
#define S10414	"nel descrittore &USB"
#define S10415	"Firmware data"
#define S10416	"Grigio blu &LED"
#define S10417	"Valore di avvio per &ECR (modalità operativa):"

#define S10500	"Monitor"
#define S10501	"&Porta dati (+0)"
#define S10502	"Leggere"
#define S10503	"Porta di &stato (+1)"
#define S10504	"Porta di &controllo (+2)"
#define S10506	"&Leggere byte"
#define S10507	"&Indirizzo"
#define S10508	"&Dati"
#define S10509	"&Scrivere byte"
#define S10512	"FIFO pieno"
#define S10513	"FIFO vuoto"
#define S10514	"Registri della direzione (specifiche per USB2LPT)"

// STRINGTABLE 	// Emulazione+Statistiche
#define S16 "Avete veramente desidera utilizzare questa porta non-standard elettronica?\n\nÈ possibile crash il computer!"
#define S17 "Questo indirizzo non è un numero esadecimale valido!"
#define S18 "Questo indirizzo è sbagliato: troppo di meno, troppo grande, o non divisable da 4!"
#define S19 "Il porto indirizzi non divisibile da 8, PPE emulazione non è possibile!"
#define S20 "Numero errato come valore di Timeout! (Non deve essere superiore a 1000 ms.)"
#define S21 "Assegnazione di indirizzi di porta emulata"
#define S22 "Questo indirizzo deve corrispondere a l'indirizzo da utilizzare il software applicativo."
#define S23 "Questo firmware non supporta la funzione di registro USB2LPT specifiche!"
#define S24 "nessuno"	// serial number
#define S25 "Contesto &aiutare"
#define S26 "&Bulk transfer (faster; good for XP and earlier)"
#define S27 "Interrupt transfer otherwise"
#define S28 "Activating bulk transfer renders USB2LPT defective under Windows Vista and newer!\nRemember to reset this bit before upgrading your system.\n\nProcedere?"
#define S29 "La cattura registri di debug verrà rilevato da PatchGuard alcuni minuti più tardi, che si traduce il BSoD 0x109 quando non contromisura è stata presa.\n\nProcedere?"

#define S32 "Il firmware è stato scritto con successo alla EEPROM.\nCycle power to device to start the new firmware."
#define S33 "Errore sconosciuto durante la scrittura del firmware di EEPROM. Si prega di non scollegare dispositivo e riprovare. USB2LPT Il dispositivo potrebbe non funziona più!"
#define S34 "File „%s“ non è accettata come firmware che USB2LPT dispositivo!"
#define S35 "Data di effettivi (esecuzione) del firmware: %s\n\nData di firmware per essere scritto: %s\n\nProcedere?"
#define S36 "sconosciuto"	// firmware data (primi rilasci non hanno data interno)
#define S37 "Firmware\0*.iic"	// filtro per aprire il file standard finestra
#define S38 "Perfetto funzionamento."
#define S39 "Funzione non corretta rilevato! Esegui Kurzschlusstest.EXE per i dettagli."
#define S40 "Avete misura ad alto livello su tutte le 17 righe?"
#define S41 "Il firmware per la bassa velocità USB2LPT dispositivo non può essere aggiornato, ma è possibile impostare un flag per l'attivazione di un built-in boot loader. Dopo potere ciclismo è necessario aggiornare l'utilizzo di „bootloadHID.EXE“.\n\nProcedere?"
#define S42 "This USB2LPT doesn't support firmware update via USB.\n\n(First EEPROM byte = 0x%02X)"

#define S48 "SPP (Nibble)\0Bidirezionale\0AutoStrobe\0ECP\0EPP\0Riservati\0Modalità di prova\0Configurazione"
#define S49 "supplementari\0riservati\0Interrupt\0Errore\0Online\0Libro Fine\0Riconoscere\0Busy"
#define S50 "Strobo\0AutoFeed\0Init\0Select Input\0Interrupt Enable\0Data di direzione, 1 = ingresso\0non attuato"
#define S51 "C&omando\0&Indirizzo"	// Modificare il testo per un RadioButton
#define S52 "Convenzionali\0Nibble Mode\0Byte Mode\0ECP Mode\0EPP Mode"

// VERSIONINFO
#define I002	16,1
#define S003	"Proprietà Provider"		// FileDescription
#define S005	"Freeware"			// LegalCopyright
#define S006	"h#s convertitore da USB a LPT"	// ProductName
#define S007	"Driver in modalità kernel"	// FileDescription for .SYS driver

/* INF file strings (processed by makefile)
InstDisk="h#s driver disco per il convertitore da USB a LPT"
DevDesc ="h#s convertitore da USB a LPT"
SvcDesc ="h#s driver per il convertitore da USB a LPT (usb2lpt.sys)"
*/

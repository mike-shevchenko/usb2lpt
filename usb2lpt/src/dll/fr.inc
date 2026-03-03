// Translated using Google Translation by h#s 090223
// Corrected by Jean-Francois Serviere (jean-francois.serviere@transport.alstom.com) 090304

// DIALOG(EX)
#define S10000	"Émulation"
#define S10001	"&Adresse du port émulé:"
#define S10002	"&Mode amélioration du LPT:"
#define S10003	"Méthode de capture pour les instructions IN et OUT"
#define S10004	"capture en registre de &débogage"
#define S10005	"(il capture toutes les nuances des instructions IN et OUT)"
#define S10006	"&forcer une occupation, même si elle a déjà été allouée"
#define S10007	"&READ/WRITE_PORT_UCHAR/USHORT/ULONG redirection"
#define S10008	"(plus rapide, plus solide)"
#define S10009	"Options de performance"
#define S10010	"Utilisez &write-back pour les instructions OUT"
#define S10011	"(concaténer plusieurs instructions OUT à un seul paquet USB)"
#define S10012	"Envoyer un paque&t après cette valeur de délai:"
#define S10014	"Re&lire une copie locale de registres de sortie"
#define S10015	"&Plus précisément…"
#define S10016	"(retour de la dernière valeur écrite sur l'instruction d'entrée; ne fonctionne pas lorsque les sorties sont forcées au niveau « bas »)"

#define S10100	"Statistiques"
#define S10101	"Capture en registre de débogage"
#define S10102	"&Remise à zéro"
#define S10103	"instructions OUT:"
#define S10104	"instructions IN:"
#define S10105	"erreurs d'émulation:"
#define S10106	"(instructions non prises en charge, par ex. OUTSB)"
#define S10107	"registres de débogage volés:"
#define S10108	"(un pilote inconnu a écrit dans les registres de débogage)"
#define S10109	"Les reg's sont écrasés sont détectés par une temporisation de 100 ms."
#define S10110	"réellement affectés à:"
#define S10114	"READ_PORT_UCHAR / WRITE_PORT_UCHAR redirection"
#define S10115	"Remise à &zéro"
#define S10116	"dépassement de capacité:"
#define S10117	"USB2LPT spé&cial…"
#define S10118	"Mise à jour d&u firmware…"
#define S10119	"haftmann#software, Technische Universität Chemnitz,\nFakultät für Elektrotechnik und Informationstechnik,\nProfessur für Mess- und Sensortechnik, 2006-2009"

#define S10200	"Relire une copie locale des registres de sortie"
#define S10201	"Port de &données (adresse de base + 0)"
#define S10202	"Port de &contrôle (adresse de base + 2)"
#define S10203	"Autres p&orts (ECP, EPP)"
#define S10204	"OK"
#define S10205	"Annuler"

// Page finale de l'assistant d'installation de nouveau matériel (CoInstaller, Win32 seulement)
#define S10302	"Un mauvais paramétrage peut bloquer votre ordinateur ou endommager le contenu de votre disque dur ! Utiliser l'une des trois adresses prédéfinies qui doivent être sûres."
#define S10304	"Le mode amélioration LPT permet l'amélioration de la capture de nouvelles adresses de port.\nCelles-ci sont très rarement utilisées, de sorte que le paramétrage SPP est OK."
#define S10305	"Ces paramétrage et d'autres peuvent se faire plus tard dans le Gestionnaire de périphériques de Windows. Pointer le périphérique USB2LPT, et ouvrir Propriétés."

#define S10400	"USB2LPT Spécial"
#define S10401	"Registre particularité"
#define S10402	"(Le registre particularité sera maintenu permanent dans l'EEPROM du périphérique. Accès via DeviceIoControl)"
#define S10403	"Collecteurs ouvert pour port de &données (+0)"
#define S10404	"Collecteurs ouvert pour port de &contrôle (+2)"
#define S10405	"Fermeture des &pullups (Et du 5V à la broche 25…)"
#define S10406	"Mode d&irect In/Out (USB2LPT devient un périphérique I/O)"
#define S10407	"(5V faible au lieu de 3.3V fort pour les sorties haut niveau)"
#define S10408	"(Une coche grisée signifie: Collecteurs ouvert en mode SPP, Push-Pull autrement)"
#define S10409	"(…si il est connecté – économie d'énergie seulement)"
#define S10410	"(Plus d'inversion des lignes de contrôle et d'état ! Par conséquent, aucune émulation utile de LPT, pour l'accès via DeviceIoControl. Rev.4 a 20 lignes d'E/S, 17 autrement. La direction est sélectionnable par bit et par broche.)"
#define S10411	"Écrire et re&lire"
#define S10412	"&Recherche de courts-circuits"
#define S10413	"Numéro de série"
#define S10414	"dans le descripteur de &USB"
#define S10415	"Date du Firmware"
#define S10416	"foncé &LED bleu"
#define S10417	"Valeur de démarrage pour &ECR (mode de fonctionnement):"

#define S10500	"Supervision"
#define S10501	"&Port de données (+0)"
#define S10502	"Lecture"
#define S10503	"Port d'é&tat (+1)"
#define S10504	"Port de &contrôle (+2)"
#define S10506	"&Lire octet"
#define S10507	"&Adresse"
#define S10508	"&Données"
#define S10509	"&Écrire octet"
#define S10512	"FIFO plein"
#define S10513	"FIFO vides"
#define S10514	"Registre de direction (USB2LPT spécifiques)"

// STRINGTABLE 	// Émulation+Statistiques
#define S16 "Voulez-vous vraiment utiliser cette adresse de port non-standard ?\n\nVous risquez de bloquer votre ordinateur !"
#define S17 "Ce port n'est pas un nombre hexadécimal valide !"
#define S18 "Fausse adresse du port: pas assez, trop grande, ou pas divisible par 4 !"
#define S19 "Adresse non divisible par 8 sur le port, émulation EPP impossible !"
#define S20 "Mauvais temps de retard ! (Ne doit pas être supérieur à 1000 ms.)"
#define S21 "Affectation des adresses de port émulées"
#define S22 "Cette adresse de port doit correspondre à l'adresse utilisée par votre logiciel d'application."
#define S23 "Ce firmware ne supporte pas le registre spécifique des propriétés de USB2LPT !"
#define S24 "aucun"	// Numéro de série
#define S25 "&Aide contextuelle"
#define S26 "&Bulk transfer (faster; good for XP and earlier)"
#define S27 "Interrupt transfer otherwise"
#define S28 "Activating bulk transfer renders USB2LPT defective under Windows Vista and newer!\nRemember to reset this bit before upgrading your system.\n\nContinuer ?"
#define S29 "Le capture de registre de débogage sera détecté par PatchGuard quelques minutes plus tard, ce qui entraîne écran bleu #0x109 en l'absence de contre-mesure a été prise.\n\nContinuer ?"

// STRINGTABLE 	// Mise à jour du firmware
#define S32 "Le firmware est écrit dans l'EEPROM avec succès.\nRedémarrer l'appareil pour lancer le nouveau firmware."
#define S33 "Erreur inconnue lors de l'écriture du firmware de l'EEPROM. S'il vous plaît ne pas débrancher l'appareil et réessayez. Le dispositif USB2LPT peut ne plus fonctionner !"
#define S34 "Le fichier « %s » n'est pas accepté en tant que firmware pour l'appareil USB2LPT !"
#define S35 "Date du firmware actuel (installé): %s\n\nDate du firmware à écrire: %s\n\nContinuer ?"
#define S36 "inconnue"		// Date du firmware (les versions plus anciennes n'ont pas de date)
#define S37 "Firmware\0*.iic"	// filtre pour dialogue de standard « ouvrir le fichier »
#define S38 "Parfait fonctionnement."
#define S39 "Fonctionnement incorrect détecté ! Lancer Kurzschlusstest.EXE pour plus de détails."
#define S40 "Mesurez vous un niveau haut sur les 17 lignes ?"
#define S41 "Le firmware de bas débit USB2LPT appareil ne peut pas être mise à jour ici, mais vous pouvez configurer un drapeau pour activer un module de démarrage. Après le vélo, vous devez pouvoir mettre à jour en utilisant « bootloadHID.EXE ».\n\nContinuer ?"
#define S42 "This USB2LPT doesn't support firmware update via USB.\n\n(First EEPROM byte = 0x%02X)"

// STRINGTABLE 	// Fiche de suivi des propriétés
#define S48 "SPP (demi-octet)\0Bidirectionnel\0AutoStrobe\0ECP\0EPP\0réservés\0mode de test\0Configuration"
#define S49 "supplémentaires\0réservés\0Requête d'interruption\0Erreur\0En ligne\0Papier fin\0Reconnaître\0Occupé"
#define S50 "Strobe\0AutoFeed\0Init\0Select Input\0Autorisation d'interruption\0Direction de données, 1 = entrée\0pas mis en œuvre"
#define S51 "C&ommande\0&Adresse"	// changer le texte d'un bouton radio
#define S52 "normal\0mode demi-octet\0mode byte\0mode ECP\0mode EPP"

#define I002	12,1
#define S003	"Fournisseur de fiche de propriétés"	// FileDescription
#define S005	"Logiciel public"			// LegalCopyright
#define S006	"h#s convertisseur USB vers LPT"	// ProductName
#define S007	"Pilote en mode noyau"			// FileDescription for .SYS driver

/* INF file strings (processed by makefile)
InstDisk="driver disquette pour le h#s convertisseur USB vers LPT"
DevDesc ="h#s convertisseur USB vers LPT"
SvcDesc ="driver pour le h#s convertisseur USB vers LPT (usb2lpt.sys)"
*/

// Übersetzung von Sven Köhler

// DIALOG(EX)
#define S10000	"Emulation"
#define S10001	"Dirección de interfaz emulad&a:"
#define S10002	"&Expansión de LPT:"
#define S10003	"Método de conexión para comandos de IN y OUT"
#define S10004	"por medio de registro del &debug"
#define S10005	"(registra todos los tipos de comandos de IN y OUT)"
#define S10006	"&forzar ocupación, también cuando aparece ocupado"
#define S10007	"desviación de &READ_PORT_UCHAR / WRITE_PORT_UCHAR"
#define S10008	"(más rápido y más estable)"
#define S10009	"Opción de rendimiento"
#define S10010	"caché de e&scribir para OUT-comandos"
#define S10011	"(Unión de varios comandos de OUT para un paquete de USB)"
#define S10012	"for&zar a enviar los paquetes para"
#define S10014	"releer una &cópia de los registros de salida"
#define S10015	"E&xacto…"
#define S10016	"(da el último valor escrito en comando de IN; no funciona cuando las salidas están controladas por hardware exterior «low»)"

#define S10100	"Estadistica"
#define S10101	"Conexión de los registros de debug"
#define S10102	"Poner a &cero"
#define S10103	"Comandos de OUT:"
#define S10104	"Comandos de IN:"
#define S10105	"error de emulación:"
#define S10106	"(comando de IN o OUT no esta apoyado, por exemplo OUTSB)"
#define S10107	"registros de debug robados:"
#define S10108	"(controlador desconocido sobreescribió los reg's de debug)"
#define S10109	"Registros de debug sobreescrito están detectados con un reloj de 100 ms."
#define S10110	"de hecho asignado a:"
#define S10114	"Desviación READ_PORT_UCHAR / WRITE_PORT_UCHAR"
#define S10115	"&Poner a cero"
#define S10116	"exceso de largura:"
#define S10117	"USB2LPT-&Extras…"
#define S10118	"Actualización del &firmware…"
#define S10119	"haftmann#software, Universidad Técnica de Chemnitz,\nfacultad para Microelectrónica y Sistema Microelectrónico,\ncátedra para técnica de medición y sensor, 2006-2009"

#define S10200	"Copia de releer de los registros de salida"
#define S10201	"Interfaz de &dados (dir. de base + 0)"
#define S10202	"Interfaz del &control (dir. de base + 2)"
#define S10203	"&Extra interfaz (ECP, EPP)"
#define S10204	"Aceptar"
#define S10205	"Cancelar"

#define S10302	"¡Dar una dirección equivocada puede congelar su ordenador o hacer cosas peores! Usar uno de los 3 parámetros esta considerado seguro."
#define S10304	"La expansión de LPT efectua la emulación de otras direcciones de la interfaz.\nLa se necesita raras veces y la configuración de SPP está bien."
#define S10305	"Esta y otras configuraciónes se pueden hacer en las páginas de las opciones en el Administrador de dispositivos."

#define S10400	"USB2LPT-Extras"
#define S10401	"Registro de feature"
#define S10402	"(El registro de feature está guardado en el EEPROM del USB2LPT. Acceso vía DeviceIoControl)"
#define S10403	"Colectores abiertos para la interfaz de &datos (+0)"
#define S10404	"Colectores abiertos para la interfaz de &control (+2)"
#define S10405	"Desligar los &Pullup (y 5V en Pin25, en caso de que esta…)"
#define S10406	"Modo &directo entrada y salida (USB2LPT se cambia para…)"
#define S10407	"(efectua tensión de salida debil 5V en lugar de 3,3V fuerte)"
#define S10408	"(Un campo gris signífica: colectores abiertos en el modo de SPP, Push-Pull y en todas las otros modos)"
#define S10409	"(…reequipado — Medida para economizar energía)"
#define S10410	"(…equipo de I/O — Inversión ya no necesario. Por eso no hay una simulación correcta de la interfaz de paralelo, para acceso con DeviceIoControl. La dirección se pueden desligar individualmente para cada Pin)"
#define S10411	"Poner && &releer"
#define S10412	"Prueba de c&ortocircuito (sin periferia)"
#define S10413	"Número de serie"
#define S10414	"en descriptor de &USB"	// Google
#define S10415	"fecha de firmware"
#define S10416	"Atenuado &LED azul"	// Google
#define S10417	"Valor de inicio para &ECR (modo de funcionamiento):"	// Google

#define S10500	"Monitor"
#define S10501	"Interfaz de &datos (+0)"
#define S10502	"Leer"
#define S10503	"Interfaz del &status (+1)"
#define S10504	"Interfaz del &control (+2)"
#define S10506	"&Leer el byte"
#define S10507	"D&irección"
#define S10508	"D&atos"
#define S10509	"En&tregar el Byte"
#define S10512	"FIFO lleno"
#define S10513	"FIFO vacío"
#define S10514	"Registro de la dirección (USB2LPT-específico)"

// STRINGTABLE 	// Emulation+Statistik
#define S16 "¿Usted en serio quiere usar esta dirección del interfaz que no es usado normalemente?\n\n¡Con esta valor tienes la capacidad para enviar el ordenador al nirvana!"
#define S17 "La dirección de la interfaz no es un número hexadecimal valido."
#define S18 "¡La dirección de la interfaz no es valido: demasiado pequeño, demasiado grando ó no divisible con 4!"
#define S19 "Con todas las direcciones, que se pueden dividir con 8, una emulación de EPP no es possible."
#define S20 "¡Valor no válido con atravesar el tiempo! (No puede ser más grande que 1000 ms.)"
#define S21 "Fijación de la dirección del interfaz emulada"
#define S22 "La dirección de la interfaz tiene que coincidir con el software de aplicación adecuado."
#define S23 "¡El firmware (Software en el equipo de USB2LPT) no apoya el registro de feature!"
#define S24 "ningún"	// Seriennummer
#define S25 "contexto &ayuda"
#define S26 "&Bulk transfer (faster; good for XP and earlier)"
#define S27 "Interrupt transfer otherwise"
#define S28 "Activating bulk transfer renders USB2LPT defective under Windows Vista and newer!\nRemember to reset this bit before upgrading your system.\n\n¿Seguir?"
#define S29 "The debug register trap hook will be detected by PatchGuard some minutes later, which results in BSoD 0x109 when no countermeasure was taken.\n\n¿Seguir?"

#define S32 "El firmware fue escrito al EEPROM correcto.\nDesconecta el equipo y reconéctalo para iniciar el firmware nuevo."
#define S33 "Error confuso al escribir el firmware. ¡No desconecta é intenta de nuevo! A lo mejor el equipo ya no esta apto para el funcionamento."
#define S34 "¡El archivo «%s» no es propio para este equipo de USB2LPT!"
#define S35 "Fecha del firmware actual: %s\n\nFecha del firmware para grabar: %s\n\n¿Seguir?"
#define S36 "desconocido"		// Firmware-Datum (frühe Ausgaben enthielten kein Datum)
#define S37 "Firmware\0*.iic"		// Filter für Datei-öffnen-Standarddialog
#define S38 "Función correcta"
#define S39 "!Función incorrecta hallado! Kurzschlusstest.EXE da descripciones detalladas."
#define S40 "¿Hay en todos los 17 conexiones HIGH-potencial disponible?"
#define S41 "El firmware de baja velocidad USB2LPT de dispositivo no se puede actualizar, pero se puede configurar para activar un pabellón construido en un gestor de arranque. Después del circuito de arranque debe poder actualizar usando «bootloadHID.EXE».\n\n¿Seguir?"
#define S42 "This USB2LPT doesn't support firmware update via USB.\n\n(First EEPROM byte = 0x%02X)"

#define S48 "SPP (Nibble)\0bidireccional\0AutoPalpar\0ECP\0EPP\0reservado\0test\0Configuración"
#define S49 "Extra-Bit\0reservado\0Exigencia del interrupt\0error\0iniciado\0al final de las hojas\0Confirmación\0ocupado"
#define S50 "Palpar\0AutoFeed\0Reset\0Selección\0desbloquear el interrupt\0dirección de los datos: entrada de datos\0no disponible"
#define S51 "C&omando\0D&irección"	// Beschriftung für Wellenschalter (Radiobutton) 105/122
#define S52 "normalmente\0Nibble modo\0Byte modo\0ECP modo\0EPP modo"

// VERSIONINFO
#define I002	10,1
#define S003	"Proveedor de hojas de propiedades"	// FileDescription
#define S005	"Dominio Público"			// LegalCopyright
#define S006	"h#s convertidor de USB a LPT"		// ProductName
#define S007	"Driver de modo de núcleo"		// FileDescription for .SYS driver

/* INF file strings (processed by makefile)
InstDisk="h#s Disco de controladores para convertidor de USB a LPT"
DevDesc ="h#s convertidor de USB a LPT"
SvcDesc ="h#s Driver para convertidor de USB a LPT (usb2lpt.sys)"
*/

// Aufgeschnappt:
// Administrador de dispositivos
// Puertos (COM & LPT)
// Controladoras de bus serie universal (USB)
// Aceptar Cancelar

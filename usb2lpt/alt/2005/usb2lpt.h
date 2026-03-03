/* Kopfdatei für USB2LPT.SYS sowie für CoInstaller / Einstellprogramme
   sowie Sonderfunktionen (bspw. Firmware-Download)
*/
#ifdef DRIVER
# define _X86_ 1
# include <wdm.h>
# include <usbdi.h>
# include <usbdlib.h>	// (enthält störende USB-Hub-GUID-Definition)
# ifdef INIT_MY_GUID
#  include <initguid.h>	// Erst ab jetzt GUIDs im Speicher ablegen!
# endif
# ifndef IoInitializeRemoveLock	// im 98DDK nicht definiert
#  include "w2k.h"	// Fehlendes "nachreichen"
# endif
#endif

typedef unsigned char BYTE,*PBYTE;
typedef unsigned short WORD,*PWORD;
#define elemof(x) (sizeof(x)/sizeof((x)[0]))
#define T(x) TEXT(x)

// {DA6B195A-AC68-4c67-B236-C1455804B1A8}
DEFINE_GUID(Vlpt_GUID,0xda6b195aL,0xac68,0x4c67,0xb2,0x36,0xc1,0x45,0x58,0x4,0xb1,0xa8);

typedef struct{	// Zähler für die Statistik
 ULONG out;	// OUT-Zugriffe
 ULONG in;	// IN-Zugriffe
 ULONG fail;	// bspw: Nicht unterstützte Opcodes (REP INSB u.ä.)
 ULONG steal;	// Gestohlene Debugregister
 ULONG wpu;	// WRITE_PORT_UCHAR-Aufrufe
 ULONG rpu;	// READ_PORT_UCHAR-Aufrufe
}TAccessCnt, *PAccessCnt;

typedef struct{	// Einstellbare Eigenschaften des USB2LPT-Gerätes
 WORD LptBase;	// Abgefangene Basisadresse
 WORD TimeOut;	// Zeit zum Aufsammeln von OUT-Befehlen in ms (wenn WriteCache)
 BYTE flags;
#define UCB_Debugreg	0	// Verwendung von Debugregistern
#define UCB_Function	1	// Anzapfung der Kernel-Zugriffsfunktion
#define UCB_WriteCache	2	// Verwendung von TimeOut
#define UCB_ReadCache0	3	// Lokales Vorhalten von Basisadresse+0
#define UCB_ReadCache2	4	// Lokales Vorhalten von Basisadresse+2
#define UCB_ReadCacheN	5	// Lokales Vorhalten übriger Register
#define UC_Debugreg	0x01	// Verwendung von Debugregistern
#define UC_Function	0x02	// Anzapfung der Kernel-Zugriffsfunktion
#define UC_WriteCache	0x04	// Verwendung von TimeOut
#define UC_ReadCache0	0x08	// Lokales Vorhalten von Basisadresse+0
#define UC_ReadCache2	0x10	// Lokales Vorhalten von Basisadresse+2
#define UC_ReadCacheN	0x20	// Lokales Vorhalten übriger Register
 BYTE Mode;	// enum SPP,EPP,ECP,EPP+ECP
}TUserCfg,*PUserCfg;	// etwas 16-bit-lastig für Win9x-Eigenschaftsseite
// Der Inline-Assembler hat schwere Probleme mit Bitfeldern,
// was mich 6 Stunden Debuggen gekostet hat!

#define Vlpt_IOCTL_INDEX  0x0800

/* Konfigurieren (OutBytes==sizeof(TUserCfg)) bzw. Abfrage der
   Konfiguration (InBytes==sizeof(TUserCfg))
   Für den Eigenschaftsseiten-Lieferanten! */
#define IOCTL_VLPT_UserCfg		CTL_CODE(FILE_DEVICE_UNKNOWN,\
	Vlpt_IOCTL_INDEX+2,METHOD_BUFFERED,FILE_ANY_ACCESS)

/* Setzen (OutBytes==sizeof(TAccessCnt)) bzw. Abfrage der
   Zugriffe (InBytes==sizeof(TAccessCnt))
   Für den Eigenschaftsseiten-Lieferanten! */
#define IOCTL_VLPT_AccessCnt		CTL_CODE(FILE_DEVICE_UNKNOWN,\
	Vlpt_IOCTL_INDEX+3,METHOD_BUFFERED,FILE_ANY_ACCESS)

/* Schreiben und (optional) anschließendes Lesen von Daten
   über die beiden Pipes zum USB2LPT-Konverter.
   Synchrone Operation! Blockiert bis zum (bitteren) Ende!
   ShortTransferOK, dh. USB-Gerät darf auch
   einen kürzeren Datenblock zurücksenden.
   ACHTUNG: InBuffer sind (Bulk)OUT-Daten, OutBuffer (Bulk)IN-Daten! */
#define IOCTL_VLPT_OutIn		CTL_CODE(FILE_DEVICE_UNKNOWN,\
	Vlpt_IOCTL_INDEX+4,METHOD_BUFFERED,FILE_ANY_ACCESS)

//(DWORD)InputBuffer: 0=OUT-Pipe, 1=IN-Pipe, OutputBuffer ungenutzt
#define IOCTL_VLPT_AbortPipe		CTL_CODE(FILE_DEVICE_UNKNOWN,\
	Vlpt_IOCTL_INDEX+15,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_VLPT_GetLastError		CTL_CODE(FILE_DEVICE_UNKNOWN,\
	Vlpt_IOCTL_INDEX+23,METHOD_BUFFERED,FILE_ANY_ACCESS)

/* (Lesen und) Schreiben des internen Mikrocontroller-RAMs
   lpInBuffer: WORD offset, nInBufferSize: 2
   lpOutBuffer: Download-Daten (gehen ZUM Treiber)
   nOutputBufferSize: Länge der Download-Daten */
#define IOCTL_VLPT_AnchorDownload	CTL_CODE(FILE_DEVICE_UNKNOWN,\
	Vlpt_IOCTL_INDEX+27,METHOD_IN_DIRECT,FILE_ANY_ACCESS)


#ifdef DRIVER	// Ab hier treiber-interne Daten

#if DBG
# undef ASSERT
# define ASSERT(e) if(!(e)){DbgPrint("Verletzung einer Annahme in " \
  __FILE__", Zeile %d: " #e "\n", __LINE__); \
  _asm int 3}
# define Vlpt_KdPrint(_x_) { DbgPrint("usb2lpt.sys: "); DbgPrint _x_; }
# ifdef VERBOSE
#  define Vlpt_KdPrint2(_x_) { DbgPrint("usb2lpt.sys: "); DbgPrint _x_; }
# else
#  define Vlpt_KdPrint2(_x_)
# endif
# define TRAP() _asm int 3
#else
# define Vlpt_KdPrint(_x_)
# define Vlpt_KdPrint2(_x_)
# define TRAP()
#endif

typedef struct{		// Zeitgeber + zugehöriges DPC
 KTIMER tmr;		// Zeitgeber
 KDPC dpc;		// Rückruf (im Dispatch-Level)
}MYTIMER,*PMYTIMER;
 


typedef struct {	// Frei nach W. Oney:
 PDEVICE_OBJECT fdo;	// Rückwärts-Zeiger zu unserem Gerät
 PDEVICE_OBJECT pdo;	// Physikalisches Gerät (Bustreiber)
 PDEVICE_OBJECT ldo;	// "Tieferliegendes" Gerät, wohin URBs/IRPs gehen
/*
 DEVSTATE devstate;		// Gerätezustand
 DEVSTATE prevstate;
 POWERSTATE powerstate;		// Eigener Geräte-Stromversorgungs-Zustand
 DEVICE_POWER_STATE devpower;	// System-definierte Gerätestromversorgung
 SYSTEM_POWER_STATE syspower;	// Systemstromversorgung
 DEVICE_CAPABILITIES devcaps;	// zum Thema Stromversorgung
*/
 int instance;		// Zähler für Gerätenamen "LPTx"
 BYTE f;		// Zustands-Flags
#define Stopped 1	// Indicates that we have recieved a STOP message
   // Indicates that we are enumerated and configured.  Used to hold
   // of requests until we are ready for them
#define Started 2
   // Indicates the device needs to be cleaned up (ie., some configuration
   // has occurred and needs to be torn down).
// BOOLEAN NeedCleanup:1;
   // TRUE if we're trying to remove this device
#define removing 4
#define surprise 8
//#define trapping 16	// Trap-ISR gerade in Bearbeitung
//#define BiosRamPatch	0x40
#define No_Function	0x80
 WORD oldlpt;		// Gerettete LPTx-Adresse aus BIOS-Bereich
 WORD oldsys;		// Gerettetes BIOS-Ausstattungsbyte (410h)
 char debugreg[3];	// 0: 378-37B, 1: 37C-37F (EPP) 2: 778-77B (ECP)
 BYTE mirror[4];	// 0: Gültigkeits-Bits, 1: 378, 2: 37A
 BYTE bfill;		// Füllstand des OUT-Puffers
 BYTE buffer[63];	// Zum Puffern der OUT-Bytes + 1 IN-Byte
 TAccessCnt ac;
 TUserCfg uc;
 KMUTEX bmutex;		// Puffer-Mutex
   // configuration handle for the configuration the
   // device is currently in
// USBD_CONFIGURATION_HANDLE ConfigurationHandle;
 PUSB_DEVICE_DESCRIPTOR DeviceDescriptor;	// USB-Gerätebeschreiber
 PUSBD_INTERFACE_INFORMATION Interface;		// USB-Interface (nur eins)
// ULONG OpenHandles;
 USBD_STATUS LastFailedUrbStatus;		// Letzter USB-Fehler
 MYTIMER wrcache;	// Schreibcache-Zeitgeber (nichtperiodisch)
 KEVENT ev;		// ROT solange asynchroner URB verarbeitet wird
// NTSTATUS st;		// wird für Komplettierung gebraucht...???
// PURB u;		// zur Freigabe bei Komplettierung
// ULONG len;		// Länge der letzten Übertragung
// UNICODE_STRING ucDeviceLink;
 UNICODE_STRING ifname;	// Interface-Name für GUID-basierten Zugriff
 IO_REMOVE_LOCK rlock;	// zur Synchronisation beim Löschen des Gerätes
}DEVICE_EXTENSION, *PDEVICE_EXTENSION;

// Öffentliche Funktionen von usb2lpt.c
extern void _stdcall HandleOut(PDEVICE_EXTENSION,WORD,BYTE);
extern BYTE _stdcall HandleIn (PDEVICE_EXTENSION,WORD);

// Öffentliche Funktionen von vlpt.c
extern ULONG CurThreadPtr;
int _stdcall AllocDR(WORD,PDEVICE_EXTENSION);
int _stdcall FreeDRN(int);

void PrepareDR(void);	// Debugregister-Trap vorbereiten (kein Unprepare)

void HookSyscalls(void);	// READ|WRITE_PORT_UCHAR-Anzapfung setzen
void UnhookSyscalls(void);	// READ|WRITE_PORT_UCHAR-Anzapfung rückgängig

#endif	// Ende treiber-interne Daten



/* Kopfdatei für USB2LPT.SYS sowie für CoInstaller / Einstellprogramme
   sowie Sonderfunktionen (bspw. Firmware-Download)
*/
#ifdef DRIVER
# ifdef NTDDK
#  include <ntddk.h>
#  include <wmilib.h>
#  include <wmidata.h>
   NTKERNELAPI VOID ExFreePoolWithTag(PVOID,ULONG);	// Im W2K-DDK-.H nicht definiert, aber in .LIB vorhanden
# else
#  define _X86_ 1
#  include <wdm.h>
#  define ExFreePoolWithTag(p,t) ExFreePool(p)	// sonst läuft's nicht unter Win98
#  ifndef IoInitializeRemoveLock	// im 98DDK nicht definiert
#   include "w2k.h"			// Fehlendes "nachreichen"
#   define ULONG_PTR ULONG
#  endif
# endif
# include <usbdi.h>
# include <usbdlib.h>	// (enthält störende USB-Hub-GUID-Definition)
# ifdef INIT_MY_GUID
#  include <initguid.h>	// Erst ab jetzt GUIDs im Speicher ablegen!
# endif
#endif

#ifndef elemof
# define elemof(x) (sizeof(x)/sizeof((x)[0]))
#endif
#ifndef T
# define T(x) TEXT(x)
#endif

// {DA6B195A-AC68-4c67-B236-C1455804B1A8}
//DEFINE_GUID(Vlpt_GUID,0xda6b195aL,0xac68,0x4c67,0xb2,0x36,0xc1,0x45,0x58,0x4,0xb1,0xa8);
// same as GUID_DEVINTERFACE_PARALLEL and GUID_PARALLEL_DEVICE (111022)
DEFINE_GUID(Vlpt_GUID,0x97F76EF0L,0xF883,0x11D0,0xAF,0x1F,0x00,0x00,0xF8,0x00,0x84,0x5C);

typedef struct{	// Zähler für die Statistik
 ULONG out;	// OUT-Zugriffe
 ULONG in;	// IN-Zugriffe
 ULONG fail;	// bspw: Nicht unterstützte Opcodes (REP INSB u.ä.)
 ULONG steal;	// Gestohlene Debugregister (XP-Problem)
 ULONG wpu;	// WRITE_PORT_UCHAR-Aufrufe
 ULONG rpu;	// READ_PORT_UCHAR-Aufrufe
 ULONG wdw;	// Word- und DWord-Zugriffe (auch beim Debugregister-Trap)
 UCHAR debregs;	// Zugewiesene Debugregister für SPP(0), EPP(1), ECP(2), Extra(3), Avail(7)
 UCHAR trapped;	// Angezapft für SPP(0), EPP(1), ECP(2), Extra(3)
 UCHAR rsv[2];
}TAccessCnt, *PAccessCnt;

typedef struct{	// Einstellbare Eigenschaften des USB2LPT-Gerätes
 USHORT LptBase; // Abgefangene Basisadresse
 USHORT TimeOut; // Zeit zum Aufsammeln von OUT-Befehlen in ms (wenn WriteCache)
 UCHAR flags;
#define UCB_Debugreg	0	// Verwendung von (freien) Debugregistern
#define UCB_Function	1	// Anzapfung der Kernel-Zugriffsfunktion (nicht bei AMD64)
#define UCB_WriteCache	2	// Verwendung von TimeOut
#define UCB_ReadCache0	3	// Lokales Vorhalten von Basisadresse+0
#define UCB_ReadCache2	4	// Lokales Vorhalten von Basisadresse+2
#define UCB_ReadCacheN	5	// Lokales Vorhalten übriger Register (ungenutzt)
#define UCB_UseEp0Xfer	6	// (Low-Speed) Benutze EP0 anstatt INTERRUPT-Endpoints (TODO)
#define UCB_ForceAlloc	7	// Belegung erzwingen (siehe unten)
#define UC_Debugreg	0x01	// Use debug register trap
#define UC_Function	0x02	// Use READ/WRITE_PORT_/BUFFER_UCHAR/USHORT/ULONG redirection
#define UC_WriteCache	0x04	// Use TimeOut
#define UC_ReadCache0	0x08	// Use write-back cache for base address+0 (data port)
#define UC_ReadCache2	0x10	// Use write-back cahce for base address+2 (control port)
#define UC_ReadCacheN	0x20	// Use write-back cache for other ports (unimplemented)
#define UC_UseEp0Xfer	0x40	// Use EP0 rather than INTERRUPT endpoints
#define UC_ForceAlloc	0x80	// Adresse belegen, auch wenn alles voll:
				// In Verbindung mit UC_Debugreg: Debugregister (1 aus 4) erzwingen;
				// ohne UC_DebugReg: Nicht-Debugregister-Umleitung (1 aus 28) erzwingen;
				// auch: 98-Problem mit anscheinend vorbelegten Debugregistern erschlagen.
				// Überzählige Adressen rutschen zur Nicht-Debugregister-Umleitung
				// oder fallen ganz heraus.
 UCHAR Mode;	// enum SPP,EPP,ECP,EPP+ECP
// USHORT EcpBase;	// Abgefangene ECP-Adresse (normalerweise LptBase+400h, aber das ist kein Muss!)
}TUserCfg,*PUserCfg;	// etwas 16-bit-lastig für Win9x-Eigenschaftsseite
// Der Inline-Assembler hat schwere Probleme mit Bitfeldern,
// was mich 6 Stunden Debuggen gekostet hat!

#define Vlpt_CTL(a,b) CTL_CODE(FILE_DEVICE_UNKNOWN,0x0800+(a),METHOD_##b,FILE_ANY_ACCESS)

/* Konfigurieren (OutBytes==sizeof(TUserCfg)) bzw. Abfrage der
   Konfiguration (InBytes==sizeof(TUserCfg))
   Für den Eigenschaftsseiten-Lieferanten! */
#define IOCTL_VLPT_UserCfg		Vlpt_CTL(2,BUFFERED)

/* Setzen (OutBytes==sizeof(TAccessCnt)) bzw. Abfrage der
   Zugriffe (InBytes==sizeof(TAccessCnt))
   Für den Eigenschaftsseiten-Lieferanten! */
#define IOCTL_VLPT_AccessCnt		Vlpt_CTL(3,BUFFERED)

/* Schreiben und (optional) anschließendes Lesen von Daten
   über die beiden Pipes zum USB2LPT-Konverter.
   Asynchron-fähig!
   ShortTransferOK, dh. USB-Gerät darf auch
   einen kürzeren Datenblock zurücksenden.
   ACHTUNG: InBuffer sind (Bulk)OUT-Daten, OutBuffer (Bulk)IN-Daten! */
#define IOCTL_VLPT_OutIn		Vlpt_CTL(4,BUFFERED)

//(DWORD)InputBuffer: 0=OUT-Pipe, 1=IN-Pipe, OutputBuffer ungenutzt
#define IOCTL_VLPT_AbortPipe		Vlpt_CTL(15,BUFFERED)

//Rückfrage des letzten USB-Fehlerkodes, OutputBuffer = DWORD* Fehlerkode
#define IOCTL_VLPT_GetLastError		Vlpt_CTL(23,BUFFERED)

/* (Lesen und) Schreiben des internen Mikrocontroller-RAMs
   lpInBuffer: WORD offset, nInBufferSize: 2
   lpOutBuffer: Up/Download-Daten (gehen ZUM Treiber bei IN_DIRECT)
   nOutputBufferSize: Länge der Up/Download-Daten */
#define IOCTL_VLPT_AnchorDownload	Vlpt_CTL(0xA0,IN_DIRECT)
#define IOCTL_VLPT_RamRead		Vlpt_CTL(0xA1,OUT_DIRECT)
#define IOCTL_VLPT_RamWrite		Vlpt_CTL(0xA1,IN_DIRECT)
#define IOCTL_VLPT_EepromRead		Vlpt_CTL(0xA2,OUT_DIRECT)
#define IOCTL_VLPT_EepromWrite		Vlpt_CTL(0xA2,IN_DIRECT)
#define IOCTL_VLPT_XramRead		Vlpt_CTL(0xA3,OUT_DIRECT)
#define IOCTL_VLPT_XramWrite		Vlpt_CTL(0xA3,IN_DIRECT)
#define IOCTL_VLPT_FlashRead		IOCTL_VLPT_XramRead	// ATmega-Variante


#ifdef DRIVER	// Ab hier treiber-interne Daten

#if DBG
# undef ASSERT
# define ASSERT(e) if(!(e)){DbgPrint("Verletzung einer Annahme in %s, Zeile %d: %s\n", __FILE__, __LINE__, #e); \
  _asm int 3}
# define Vlpt_KdPrint(_x_) { DbgPrint("usb2lpt.sys:%d: ",__LINE__); DbgPrint _x_; }
# if DBG>1
#  define Vlpt_KdPrint2(_x_) { DbgPrint("usb2lpt.sys:%d: ",__LINE__); DbgPrint _x_; }
# else
#  define Vlpt_KdPrint2(_x_)
# endif
# define TRAP() _asm int 3
#else
# define Vlpt_KdPrint(_x_)
# define Vlpt_KdPrint2(_x_)
# define TRAP()
#endif

typedef enum {false,true} bool;

typedef struct{		// Zeitgeber + zugehöriges DPC
 KTIMER tmr;		// Zeitgeber
 KDPC dpc;		// Rückruf (im Dispatch-Level)
}MYTIMER,*PMYTIMER;


typedef struct {	// Frei nach W. Oney:
 PDEVICE_OBJECT fdo;	// Rückwärts-Zeiger zu unserem Gerät
 PDEVICE_OBJECT pdo;	// Physikalisches Gerät (Bustreiber)
 PDEVICE_OBJECT ldo;	// "Tieferliegendes" Gerät, wohin URBs/IRPs gehen
 int instance;		// Zähler für Gerätenamen "LPTx", nullbasiert (0 = LPT1)
 UCHAR f;		// Zustands-Flags
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
 USHORT oldlpt;		// Gerettete LPTx-Adresse aus BIOS-Bereich
 USHORT oldsys;		// Gerettetes BIOS-Ausstattungsbyte (410h)
// char debugreg[3];	// 0: 378-37B, 1: 37C-37F (EPP) 2: 778-77B (ECP)
 UCHAR mirror[4];	// 0: Gültigkeits-Bits, 1: 378, 2: 37A
 WCHAR portName[8];	// L"LPT1", L"LPT2" usw., nullterminiert (wird öfters gebraucht)
 UCHAR bfill;		// Füllstand des OUT-Puffers
 UCHAR buffer[63];	// Zum Puffern der OUT-Bytes + 1 IN-Byte
 TAccessCnt ac;
 TUserCfg uc;
 KMUTEX bmutex;		// Puffer-Mutex
 PUSB_DEVICE_DESCRIPTOR DeviceDescriptor;	// USB-Gerätebeschreiber
 PUSBD_INTERFACE_INFORMATION Interface;		// USB-Interface (nur eins)
 USBD_STATUS LastFailedUrbStatus;		// Letzter USB-Fehler
 MYTIMER wrcache;	// Schreibcache-Zeitgeber (nichtperiodisch)
 KEVENT ev;		// ROT solange asynchroner URB verarbeitet wird
 UNICODE_STRING ifname;	// Interface-Name für GUID-basierten Zugriff
 IO_REMOVE_LOCK rlock;	// zur Synchronisation beim Löschen des Gerätes
#ifdef NTDDK
 WMILIB_CONTEXT WmiLibCtx;
 PARPORT_WMI_ALLOC_FREE_COUNTS WmiPortAllocFreeCounts;
#endif
}DEVICE_EXTENSION, *PDEVICE_EXTENSION;

// Öffentliche Funktionen von usb2lpt.c
//(Diese unterstützen nun auch Word- und Dword-Zugriffe)
extern bool _stdcall HandleOut(PDEVICE_EXTENSION,USHORT, ULONG,UCHAR);
extern bool _stdcall HandleIn (PDEVICE_EXTENSION,USHORT,PULONG,UCHAR);

// Öffentliche Funktionen von vlpt.c
#ifndef NTDDK
extern ULONG CurThreadPtr;
#endif
extern ULONG DebRegStolen[3];
char _stdcall GetIndexDR(USHORT);
char _stdcall AllocDR(USHORT,PDEVICE_EXTENSION,UCHAR);
char _stdcall FreeDR(USHORT);

BOOLEAN IsPentium(void);
void PrepareDR(void);
#ifdef _X86_
void HookSyscalls(void);	// READ|WRITE_PORT_UCHAR-Anzapfung setzen
void UnhookSyscalls(void);	// READ|WRITE_PORT_UCHAR-Anzapfung rückgängig
#else	// AMD64-HAL hat keine derartigen Einsprungpunkte!
# define HookSyscalls()
# define UnhookSyscalls()
#endif

#endif	// Ende treiber-interne Daten



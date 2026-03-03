#ifndef _PROP_H_
#define _PROP_H_

#if !defined(__cplusplus) || defined(__BCPLUSPLUS__) && __BCPLUSPLUS__<0x400
 typedef enum {false,true} bool;
#endif

#define WIN32_LEAN_AND_MEAN	// wirkungslos bei Win16
#define NONAMELESSUNION		// BC verträgt keine namenlosen Union-Komponenten
#define STRICT			// wichtig für Win16
#define _WIN32_WINNT	0x0501	// Win32
#define WINVER		0x0500	// Win16, aber auch IDC_HAND bei Win32
#include <windows.h>
#include <windowsx.h>		// Message Cracker
#include <commdlg.h>
#include <commctrl.h>
#include <shellapi.h>
#include <prsht.h>

/*** WIN32-spezifische DEFINEs ***/
#ifdef WIN32	// Win32 wird zurzeit mit MSVC6 übersetzt
# include <shlwapi.h>
# include <setupapi.h>	// ->setupapi.lib
# include <cfgmgr32.h>	// ->cfgmgr32.lib
# include <objbase.h>
# include <winioctl.h>
# include "msports.h"
# define _ss		// keine Segmentregister-Überschreibungen
# define _ds		// will BC31++ für NEAR-Zeiger haben, trotz Speichermodell <small>
# define _loadds	// Laden von DS unnötig
extern HINSTANCE hInst;		// in prop.c
# ifndef _WIN64
#  undef RtlZeroMemory
#  undef RtlFillMemory
#  undef RtlCopyMemory
#  undef RtlMoveMemory
EXTERN_C void _declspec(dllimport) WINAPI RtlZeroMemory(void*,size_t);
EXTERN_C void _declspec(dllimport) WINAPI RtlFillMemory(void*,size_t,BYTE);
EXTERN_C void _declspec(dllimport) WINAPI RtlMoveMemory(void*,void*,size_t);
# endif
# define GetWindowPtr(w,a) (PVOID)GetWindowLongPtr(w,a)	// Für LocalAlloc-Zgr.
# define SetWindowPtr(w,a,p) SetWindowLongPtr(w,a,(LONG_PTR)(p))

/*** WIN16-spezifische DEFINEs ***/
#else		// Win16 wird mit BC3.1 oder BC4.x übersetzt
# include <memory.h>	// _fmemcmp für GUID-Vergleich (nur C++)
# include <setupx.h>
# include <compobj.h>	// EXTERN_C, benötigt _fmemcmp
# include "thunk16.h"
# include <stdarg.h>
// Warnungen durch "kuriose" Kopfdateien unterdrücken
typedef struct tagDEVMODE {int dummy;};	// windows
void WINAPI InitCommonControls(void);	// commctrl
typedef struct _TREEITEM {int dummy;};	// commctrl
typedef struct _IMAGELIST {int dummy;};	// commctrl
typedef struct IStream {int dummy;};	// setupx
typedef struct _INFLINE {int dummy;};	// setupx
typedef struct _INF {int dummy;};	// setupx
typedef struct _PSP {int dummy;};	// prsht

typedef char TCHAR,*PTSTR,FAR*LPTSTR,NEAR*NPTSTR;
typedef void *PVOID,FAR*LPVOID,NEAR*NPVOID;
typedef BYTE UCHAR;
typedef const void *PCVOID,FAR*LPCVOID,NEAR*NPCVOID;
typedef long LONG_PTR;
typedef int INT_PTR;
# define GetWindowLongPtr GetWindowLong	// Für MsgResult
# define SetWindowLongPtr SetWindowLong
# define GetWindowPtr(w,a) (PVOID)GetWindowWord(w,a)	// Für LocalAlloc-Zgr.
# define SetWindowPtr(w,a,p) SetWindowWord(w,a,(WORD)(p))
# define DWLP_USER DWL_USER
# define DWLP_MSGRESULT DWL_MSGRESULT
# define TEXT(x) x		// nur ANSI bzw. Multibyte-CP
# define _stdcall _cdecl	// gleiche Parameter-Reihenfolge!
# define MAKEPOINTS MAKEPOINT	// Punktkoordinaten immer "short"
# define hInst (HINSTANCE)_DS	// in 16-Bit-DLLs immer
# define PSP_USEFUSIONCONTEXT 0	// kein solcher Teletubbie-Optik-Schalter
# define PSPCB_ADDREF	0	// Ob diese Message kommt?
# define TTS_BALLOON	0x40
# define TTM_SETMAXTIPWIDTH	(WM_USER+24)
# define TPM_TOPALIGN		0
# define TPM_RETURNCMD		0x0100
# define TPM_NONOTIFY		0x0080
# define TPM_HORPOSANIMATION	0x0400
# define CCSIZEOF_STRUCT(structname, member)  (((int)((LPBYTE)(&((structname*)0)->member) - ((LPBYTE)((structname*)0)))) + sizeof(((structname*)0)->member))
# define LPTTHITTESTINFO LPHITTESTINFO
# define OFN_DONTADDTORECENT	0x02000000L
# define MB_HELP		0x4000	// Hilfeknopf in MessageBox
# define ParallelPortPropPageProvider(a,b,c) TRUE
//==== UL_C0DS.ASM ====
EXTERN_C unsigned long _pascal ss_strtoul(char _ss*, char _ss* _ss*, int radix);
// die Assembler-Funktion erfordert Radix!=0
EXTERN_C void _pascal RtlFillMemory(LPVOID,WORD,int);
#define RtlZeroMemory(p,l) RtlFillMemory(p,l,0)
EXTERN_C void _pascal RtlMoveMemory(LPVOID,LPVOID,WORD);

#define CTL_CODE(DeviceType,Function,Method,Access) \
  (((DWORD)(DeviceType)<<16)|((Access)<<14)|((Function)<<2)|(Method))

#define FILE_DEVICE_UNKNOWN	0x22
#define METHOD_BUFFERED		0
#define METHOD_IN_DIRECT	1
#define METHOD_OUT_DIRECT	2
#define METHOD_NEITHER		3
#define FILE_ANY_ACCESS		0

#endif

#ifdef INITGUID
# include <initguid.h>	// DEFINE_GUID, benötigt EXTERN_C
#endif
#include "usb2lpt.h"	// defeiniert u.a. "elemof" und "T"

/*** Allgemeine DEFINEs ***/
#define nobreak
typedef const TCHAR *PCTSTR, FAR*LPCTSTR, NEAR*NPCTSTR;
# define RtlCopyMemory RtlMoveMemory
#ifdef _DEBUG
# define TRAP() _asm int 3
# define GUARD(x) x
# define ASSERT(x) ASSERTX(x,__LINE__)
# define ASSERTX(x,l) if (!(x)) OutputDebugStringA("USB2LPT.dll: Assert fail: " #x " in " __FILE__ ":" #l "\r\n"),DebugBreak()
#else
# define TRAP()
# define GUARD(x)
# define ASSERT(x)
#endif

#define FATDATE(y,m,d) ((((y)-1980)<<9)+((m)<<5)+(d))

typedef struct{
 BYTE usage;		// Benutzungszähler, für 3 Dialoge
 BYTE wizard;		// Install-Wizard aktiv (CoDeviceInstall, nur Win32)
 TUserCfg uc;		// Konfiguration für Treiber (3 WORDs) - hat in TSetup eigentlich nichts zu suchen!
 TAccessCnt ac;		// Zugriffszähler aus Treiber (8 DWORDs) - hat in TSetup eigentlich nichts zu suchen!
 HFONT bold,italic;	// Für hübscheren Dialog, fette und kursive Schrift
#ifdef WIN32
 HANDLE dev;		// Griff zum .SYS-Treiber
 HDEVINFO Devs;		// brauchen wir fast nicht: SetupDi==Holzweg?
 PSP_DEVINFO_DATA pdid;
#else
 DWORD dev;		// Griff zum .SYS-Treiber (32bit!)
 DWORD kernel32;	// Geladene 32-bit-DLL
 LPDEVICE_INFO info;
#endif
}TSetup,_ds*PSetup,FAR*LPSetup,NEAR*NPSetup;

// für alle Windows-Strukturen:
#define InitStruct(p,l) {RtlZeroMemory(p,l); *(UINT*)(p)=l;}

//==== PROP.C ====
bool OpenDev(PSetup);
int DevIoctl(PSetup, DWORD code, LPCVOID p1, int l1, LPVOID p2, int l2);
void CloseDev(PSetup);
int vMBox(HWND Wnd, UINT id, UINT style, va_list arglist);
int _cdecl MBox(HWND Wnd, UINT id, UINT style,...);
bool GetDlgItemHex(HWND w, UINT id, UINT _ss* v);
void ChangeFonts(HWND, PSetup);
extern const TCHAR HelpFileName[];
void WM_ContextMenu_to_WM_Help(HWND, WPARAM, LPARAM, bool TrackDisabled=false);
// Bei TrackDisabled==true gibt's auch dann WM_HELP, wenn eine
// Groupbox oder ein gesperrtes Fenster (jeweils mit ID!=-1)
// getroffen wurde, sonst nur sichtbare und ungesperrte Fenster
#ifdef WIN32
void AddContextHelpButton(HWND Wnd);
// für Windows 7: den plötzlich fehlenden Knopf in die Titelleiste setzen
// Aus Faulheit nehme ich den Knopf nicht wieder weg (beim Verlassen einer meiner Eigenschaftsseiten)
#else
# define AddContextHelpButton(x)	// leere Expansion unter Win16
#endif

//==== MON.C ====
INT_PTR CALLBACK _loadds MonDlgProc(HWND, UINT, WPARAM, LPARAM);
void PopulateEcrComboBox(HWND);

#endif//_PROP_H_

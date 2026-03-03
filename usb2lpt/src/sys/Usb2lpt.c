/* Treiber für das USB2LPT-Gerät, haftmann#software 2004-2012
 0410xx	Zarte Versuche mit tausenden Abstürzen
 0501xx	Entdeckung der Notwendigkeit der Disassemblierung von Windows' ISRs (HAL)
 050410	Erste ordentliche Ausgabe
+051104	Zweite Ausgabe mit WORD+DWORD-Unterstützung (wegen NI LabVIEW),
	leider Problem mit Xilinx iMPACT
+061007	GALEP-Murks: IRQL runter- und wieder raufsetzen
	(GALEP32.EXE läuft dann - allerdings nur mit einem weiteren Patch)
+061008	Low-Speed über die beiden Interrupt-Pipes (ungetestet, für 1.5)
	Erweist sich späterhin unter Vista für notwendig
+061011	Priority-Boost nach dem Warten (sonst pennt Galep bei paralleler DOS-Box)
*061203	Debugregister und INT1-Anzapfung bei NT/XP standardmäßig AUS
	(wegen zunehmendem SMP-Problem);
	der Treiber macht sich erst bei Debugregister-Aktivierung
	an der IDT zu schaffen (das betrifft auch den Lesezugriff)
	Hoffentlich hilft das auch! - Nein, es hilft nicht.
-070209	Bug: EP0STALL (EEPROM-Zugr.) führt zum Absturz; Speicherleck
*070602	READ_PORT_UCHAR-Anzapfung bei NT/XP standardmäßig AUS
	Hoffentlich hilft das auch! - Nein, es hilft nicht.
-070602	BUG:Bluescreen: durch Timer-DPC nach RemoveDevice (erledigt?)
-070930	Low-Speed-Extrawurst raus: unnötig unter Windows
	64-bit-Zeiger-Kompatibilität (ULONG_PTR)
	Windows-95-Extrawürste raus (Treiber funktionierte eh' nie unter 95)
	IRP_MN_QUERY_RESOURCE_REQUIREMENTS raus (hat nie funktioniert)
	HookSyscalls beim Treiber-Start (Absturz von Gerätemanager?)
	Flags retten bei Read_Port_Uchar() usw.
	Supervisor Mode Bit in Ausgangszustand (Absturz svchost.exe bei SMP?)
	Xilinx iMPACT läuft wieder, aber nur bei Einkernmaschinen
+081219	Vista-Anpassung für modifizierte Low-Speed-Firmware (TEST!)
+090623	Vista-Anpassung als Low-Speed-Extrawurst (noch nicht via Control-Pipe)
+090926	Debugregister-Klau-Problem erschlagen? Nicht ganz! Hauptproblem SMP!
	Auch bei Einkernmaschinen unsauber, Zähler nur deutlich langsamer (3/h statt 5/min)
+090927	Ausgabe der aktuellen Debugregister-Belegung für Gerätemanager
	Globaler (statt mehrere lokale) Debugregisterklau-Zähler
	Unschönes Verwaltungsproblem mit READ_PORT_UCHAR-Umleitung entdeckt
+091020	Einigermaßen saubere Mehrkernunterstützung (erheblich neuer Kode),
	getrennte .SYS-Treiber für 98/Me und NT, Abschied vom reinen WDM
+091101	Win98/Me: Portzugriffs-Umleitung via IOPM, zählermäßig parallelgeschaltet
	zur READ_PORT_UCHAR-Umleitung, vermeidet (langsamere?) Debugregister-Traps
	(unterdokumentiertes WDM-Featue), leider ohne Erfolg für das GhaiRacer-Problem
-091106	Bluescreen BAD_POOL_CALLER erschlagen, dafür neuer Bluescreen:
-091109	Bluescreen MULTIPLE_IRP_REQUESTS erschlagen
-091113	Bluescreen PAGE_FAULT (beim Herunterfahren, ISA-PnP, sehr lange bekannt!) erschlagen
-110707	Bluescreen DRIVER_PAGE_FAULT_IN_FREED_SPECIAL_POOL (nur mit DriverVerifier)
+110708	IRP_MJ_INTERNAL_DEVICE_CONTROL dazu, aber erst mal ungetestet
-111022	Bluescreen-Jagd (ergebnisoffen; erhebliche Änderngen bei MJ_POWER)
	Neue GUID für Treiber-Interface: nunmehr die gleiche wie in parport.sys.
	Gebracht hat's anscheinend nichts.
	(Ich hatte die Hoffnung, dass dies das falsche-LPT-Nummer-Anzeige-Problem löst.)
-120214	Rückgabewert bei EEPROM/XRAM-READ/WRITE jetzt in Ordnung
*120307	Selektion Low-Speed BULK/INTERRUPT unabhängig von Reihenfolge der Alternate Settings
	Neue INF-Datei löst das "Zypern-Problem" BAD_POOL_CALLER(C2):ALREADY_FREED(7)
+120309	Durchlass von IOCTL_VLPT_RamRead und _RamWrite - fehlt noch Firmware auf 8051
	Vorbereitung für den EP0-Datentransfer (zur Umgehung der INTERRUPT-Pipes)
+130515	WMI-Unterstützung (Driver Verifier hat sich sonst beschwert)
-130527	vergessenes WMI-Unregister führte zum fehlerhaften Treiber-Entladen
-150302	Bluescreen beim Treiberstart bei bestimmten Prozessorsteppings (oops!)
-150304	READ_PORT_UCHAR-Trap konnte nicht abgeschaltet werden
 150509	Inangriffnahme DriverVerifier-Bluescreen: erfolglos
TODO	Nativer Control-Transfer-Support (für Low-Speed unter Win6+)
	Hoffentlich bringt das ein bisschen mehr Speed!
*/

#define INIT_MY_GUID	// In diese .OBJ-Datei kommt die 16-Byte-GUID hinein
#include "usb2lpt.h"
#include <stdio.h>	// _snwprintf

// Nach dem Warten auf den (meistens) USB-IN-Transfer wird die dynamische
// Thread-Priorität um folgenden Betrag angehoben:
#define USB2LPT_BOOST IO_PARALLEL_INCREMENT	// =1

/****************************
 ** PnP-Standardbehandlung **
 ****************************/

NTSTATUS DefaultPnpHandler(PDEVICE_EXTENSION X, PIRP I) {
 IoSkipCurrentIrpStackLocation(I);
 return IoCallDriver(X->ldo,I);
}

NTSTATUS OnRequestComplete(IN PDEVICE_OBJECT fdo,IN PIRP Irp,IN PKEVENT pev) {
 KeSetEvent(pev, 0, FALSE);
 return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS ForwardAndWait(PDEVICE_EXTENSION X,IN PIRP Irp) {
/* Forward request to lower level and await completion
   The processor must be at PASSIVE IRQL because this function initializes
   and waits for non-zero time on a kernel event object.
*/
 KEVENT event;
 NTSTATUS ntStatus;

 ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
  // Initialize a kernel event object to use in waiting for the lower-level
	// driver to finish processing the object. 
 KeInitializeEvent(&event, NotificationEvent, FALSE);
 IoCopyCurrentIrpStackLocationToNext(Irp);
 IoSetCompletionRoutine(Irp, (PIO_COMPLETION_ROUTINE) OnRequestComplete,
   (PVOID) &event, TRUE, TRUE, TRUE);
 ntStatus = IoCallDriver(X->ldo, Irp);
 if (ntStatus==STATUS_PENDING) {
  KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
  ntStatus = Irp->IoStatus.Status;
 }
 return ntStatus;
}

NTSTATUS CompleteRequest(IN PIRP I,IN NTSTATUS ret,IN ULONG_PTR info) {
/* Mark I/O request complete
Arguments:
   Irp - I/O request in question
   status - returned status code
   info Additional information related to status code
Reicht <status> durch
*/
 I->IoStatus.Status=ret;
 I->IoStatus.Information=info;
 IoCompleteRequest(I,IO_NO_INCREMENT);
 return ret;
}

/*****************************
 ** Asynchrone USBD-Aufrufe **
 *****************************/

typedef struct{	// "Arbeitsauftrag" für kombinierte Bulk-Aus- und Eingabe
 PDEVICE_EXTENSION x;	// Unsere Geräteerweiterung
 PIRP irpj;	// Job-IRP, oder NULL für angezapften Ein/Ausgabebefehl
 PIRP irpw;	// IRP zum Bulk-Schreiben
 PIRP irpr;	// IRP zum Bulk-Lesen (NULL für Nur-Schreiben)
 NTSTATUS stat;	// zurückzugebender Status (initialisieren mit 0)
 ULONG rlen;	// Gelesene Bytes (initialisieren mit 0)
 LONG ct;	// Zähler für Callback-Aufrufe
 struct _URB_BULK_OR_INTERRUPT_TRANSFER urbw;	// Ohne extra ExAllocatePool
 struct _URB_BULK_OR_INTERRUPT_TRANSFER urbr;	// entfällt bei irpr=0
}JOB,*PJOB;

void SetUsbStatus(PDEVICE_EXTENSION X, NTSTATUS *stat, PURB U) {
 if (!USBD_SUCCESS(U->UrbHeader.Status)) {
  Vlpt_KdPrint(("*** USB-Fehlerkode %X\n",U->UrbHeader.Status));
  X->LastFailedUrbStatus=U->UrbHeader.Status;
  if (stat && NT_SUCCESS(*stat)) *stat=STATUS_UNSUCCESSFUL;
 }
}

// Komplettierungsroutine für IOCTL_OutIn und PortTrap
// IRQL<=2
// TODO: Für Control-Transfer (Low-Speed) erweitern
NTSTATUS OutInFertig(PDEVICE_OBJECT fdo,PIRP I,PJOB J) {
 if (NT_SUCCESS(J->stat)) J->stat=I->IoStatus.Status;
 if (I==J->irpw) {
  J->irpw=NULL;			// erledigt!
  if (fdo!=(PVOID)1) {
   SetUsbStatus(J->x,&J->stat,(PURB)&J->urbw);
   Vlpt_KdPrint2(("Ausgabe von %d Bytes FERTIG\n",J->urbw.TransferBufferLength));
  }else{
   Vlpt_KdPrint(("Ausgabe: Abbruch\n"));
  }
  IoFreeIrp(I);
 }else if (I==J->irpr) {
  J->irpr=NULL;			// erledigt!
  if (fdo!=(PVOID)1) {
   SetUsbStatus(J->x,&J->stat,(PURB)&J->urbr);
   J->rlen=J->urbr.TransferBufferLength;
   Vlpt_KdPrint2(("Eingabe von %d Bytes FERTIG\n",J->rlen));
  }else{
   Vlpt_KdPrint(("Eingabe: Abbruch\n"));
  }
  IoFreeIrp(I);
 }else Vlpt_KdPrint(("Unbekanntes IRP!\n"));
 if (!InterlockedDecrement(&J->ct)) {	// Alles erledigt?
  if (J->irpj) {		// wenn ursächlich DeviceIoControl (User)
   J->irpj->IoStatus.Status=J->stat;
   J->irpj->IoStatus.Information=J->rlen;
   IoCompleteRequest(J->irpj,USB2LPT_BOOST);
  }else{
   J->x->bfill=0;		// ursächlich Port-Trap (System)
   KeSetEvent(&J->x->ev,USB2LPT_BOOST,FALSE);	// bfill-Ampel auf Grün
  }
  ExFreePoolWithTag(J,'tplJ');		// Job (Arbeitsauftrag) samt URBs erledigt
 }
 return STATUS_MORE_PROCESSING_REQUIRED;
}

// Routine zum "gleichzeitigen" Laufenlassen zweier USB-Transfers!
// J muss von ExAllocatePoolWithTag(...'tplJ') kommen, wird letztendlich freigegeben
// I muss von IoAllocateIrp kommen, wird am Ende freigegeben
// U=ein Zeiger in J
// TODO: Für Control-Transfer (Low-Speed) erweitern
NTSTATUS AsyncCallUSBDJob(PDEVICE_EXTENSION X,PJOB J,PIRP I,PURB U) {
 PIO_STACK_LOCATION nextStack;
 Vlpt_KdPrint2(("Aufruf AsyncCallUSBDJob\n"));
 ASSERT(I);
 nextStack=IoGetNextIrpStackLocation(I);		//IRQL<=2
 ASSERT(nextStack);
 if (!nextStack) return STATUS_UNSUCCESSFUL;
 nextStack->MajorFunction=IRP_MJ_INTERNAL_DEVICE_CONTROL;
 nextStack->Parameters.DeviceIoControl.IoControlCode
   =IOCTL_INTERNAL_USB_SUBMIT_URB;
 nextStack->Parameters.Others.Argument1=U;
 IoSetCompletionRoutine(I,OutInFertig,J,TRUE,TRUE,TRUE);//IRQL<=2
 return IoCallDriver(X->ldo,I);				//IRQL<=2
}

// Komplettierungsroutine für asynchrone Bearbeitung von RAM/EEPROM/XRAM-Blocktransfers
// IRQL<=2
NTSTATUS AsyncCallReady(PDEVICE_OBJECT fdo,PIRP I,PURB U) {
 NTSTATUS ret=I->IoStatus.Status;
 PDEVICE_EXTENSION X=fdo->DeviceExtension;
 if (I->PendingReturned) IoMarkIrpPending(I);	// will Microsoft so, sonst TOT
 if (NT_SUCCESS(ret)) {
  I->IoStatus.Information=U->UrbControlVendorClassRequest.TransferBufferLength;
  Vlpt_KdPrint2(("Transfer von %d Bytes OK\n",I->IoStatus.Information));
 }else{
  Vlpt_KdPrint(("Transfer versagt, Code %X, USB-Code=%X\n",ret,U->UrbHeader.Status));
  TRAP();
  SetUsbStatus(X,NULL,U);
 }
 ExFreePoolWithTag(U,'tplU');
 return ret;
}

// Routine für asynchrone Bearbeitung von RAM/EEPROM/XRAM-Blocktransfers
// I wird NICHT freigegeben (IRP stets "von außen")
// U muss von ExAllocatePoolWithTag(...'tplU') kommen, wird freigegeben
NTSTATUS AsyncCallUSBD(PDEVICE_EXTENSION X,PIRP I,PURB U) {
 PIO_STACK_LOCATION nextStack;
 Vlpt_KdPrint2(("Aufruf AsyncCallUSBD\n"));
 ASSERT(I);
 nextStack=IoGetNextIrpStackLocation(I);		//IRQL<=2
 ASSERT(nextStack);
 if (!nextStack) return STATUS_UNSUCCESSFUL;
 nextStack->MajorFunction=IRP_MJ_INTERNAL_DEVICE_CONTROL;
 nextStack->Parameters.DeviceIoControl.IoControlCode
   =IOCTL_INTERNAL_USB_SUBMIT_URB;
 nextStack->Parameters.Others.Argument1=U;
 IoSetCompletionRoutine(I,AsyncCallReady,U,TRUE,TRUE,TRUE);//IRQL<=2
 return IoCallDriver(X->ldo,I);				//IRQL<=2
}

/***********************
 ** Ein/Ausgabe-IOCTL **
 ***********************/

NTSTATUS OutInCheck(PDEVICE_EXTENSION X, ULONG ol, ULONG il) {
// Prüfen der Parameter für Aus- und Eingabe, nur für IOCTL
 PUSBD_INTERFACE_INFORMATION ii=X->Interface;
 int i;
 if (!ii) {
  Vlpt_KdPrint(("OutInCheck(): keine Interface-Info!\n"));
  return STATUS_UNSUCCESSFUL;
 }
 if (ii->NumberOfPipes<2) {
  Vlpt_KdPrint(("OutInCheck(): Zu wenig Pipes!\n"));
  return STATUS_UNSUCCESSFUL;
 }
 for (i=0; i<2; i++) {
  PUSBD_PIPE_INFORMATION p=ii->Pipes+i;
  if (p->PipeType!=UsbdPipeTypeBulk && p->PipeType!=UsbdPipeTypeInterrupt) {
   Vlpt_KdPrint(("OutInCheck(): Pipe nicht vom Typ BULK oder INTERRUPT!\n"));
   return STATUS_UNSUCCESSFUL;
  }
  if (!p->PipeHandle) {
   Vlpt_KdPrint(("OutInCheck(): kein Pipe-Handle!\n"));
   return STATUS_UNSUCCESSFUL;
  }
 }
 if (ol>ii->Pipes[0].MaximumTransferSize) {
  Vlpt_KdPrint(("OutInCheck(): Zu große Ausgabe-Transferlänge!\n"));
  return STATUS_INVALID_PARAMETER;
 }
 if (il>ii->Pipes[1].MaximumTransferSize) {
  Vlpt_KdPrint(("OutInCheck(): Zu große Eingabe-Transferlänge!\n"));
  return STATUS_INVALID_PARAMETER;
 }
 return STATUS_SUCCESS;
}

NTSTATUS OutIn(PDEVICE_EXTENSION X, PUCHAR ob, ULONG ol, PUCHAR ib, ULONG il,
  PULONG bytesread, PIRP I) {
// Aus- und Eingabe über die beiden Pipes zum/vom USB2LPT-Gerät
// ob: Ausgabe-Bytes (i.d.R. Adresse-Data-Adresse-Data...-Adresse+0x10)
// ol: Ausgabe-Länge (i.d.R. <= 64 Bytes)
// ib: Eingabe-Puffer (oft nur 1 Byte)
// il: Eingabe-Pufferlänge (oft == 1)
// bytesread ("gelesene Bytes") == NULL -> kurzer Transfer nicht OK
// I == NULL -> Aufruf kommt vom Port-Trap (kein IRP; keine Komplettierung)
// Gänsemarsch (X->bmutex) erforderlich! (Aufrufer muss dafür sorgen.)
// Für maximalen Durchsatz werden beide USB-Transfers gleichzeitig initiiert.
// Fertig wird der gesamte OutIn-Transfer, wenn beide USB-Transfers
// fertig sind (normalerweise ist der IN-Transfer zuletzt fertig)
// Diese Routine ist asynchron unabhängig von I.
// Bei Aufruf mit I!=0 muss IoMarkIrpPending() gesetzt sein,
// dann ist der Aufruf mit il==ol==0 unzulässig.
// IRQL <= DISPATCH_LEVEL
// TODO: Für Control-Transfer (Low-Speed) erweitern
 PUSBD_INTERFACE_INFORMATION ii=X->Interface;
 PJOB J;
#define USIZE sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER)

 ASSERT(!I|il|ol);	// Bei I!=NULL muss eine Länge !=0 sein!
 if (!ol && !il) return STATUS_SUCCESS;
 Vlpt_KdPrint2(("Aufruf OutIn (ol=%d, il=%d)\n",ol,il));
 J=ExAllocatePoolWithTag(NonPagedPool,il?sizeof(JOB):sizeof(JOB)-USIZE,'tplJ');
 ASSERT(J);
 if (!J) return STATUS_NO_MEMORY;
 RtlZeroMemory(J,sizeof(JOB)-USIZE-USIZE);
 J->x=X;
 J->irpj=I;
 if (ol) {
  J->ct++;
// IoBuildDeviceIoControlRequest geht nicht wegen PASSIVE_LEVEL!
  UsbBuildInterruptOrBulkTransferRequest((PURB)&J->urbw,
    USIZE,			//size of urb
    ii->Pipes[0].PipeHandle,	// Erste Pipe = Schreiben
    ob,NULL,ol,0,NULL);
  J->irpw=IoAllocateIrp(X->ldo->StackSize,FALSE);
  ASSERT(J->irpw);		// Nach diesem ASSERT kracht DriverVerifier mit C9_23E
  if (!J->irpw) {		// aber warum IoAllocateIrp() nicht will ist unklar
   ExFreePoolWithTag(J,'tplJ');
   return STATUS_INSUFFICIENT_RESOURCES;
  }
 }
 if (il) {
  J->ct++;
  UsbBuildInterruptOrBulkTransferRequest((PURB)&J->urbr,
    USIZE,			//size of urb
    ii->Pipes[1].PipeHandle,	// Zweite Pipe = Lesen
    ib,NULL,il,
    bytesread			// je nachdem!
     ? USBD_TRANSFER_DIRECTION_IN|USBD_SHORT_TRANSFER_OK
     : USBD_TRANSFER_DIRECTION_IN,
    NULL);
  J->irpr=IoAllocateIrp(X->ldo->StackSize,FALSE);
  ASSERT(J->irpr);
  if (!J->irpr) {
   if (J->irpw) IoFreeIrp(J->irpw);	// Rückgängig machen der irpw-Allokation
   ExFreePoolWithTag(J,'tplJ');		// der Compiler wird's schon optimieren
   return STATUS_INSUFFICIENT_RESOURCES;
  }
 }
 if (!I) KeClearEvent(&X->ev);	// Ampel auf ROT
 if (ol) {
  I=J->irpw;
  if (!NT_SUCCESS(AsyncCallUSBDJob(X,J,I,(PURB)&J->urbw))) {
   Vlpt_KdPrint((" Problem 1\n"));
   OutInFertig((PVOID)1,I,J);	// gibt ggf. Puffer und IRP frei
  }
 }
 if (il) {
  I=J->irpr;			// 110707: Driver Verifier, J giftig wenn !il
  if (!NT_SUCCESS(AsyncCallUSBDJob(X,J,I,(PURB)&J->urbr))) {
   Vlpt_KdPrint((" Problem 2\n"));
   OutInFertig((PVOID)1,I,J);	// gibt ggf. Puffer und IRP frei
  }
 }
#undef USIZE
 return STATUS_PENDING;		// Jetzt sind bis zu zwei URBs in Arbeit
}

/****************************
 ** Ein/Ausgabe-Simulation **
 ****************************/

// Konvertieren LPT-Adresse in Adressbyte
static UCHAR ab(PDEVICE_EXTENSION X, USHORT adr) {	// Adress-Byte ermitteln
 adr-=X->uc.LptBase;	// sollte 0..7 oder 400h..403h liefern
 if (adr&0x400) adr|=8;	// ECP-Adressbyte draus machen
 return (UCHAR)adr;
}

#define GHAIRACER

static void __fastcall WaitForUrbComplete(PDEVICE_EXTENSION X) {
#if !defined(NTDDK) && defined(GHAIRACER)
 static const LARGE_INTEGER to={0,0};
 while (KeWaitForSingleObject(&X->ev,Suspended,KernelMode,FALSE,(PLARGE_INTEGER)&to)!=STATUS_SUCCESS);
#else
 KeWaitForSingleObject(&X->ev,Suspended,KernelMode,FALSE,NULL);
#endif
}

// Aufruf nur im Gänsemarsch möglich! Flags f:
// 1=ForceFlush (sonst nur wenn voll; "voll" heißt Füllstand >=62)
// 2=WaitReady (IRQL muss 0 sein!)
// 4=CancelTimer
static NTSTATUS FlushBuffer(PDEVICE_EXTENSION X,PUCHAR ib,ULONG il,UCHAR f) {
 NTSTATUS ret=STATUS_SUCCESS;
 if (f&1 || X->bfill>=62) {
  if (f&UC_WriteCache&X->uc.flags) KeCancelTimer(&X->wrcache.tmr);
  ret=OutIn(X,X->buffer,X->bfill,ib,il,NULL,NULL);
  if (f&2 && NT_SUCCESS(ret)) {
//   Vlpt_KdPrint2(("Warten auf Ende von OutIn()\n"));
   WaitForUrbComplete(X);
//   Vlpt_KdPrint2(("Das Warten hat ein Ende!\n"));
  }
 }
 return ret;
}

// Umleiten (oder Puffern) eines (bereits geTRAPten) OUT-Befehls, IRQL==0
// adr=Parallelport-Adresse (wird später Adress-Byte)
// b  =Datenbyte, -word oder -dword (entspr. OUT DX,AL, DX,AX oder DX,EAX)
// len=Länge (1, 2 oder 4)
bool _stdcall HandleOut(PDEVICE_EXTENSION X,USHORT adr,ULONG b, UCHAR len) {
 UCHAR a;	// Adress-Byte für USB
 bool ok=false;
#if DBG
 ULONG mask=(1<<(len<<3))-1; // Nicht auszugebende Bytes ausmaskieren (Debug)
#endif
 KIRQL irql;

 Vlpt_KdPrint2(("HandleOut(%03Xh,%0*Xh)\n",adr,2*len,b&mask));
 irql=KeGetCurrentIrql();
 if (irql) KeLowerIrql(0);		// GALEP: absenken!
 if (/*!KeGetCurrentIrql()	// kann nicht behandeln, wenn <>0!
 &&*/ NT_SUCCESS(IoAcquireRemoveLock(&X->rlock,NULL))) {
// Ab hier Gänsemarsch erzwingen, anderer OUT- oder IN-Befehl muss warten
  if (!KeWaitForMutexObject(&X->bmutex,Executive,KernelMode,FALSE,NULL)) {
   WaitForUrbComplete(X);
   ASSERT(X->bfill<=62 && !(X->bfill&1));	// Füllstand muss gerade sein!
   a=ab(X,adr);		// Adressbyte für USB2LPT-Firmware
   do{			// Über die 1-4 Bytes von b iterieren...
    if (!NT_SUCCESS(FlushBuffer((X),NULL,0,6))) goto ex;// Puffer voll? Dann ausgeben!
    switch (a) {
     case 0: {		// 378h, Datenport
      X->mirror[0]|=UC_ReadCache0;
      X->mirror[1]=(UCHAR)b;
     }break;
     case 2: {		// 37Ah, Steuerport
      X->mirror[0]|=UC_ReadCache2;
      X->mirror[2]=(UCHAR)b;
     }break;
    }
    X->buffer[X->bfill++]=a;
    X->buffer[X->bfill++]=(UCHAR)b;
    a++;
    b>>=8;
   }while(--len);
   if (/*!irql && */X->uc.flags&UC_WriteCache) {
    if (NT_SUCCESS(KeSetTimer(&X->wrcache.tmr,
      RtlConvertLongToLargeInteger(X->uc.TimeOut*-10000),
      &X->wrcache.dpc))) ok++;	// Zeit wird nicht größer als DWORD
   }else if (NT_SUCCESS(FlushBuffer(X,NULL,0,3))) ok++;	// sofort versenden!
ex:
   KeReleaseMutex(&X->bmutex,FALSE);
  }
  IoReleaseRemoveLock(&X->rlock,NULL);
 }
 if (irql) KeRaiseIrql(irql,&irql);	// GALEP: wieder anheben
 Vlpt_KdPrint2(("HandleOut EXIT\n"));
 if (!ok) {
  Vlpt_KdPrint(("HandleOut(%03Xh) versagt! IRQL=%u\n",adr,KeGetCurrentIrql()));
  TRAP();
 }
 return ok;
}

// Umleiten eines (bereits geTRAPten) IN-Befehls, IRQL==0
// Parameter wie bei HandleOut()
bool _stdcall HandleIn(PDEVICE_EXTENSION X,USHORT adr,PULONG ret,UCHAR len) {
 UCHAR a,ll;
 bool ok=false;
 ULONG mask=(1UL<<(len<<3))-1;
 KIRQL irql;

 *ret|=mask;	// Low-Teil im Fehlerfall FFh (oder FFFFh oder FFFFFFFFh)
 Vlpt_KdPrint2(("HandleIn ENTER\n"));
 irql=KeGetCurrentIrql();
 if (irql) KeLowerIrql(0);		// GALEP: absenken!
 if (/*!KeGetCurrentIrql()		// kann nicht behandeln!
 &&*/ NT_SUCCESS(IoAcquireRemoveLock(&X->rlock,NULL))) {
// Ab hier Gänsemarsch erzwingen, anderer OUT- oder IN-Befehl muss warten
  if (!KeWaitForMutexObject(&X->bmutex,Executive,KernelMode,FALSE,NULL)) {
   WaitForUrbComplete(X);
   ASSERT(X->bfill<=62 && !(X->bfill&1));	// Füllstand muss gerade sein!

   if (len+X->bfill>63	// Puffer würde überlaufen? Ausgeben!
   && !NT_SUCCESS(FlushBuffer(X,NULL,0,7))) goto ex;

   a=ab(X,adr);
   if (len==1) switch (a) {
    case 0: if (X->mirror[0]&X->uc.flags&UC_ReadCache0) {
     *(PUCHAR)ret=X->mirror[1];
     ok++;
     goto ex;
    }break;
    case 2: if (X->mirror[0]&X->uc.flags&UC_ReadCache2) {
     *(PUCHAR)ret=X->mirror[2]|0xE0;
     ok++;
     goto ex;
    }break;
   }
// Bei Word- und DWord-Zugriffen (len!=1) wird kein Cache benutzt
   ll=len; do{
    X->buffer[X->bfill++]=a|0x10;	// 1-4 Adress-Bytes einsetzen
    a++;
   }while(--ll);

   if (NT_SUCCESS(FlushBuffer(X,(PUCHAR)ret,len,7))) ok++;
ex:
   KeReleaseMutex(&X->bmutex,FALSE);
  }
  IoReleaseRemoveLock(&X->rlock,NULL);
 }
 if (irql) KeRaiseIrql(irql,&irql);	// GALEP: wieder anheben
 Vlpt_KdPrint2(("HandleIn(%03Xh) liefert %0*Xh\n",adr,2*len,*ret&mask));
 if (!ok) {
  Vlpt_KdPrint(("HandleIn(%03Xh) versagt! IRQL=%u\n",adr,KeGetCurrentIrql()));
  TRAP();
 }
 return ok;
}

#if 0
typedef enum {PT_IO16=8,PT_IO32=16,PT_DEBREG=2,PT_STD=256,PT_OUT=4};	// flag bits; loops==1 for regular (non-string) I/O
bool _stdcall HandleIO(PDEVICE_EXTENSION X,USHORT adr,PVOID data,SIZE_T loops,ULONG flags) {
 UCHAR l;
 if (flags&PT_DEBREG) {
  if (flags&PT_OUT) X->ac.out++;
  else X->ac.in++;
 }else{
  if (flags&PT_OUT) X->ac.wpu++;
  else X->ac.rpu++;
 }
 l=1<<(flags&(PT_IO16|PT_IO32));	// returns 1, 2, 4, maybe 8 if "in rax,dx" exists for X64
 if (l!=1) X->ac.wdw++;
 if (loops!=1) {
  X->ac.fail++;
  return false;
 }
 if (flags&PT_OUT) return HandleOut(X,adr,*(PULONG)data,l);
 else return HandleIn(X,adr,data,l);
}
#endif

// Aufruf vom Kernel, wenn Schreibcache-Haltezeit abgelaufen ist
VOID TimerDpc(IN PKDPC Dpc,PDEVICE_EXTENSION X,PVOID a,PVOID b) {
 ASSERT(KeGetCurrentIrql()==DISPATCH_LEVEL);
 Vlpt_KdPrint2(("TimerDpc, Ausgabe %u Bytes\n",X->bfill));
 FlushBuffer(X,NULL,0,1);
}

/****************************
 ** Synchrone USBD-Aufrufe **
 ****************************/

NTSTATUS CallUSBD(PDEVICE_EXTENSION X, PURB U) {
/* Passes a Usb Request Block (URB) to the USB class driver (USBD)
   Diese Routine blockiert!! IRQL=0!!
   Ein sinnvolles TimeOut wäre hier dringend angeraten!!??
*/
 NTSTATUS ret;
 PIRP I;
 PIO_STACK_LOCATION nextStack;
 KEVENT ev;
 IO_STATUS_BLOCK ios;

 Vlpt_KdPrint2(("Aufruf CallUSBD\n"));
 ASSERT(!KeGetCurrentIrql());
 KeInitializeEvent(&ev,NotificationEvent,FALSE);
 I=IoBuildDeviceIoControlRequest(			//IRQL=0
   IOCTL_INTERNAL_USB_SUBMIT_URB,X->ldo,NULL,0,NULL,0,TRUE,&ev,&ios);
 ASSERT(I);
 if (!I) return STATUS_INSUFFICIENT_RESOURCES;
 nextStack=IoGetNextIrpStackLocation(I);
 ASSERT(nextStack);
 nextStack->Parameters.Others.Argument1=U;
 ret=IoCallDriver(X->ldo,I);
 Vlpt_KdPrint2(("IoCallDriver(USBD) returns %x\n",ret));
 if (ret==STATUS_PENDING) {
  KeWaitForSingleObject(&ev,Suspended,KernelMode,FALSE,NULL);
  ret=ios.Status;					//IRQL=0 sonst Crash!
 }
#if DBG
 if (U->UrbHeader.Status || ret)	// im Fehlerfall zucken!
   Vlpt_KdPrint(("URB status %X, IRP status %X\n",U->UrbHeader.Status,ret));
#endif
 SetUsbStatus(X,&ret,U);
 return ret;
}

NTSTATUS SelectInterfaces(PDEVICE_EXTENSION X,
//XREF: ConfigureDevice
  IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
  IN PUSBD_INTERFACE_INFORMATION Interface) {
/*
    Initializes an Vlpt Device with multiple interfaces
Arguments:
    fdo            -  this instance of the Vlpt Device
    ConfigurationDescriptor
		- the USB configuration descriptor containing the interface
		  and endpoint descriptors.
    Interface	- pointer to a USBD Interface Information Object
		- If this is NULL, then this driver must choose its interface
		  based on driver-specific criteria, and the driver must also
		  CONFIGURE the device.
		- If it is NOT NULL, then the driver has already been given
		  an interface and the device has already been configured by
		  the parent of this device driver.
Return Value:
    NT status code
*/
 NTSTATUS ntStatus;
 PURB urb;
 ULONG j;
 PUSBD_INTERFACE_INFORMATION interfaceObject;
 USBD_INTERFACE_LIST_ENTRY interfaceList[2];
 LONG AlternateSetting;

 Vlpt_KdPrint2(("enter SelectInterfaces\n"));

	// Search the configuration descriptor for the first interface/alternate setting
 for (AlternateSetting=0; AlternateSetting<2; AlternateSetting++) {
  interfaceList[AlternateSetting].InterfaceDescriptor =
   USBD_ParseConfigurationDescriptorEx(ConfigurationDescriptor,
   ConfigurationDescriptor,
   -1,		// Interface - don't care
   AlternateSetting,
   -1,		// Class - don't care
   -1,		// SubClass - don't care
   -1);		// Protocol - don't care
 }

 ASSERT(interfaceList[0].InterfaceDescriptor);
 if (interfaceList[1].InterfaceDescriptor) {	// Low-Speed-Gerät mit 2 Alternate Settings
// Bis Februar 2012 hatten Low-Speed-Geräte zuerst BULK, dann INTERRUPT als Alternate Settings.
// Damit lädt sich aber der Treiber "usbccgp.sys" (USB-Verbundgerät) nicht unter Windows Vista und neuer.
// Seit der Einführung des HID-Interfaces Dezember 2010. Oops!
// Hingegen beschwert sich "usbccgp.sys" nicht bei der Reihenfolge INTERRUPT, dann BULK.
// Ergo: Neue Firmware hat diese neue Reihenfolge.
// Sonst lässt sie sich schlichtweg unter Vista nicht nutzen, wie man sich auch auf den Kopf stellt.
// Da sich der Treiber per INF-Datei bisher auch auf das ganze Gerät aufbinden ließ (tatsächlich!),
// konnte der Anwender diesen Treiber auch auf das Verbundgerät als Ganzes loslassen:
// * Treiber lief nicht (wie soll er auch?)
// * Schutzverletzung BAD_POOL_CALLER(C2):ALREADY_FREED(7) beim Abziehen irgendwo.
// Da nun zwei Reihenfolgen im Umlauf sind, muss hier die richtige Alternate Setting herausgepickt werden:
// * BULK unter XP und früher für maximale Performance,
// * INTERRUPT unter Vista und neuer, damit es überhaupt funktioniert.

  UCHAR WantAttr = IoIsWdmVersionAvailable(6,0) ? USB_ENDPOINT_TYPE_INTERRUPT/*3*/ : USB_ENDPOINT_TYPE_BULK/*2*/;

// Endpoint-Deskriptor folgt direkt bei allen USB2LPT (es gibt keinen sonstigen Deskriptor)
// Ansonsten müsste man sich per bLength von Deskriptor zu Deskriptor durchhangeln
  PUSB_ENDPOINT_DESCRIPTOR ep=(PUSB_ENDPOINT_DESCRIPTOR)&interfaceList[1].InterfaceDescriptor[1];
  ASSERT(ep->bDescriptorType==USB_ENDPOINT_DESCRIPTOR_TYPE);
  if ((ep->bmAttributes&3)==WantAttr)	// dann diese Einstellung nehmen
    interfaceList[0].InterfaceDescriptor=interfaceList[1].InterfaceDescriptor;
  Vlpt_KdPrint(("USB2LPT Low-Speed selecting AltSetting #%d (%s endpoints)\n",
    (ep->bmAttributes&3)==WantAttr,
    WantAttr==USB_ENDPOINT_TYPE_INTERRUPT ? "INTERRUPT" : "BULK"));
 }

 interfaceList[1].InterfaceDescriptor=NULL;
 interfaceList[1].Interface=NULL;

 urb=USBD_CreateConfigurationRequestEx(ConfigurationDescriptor,&interfaceList[0]);

 if (!urb) Vlpt_KdPrint((" USBD_CreateConfigurationRequestEx failed\n"));
// DumpBuffer(urb, urb->UrbHeader.Length);

 interfaceObject=(PUSBD_INTERFACE_INFORMATION) (&(urb->UrbSelectConfiguration.Interface));
   // We set up a default max transfer size for the endpoints.  Your driver will
   // need to change this to reflect the capabilities of your device's endpoints.
 for (j=0; j<interfaceList[0].InterfaceDescriptor->bNumEndpoints; j++)
   interfaceObject->Pipes[j].MaximumTransferSize = (64 * 1024) - 1;


 ntStatus=CallUSBD(X, urb);
// DumpBuffer(urb, urb->UrbHeader.Length);

 if (NT_SUCCESS(ntStatus) && USBD_SUCCESS(urb->UrbHeader.Status)) {
      // Save the configuration handle for this device
//  X->ConfigurationHandle=urb->UrbSelectConfiguration.ConfigurationHandle;
  X->Interface=ExAllocatePoolWithTag(NonPagedPool,interfaceObject->Length,'tplV');
      // save a copy of the interfaceObject information returned
  RtlCopyMemory(X->Interface,interfaceObject,interfaceObject->Length);
      // Dump the interfaceObject to the debugger
  Vlpt_KdPrint (("---------\n"));
  Vlpt_KdPrint (("NumberOfPipes    %d\n",  X->Interface->NumberOfPipes));
  Vlpt_KdPrint (("Length           %d\n",  X->Interface->Length));
  Vlpt_KdPrint (("Alt Setting      0x%x\n",X->Interface->AlternateSetting));
  Vlpt_KdPrint (("Interface Number 0x%x\n",X->Interface->InterfaceNumber));

      // Dump the pipe info
  for (j=0; j<interfaceObject->NumberOfPipes; j++) {
   PUSBD_PIPE_INFORMATION i;
   i=&X->Interface->Pipes[j];
   Vlpt_KdPrint (("---------\n"));
   Vlpt_KdPrint (("PipeType        0x%x\n",i->PipeType));
   Vlpt_KdPrint (("EndpointAddress 0x%x\n",i->EndpointAddress));
   Vlpt_KdPrint (("MaxPacketSize   %d\n",  i->MaximumPacketSize));
   Vlpt_KdPrint (("Interval        %d\n",  i->Interval));
   Vlpt_KdPrint (("Handle          0x%x\n",i->PipeHandle));
   Vlpt_KdPrint (("MaxTransferSize %d\n",  i->MaximumTransferSize));
  }
  Vlpt_KdPrint (("---------\n"));
 }

 Vlpt_KdPrint2(("leave SelectInterfaces (%x)\n",ntStatus));
 return ntStatus;
}


NTSTATUS ConfigureDevice(PDEVICE_EXTENSION X) {
//XREF: StartDevice
/*
Routine Description:
   Configures the USB device via USB-specific device requests and interaction
   with the USB software subsystem.

Arguments:
   fdo - pointer to the device object for this instance of the Vlpt Device

Return Value:
   NT status code
*/
 NTSTATUS ntStatus=STATUS_NO_MEMORY;
// PURB urb=NULL;
 struct _URB_CONTROL_DESCRIPTOR_REQUEST urb;
 PUSB_CONFIGURATION_DESCRIPTOR configurationDescriptor=NULL;
 ULONG siz;

 Vlpt_KdPrint2(("enter ConfigureDevice\n"));

   // Get memory for the USB Request Block (urb).
// if (!Alloc(&urb,sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST))) goto NoMemory;
      //
      // Set size of the data buffer.  Note we add padding to cover hardware faults
      // that may cause the device to go past the end of the data buffer
      //
 siz=sizeof(USB_CONFIGURATION_DESCRIPTOR)+16;
        // Get the nonpaged pool memory for the data buffer
 if (!(configurationDescriptor=ExAllocatePoolWithTag(NonPagedPool,siz,'tplV'))) goto NoMemory;
 UsbBuildGetDescriptorRequest((PURB)&urb,
   (USHORT)sizeof(urb),
   USB_CONFIGURATION_DESCRIPTOR_TYPE,0,0,configurationDescriptor,
   NULL,sizeof(USB_CONFIGURATION_DESCRIPTOR),/* Get only the configuration descriptor */
   NULL);

 ntStatus=CallUSBD(X,(PURB)&urb);

 if (NT_SUCCESS(ntStatus)) {
  Vlpt_KdPrint2(("Configuration Descriptor is at %x, bytes txferred: %d\n"
    "Configuration Descriptor Actual Length: %d\n",
    configurationDescriptor,
    urb./*UrbControlDescriptorRequest.*/TransferBufferLength,
    configurationDescriptor->wTotalLength));
 }//if
 siz=configurationDescriptor->wTotalLength+16;	// Größe merken
 ExFreePoolWithTag(configurationDescriptor,'tplV');		// bisherigen Deskriptor verwerfen
 ntStatus=STATUS_NO_MEMORY;
 if (!(configurationDescriptor=ExAllocatePoolWithTag(NonPagedPool,siz,'tplV'))) goto NoMemory;	// neu allozieren
 UsbBuildGetDescriptorRequest((PURB)&urb,
   (USHORT)sizeof(urb),
   USB_CONFIGURATION_DESCRIPTOR_TYPE,0,0,configurationDescriptor,
   NULL,siz,  /* Get all the descriptor data*/ NULL);
 ntStatus=CallUSBD(X,(PURB)&urb);

 if (NT_SUCCESS(ntStatus)) {
  Vlpt_KdPrint2(("Entire Configuration Descriptor is at %x, bytes txferred: %d\n",
    configurationDescriptor,
    urb./*UrbControlDescriptorRequest.*/TransferBufferLength));
// We have the configuration descriptor for the configuration
// we want.
// Now we issue the SelectConfiguration command to get
// the  pipes associated with this configuration.
  ntStatus=SelectInterfaces(X,configurationDescriptor,NULL); // NULL=Device not yet configured
 }
 ExFreePoolWithTag(configurationDescriptor,'tplV');
NoMemory:
// Free(&urb);
 Vlpt_KdPrint2(("leave ConfigureDevice (%x)\n", ntStatus));
 return ntStatus;
}

/***************************
 ** Aktivierung der Traps **
 ***************************/

void FreeTraps(PDEVICE_EXTENSION X) {
 if (X->instance<3) {
#ifdef NTDDK					// Patch wegen WGif12NT.exe, mapmem.sys auch unter NT erforderlich
  PHYSICAL_ADDRESS a={0x408};
  USHORT*p=MmMapIoSpace(a,10,TRUE);		// sollte 0x80000408 liefern. Keine Ahnung bei amd64 oder gar ia64 …
  if (p) {
   p[X->instance]=X->oldlpt;
   p[4]=X->oldsys;
   MmUnmapIoSpace(p,10);
  }
#else
  USHORT*p=(USHORT*)0x408;
  p[X->instance]=X->oldlpt;	// Patch bei 9x
  p[4]=X->oldsys;
#endif
 }
 if (X->ac.trapped&1) FreeDR(X->uc.LptBase);
 if (X->ac.trapped&2) FreeDR((USHORT)(X->uc.LptBase+4));
 if (X->ac.trapped&4) FreeDR((USHORT)(X->uc.LptBase+0x400));
 if (X->ac.trapped&8) FreeDR((USHORT)(X->uc.LptBase+0x404));
 X->ac.trapped=0;
 X->mirror[0]=0;		// Spiegel löschen
 X->f|=No_Function;		// READ_PORT_UCHAR-Umleitung sperren
}

// Kapsel für AllocDR() aus vlpt.c
static void allocdr(PDEVICE_EXTENSION X, UCHAR m, USHORT o) {
 char e=AllocDR((USHORT)(X->uc.LptBase+o),X,X->uc.flags);
 if (e>=0) X->ac.trapped|=m;
 else Vlpt_KdPrint(("Kann LptBase+%Xh nicht anzapfen (%d)\n",o,e));
}

void SetTraps(PDEVICE_EXTENSION X) {
 if (X->uc.flags&UC_Function) X->f&=~No_Function;
// Debugregister-Anzapfung setzen
 if (X->instance<3) {
#ifdef NTDDK
  PHYSICAL_ADDRESS a={0x408};
  USHORT*p=MmMapIoSpace(a,10,TRUE);
  if (p) {
#else
  USHORT*p=(USHORT*)0x408;	// Patch bei 9x
#endif
   X->oldlpt=p[X->instance];
   p[X->instance]=X->uc.LptBase;
   X->oldsys=p[4];
   if (X->oldsys>>14 <= X->instance) {
    p[4]=X->oldsys&0x3FFF | ((X->instance+1)<<14);
   }
#ifdef NTDDK
   MmUnmapIoSpace(p,10);
  }
#endif
 }
 allocdr(X,1,0);			// SPP (immer)
 if (X->uc.Mode&1) allocdr(X,2,4);	// EPP
 if (X->uc.Mode&2) allocdr(X,4,0x400);	// ECP
 if (X->uc.Mode&4) allocdr(X,8,0x404);	// mit Zusatzregister (undokumentierter Trap)
}

NTSTATUS StartDevice(PDEVICE_EXTENSION X) {
//XREF: HandleStartDevice
/* Initializes a given instance of the Vlpt Device on the USB.
   fdo - pointer to the device object for this instance of a Vlpt Device*/
 NTSTATUS ntStatus;
 PUSB_DEVICE_DESCRIPTOR DDesc;
// PURB urb;
 struct _URB_CONTROL_DESCRIPTOR_REQUEST U;
// const ULONG siz=sizeof(USB_DEVICE_DESCRIPTOR);

 Vlpt_KdPrint (("enter StartDevice\n"));

    // Get some memory from then non paged pool (fixed, locked system memory)
    // for use by the USB Request Block (urb) for the specific USB Request we
    // will be performing below (a USB device request).
// if (Alloc(&urb,sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST))) {
        // Get some non paged memory for the device descriptor contents
  if ((DDesc=ExAllocatePoolWithTag(NonPagedPool,sizeof(*DDesc),'tplV'))) {
            // Use a macro in the standard USB header files to build the URB
   UsbBuildGetDescriptorRequest((PURB)&U,sizeof(U),
     USB_DEVICE_DESCRIPTOR_TYPE,0,0,DDesc,NULL,sizeof(*DDesc),NULL);
            // Get the device descriptor
   ntStatus=CallUSBD(X,(PURB)&U);
            // Dump out the descriptor info to the debugger
   if (NT_SUCCESS(ntStatus)) {
    Vlpt_KdPrint((
      "Device Descriptor = %x, len %x\n",
      "Vlpt Device Descriptor:\n"
      "-------------------------\n"
      "bLength\t%d\n"
      "bDescriptorType\t0x%x\n"
      "bcdUSB\t%#x\n"
      "bDeviceClass\t%#x\n"
      "bDeviceSubClass\t%#x\n"
      "bDeviceProtocol\t%#x\n"
      "bMaxPacketSize0\t%#x\n"
      "idVendor\t%#x\n"
      "idProduct\t%#x\n"
      "bcdDevice\t%#x\n"
      "iManufacturer\t%#x\n"
      "iProduct\t%#x\n"
      "iSerialNumber\t%#x\n"
      "bNumConfigurations\t%#x\n",
      DDesc,U.TransferBufferLength,
      DDesc->bLength,
      DDesc->bDescriptorType,
      DDesc->bcdUSB,
      DDesc->bDeviceClass,
      DDesc->bDeviceSubClass,
      DDesc->bDeviceProtocol,
      DDesc->bMaxPacketSize0,
      DDesc->idVendor,
      DDesc->idProduct,
      DDesc->bcdDevice,
      DDesc->iManufacturer,
      DDesc->iProduct,
      DDesc->iSerialNumber,
      DDesc->bNumConfigurations));
   }
  }else ntStatus = STATUS_NO_MEMORY;
  if (NT_SUCCESS(ntStatus)) {
            // Put a ptr to the device descriptor in the device extension for easy
            // access (like a "cached" copy).  We will free this memory when the
            // device is removed.  See the "RemoveDevice" code.
   X->DeviceDescriptor = DDesc;
   X->f&=~Stopped;
  }else if (DDesc) {
            /*
            // If the bus transaction failed, then free up the memory created to hold
            // the device descriptor, since the device is probably non-functional
            */
   ExFreePoolWithTag(DDesc,'tplV');
   X->DeviceDescriptor=NULL;
  }
//  ExFreePool(urb);
// }else ntStatus = STATUS_NO_MEMORY;
    // If the Get_Descriptor call was successful, then configure the device.
 if (NT_SUCCESS(ntStatus)) ntStatus = ConfigureDevice(X);
 if (NT_SUCCESS(ntStatus)) {
  PUSBD_INTERFACE_INFORMATION	ii;
  PUSBD_PIPE_INFORMATION	pi;
  ii=X->Interface;
  if (!ii) goto ex;
  if (ii->NumberOfPipes<2) goto ex;
  pi=ii->Pipes;		// Erste Pipe = Schreiben (s.a. Firmware)
  if (pi->PipeType!=UsbdPipeTypeBulk /*=2*/
  && pi->PipeType!=UsbdPipeTypeInterrupt /*=3*/) goto ex;
  if (pi->MaximumTransferSize<64) goto ex;
  if (!(pi->PipeHandle)) goto ex;
  pi=ii->Pipes+1;		// Zweite Pipe = Lesen
  if (pi->PipeType!=UsbdPipeTypeBulk /*=2*/
  && pi->PipeType!=UsbdPipeTypeInterrupt /*=3*/) goto ex;
  if (pi->MaximumTransferSize<64) goto ex;
  if (!(pi->PipeHandle)) goto ex;
  KeInitializeMutex(&X->bmutex,0);
  KeInitializeTimer(&X->wrcache.tmr);
  KeInitializeDpc(&X->wrcache.dpc,TimerDpc,X);
  KeInitializeEvent(&X->ev,NotificationEvent,TRUE);
//IoInitializeDpcRequest(X->fdo,...)
  if (X->uc.LptBase&0xFF00) SetTraps(X);
ex:;
 }
#ifdef NTDDK
 {
  NTSTATUS InitWmi(PDEVICE_EXTENSION);
  InitWmi(X);
 }
#endif
 Vlpt_KdPrint (("leave StartDevice (%x)\n", ntStatus));
 return ntStatus;
}

NTSTATUS Quatsch(PUNICODE_STRING us,PCSZ str) {
// Unicode-String aus ASCIIZ initialisieren
// Gefüllter Unicode-String <us> muss mit RtlFreeUnicodeString() freigegeben werden!
 ANSI_STRING as;
 RtlInitAnsiString(&as,str);	// svw. Länge durchzählen
 return RtlAnsiStringToUnicodeString(us,&as,TRUE);
}

NTSTATUS HandleStartDevice(PDEVICE_EXTENSION X, PIRP I) {
//XREF: DispatchPnp
 NTSTATUS ntStatus;
 HANDLE key;
 ntStatus=ForwardAndWait(X,I);	// Zuerst niedere Treiber starten lassen
 if (!NT_SUCCESS(IoSetDeviceInterfaceState(&X->ifname,TRUE))) {
  Vlpt_KdPrint (("IoSetDeviceInterfaceState versagt!\n"));
  TRAP();
 }
 if (!NT_SUCCESS(IoOpenDeviceRegistryKey(X->pdo,PLUGPLAY_REGKEY_DEVICE,
   KEY_QUERY_VALUE,&key))) {
  Vlpt_KdPrint (("IoOpenDeviceRegistryKey versagt!\n"));
  TRAP();
 }else{
// Benutzerdefinierte (binäre) Konfigurationsdaten ("UserCfg") laden
  UNICODE_STRING us;
  struct {
   KEY_VALUE_PARTIAL_INFORMATION v;	// 3 DWORDs = 12 Bytes
   UCHAR data[sizeof(TUserCfg)-1];	// insgesamt 6 Bytes Daten, Summe 18
  }val;
  ULONG needed;
  static const CHAR Tag[]="UserCfg";
  if (NT_SUCCESS(Quatsch(&us,Tag))) {
   if (NT_SUCCESS(ZwQueryValueKey(key,&us,KeyValuePartialInformation,
     &val.v,sizeof(val),&needed))
   && val.v.Type==REG_BINARY) {
    X->uc=*(PUserCfg)val.v.Data;		// Persistente Daten kopieren
   }
   RtlFreeUnicodeString(&us);
  }
  ZwClose(key);
 }
 if (!NT_SUCCESS(ntStatus)) return CompleteRequest(I,ntStatus,I->IoStatus.Information);
  // now do whatever we need to do to start the device
 return CompleteRequest(I,StartDevice(X),0);
}

NTSTATUS StopDevice(PDEVICE_EXTENSION X) {
//XREF: DispatchPnp
 NTSTATUS ret=STATUS_SUCCESS;
 PURB U;

 Vlpt_KdPrint2(("enter StopDevice\n"));
 KeCancelTimer(&X->wrcache.tmr);
 FreeTraps(X);
 X->f|=Stopped;
#define USIZE sizeof(struct _URB_SELECT_CONFIGURATION)
 U=ExAllocatePoolWithTag(NonPagedPool,USIZE,'tplU');
 if (U) {
  NTSTATUS status;
  UsbBuildSelectConfigurationRequest(U,USIZE,NULL);	// Auf "unkonfiguriert"
  status=CallUSBD(X,U);
  Vlpt_KdPrint(("Device Configuration Closed status = %x usb status = %x.\n",
    status, U->UrbHeader.Status));
  ExFreePoolWithTag(U,'tplU');
#undef USIZE
 }else ret=STATUS_NO_MEMORY;
 Vlpt_KdPrint2(("leave StopDevice (%x)\n",ret));
 return ret;
}

NTSTATUS AbortPipe(PDEVICE_EXTENSION X,IN USBD_PIPE_HANDLE PipeHandle) {
//XREF: HandleRemoveDevice, ProcessIOCTL
/*   cancel pending transfers for a pipe */
 NTSTATUS ntStatus;
 PURB U;

 Vlpt_KdPrint2(("USB2LPT.SYS: AbortPipe \n"));
#define USIZE sizeof(struct _URB_PIPE_REQUEST)
 U=ExAllocatePoolWithTag(NonPagedPool,USIZE,'tplU');
 if (U) {
  RtlZeroMemory(U,USIZE);
  U->UrbHeader.Length=(USHORT)USIZE;
  U->UrbHeader.Function=URB_FUNCTION_ABORT_PIPE;
  U->UrbPipeRequest.PipeHandle=PipeHandle;
  ntStatus=CallUSBD(X,U);
  ExFreePoolWithTag(U,'tplU');
#undef USIZE
 }else ntStatus=STATUS_INSUFFICIENT_RESOURCES;	// Wieso diesmal Resources??
 return ntStatus;
}


NTSTATUS DevName(PUNICODE_STRING us,int Instance) {
// liefert "durchnummerierten" Device-String
 static const CHAR deviceName[]="\\Device\\Parallel????";
 NTSTATUS ret;
 ret=Quatsch(us,deviceName);
 if (NT_SUCCESS(ret)) us->Length=(USHORT)((sizeof(deviceName)-5+
   _snwprintf(us->Buffer+sizeof(deviceName)-5,5,L"%d",Instance))*sizeof(WCHAR));
 return ret;
}

NTSTATUS DevLink(PUNICODE_STRING us,int Instance) {
// liefert "durchnummerierten" DosDevices-String
 static const CHAR deviceLink[]="\\DosDevices\\LPT????";
 NTSTATUS ret;
 ret=Quatsch(us,deviceLink);
 if (NT_SUCCESS(ret)) us->Length=(USHORT)((sizeof(deviceLink)-5+
   _snwprintf(us->Buffer+sizeof(deviceLink)-5,5,L"%d",Instance+1))*sizeof(WCHAR));
 return ret;
}

NTSTATUS RemoveDevice(PDEVICE_EXTENSION X) {
//XREF: HandleRemoveDevice
 UNICODE_STRING ucDevLink,ucDevName;

 Vlpt_KdPrint(("RemoveDevice\n"));
 if (X->DeviceDescriptor) ExFreePoolWithTag(X->DeviceDescriptor,'tplV');
   // Free up any interface structures in our device extension
 if (X->Interface) ExFreePoolWithTag(X->Interface,'tplV');
   // remove the device object's symbolic link
 if (NT_SUCCESS(DevLink(&ucDevLink,X->instance))) {
  IoDeleteSymbolicLink(&ucDevLink);
  RtlFreeUnicodeString(&ucDevLink);
 }
 if (NT_SUCCESS(DevName(&ucDevName,X->instance))) {
  RtlDeleteRegistryValue(RTL_REGISTRY_DEVICEMAP,L"PARALLEL PORTS",ucDevName.Buffer);
  RtlFreeUnicodeString(&ucDevName);
 }
 RtlFreeUnicodeString(&X->ifname);
 IoDetachDevice(X->ldo);
 IoDeleteDevice(X->fdo);
 return STATUS_SUCCESS;
}


NTSTATUS HandleRemoveDevice(PDEVICE_EXTENSION X, PIRP I) {
//XREF: DispatchPnp
 PUSBD_INTERFACE_INFORMATION ii=X->Interface;
 ULONG i;
 HANDLE key;
   // set the removing flag to prevent any new I/O's
 X->f|=removing;
 IoSetDeviceInterfaceState(&X->ifname,FALSE);
   // brute force - send an abort pipe message to all pipes to cancel any
   // pending transfers.  this should solve the problem of the driver blocking
   // on a REMOVE message because there is a pending transfer.
 if (!NT_SUCCESS(IoOpenDeviceRegistryKey(X->pdo,PLUGPLAY_REGKEY_DEVICE,
   KEY_WRITE,&key))) {
  Vlpt_KdPrint (("IoOpenDeviceRegistryKey versagt!\n"));
  TRAP();
 }else{
// Aktuelle Benutzer-Konfiguration retten!
  UNICODE_STRING us;
  static const CHAR Tag[]="UserCfg";
  if (NT_SUCCESS(Quatsch(&us,Tag))) {
   ZwSetValueKey(key,&us,0,REG_BINARY,&X->uc,sizeof(X->uc));
   RtlFreeUnicodeString(&us);
  }
  ZwClose(key);
 }
 if (ii) for (i=0; i<ii->NumberOfPipes; i++) {
  AbortPipe(X,(USBD_PIPE_HANDLE)ii->Pipes[i].PipeHandle);
 }
#ifdef NTDDK
 IoWMIRegistrationControl(X->fdo,WMIREG_ACTION_DEREGISTER);
#endif
 IoReleaseRemoveLockAndWait(&X->rlock,I);
 RemoveDevice(X);
 return DefaultPnpHandler(X,I);	// lower-level completed IoStatus already
}

#if DBG
const char* LocateStr(const char*l,int n) {
// Findet in Multi-SZ-String <l> den String mit Index <n>
// und liefert Zeiger darauf. Ist n zu groß, Zeiger auf letzte Null
 for(;;n--){
  if (!n || !*l) return l;	// gefunden oder Liste zu Ende
  while (*++l);			// kein strlen zu finden!!
  l++;				// Hinter die Null
 }
}
#endif

NTSTATUS DispatchPnp(IN PDEVICE_OBJECT fdo,IN PIRP I) {
 PIO_STACK_LOCATION irpStack;
 PDEVICE_EXTENSION X=fdo->DeviceExtension;
 ULONG fcn;
 NTSTATUS ret;
#if DBG
 static const char MsgNames[]=
   "START_DEVICE\0"
   "QUERY_REMOVE_DEVICE\0"
   "REMOVE_DEVICE\0"
   "CANCEL_REMOVE_DEVICE\0"
   "STOP_DEVICE\0"
   "QUERY_STOP_DEVICE\0"
   "CANCEL_STOP_DEVICE\0"
   "QUERY_DEVICE_RELATIONS\0"
   "QUERY_INTERFACE\0"
   "QUERY_CAPABILITIES\0"
   "QUERY_RESOURCES\0"
   "QUERY_RESOURCE_REQUIREMENTS\0"
   "QUERY_DEVICE_TEXT\0"
   "FILTER_RESOURCE_REQUIREMENTS\0"
   "?\0"
   "READ_CONFIG\0"
   "WRITE_CONFIG\0"
   "EJECT\0"
   "SET_LOCK\0"
   "QUERY_ID\0"
   "QUERY_PNP_DEVICE_STATE\0"
   "QUERY_BUS_INFORMATION\0"
   "DEVICE_USAGE_NOTIFICATION\0"
   "SURPRISE_REMOVAL\0";
#endif

 ret=IoAcquireRemoveLock(&X->rlock,I);
 if (!NT_SUCCESS(ret)) return CompleteRequest(I,ret,0);
   // Get a pointer to the current location in the Irp. This is where
   //     the function codes and parameters are located.
 irpStack=IoGetCurrentIrpStackLocation(I);

 ASSERT(irpStack->MajorFunction==IRP_MJ_PNP);
 fcn=irpStack->MinorFunction;
 Vlpt_KdPrint(("IRP_MJ_PNP:IRP_MN_%s\n",LocateStr(MsgNames,fcn)));

 switch (fcn) {
  case IRP_MN_START_DEVICE: {
   ret=HandleStartDevice(X,I);
   if (NT_SUCCESS(ret)) X->f|=Started;
  }break;

  case IRP_MN_STOP_DEVICE: {
   DefaultPnpHandler(X,I);
   ret=StopDevice(X);
  }break;

  case IRP_MN_SURPRISE_REMOVAL: {
   if (!(X->f&Stopped))  KeCancelTimer(&X->wrcache.tmr);
   FreeTraps(X);
   X->f|=surprise;
  }goto def;

  case IRP_MN_REMOVE_DEVICE: {
   if (!(X->f&surprise)) {		// unter NT kommen beide Nachrichten
    if (!(X->f&Stopped))  KeCancelTimer(&X->wrcache.tmr);
    FreeTraps(X);
   }
  }return HandleRemoveDevice(X,I);	// ohne IoReleaseRemoveLock()

  case IRP_MN_QUERY_DEVICE_TEXT: {	// 111022: Löst das das LPT-Nummern-Anzeigeproblem ab XP? Nee.
   PWSTR info=NULL;
   if (irpStack->Parameters.QueryDeviceText.DeviceTextType == DeviceTextLocationInformation) {
    info=ExAllocatePool(PagedPool,8*sizeof(WCHAR));
    if (info) {
     memcpy(info,X->portName,8*sizeof(WCHAR));
     Vlpt_KdPrint2(("QueryDeviceText(DeviceTextLocationInformation) -> %ws\n",info));
    }else{
     Vlpt_KdPrint(("Kein RAM!\n"));
     ret=STATUS_NO_MEMORY;
    }
   }else{
    Vlpt_KdPrint(("QueryDeviceText(%d) -> unbeantwortet\n",irpStack->Parameters.QueryDeviceText.DeviceTextType));
    goto def;
   }
   ret=CompleteRequest(I,ret,(ULONG_PTR)info);
  }break;

  case IRP_MN_QUERY_CAPABILITIES: {
	// This code swiped from Walter Oney.  Please buy his book!!
   PDEVICE_CAPABILITIES pdc=irpStack->Parameters.DeviceCapabilities.Capabilities;
   if (pdc->Version<1) goto def;
   ret=ForwardAndWait(X,I);
   if (NT_SUCCESS(ret)) {				// IRP succeeded
    pdc=irpStack->Parameters.DeviceCapabilities.Capabilities;
	// setting this field prevents NT5 from notifying the user
	// when the device is removed.
    pdc->SurpriseRemovalOK = TRUE;
   }						// IRP succeeded
   ret=CompleteRequest(I,ret,I->IoStatus.Information);
  }break;
def:
  default: ret=DefaultPnpHandler(X,I);
 }
 IoReleaseRemoveLock(&X->rlock,I);
 return ret;
}


NTSTATUS DispatchPower(IN PDEVICE_OBJECT fdo,IN PIRP I) {
 PDEVICE_EXTENSION X=fdo->DeviceExtension;
 NTSTATUS ret;
 Vlpt_KdPrint (("Vlpt_DispatchPower\n"));
 ret=IoAcquireRemoveLock(&X->rlock,I);
 if (!NT_SUCCESS(ret)) {
  return CompleteRequest(I,ret,0);
 }
// IoSkipCurrentIrpStackLocation(I);	// Geht nicht! Warum?
// IoCopyCurrentIrpStackLocationToNext(I);
 if (X->f&surprise) ret=CompleteRequest(I,STATUS_DELETE_PENDING,0);	//111022
 else{
  PoStartNextPowerIrp(I);
  IoSkipCurrentIrpStackLocation(I);	//111022
  ret=PoCallDriver(X->ldo,I);
  ASSERT(NT_SUCCESS(ret));
 //if (ntStatus==STATUS_PENDING) IoMarkIrpPending(I);
 }
 IoReleaseRemoveLock(&X->rlock,I);
 return ret;
}


NTSTATUS CreateDeviceObject(IN PDRIVER_OBJECT DriverObject,
  OUT PDEVICE_OBJECT *fdo,LONG Instance) {
/*  Creates a Functional DeviceObject
Arguments:
    Instance - Geräte-Nummer, null-basiert
Return Value:
    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise
*/

 NTSTATUS ntStatus;
 UNICODE_STRING ucDevName, ucDevLink;

 if (NT_SUCCESS(ntStatus=DevName(&ucDevName,Instance))) {
  Vlpt_KdPrint(("Gerätename: <%ws>\n", ucDevName.Buffer));
  ntStatus=IoCreateDevice(DriverObject,sizeof(DEVICE_EXTENSION),
    &ucDevName,FILE_DEVICE_PARALLEL_PORT,
#ifdef NTDDK    
    FILE_DEVICE_SECURE_OPEN,
#else
    0,
#endif
    FALSE/*TRUE im Parallelport-Beispiel!*/,fdo);
  if (NT_SUCCESS(ntStatus)) {
   if (NT_SUCCESS(DevLink(&ucDevLink,Instance))) {
    Vlpt_KdPrint(("DOS-Gerätename: <%ws>\n",ucDevLink.Buffer));
    IoCreateUnprotectedSymbolicLink(&ucDevLink,&ucDevName);
    RtlWriteRegistryValue(RTL_REGISTRY_DEVICEMAP,L"PARALLEL PORTS",
      ucDevName.Buffer,REG_SZ,ucDevLink.Buffer,ucDevLink.Length+sizeof(WCHAR));
    RtlFreeUnicodeString(&ucDevLink);
   }
  }
  RtlFreeUnicodeString(&ucDevName);
 }
 return ntStatus;
}

int GetPortIndex(PDEVICE_OBJECT pdo) {
// Holt den vom System oder Anwender zugewiesenen Port-Index (0-basiert)
// negativ (-1, -2) bei Fehler (bspw. nicht zugewiesen)
 HANDLE k;
 int ret=-1;
 if (NT_SUCCESS(IoOpenDeviceRegistryKey(pdo,
      PLUGPLAY_REGKEY_DEVICE,KEY_QUERY_VALUE,&k))) {
  UNICODE_STRING us;
  struct {
   KEY_VALUE_PARTIAL_INFORMATION v;
   UCHAR data[16-1];	// maximal L"LPTxxxx" = 16 Bytes Daten
  }val;
// Bereits beim allerersten Treiber-Aufruf ist PortName gesetzt, und zwar auf LPT3.
// Da das unerwünscht ist, liefert diese Funktion zunächst -1, wenn kein UserCfg
// gesetzt ist, damit der Treiber mit LPT1 anfängt.
// Wahrscheinlich ist es besser, diese Aufgabe vom CoInstaller erledigen zu lassen,
// aber wie und bei welcher Nachricht?
// Außerdem gibt's keinen CoInstaller unter Win98/Me. (Wie ist das dort gelöst?)
  ULONG needed;
  if (NT_SUCCESS(Quatsch(&us,"UserCfg"))) {
   NTSTATUS e=ZwQueryValueKey(k,&us,KeyValuePartialInformation,
     &val.v,sizeof(val),&needed);
   RtlFreeUnicodeString(&us);
   if (NT_SUCCESS(e)
   && NT_SUCCESS(Quatsch(&us,"PortName"))) {
    if (NT_SUCCESS(ZwQueryValueKey(k,&us,KeyValuePartialInformation,
      &val.v,sizeof(val),&needed))) {
     wchar_t *p=(wchar_t*)val.v.Data;
     if (p[0]=='L' && p[1]=='P' && p[2]=='T') {	// müsste "LPT%u" sein
      UNICODE_STRING us;
      RtlInitUnicodeString(&us,p+3);
      RtlUnicodeStringToInteger(&us,10,(PULONG)&ret);	// bei Fehler sollte ret = -1 bleiben, oder?
      ret--;
     }
    }
    RtlFreeUnicodeString(&us);
   }
  }
 }
 return ret;
}

void SetPortName(PDEVICE_EXTENSION X) {
// Setzt "FriendlyName" und "PortName" - das genügt PonyProg2000 für W98
// Eigentlich müssen noch Ressourcen angemeldet werden!
 HANDLE k;
 if (NT_SUCCESS(IoOpenDeviceRegistryKey(X->pdo,
   PLUGPLAY_REGKEY_DEVICE,KEY_SET_VALUE,&k))) {
  UNICODE_STRING us;
  if (NT_SUCCESS(Quatsch(&us,"PortName"))) {
   ZwSetValueKey(k,&us,0,REG_SZ,X->portName,(ULONG)((wcslen(X->portName)+1)*sizeof(WCHAR)));
   RtlFreeUnicodeString(&us);
  }
#ifndef NTDDK
  {ULONG r;
   if (IoGetDeviceProperty(X->pdo,DevicePropertyDeviceDescription,
    0,NULL,&r)==STATUS_BUFFER_TOO_SMALL) {
    PWCHAR buf;
    r+=10*sizeof(WCHAR);	// Platz für " (LPTxxxx)"
    buf=ExAllocatePoolWithTag(PagedPool,r,'TPLV');
    if (buf) {
     if (NT_SUCCESS(IoGetDeviceProperty(X->pdo,
       DevicePropertyDeviceDescription,r,buf,&r))) {
//W98: (keine Device-Parameter!)
//W2K: CCS/Enum/USB/Vid_5348&Pid_2131&MI_00/5&3A843828&1&0/Device Parameters
//W2K: FriendlyName wird (offenbar) ein Verzeichnis hoch kopiert, PortName nicht
//WXP: Nichts funktioniert wie unter W2K! Man müsste ein Verzeichnis hochkommen...
//	Vielleicht mit ZwQueryKey()? Nein! Das muss der CoInstaller im User-Mode tun!
      wcscat(buf,L" (");
      wcscat(buf,X->portName);
      wcscat(buf,L")");
      if (NT_SUCCESS(Quatsch(&us,"FriendlyName"))) {
       ZwSetValueKey(k,&us,0,REG_SZ,buf,(ULONG)((wcslen(buf)+1)*sizeof(WCHAR)));
       RtlFreeUnicodeString(&us);
      }
     }
     ExFreePool(buf);
    }
   }
  }
#endif
  ZwClose(k);
 }
}

#define MAX_VLPT_DEVICES 255	// maximal vierstellig (9998) wäre erlaubt
static const USHORT Ports[]={0x378,0x278,0x3BC};

NTSTATUS AddDevice(IN PDRIVER_OBJECT drv,IN PDEVICE_OBJECT pdo) {
/*  This routine is called to create a new instance of the device
Arguments:
    DriverObject - driver object for this instance of Vlpt
    pdo - a device object created by the bus
*/
 NTSTATUS		ntStatus=-1,j;
 PDEVICE_OBJECT		fdo;
 PDEVICE_EXTENSION	X;
 int instance;
 bool doSetPortName=false;

 Vlpt_KdPrint2(("enter AddDevice\n"));
 instance=GetPortIndex(pdo);
 if (instance>=0) {
  ntStatus=CreateDeviceObject(drv,&fdo,instance);
 }
 if (!NT_SUCCESS(ntStatus)) {
  doSetPortName=true;
  instance=-1;
  do{
   ++instance;
#ifndef NTDDK
   if (instance<3
   && *(PUSHORT)(0x408+instance*2)) ntStatus=-1;	// nur 9x
   else
#endif  
        ntStatus=CreateDeviceObject(drv,&fdo,instance);
  }while (!NT_SUCCESS(ntStatus) && (instance<MAX_VLPT_DEVICES-1));
 }

 if (NT_SUCCESS(ntStatus)) {
  fdo->Flags|=DO_DIRECT_IO|DO_POWER_PAGABLE;
  X=fdo->DeviceExtension;
  RtlZeroMemory(X,sizeof(DEVICE_EXTENSION));
  X->instance=instance;
  X->f=No_Function;	// noch keine READ_PORT_UCHAR-Anzapfung
  X->fdo=fdo;	// Rück-Bezug setzen
  X->pdo=pdo;	// Physikalisches Gerät (Bustreiber) setzen
  _snwprintf(X->portName,8,L"LPT%u",instance+1);	// Name verfügbar machen
  j=IoRegisterDeviceInterface(pdo,&Vlpt_GUID,NULL,&X->ifname);
  if (NT_SUCCESS(j)) {
   Vlpt_KdPrint(("Interface-Name: <%ws>, Ergebnis %d\n",X->ifname.Buffer,j));
  }
  IoInitializeRemoveLock(&X->rlock,0,1,100);
  X->ldo=IoAttachDeviceToDeviceStack(fdo,pdo);	// niederes Gerät ankoppeln
  ASSERT(X->ldo);
  if (X->instance<elemof(Ports)) {
   X->uc.LptBase=Ports[X->instance];	// Vorgaben setzen
   X->uc.TimeOut=200;
#ifdef _X86_
   X->uc.flags=UC_Debugreg|UC_Function|UC_WriteCache	// 110707: Rabiate Vorgaben
     |UC_ReadCache0|UC_ReadCache2|UC_ReadCacheN;
#endif
// 150302: Deburegister-Anzapfung stützt auf EINIGEN Rechnern ab! So dem Rechner COUCHSURF.
// Lag aber an Fehler in IsPentium()
   if (!IoIsWdmVersionAvailable(1,10)) X->uc.flags|=UC_ForceAlloc;
  }
  if (doSetPortName) SetPortName(X);
  fdo->Flags&=~DO_DEVICE_INITIALIZING;
 }
 Vlpt_KdPrint2(("leave AddDevice (%x)\n", ntStatus));
 return ntStatus;
}

/********************************************************
 * Einzelfestlegung und -feststellung der Portanzapfung *
 ********************************************************/

static void setdebreg(PDEVICE_EXTENSION X, UCHAR m, UCHAR d, UCHAR t, USHORT o) {
 o+=X->uc.LptBase;			// Adresse machen
 if (X->ac.trapped&(~t|X->ac.debregs^d)&m) {	// Bei Ausschalten oder Änderung der Debugregister-Zuweisung
  FreeDR(o);
  X->ac.trapped&=~m;
 }
 if ((~X->ac.trapped|X->ac.debregs^d)&t&m) {	// Bei Einschalten oder Änderung der Debugregister-Zuweisung
  if ((UCHAR)AllocDR(o,X,(UCHAR)(d&m?UC_ForceAlloc|UC_Debugreg:UC_ForceAlloc))<4) X->ac.debregs|=m;
  X->ac.trapped|=m;
 }
}

static void setdebregs(PDEVICE_EXTENSION X, UCHAR d, UCHAR t) {
 if (!(d&t&0x80)) return;		// Müssen in beiden Bytes Bit7 gesetzt sein!
 setdebreg(X,1,d,t,0);
 setdebreg(X,2,d,t,4);
 setdebreg(X,4,d,t,0x400);
 setdebreg(X,8,d,t,0x404);
}

static void updatedebreg(PDEVICE_EXTENSION X, UCHAR m, USHORT o) {
 char i;
 if (!(X->ac.trapped&m)) return;	// nichts tun, wenn zz. keine Anzapfung besteht
 i=GetIndexDR((USHORT)(X->uc.LptBase+o));// Wenn Anzapfung besteht, Typ der Anzapfung erfragen
 if (i<0) X->ac.trapped&=~m;		// Wenn Anzapfung nicht mehr besteht, Bit löschen (nachführen)
 if ((UCHAR)i<4) X->ac.debregs|=m;	// Debugregister-Anzapfung anzeigen
}

static void updatedebregs(PDEVICE_EXTENSION X) {
 X->ac.debregs=0x80;	// Gültigkeit für neue Gerätemanager-Eigenschaften anzeigen
 updatedebreg(X,1,0);
 updatedebreg(X,2,4);
 updatedebreg(X,4,0x400);
 updatedebreg(X,8,0x404);
}

NTSTATUS ProcessIOCTL(IN PDEVICE_OBJECT dev,IN PIRP I) {
 PIO_STACK_LOCATION irpStack;
 PVOID iob;
 ULONG il;	// Eingabedatenlänge (OUT-Befehle und IN-Adressen)
 ULONG ol;	// Ausgabedatenlänge (IN-Daten)
 ULONG cc;
 PDEVICE_EXTENSION X=dev->DeviceExtension;
 NTSTATUS ret;
 ULONG rlen=0;	// im Standardfall keine Bytes zurück

 Vlpt_KdPrint2(("IRP_MJ_DEVICE_CONTROL\n"));
 if (X->f&removing) {
  Vlpt_KdPrint((" f&removing..\n"));
  ret=STATUS_DEVICE_REMOVED;
  goto exi;
 }
 ret=IoAcquireRemoveLock(&X->rlock,I);
 if (!NT_SUCCESS(ret)) {
  Vlpt_KdPrint((" Kein RemoveLock!\n"));
  goto exi;
 }

 irpStack=IoGetCurrentIrpStackLocation(I);
 cc=irpStack->Parameters.DeviceIoControl.IoControlCode;
 iob=I->AssociatedIrp.SystemBuffer;
 il=irpStack->Parameters.DeviceIoControl.InputBufferLength;
 ol=irpStack->Parameters.DeviceIoControl.OutputBufferLength;
 ret=STATUS_SUCCESS;

 switch (cc) {
  case IOCTL_VLPT_UserCfg: {
   Vlpt_KdPrint(("IOCTL_VLPT_UserCfg\n"));
   if (il>sizeof(TUserCfg)) il=sizeof(TUserCfg);
   if (il && RtlCompareMemory(&X->uc,iob,il)!=il) {
    Vlpt_KdPrint((" Konfiguration setzen\n"));
    FreeTraps(X);
    RtlCopyMemory(&X->uc,iob,il);	// Konfiguration setzen
    SetTraps(X);
   }
   if (ol>sizeof(TUserCfg)) ol=sizeof(TUserCfg);
   RtlCopyMemory(iob,&X->uc,ol);	// Konfiguration lesen
   rlen=ol;
  }break;

  case IOCTL_VLPT_AccessCnt: {
   Vlpt_KdPrint(("IOCTL_VLPT_AccessCnt\n"));
   if (il>=sizeof(TAccessCnt)) setdebregs(X,((PAccessCnt)iob)->debregs,((PAccessCnt)iob)->trapped);
   if (il>sizeof(TAccessCnt)-4) il=sizeof(TAccessCnt)-4;	// begrenzen; debregs nicht setzen
   if (il) {
    KIRQL o;
    KeRaiseIrql(DISPATCH_LEVEL,&o);	// Kontextwechsel verhindern
    RtlCopyMemory(&X->ac,iob,il);	// Zugriffszähler (null)setzen
    if (il>=4*4) DebRegStolen[0]=X->ac.steal;	// globalen Zähler (null)setzen
    KeLowerIrql(o);
   }
   if (ol>sizeof(TAccessCnt)) ol=sizeof(TAccessCnt);	// begrenzen
   X->ac.steal=DebRegStolen[0];
   updatedebregs(X);
   RtlCopyMemory(iob,&X->ac,ol);	// Zugriffszähler lesen
   rlen=ol;
  }break;

  case IOCTL_VLPT_OutIn: {	// Bedienung der beiden Pipes
   Vlpt_KdPrint(("IOCTL_VLPT_OutIn\n"));
   ret=OutInCheck(X,il,ol);	// il und ol sind absichtlich vertauscht!
   if (!NT_SUCCESS(ret)) {
    Vlpt_KdPrint((" OutInCheck mit Fehler!\n"));
    break;
   }
   if (!(il|ol)) {
    Vlpt_KdPrint((" weder OUT noch IN!\n"));
    break;		// Nichts tun: kein IoMarkIrpPending()!!
   }
   if (X->f&surprise) {
    Vlpt_KdPrint((" SurpriseRemoval..\n"));
    ret=STATUS_DEVICE_REMOVED;
    break;
   }
   IoMarkIrpPending(I);		// Eigentlich per Mutex Sequenz schützen!?
   ret=KeWaitForMutexObject(&X->bmutex,Executive,KernelMode,FALSE,NULL);
   if (!NT_SUCCESS(ret)) {
    Vlpt_KdPrint((" Hoppla! Kein Mutex!\n"));
    break;
   }
   WaitForUrbComplete(X);
   FlushBuffer(X,NULL,0,5);	// Puffer ausräumen, Timer killen
   ret=OutIn(X,iob,il,iob,ol,&rlen,I);
   KeReleaseMutex(&X->bmutex,FALSE);
  }break;

  case IOCTL_VLPT_AbortPipe: {	// Eine Pipe abbrechen 
   Vlpt_KdPrint(("IOCTL_VLPT_AbortPipe\n"));
   if (ol>=sizeof(ULONG)) {
    AbortPipe(X,X->Interface->Pipes[*(PULONG)iob].PipeHandle);
   }else{
    Vlpt_KdPrint((" Huh, ol zu klein!\n"));
    ret=STATUS_INVALID_PARAMETER;
   }
  }break;

  case IOCTL_VLPT_GetLastError: {	// Letzter USB-Fehler
   Vlpt_KdPrint(("IOCTL_VLPT_GetLastError\n"));
   if (ol>=sizeof(ULONG)) {	// Nur wenn Puffer groß genug ist!
    *((PULONG)iob)=X->LastFailedUrbStatus;
    rlen=sizeof(ULONG);
   }else{
    Vlpt_KdPrint((" Huh, ol zu klein!\n"));
    ret=STATUS_INVALID_PARAMETER;
   }
  }break;

  case IOCTL_VLPT_AnchorDownload:
  case IOCTL_VLPT_RamRead:
  case IOCTL_VLPT_RamWrite:
  case IOCTL_VLPT_EepromRead:
  case IOCTL_VLPT_EepromWrite:
  case IOCTL_VLPT_XramRead:
  case IOCTL_VLPT_XramWrite: {
   ULONG a=0;	// wValue (Low-Teil) und wIndex (High-Teil)
   PURB U;
   Vlpt_KdPrint(("IOCTL_VLPT_%X\n",(UCHAR)(cc>>2)));
   if (il>4) il=4;
   if (X->f&surprise) {
    Vlpt_KdPrint((" SurpriseRemoval..\n"));
    ret=STATUS_DEVICE_REMOVED;
    break;
   }
   RtlCopyBytes(&a,iob,il);
#define USIZE sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST)
   U=ExAllocatePoolWithTag(NonPagedPool,USIZE,'tplU');
   if (!U) {
    Vlpt_KdPrint((" Kein Speicher!\n"));
    return STATUS_NO_MEMORY;
   }
   UsbBuildVendorRequest(U,URB_FUNCTION_VENDOR_DEVICE,USIZE,
     cc&METHOD_OUT_DIRECT ? USBD_TRANSFER_DIRECTION_IN : 0,
     0,(UCHAR)(cc>>2),(USHORT)a,(USHORT)(a>>16),
     NULL,I->MdlAddress,ol,NULL);
   ret=AsyncCallUSBD(X,I,U);
   if (ret!=STATUS_PENDING) {
    Vlpt_KdPrint((" Komplett oder nicht komplett, das ist hier die Frage!\n"));
    TRAP();
    IoReleaseRemoveLock(&X->rlock,I);
    return ret;	// Ist das richtig?? (Kein CompleteRequest??)
   }
#undef USIZE
  }break;

  default: {
   Vlpt_KdPrint(("IOCTL_VLPT_unbekannt_%d!\n",cc));
   ret=STATUS_INVALID_PARAMETER;
  }
 }
 IoReleaseRemoveLock(&X->rlock,I);
exi:
 if (ret==STATUS_PENDING) return ret;
 return CompleteRequest(I,ret,rlen);
}

#ifdef NTDDK
#include <parallel.h>
/******************************
 ** Interne IOCTL-Behandlung **
 ******************************/
 
// Callbacks für ProcessIntIoctl
BOOLEAN TryAllocatePort(PVOID Context) {
 return TRUE;
}
VOID FreePort(PVOID Context) {
}
ULONG QueryNumWaiters(PVOID Context) {
 return 0;
}
NTSTATUS TrySetChipMode(PVOID Context, UCHAR ChipMode) {
// TODO: Modus (ECP, EPP) schalten
 UCHAR buf[2];
 buf[0]=0x0B;	// ECR
 buf[1]=ChipMode<<5;
//  ret=OutIn(X,buf,2,NULL,0,NULL,I);
 return STATUS_SUCCESS;
}
NTSTATUS ClearChipMode(PVOID Context, UCHAR ChipMode) {
 return STATUS_SUCCESS;
}
NTSTATUS TrySelectDevice(PVOID Context, PVOID Command) {
 PARALLEL_1284_COMMAND *pc=Command;
// TODO
 return STATUS_UNSUCCESSFUL;
}
NTSTATUS DeselectDevice(PVOID Context, PVOID Command) {
 PARALLEL_1284_COMMAND *pc=Command;
 return STATUS_UNSUCCESSFUL;
}
USHORT DetermineIeeeModes(PVOID Context) {
// Stimmt das?
 return BOUNDED_ECP|ECP_HW_NOIRQ|IEEE_COMPATIBILITY|CENTRONICS;
}
NTSTATUS NegotiateIeeeMode(PVOID Context, USHORT ModeMaskFwd, USHORT ModeMaskRev,
  PARALLEL_SAFETY ModeSafety, BOOLEAN IsForward) {
//TODO
 return STATUS_SUCCESS;
}
NTSTATUS TerminateIeeeMode(PVOID Context) {
//TODO
 return STATUS_SUCCESS;
}
NTSTATUS IeeeFwdToRevMode(PVOID Context) {
//TODO
 return STATUS_SUCCESS;
}
NTSTATUS IeeeRevToFwdMode(PVOID Context) {
//TODO
 return STATUS_SUCCESS;
}
NTSTATUS ParallelRead(PVOID Context, OUT PVOID Buffer,
  ULONG NumBytesToRead, OUT PULONG NumBytesRead, UCHAR Channel) {
//TODO: Arbeitspferd für Scanner und ZIP-Laufwerke?
 return STATUS_SUCCESS;
}
NTSTATUS ParallelWrite(PVOID Context, PVOID Buffer,
  ULONG NumBytesToWrite, OUT PULONG NumBytesWritten, UCHAR Channel) {
//TODO: Arbeitspferd für Scanner und ZIP-Laufwerke?
 return STATUS_SUCCESS;
}

// Nachahmung des Microsoft-Parallelport-Treibers
NTSTATUS ProcessIntIoctl(IN PDEVICE_OBJECT dev,IN PIRP I) {
 PIO_STACK_LOCATION irpStack;
 PVOID iob;
 ULONG il;	// Eingabedatenlänge (OUT-Befehle und IN-Adressen)
 ULONG ol;	// Ausgabedatenlänge (IN-Daten)
 ULONG cc;
 PDEVICE_EXTENSION X=dev->DeviceExtension;
 NTSTATUS ret;
 ULONG rlen=0;	// im Standardfall keine Bytes zurück

 Vlpt_KdPrint2(("IRP_MJ_INTERNAL_DEVICE_CONTROL\n"));
 if (X->f&removing) {
  ret=STATUS_DEVICE_REMOVED;
  goto exi;
 }
 ret=IoAcquireRemoveLock(&X->rlock,I);
 if (!NT_SUCCESS(ret)) goto exi;

 irpStack=IoGetCurrentIrpStackLocation(I);
 cc=irpStack->Parameters.DeviceIoControl.IoControlCode;
 iob=I->AssociatedIrp.SystemBuffer;
 il=irpStack->Parameters.DeviceIoControl.InputBufferLength;
 ol=irpStack->Parameters.DeviceIoControl.OutputBufferLength;
 ret=STATUS_SUCCESS;

 switch (cc) {
  case IOCTL_INTERNAL_PARALLEL_PORT_ALLOCATE: {		/*11*/
   Vlpt_KdPrint(("IOCTL_INTERNAL_PARALLEL_PORT_ALLOCATE\n"));
   X->WmiPortAllocFreeCounts.PortAllocates++;
   if (!TryAllocatePort(X)) ret=STATUS_UNSUCCESSFUL;
  }break;

  case IOCTL_INTERNAL_GET_PARALLEL_PORT_INFO: {		/*12*/
   Vlpt_KdPrint(("IOCTL_INTERNAL_GET_PARALLEL_PORT_INFO\n"));
   if (ol<sizeof(PARALLEL_PORT_INFORMATION)) {
    ret=STATUS_BUFFER_TOO_SMALL;
    break;
   }
   {PARALLEL_PORT_INFORMATION *ppi=iob;
    ppi->OriginalController.QuadPart=(LONGLONG)(X->uc.LptBase);
    ppi->Controller=(PUCHAR)(X->uc.LptBase);
    ppi->SpanOfController=4;
    ppi->TryAllocatePort=TryAllocatePort;
    ppi->FreePort=FreePort;
    ppi->QueryNumWaiters=QueryNumWaiters;
    ppi->Context=X;
   }
   rlen=sizeof(PARALLEL_PORT_INFORMATION);
  }break;

  case IOCTL_INTERNAL_PARALLEL_CONNECT_INTERRUPT: {	/*13*/
   Vlpt_KdPrint(("IOCTL_INTERNAL_PARALLEL_CONNECT_INTERRUPT\n"));
   if (il<sizeof(PARALLEL_INTERRUPT_SERVICE_ROUTINE)
   ||  ol<sizeof(PARALLEL_INTERRUPT_INFORMATION)) {
    ret=STATUS_BUFFER_TOO_SMALL;
    break;
   }
   rlen=sizeof(PARALLEL_INTERRUPT_INFORMATION);
   ret=STATUS_UNSUCCESSFUL;	// keine Interrupt-Emulation
  }break;

  case IOCTL_INTERNAL_PARALLEL_DISCONNECT_INTERRUPT: {	/*14*/
   Vlpt_KdPrint(("IOCTL_INTERNAL_PARALLEL_DISCONNECT_INTERRUPT\n"));
   if (il<sizeof(PARALLEL_INTERRUPT_SERVICE_ROUTINE)) {
    ret=STATUS_BUFFER_TOO_SMALL;
    break;
   }
   ret=STATUS_INVALID_PARAMETER;
  }break;

  case IOCTL_INTERNAL_RELEASE_PARALLEL_PORT_INFO: {	/*15*/
   Vlpt_KdPrint(("IOCTL_INTERNAL_RELEASE_PARALLEL_PORT_INFO\n"));
  }break;

  case IOCTL_INTERNAL_GET_MORE_PARALLEL_PORT_INFO: {	/*17*/
   Vlpt_KdPrint(("IOCTL_INTERNAL_GET_MORE_PARALLEL_PORT_INFO\n"));
   if (ol<sizeof(MORE_PARALLEL_PORT_INFORMATION)) {
    ret=STATUS_BUFFER_TOO_SMALL;
    break;
   }
   {MORE_PARALLEL_PORT_INFORMATION *ppi=iob;
    ppi->InterfaceType=PNPBus;
    ppi->BusNumber=0;
    ppi->InterruptLevel=0;
    ppi->InterruptVector=0;
    ppi->InterruptAffinity=0;
    ppi->InterruptMode=LevelSensitive;
   }
   rlen=sizeof(MORE_PARALLEL_PORT_INFORMATION);
  }break;
  
  case IOCTL_INTERNAL_PARCHIP_CONNECT: {		/*18*/
   Vlpt_KdPrint(("IOCTL_INTERNAL_PARCHIP_CONNECT\n"));
  }break;

  case IOCTL_INTERNAL_PARALLEL_SET_CHIP_MODE: {		/*19*/
   Vlpt_KdPrint(("IOCTL_INTERNAL_PARALLEL_SET_CHIP_MODE\n"));
   if (il<sizeof(PARALLEL_CHIP_MODE)) {
    ret=STATUS_BUFFER_TOO_SMALL;
    break;
   }
   ret=TrySetChipMode(X,((PARALLEL_CHIP_MODE*)iob)->ModeFlags);
  }break;

  case IOCTL_INTERNAL_PARALLEL_CLEAR_CHIP_MODE: {	/*20*/
   Vlpt_KdPrint(("IOCTL_INTERNAL_PARALLEL_CLEAR_CHIP_MODE\n"));
   if (il<sizeof(PARALLEL_CHIP_MODE)) {
    ret=STATUS_BUFFER_TOO_SMALL;
    break;
   }
   ret=ClearChipMode(X,((PARALLEL_CHIP_MODE*)iob)->ModeFlags);
  }break;

  case IOCTL_INTERNAL_GET_PARALLEL_PNP_INFO: {		/*21*/
   Vlpt_KdPrint(("IOCTL_INTERNAL_GET_PARALLEL_PNP_INFO\n"));
   if (ol<sizeof(PARALLEL_PNP_INFORMATION)) {
    ret=STATUS_BUFFER_TOO_SMALL;
    break;
   }
   {PARALLEL_PNP_INFORMATION *ppi=iob;
    ppi->OriginalEcpController.QuadPart=(LONGLONG)(X->uc.LptBase+0x400);
    ppi->EcpController=(PUCHAR)(X->uc.LptBase+0x400);
    ppi->SpanOfEcpController=3;
    ppi->PortNumber=0;
    ppi->HardwareCapabilities=PPT_1284_3_PRESENT|
      PPT_BYTE_PRESENT|PPT_ECP_PRESENT|PPT_EPP_32_PRESENT|PPT_EPP_PRESENT;
    ppi->TrySetChipMode=TrySetChipMode;
    ppi->ClearChipMode=ClearChipMode;
    ppi->FifoDepth=16;
    ppi->FifoWidth=8;
    ppi->EppControllerPhysicalAddress.QuadPart=(LONGLONG)(X->uc.LptBase+4);
    ppi->SpanOfEppController=4;
    ppi->Ieee1284_3DeviceCount=0;	// TODO
    ppi->TrySelectDevice=TrySelectDevice;
    ppi->DeselectDevice=DeselectDevice;
    ppi->Context=X;
    ppi->CurrentMode=HW_MODE_PS2;	// TODO
    ppi->PortName=X->portName;
   }
   rlen=sizeof(PARALLEL_PNP_INFORMATION);
  }break;

  case IOCTL_INTERNAL_INIT_1284_3_BUS: {		/*22*/
   Vlpt_KdPrint(("IOCTL_INTERNAL_INIT_1284_3_BUS\n"));
  }break;

  case IOCTL_INTERNAL_SELECT_DEVICE: {			/*23*/
   Vlpt_KdPrint(("IOCTL_INTERNAL_SELECT_DEVICE\n"));
   if (il<sizeof(PARALLEL_1284_COMMAND)) {
    ret=STATUS_BUFFER_TOO_SMALL;
    break;
   }
   ret=TrySelectDevice(X,iob);
  }break;

  case IOCTL_INTERNAL_DESELECT_DEVICE: {		/*24*/
   Vlpt_KdPrint(("IOCTL_INTERNAL_DESELECT_DEVICE\n"));
   if (il<sizeof(PARALLEL_1284_COMMAND)) {
    ret=STATUS_BUFFER_TOO_SMALL;
    break;
   }
   ret=DeselectDevice(X,iob);
  }break;

  case IOCTL_INTERNAL_PARCLASS_CONNECT: {		/*30*/
   Vlpt_KdPrint(("IOCTL_INTERNAL_PARCLASS_CONNECT\n"));
   if (ol<sizeof(PARCLASS_INFORMATION)) {
    ret=STATUS_BUFFER_TOO_SMALL;
    break;
   }
   {PARCLASS_INFORMATION *ppi=iob;
    ppi->Controller=(PUCHAR)(X->uc.LptBase);
    ppi->SpanOfController=4;
    ppi->DetermineIeeeModes=DetermineIeeeModes;
    ppi->NegotiateIeeeMode=NegotiateIeeeMode;
    ppi->TerminateIeeeMode=TerminateIeeeMode;
    ppi->IeeeFwdToRevMode=IeeeFwdToRevMode;
    ppi->IeeeRevToFwdMode=IeeeRevToFwdMode;
    ppi->ParallelRead=ParallelRead;
    ppi->ParallelWrite=ParallelWrite;
    ppi->ParclassContext=X;
    ppi->HardwareCapabilities=PPT_1284_3_PRESENT|PPT_BIDI_PRESENT|
      PPT_ECP_PRESENT|PPT_EPP_PRESENT|PPT_EPP_32_PRESENT|PPT_BYTE_PRESENT;
    ppi->FifoDepth=16;
    ppi->FifoWidth=8;
   }
   rlen=sizeof(PARCLASS_INFORMATION);
  }break;

  case IOCTL_INTERNAL_PARCLASS_DISCONNECT: {		/*31*/
   Vlpt_KdPrint(("IOCTL_INTERNAL_PARCLASS_DISCONNECT\n"));
// TODO
  }break;

  case IOCTL_INTERNAL_DISCONNECT_IDLE: {		/*32*/
   Vlpt_KdPrint(("IOCTL_INTERNAL_DISCONNECT_IDLE\n"));
// TODO
  }break;

  case IOCTL_INTERNAL_LOCK_PORT: {			/*37*/
   Vlpt_KdPrint(("IOCTL_INTERNAL_LOCK_PORT\n"));
// TODO
  }break;

  case IOCTL_INTERNAL_UNLOCK_PORT: {			/*38*/
   Vlpt_KdPrint(("IOCTL_INTERNAL_UNLOCK_PORT\n"));
// TODO
  }break;

  case IOCTL_INTERNAL_PARALLEL_PORT_FREE: {		/*40*/
   Vlpt_KdPrint(("IOCTL_INTERNAL_PARALLEL_PORT_FREE\n"));
   X->WmiPortAllocFreeCounts.PortFrees++;
   FreePort(X);
  }break;

  case IOCTL_INTERNAL_PARDOT3_CONNECT: {		/*41*/
   Vlpt_KdPrint(("IOCTL_INTERNAL_PARDOT3_CONNECT\n"));
// undokumentiert!
  }break;

  case IOCTL_INTERNAL_PARDOT3_DISCONNECT: {		/*42*/
   Vlpt_KdPrint(("IOCTL_INTERNAL_PARDOT3_DISCONNECT\n"));
// undokumentiert!
  }break;

  case IOCTL_INTERNAL_PARDOT3_RESET: {			/*43*/
   Vlpt_KdPrint(("IOCTL_INTERNAL_PARDOT3_RESET\n"));
// undokumentiert!
  }break;

  case IOCTL_INTERNAL_PARDOT3_SIGNAL: {			/*44*/
   Vlpt_KdPrint(("IOCTL_INTERNAL_PARDOT3_SIGNAL\n"));
// undokumentiert!
  }break;

  default: ret=STATUS_INVALID_PARAMETER;
 }
 IoReleaseRemoveLock(&X->rlock,I);
exi:
 if (ret==STATUS_PENDING) return ret;
 return CompleteRequest(I,ret,rlen);
}

/****************
 * WMI-Geraffel *
 ****************/
UNICODE_STRING gRegPath;	// copy of the registry path passed to DriverEntry()
				// \Registry\Machine\System\CurrentControlSet\Services\usb2lpt
#include <wmistr.h>	// WMIREG_FLAG_INSTANCE_PDO
// Define the (only at the moment) WMI GUID that we support
const GUID WmiAllocFreeCountsGuid = PARPORT_WMI_ALLOCATE_FREE_COUNTS_GUID;
// Array of WMI GUIDs supported by driver
const WMIGUIDREGINFO WmiGuidList[]={
 {&WmiAllocFreeCountsGuid,1,0}};

// This is our callback routine that WMI calls when it wants to find out
//   information about the data blocks and/or events that the device provides.
NTSTATUS QueryWmiRegInfo(
  IN  PDEVICE_OBJECT  dev, 
  OUT PULONG          PRegFlags,
  OUT PUNICODE_STRING PInstanceName,
  OUT PUNICODE_STRING *PRegistryPath,
  OUT PUNICODE_STRING MofResourceName,
  OUT PDEVICE_OBJECT  *Pdo) {
 PDEVICE_EXTENSION X = dev->DeviceExtension;
 UNREFERENCED_PARAMETER( PInstanceName );
 UNREFERENCED_PARAMETER( MofResourceName );
 PAGED_CODE();
 Vlpt_KdPrint(("QueryWmiRegInfo"));
 *PRegFlags     = WMIREG_FLAG_INSTANCE_PDO;
 *PRegistryPath = &gRegPath;
 *Pdo           = X->pdo;
 return STATUS_SUCCESS;
}

// This is our callback routine that WMI calls to query a data block
NTSTATUS QueryWmiDataBlock(
  IN PDEVICE_OBJECT   dev,
  IN PIRP             I,
  IN ULONG            GuidIndex,
  IN ULONG            InstanceIndex,
  IN ULONG            InstanceCount,
  IN OUT PULONG       InstanceLengthArray,
  IN ULONG            OutBufferSize,
  OUT PUCHAR          Buffer) {
 NTSTATUS status;
 ULONG size=sizeof(PARPORT_WMI_ALLOC_FREE_COUNTS);
 PDEVICE_EXTENSION X=dev->DeviceExtension;
 PAGED_CODE();
    // Only ever registers 1 instance per guid
#if DBG
 ASSERT(InstanceIndex==0 && InstanceCount==1);
#else
 UNREFERENCED_PARAMETER(InstanceCount);
 UNREFERENCED_PARAMETER(InstanceIndex);
#endif
 switch (GuidIndex) {
  case 0:
        // Request is for ParPort Alloc and Free Counts
        // If caller's buffer is large enough then return the info, otherwise
        //   tell the caller how large of a buffer is required so they can
        //   call us again with a buffer of sufficient size.
  if (OutBufferSize<size) {
   status = STATUS_BUFFER_TOO_SMALL;
   break;
  }
  *((PARPORT_WMI_ALLOC_FREE_COUNTS*)Buffer)=X->WmiPortAllocFreeCounts;
  *InstanceLengthArray=size;
  status=STATUS_SUCCESS;
  break;

  default:
        // Index value larger than our largest supported - invalid request
  status = STATUS_WMI_GUID_NOT_FOUND;
  break;
 }
 status = WmiCompleteRequest(dev,I,status,size,IO_NO_INCREMENT);	// komisch!
 return status;
}
// Initialize WMI Context that we pass to WMILIB during the handling of
//   IRP_MJ_SYSTEM_CONTROL. This context lives in our device extension
// Register w/WMI that we are able to process WMI IRPs
NTSTATUS InitWmi(PDEVICE_EXTENSION X) {
 PWMILIB_CONTEXT k = &X->WmiLibCtx;
 PAGED_CODE();
 k->GuidCount=elemof(WmiGuidList);
 k->GuidList =(WMIGUIDREGINFO*)WmiGuidList;

 k->QueryWmiRegInfo    = QueryWmiRegInfo;   // required
 k->QueryWmiDataBlock  = QueryWmiDataBlock; // required
 k->SetWmiDataBlock    = NULL; // optional
 k->SetWmiDataItem     = NULL; // optional
 k->ExecuteWmiMethod   = NULL; // optional
 k->WmiFunctionControl = NULL; // optional
  // Tell WMI that we can now accept WMI IRPs
 return IoWMIRegistrationControl(X->fdo,WMIREG_ACTION_REGISTER);
}

// We call WMILIB to process the IRP for us. WMILIB returns a disposition
// that tells us what to do with the IRP.
// Es scheint dies ist ein FDO
NTSTATUS DispatchSystemControl(PDEVICE_OBJECT dev, PIRP I) {
 SYSCTL_IRP_DISPOSITION dispo;
 PDEVICE_EXTENSION X = dev->DeviceExtension;
 NTSTATUS status = IoAcquireRemoveLock(&X->rlock,I);
 if (!NT_SUCCESS(status)) return CompleteRequest(I,status,0);
// Wirklich??
// if (X->fdo!=dev) return CompleteRequest(I,I->IoStatus.Status,I->IoStatus.Information);
 PAGED_CODE();
 status=WmiSystemControl(&X->WmiLibCtx,dev,I,&dispo);
 switch (dispo) {
  case IrpProcessed: break;	// This irp has been processed and may be completed or pending.
  case IrpNotCompleted: CompleteRequest(I,I->IoStatus.Status,I->IoStatus.Information); break;
	// This irp has not been completed, but has been fully processed. we will complete it now
  case IrpForward:	// This irp is either not a WMI irp or is a WMI irp targetted
  case IrpNotWmi:	// at a device lower in the stack.
    IoSkipCurrentIrpStackLocation(I);
    status=IoCallDriver(X->ldo,I);
  break;
  default:	// We really should never get here, but if we do just forward....
    ASSERT(FALSE);
    IoSkipCurrentIrpStackLocation(I);
    status=IoCallDriver(X->ldo,I);
  break;
 }
 IoReleaseRemoveLock(&X->rlock,I);
 return status;
}
#endif//NTDDK

/********************************
 ** Restliche Major-Funktionen **
 ********************************/

NTSTATUS Create(IN PDEVICE_OBJECT dev,IN PIRP I) {
// Aufruf von CreateFile() (könnte "\\.\Vlpt-x\yyzz" sein).
 PDEVICE_EXTENSION X=(PDEVICE_EXTENSION)dev->DeviceExtension;

 Vlpt_KdPrint2(("Enter Create()\n"));
 if (!(X->f&Started)) {
  Vlpt_KdPrint2(("Öffnen ohne zu starten!\n"));
  TRAP();
  return CompleteRequest(I,STATUS_UNSUCCESSFUL,0);
 }
 return CompleteRequest(I,STATUS_SUCCESS,0);
}
 
NTSTATUS Close(IN PDEVICE_OBJECT dev,IN PIRP I) {
// Aufruf von CloseHandle()
 PDEVICE_EXTENSION X=(PDEVICE_EXTENSION)dev->DeviceExtension;
 Vlpt_KdPrint2(("Close()\n"));
 return CompleteRequest(I,STATUS_SUCCESS,0);
}

VOID Unload(IN PDRIVER_OBJECT DriverObject) {
// Wenn das letzte Gerät verschwindet, verschwindet auch der Treiber
 UnhookSyscalls();		// Anzapfung aufheben
#ifdef NTDDK
 ExFreePoolWithTag(gRegPath.Buffer,'tplR');
#endif
 Vlpt_KdPrint2(("Unload()\n"));
 TRAP();
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject,
  IN PUNICODE_STRING RegPath) {
 Vlpt_KdPrint (("DriverEntry (Build: "__DATE__ "/" __TIME__"\n"));
 TRAP();
 if (IsPentium()) {
  PrepareDR();	// Win98: setzt CurThreadPtr!
#ifndef NTDDK
  if (!CurThreadPtr) return STATUS_UNSUCCESSFUL;
#endif
 }
#ifdef NTDDK
// Save a copy of *RegPath in driver global gRegPath for future reference.
// Terminate the path so that we can safely use gRegPath.Buffer as a PWSTR.
 {int size = RegPath->Length+sizeof(WCHAR);
  gRegPath.Buffer=ExAllocatePoolWithTag((PagedPool),size,'tplR');
  if (!gRegPath.Buffer) return STATUS_NO_MEMORY;
  gRegPath.Length=0;
  gRegPath.MaximumLength=(USHORT)size;
  RtlCopyUnicodeString(&gRegPath,RegPath);
  gRegPath.Buffer[size/sizeof(WCHAR)-1]=0;
 }
#endif
 HookSyscalls();		// Anzapfung setzen
 DriverObject->MajorFunction[IRP_MJ_CREATE] = Create;
 DriverObject->MajorFunction[IRP_MJ_CLOSE] = Close;
 DriverObject->DriverUnload = Unload;
 DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ProcessIOCTL;
#ifdef NTDDK
 DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = ProcessIntIoctl;
 DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = DispatchSystemControl;
#endif
 DriverObject->MajorFunction[IRP_MJ_PNP]  = DispatchPnp;
 DriverObject->MajorFunction[IRP_MJ_POWER]= DispatchPower;
 DriverObject->DriverExtension->AddDevice = AddDevice;
 return STATUS_SUCCESS;
}

/* Was kommt beim Anstecken des Gerätes vorbei?
Aufruf		Unterfunktion		Aktion
DriverEntry()				setzt AddDevice usw.
AddDevice()				IoCreateDevice()
DispatchPnP()	QUERY_CAPABILITIES	setzt SurpriseRemovalOK
-"-		QUERY_PNP_DEVICE_STATE	-
-"-		QUERY_PNP_DEVICE_STATE	-
-"-		QUERY_DEVICE_RELATIONS	-
-"-		QUERY_DEVICE_TEXT	liefert "LPTx" - ist Quatsch
-"-		QUERY_ID		-
-"-		QUERY_DEVICE_RELATIONS	-
-"-		QUERY_INTERFACE		-
-"-		QUERY_DEVICE_RELATIONS	-
Create()				okay
InternalIoctl	PARALLEL_PORT_ALLOCATE	WMI mitzählen, okay
 Create()
  InternalIoctl	GET_PARALLEL_PNP_INFO	PARALLEL_PNP_INFORMATION-struct füllen
 Close()				okay
 Create()				okay
  InternalIoctl	GET_PARALLEL_PORT_INFO	PARALLEL_PORT_INFORMATION-struct füllen
  InternalIoctl	GET_PARALLEL_PNP_INFO	PARALLEL_PNP_INFORMATION-struct füllen
 Close()
 InternalIoctl	PARALLEL_PORT_FREE	WMI mitzählen, okay
Close()
Keine DLL wird geladen, kein CoInstaller gerufen. 130519
*/

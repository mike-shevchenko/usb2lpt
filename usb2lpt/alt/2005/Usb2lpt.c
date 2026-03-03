#define VERBOSE

/* Laut Buch muss dieser Zwischentreiber, der IRPs zerlegt und neue erstellt:
   Dispatch:
    IoMarkIrpPending
    IoAllocateIrp (2x)
    URBs anfordern
    Anlegen Zählvariable im ursprünglichen IRP (ungenutztes Feld Params.Key)
    IoSetCompletionRoutine (2x die gleiche, pContext = Zählvariable)
    IoCallDriver (2x)
    return STATUS_PENDING
   Komplettierung:
    IRP aufräumen, zusätzliche Ressourcen (URBs) freigeben
    ExInterlockedDecrement(pContext)
    Wenn Null: IoCompleteRequest(ursprüngliches IRP)
    return STATUS_MORE_PROCESSING_REQUIRED (entfernt IRP endgültig)
 */

#define INIT_MY_GUID	// In diese .OBJ-Datei kommt die 16-Byte-GUID hinein
#include "usb2lpt.h"

//static BOOLEAN IsW2K;

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

NTSTATUS CompleteRequest(IN PIRP I,IN NTSTATUS ret,IN ULONG info) {
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

/* ------------- Asynchrone USBD-Aufrufe ------------- */

typedef struct{	// "Arbeitsauftrag" für kombinierte Bulk-Aus- und Eingabe
 PDEVICE_EXTENSION x;	// Unsere Geräteerweiterung
 PIRP irpj;	// Job-IRP, oder NULL für angezapften Ein/Ausgabebefehl
 PIRP irpw;	// IRP zum Bulk-Schreiben
 PIRP irpr;	// IRP zum Bulk-Lesen (NULL für Nur-Schreiben)
 NTSTATUS stat;	// zurückzugebender Status (initialisieren mit 0)
 ULONG rlen;	// Gelesene Bytes (initialisieren mit 0)
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
NTSTATUS OutInFertig(PDEVICE_OBJECT fdo,PIRP I,PJOB J) {
// fdo enthält keinen brauchbaren Zeiger!
 if (NT_SUCCESS(J->stat)) J->stat=I->IoStatus.Status;
 if (I==J->irpw) {
  SetUsbStatus(J->x,&J->stat,(PURB)&J->urbw);
  Vlpt_KdPrint2(("Ausgabe von %d Bytes FERTIG\n",J->urbw.TransferBufferLength));
  J->irpw=NULL;			// erledigt!
 }else if (I==J->irpr) {
  SetUsbStatus(J->x,&J->stat,(PURB)&J->urbr);
  J->rlen=J->urbr.TransferBufferLength;
  Vlpt_KdPrint2(("Eingabe von %d Bytes FERTIG\n",J->rlen));
  J->irpr=NULL;			// erledigt!
 }
 IoFreeIrp(I);
 if (!J->irpw && !J->irpr) {	// Alles erledigt?
  if (J->irpj) {
   CompleteRequest(J->irpj,J->stat,J->rlen);
  }else{
   J->x->bfill=0;
   KeSetEvent(&J->x->ev,0,FALSE);	// bfill-Ampel auf Grün
  }
  ExFreePool(J);		// Job (Arbeitsauftrag) samt URBs erledigt
 }
 return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS AsyncCallUSBD(PDEVICE_EXTENSION X,PJOB J,PIRP I,PURB U) {
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
 IoSetCompletionRoutine(I,OutInFertig,J,TRUE,TRUE,TRUE);//IRQL<=2
 return IoCallDriver(X->ldo,I);				//IRQL<=2
}

#define Alloc(len) ExAllocatePoolWithTag(NonPagedPool,len,'tplV')


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
  if (!(p->PipeType==UsbdPipeTypeBulk)) {
   Vlpt_KdPrint(("OutInCheck(): Pipe nicht vom Typ BULK!\n"));
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

NTSTATUS OutIn(PDEVICE_EXTENSION X, PBYTE ob, ULONG ol, PBYTE ib, ULONG il,
  PULONG bytesread, PIRP I) {
// Aus- und Eingabe über die beiden Pipes zum/vom USB2LPT
// bytesread ("gelesene Bytes") = NULL -> kurzer Transfer nicht OK
 PUSBD_INTERFACE_INFORMATION ii=X->Interface;
 NTSTATUS ret=STATUS_SUCCESS;
 PJOB J;
#define USIZE sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER)

 Vlpt_KdPrint2(("Aufruf OutIn (ol=%d, il=%d)\n",ol,il));
 if (!ol && !il) return ret;
 J=Alloc(il?sizeof(JOB):sizeof(JOB)-USIZE);
 if (!J) return STATUS_NO_MEMORY;
 RtlZeroMemory(J,sizeof(JOB)-USIZE-USIZE);
 J->x=X;
 J->irpj=I;
 if (!I) KeClearEvent(&X->ev);	// Ampel auf ROT
 if (ol) {
// IoBuildDeviceIoControlRequest geht nicht wegen PASSIVE_LEVEL!
  UsbBuildInterruptOrBulkTransferRequest((PURB)&J->urbw,
    USIZE,			//size of urb
    ii->Pipes[0].PipeHandle,	// Erste Pipe = Schreiben
    ob,NULL,ol,0,NULL);
  J->irpw=IoAllocateIrp(X->ldo->StackSize,FALSE);
  if (!J->irpw) ret=STATUS_INSUFFICIENT_RESOURCES;
 }
 if (il && NT_SUCCESS(ret)) {
  UsbBuildInterruptOrBulkTransferRequest((PURB)&J->urbr,
    USIZE,			//size of urb
    ii->Pipes[1].PipeHandle,	// Zweite Pipe = Lesen
    ib,NULL,il,
    bytesread			// je nachdem!
     ? USBD_TRANSFER_DIRECTION_IN|USBD_SHORT_TRANSFER_OK
     : USBD_TRANSFER_DIRECTION_IN,
    NULL);
  J->irpr=IoAllocateIrp(X->ldo->StackSize,FALSE);
  if (!J->irpr) ret=STATUS_INSUFFICIENT_RESOURCES;
 }
 if (ol && NT_SUCCESS(ret)) ret=AsyncCallUSBD(X,J,J->irpw,(PURB)&J->urbw);
 if (!NT_SUCCESS(ret) && J->irpw) {IoFreeIrp(J->irpw); J->irpw=NULL;}
 if (il && NT_SUCCESS(ret)) ret=AsyncCallUSBD(X,J,J->irpr,(PURB)&J->urbr);
 if (!NT_SUCCESS(ret) && J->irpr) {IoFreeIrp(J->irpr); J->irpr=NULL;}
// Problem, wenn das erste AsyncCallUSBD klappte und das zweite schief geht!
 if (!NT_SUCCESS(ret)) ExFreePool(J);
#undef USIZE
 return ret;		// Jetzt sind bis zu zwei URBs in Arbeit
}

BYTE ab(PDEVICE_EXTENSION X, WORD adr) {	// Adress-Byte ermitteln
 adr-=X->uc.LptBase;	// sollte 0..7 oder 400h..403h liefern
 if (adr&0x400) adr|=8;	// ECP-Adressbyte draus machen
 return (BYTE)adr;
}

void _stdcall HandleOut(PDEVICE_EXTENSION X,WORD adr,BYTE b) {
 BYTE a;	// Adress-Byte für USB
 ASSERT(KeGetCurrentIrql()==0);
 Vlpt_KdPrint2(("HandleOut(%03Xh,%02Xh)\n",adr,b));
 if (KeGetCurrentIrql()) return;		// kann nicht behandeln!
 if (!NT_SUCCESS(IoAcquireRemoveLock(&X->rlock,NULL))) return;
 if (KeWaitForMutexObject(&X->bmutex,Executive,KernelMode,FALSE,NULL)
   !=STATUS_SUCCESS) goto ex2;
 KeWaitForSingleObject(&X->ev,Suspended,KernelMode,FALSE,NULL);	// Timer abwarten
 ASSERT(X->bfill<=62 && !(X->bfill&1));	// gerade!
 if (X->bfill==62) {	// Puffer gerammelt voll: ausgeben!
//  ASSERT(!(X->f&trapping));
//  X->f|=trapping;
  if (X->uc.flags&UC_WriteCache) KeCancelTimer(&X->wrcache.tmr);
  if (!NT_SUCCESS(OutIn(X,X->buffer,X->bfill,NULL,0,NULL,NULL))) goto ex;
  Vlpt_KdPrint2(("Warten auf Ende von OutIn()\n"));
  KeWaitForSingleObject(&X->ev,Suspended,KernelMode,FALSE,NULL);
  Vlpt_KdPrint2(("Das Warten hat ein Ende!\n"));
//  X->f&=~trapping;
 }
 a=ab(X,adr);
 switch (a) {
  case 0: {
   X->mirror[0]|=UC_ReadCache0;
   X->mirror[1]=b;
  }break;
  case 2: {
   X->mirror[0]|=UC_ReadCache2;
   X->mirror[2]=b;
  }break;
 }
 X->buffer[X->bfill++]=a;
 X->buffer[X->bfill++]=b;
 if (X->uc.flags&UC_WriteCache) {
  KeSetTimer(&X->wrcache.tmr,
    RtlConvertLongToLargeInteger(X->uc.TimeOut*-10000),
    &X->wrcache.dpc);	// Zeit wird nicht größer als DWORD
 }else{				// sofort versenden!
//  ASSERT(!(X->f&trapping));
//  X->f|=trapping;
  if (!NT_SUCCESS(OutIn(X,X->buffer,X->bfill,NULL,0,NULL,NULL))) goto ex;
  KeWaitForSingleObject(&X->ev,Suspended,KernelMode,FALSE,NULL);
//  X->f&=~trapping;
 }
ex:
 KeReleaseMutex(&X->bmutex,FALSE);
ex2:
 IoReleaseRemoveLock(&X->rlock,NULL);
 Vlpt_KdPrint2(("HandleOut EXIT\n"));
}

BYTE _stdcall HandleIn(PDEVICE_EXTENSION X,WORD adr) {
 BYTE a,b=0xFF;
 
 ASSERT(KeGetCurrentIrql()==0);
 Vlpt_KdPrint2(("HandleIn ENTER\n"));
 if (KeGetCurrentIrql()) goto ex3;		// kann nicht behandeln!
 if (!NT_SUCCESS(IoAcquireRemoveLock(&X->rlock,NULL))) goto ex3;
 if (KeWaitForMutexObject(&X->bmutex,Executive,KernelMode,FALSE,NULL)
   !=STATUS_SUCCESS) goto ex2;
 KeWaitForSingleObject(&X->ev,Suspended,KernelMode,FALSE,NULL);	// Timer abwarten
 ASSERT(X->bfill<=62 && !(X->bfill&1));	// gerade!

 a=ab(X,adr);
 switch (a) {
  case 0: if (X->mirror[0]&X->uc.flags&UC_ReadCache0) {
   b=X->mirror[1];
   goto ex;
  }break;
  case 2: if (X->mirror[0]&X->uc.flags&UC_ReadCache2) {
   b=X->mirror[2]|0xE0;
   goto ex;
  }break;
 }
 X->buffer[X->bfill++]=a|0x10;

// ASSERT(!(X->f&trapping));
// X->f|=trapping;
 if (X->uc.flags&UC_WriteCache) KeCancelTimer(&X->wrcache.tmr);
 if (!NT_SUCCESS(OutIn(X,X->buffer,X->bfill,&b,1,NULL,NULL))) goto ex;
 Vlpt_KdPrint2(("Warten auf Ende von OutIn()\n"));
 KeWaitForSingleObject(&X->ev,Suspended,KernelMode,FALSE,NULL);
 Vlpt_KdPrint2(("Das Warten hat ein Ende!\n"));
// X->f&=~trapping;
ex:
 KeReleaseMutex(&X->bmutex,FALSE);
ex2:
 IoReleaseRemoveLock(&X->rlock,NULL);
ex3:
 Vlpt_KdPrint2(("HandleIn(%03Xh) liefert %02Xh\n",adr,b));
 return b;
}

VOID TimerDpc(IN PKDPC Dpc,PDEVICE_EXTENSION X,PVOID a,PVOID b) {
 ASSERT(KeGetCurrentIrql()==DISPATCH_LEVEL);
 Vlpt_KdPrint2(("TimerDpc, Ausgabe %u Bytes\n",X->bfill));
// X->f|=trapping;
 OutIn(X,X->buffer,X->bfill,NULL,0,NULL,NULL);
// X->f&=~trapping;
}

/* ------------- Synchrone USBD-Aufrufe ------------- */

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
 if (!USBD_SUCCESS(U->UrbHeader.Status)) {
  X->LastFailedUrbStatus=U->UrbHeader.Status;
  if (NT_SUCCESS(ret)) ret=STATUS_UNSUCCESSFUL;
 }
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
// UCHAR alternateSetting /*, MyInterfaceNumber*/;
 PUSBD_INTERFACE_INFORMATION interfaceObject;
 USBD_INTERFACE_LIST_ENTRY interfaceList[2];

 Vlpt_KdPrint2(("enter SelectInterfaces\n"));

// MyInterfaceNumber = SAMPLE_INTERFACE_NBR;

	// Search the configuration descriptor for the first interface/alternate setting

 interfaceList[0].InterfaceDescriptor =
 USBD_ParseConfigurationDescriptorEx(ConfigurationDescriptor,
                                       ConfigurationDescriptor,
                                       -1,         // Interface - don't care
                                       -1,         // Alternate Setting - don't care
                                       -1,         // Class - don't care
                                       -1,         // SubClass - don't care
                                       -1);        // Protocol - don't care

 ASSERT(interfaceList[0].InterfaceDescriptor);

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
  X->Interface=Alloc(interfaceObject->Length);
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
 if (!(configurationDescriptor=Alloc(siz))) goto NoMemory;
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
 ExFreePool(configurationDescriptor);		// bisherigen Deskriptor verwerfen
 ntStatus=STATUS_NO_MEMORY;
 if (!(configurationDescriptor=Alloc(siz))) goto NoMemory;	// neu allozieren
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
NoMemory:
 ExFreePool(configurationDescriptor);
// Free(&urb);
 Vlpt_KdPrint2(("leave ConfigureDevice (%x)\n", ntStatus));
 return ntStatus;
}

void FreeTraps(PDEVICE_EXTENSION X) {
 int i;
//  TRAP();
 if (CurThreadPtr && X->instance<3) {	// Patch vorerst auf 9x beschränken
  *(PUSHORT)(0x408+X->instance*2)=X->oldlpt;
  *(PUSHORT)(0x410)=X->oldsys;
 }
 for (i=0; i<3; i++) if (X->debugreg[i]) {
  FreeDRN(X->debugreg[i]-1);
  X->debugreg[i]=0;
 }
 X->mirror[0]=0;		// Spiegel löschen
}

void SetTraps(PDEVICE_EXTENSION X) {
 int e;
 X->f= X->f&~No_Function | (X->uc.flags&UC_Function ? 0 : No_Function);
 if (CurThreadPtr && X->instance<3) {	// Patch vorerst auf 9x beschränken
  register PUSHORT a1=(PUSHORT)(0x408+X->instance*2);
  X->oldlpt=*a1;
  X->oldsys=*(PUSHORT)(0x410);
  *a1=X->uc.LptBase;
  if (X->oldsys>>14 <= X->instance) {
   *(PUSHORT)(0x410)=X->oldsys&0x3FFF | ((X->instance+1)<<14);
  }
 }
 e=AllocDR(X->uc.LptBase,X);	// Debugregister anfordern
 if (e>=0) X->debugreg[0]=e+1;	// abspeichern
 else Vlpt_KdPrint(("Kann LptBase (SPP) nicht anzapfen (%d)\n",e));
 if (X->uc.Mode&1) {		// EPP
  e=AllocDR((WORD)(X->uc.LptBase+4),X);
  if (e>=0) X->debugreg[1]=e+1;
  else Vlpt_KdPrint(("Kann LptBase+4 (EPP) nicht anzapfen (%d)\n",e));
 }
 if (X->uc.Mode&2) {		// ECP
  e=AllocDR((WORD)(X->uc.LptBase+0x400),X);
  if (e>=0) X->debugreg[2]=e+1;
  else Vlpt_KdPrint(("Kann LptBase+400h (ECP) nicht anzapfen (%d)\n",e));
 }
}

static const WORD Ports[]={0x378,0x278,0x3BC};

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
  if ((DDesc=Alloc(sizeof(*DDesc)))) {
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
   ExFreePool(DDesc);
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
  if (pi->PipeType!=UsbdPipeTypeBulk /*=2*/) goto ex;
  if (pi->MaximumTransferSize<64) goto ex;
  if (!(pi->PipeHandle)) goto ex;
  pi=ii->Pipes+1;		// Zweite Pipe = Lesen
  if (pi->PipeType!=UsbdPipeTypeBulk /*=2*/) goto ex;
  if (pi->MaximumTransferSize<64) goto ex;
  if (!(pi->PipeHandle)) goto ex;
  KeInitializeMutex(&X->bmutex,0);
  KeInitializeTimer(&X->wrcache.tmr);
  KeInitializeDpc(&X->wrcache.dpc,TimerDpc,X);
  KeInitializeEvent(&X->ev,NotificationEvent,TRUE);
//IoInitializeDpcRequest(X->fdo,...)
  TRAP();
  if (X->uc.LptBase&0xFF00) SetTraps(X);
ex:;
 }
 Vlpt_KdPrint (("leave StartDevice (%x)\n", ntStatus));
 return ntStatus;
}

NTSTATUS Quatsch(PUNICODE_STRING us,PCSZ str) {
// Unicode-String aus ASCIIZ initialisieren
 ANSI_STRING as;
 RtlInitAnsiString(&as,str);
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
 if (X->instance<elemof(Ports)) {
  X->uc.LptBase=Ports[X->instance];	// Vorgaben setzen
  X->uc.TimeOut=200;
  X->uc.flags=UC_Function|UC_Debugreg|UC_WriteCache;
 }
 if (!NT_SUCCESS(IoOpenDeviceRegistryKey(X->pdo,PLUGPLAY_REGKEY_DEVICE,
   KEY_QUERY_VALUE,&key))) {
  Vlpt_KdPrint (("IoOpenDeviceRegistryKey versagt!\n"));
  TRAP();
 }else{
  UNICODE_STRING us;
  struct {
   KEY_VALUE_PARTIAL_INFORMATION v;
   UCHAR data[sizeof(TUserCfg)-1];	// insgesamt 6 Bytes Daten
  }val;
  ULONG needed;
  static const CHAR Tag[]="UserCfg";
  TRAP();
  if (NT_SUCCESS(Quatsch(&us,Tag))) {
   if (NT_SUCCESS(ZwQueryValueKey(key,&us,KeyValuePartialInformation,
     &val.v,sizeof(val),&needed))
   && val.v.Type==REG_BINARY
   && needed>=sizeof(val)) {
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
#define USIZE sizeof(struct _URB_SELECT_CONFIGURATION)
 U=Alloc(USIZE);
 if (U) {
  NTSTATUS status;
  UsbBuildSelectConfigurationRequest(U,USIZE,NULL);	// Auf "unkonfiguriert"
  status=CallUSBD(X,U);
  Vlpt_KdPrint(("Device Configuration Closed status = %x usb status = %x.\n",
    status, U->UrbHeader.Status));
  ExFreePool(U);
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

 Vlpt_KdPrint2(("VLPT.SYS: AbortPipe \n"));
#define USIZE sizeof(struct _URB_PIPE_REQUEST)
 U=Alloc(USIZE);
 if (U) {
  RtlZeroMemory(U,USIZE);
  U->UrbHeader.Length=(USHORT)USIZE;
  U->UrbHeader.Function=URB_FUNCTION_ABORT_PIPE;
  U->UrbPipeRequest.PipeHandle=PipeHandle;
#ifdef WIN95
  {
   USBD_VERSION_INFORMATION VersionInformation;
      // kludge.  Win98 changed the size of the URB_PIPE_REQUEST.
      // Check the USBDI version.  If it is prior to win98 (0x101)
      // make the structure smaller.
   USBD_GetUSBDIVersion(&VersionInformation);
   if (VersionInformation.USBDI_Version<0x101) {
    Vlpt_KdPrint(("AbortPipe() Detected OSR2.1\n"));
    U->UrbHeader.Length-=sizeof(ULONG);
   }
  }
#endif
  ntStatus=CallUSBD(X,U);
  ExFreePool(U);
#undef USIZE
 }else ntStatus=STATUS_INSUFFICIENT_RESOURCES;	// Wieso diesmal Resources??
 return ntStatus;
}


NTSTATUS DevName(PUNICODE_STRING us,int Instance) {
// liefert "durchnummerierten" Device-String
 static const CHAR deviceName[]="\\Device\\Parallel?";
 NTSTATUS ret;
 ret=Quatsch(us,deviceName);
 if (NT_SUCCESS(ret)) us->Buffer[sizeof(deviceName)-2]=(USHORT)('0'+Instance);
 return ret;
}

NTSTATUS DevLink(PUNICODE_STRING us,int Instance) {
// liefert "durchnummerierten" DosDevices-String
 static const CHAR deviceLink[]="\\DosDevices\\LPT?";
 NTSTATUS ret;
 ret=Quatsch(us,deviceLink);
 if (NT_SUCCESS(ret)) us->Buffer[sizeof(deviceLink)-2]=(USHORT)('1'+Instance);
 return ret;
}

NTSTATUS RemoveDevice(PDEVICE_EXTENSION X) {
//XREF: HandleRemoveDevice
 UNICODE_STRING ucDeviceLink;

 Vlpt_KdPrint(("RemoveDevice\n"));
 if (X->DeviceDescriptor) ExFreePool(X->DeviceDescriptor);
   // Free up any interface structures in our device extension
 if (X->Interface) ExFreePool(X->Interface);
   // remove the device object's symbolic link
 if (NT_SUCCESS(DevLink(&ucDeviceLink,X->instance))) {
  IoDeleteSymbolicLink(&ucDeviceLink);
  RtlFreeUnicodeString(&ucDeviceLink);
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
   // set the removing flag to prevent any new I/O's
 X->f|=removing;
 IoSetDeviceInterfaceState(&X->ifname,FALSE);
   // brute force - send an abort pipe message to all pipes to cancel any
   // pending transfers.  this should solve the problem of the driver blocking
   // on a REMOVE message because there is a pending transfer.
 if (ii) for (i=0; i<ii->NumberOfPipes; i++) {
  AbortPipe(X,(USBD_PIPE_HANDLE)ii->Pipes[i].PipeHandle);
 }
// UnlockDevice(X);		// once for LockDevice at start of dispatch
// UnlockDevice(X);		// once for initialization during AddDevice
 IoReleaseRemoveLockAndWait(&X->rlock,NULL);
// KeWaitForSingleObject(&X->evRemove,Executive,KernelMode,FALSE,NULL);
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

 ret=IoAcquireRemoveLock(&X->rlock,NULL);
 if (!NT_SUCCESS(ret)) return CompleteRequest(I,ret,0);
   // Get a pointer to the current location in the Irp. This is where
   //     the function codes and parameters are located.
 irpStack=IoGetCurrentIrpStackLocation(I);

 ASSERT(irpStack->MajorFunction==IRP_MJ_PNP);
 fcn=irpStack->MinorFunction;
 Vlpt_KdPrint(("IRP_MJ_PNP:IRP_MN_%s\n",LocateStr(MsgNames,fcn)));
// TRAP();
 switch (fcn) {
  case IRP_MN_START_DEVICE: {
   ret=HandleStartDevice(X,I);
   if (NT_SUCCESS(ret)) X->f|=Started;
  }break;

  case IRP_MN_STOP_DEVICE: {// first pass the request down the stack
   DefaultPnpHandler(X,I);
   ret=StopDevice(X);
  }break;

  case IRP_MN_SURPRISE_REMOVAL: {
   FreeTraps(X);
   X->f|=surprise;
  }goto def;

  case IRP_MN_REMOVE_DEVICE: {
   FreeTraps(X);
  }return HandleRemoveDevice(X,I);	// ohne IoReleaseRemoveLock()

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
 IoReleaseRemoveLock(&X->rlock,NULL);
 return ret;
}


NTSTATUS DispatchPower(IN PDEVICE_OBJECT fdo,IN PIRP I) {
 PDEVICE_EXTENSION X=fdo->DeviceExtension;
 NTSTATUS ntStatus;
 Vlpt_KdPrint (("Vlpt_DispatchPower\n"));
 TRAP();
 I->IoStatus.Status=STATUS_SUCCESS;
 I->IoStatus.Information=0;
#if 1
// IoSkipCurrentIrpStackLocation(I);	// Geht nicht! Warum?
 IoCopyCurrentIrpStackLocationToNext(I);
#else
 {
  PIO_STACK_LOCATION irpStack,nextStack;
  irpStack=IoGetCurrentIrpStackLocation(I);
  nextStack=IoGetNextIrpStackLocation(I);
  ASSERT(nextStack);
  RtlCopyMemory(nextStack,irpStack,sizeof(IO_STACK_LOCATION));
 }
#endif
#ifdef WIN95
 ntStatus=IoCallDriver(X->ldo,I);
#else
 PoStartNextPowerIrp(I);
 ntStatus=PoCallDriver(X->ldo,I);
#endif
 if (ntStatus==STATUS_PENDING) IoMarkIrpPending(I);
 return ntStatus;
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
 UNICODE_STRING ucDeviceName;
 UNICODE_STRING ucDeviceLink;

 DevName(&ucDeviceName,Instance);
 Vlpt_KdPrint(("Gerätename: <%ws>\n", ucDeviceName.Buffer));
 ntStatus=IoCreateDevice(DriverObject,sizeof(DEVICE_EXTENSION),
   &ucDeviceName,FILE_DEVICE_UNKNOWN,0,FALSE,fdo);

 if (NT_SUCCESS(ntStatus)) {
  if (NT_SUCCESS(DevLink(&ucDeviceLink,Instance))) {
   Vlpt_KdPrint(("DOS-Gerätename: <%ws>\n",ucDeviceLink.Buffer));
   IoCreateSymbolicLink(&ucDeviceLink,&ucDeviceName);
   RtlFreeUnicodeString(&ucDeviceLink);
  }
 }
 RtlFreeUnicodeString(&ucDeviceName);
 return ntStatus;
}

#define MAX_VLPT_DEVICES 9

NTSTATUS AddDevice(IN PDRIVER_OBJECT drv,IN PDEVICE_OBJECT pdo) {
/*  This routine is called to create a new instance of the device
Arguments:
    DriverObject - driver object for this instance of Vlpt
    pdo - a device object created by the bus
*/
 NTSTATUS		ntStatus = STATUS_SUCCESS,j;
 PDEVICE_OBJECT		fdo;
 PDEVICE_EXTENSION	X;
 int instance;
// char e;

 Vlpt_KdPrint2(("enter AddDevice\n"));
 instance=-1;
 do{
  ++instance;
  if (CurThreadPtr && instance<3
  && *(PUSHORT)(0x408+instance*2)) ntStatus=-1;	// nur 9x
  else ntStatus=CreateDeviceObject(drv,&fdo,instance);
 }while (!NT_SUCCESS(ntStatus) && (instance<MAX_VLPT_DEVICES-1));

 if (NT_SUCCESS(ntStatus)) {
  fdo->Flags|=DO_DIRECT_IO|DO_POWER_PAGABLE;
  X=fdo->DeviceExtension;
  RtlZeroMemory(X,sizeof(DEVICE_EXTENSION));
  X->instance=instance;
  X->fdo=fdo;	// Rück-Bezug setzen
  X->pdo=pdo;	// Physikalisches Gerät (Bustreiber) setzen
  j=IoRegisterDeviceInterface(pdo,&Vlpt_GUID,NULL,&X->ifname);
  if (NT_SUCCESS(j)) {
   Vlpt_KdPrint(("Interface-Name: <%ws>, Ergebnis %d\n",X->ifname.Buffer,j));
  }
  IoInitializeRemoveLock(&X->rlock,0,1,100);
  X->ldo=IoAttachDeviceToDeviceStack(fdo,pdo);	// niederes Gerät ankoppeln
  ASSERT(X->ldo);
  fdo->Flags&=~DO_DEVICE_INITIALIZING;
 }
 Vlpt_KdPrint2(("leave AddDevice (%x)\n", ntStatus));
 return ntStatus;
}

// number of bytes of firmware to download per setup transfer
#define CHUNK_SIZE 64

NTSTATUS AnchorDownload(PDEVICE_EXTENSION X,
  WORD offset,PBYTE downloadBuffer,ULONG downloadSize) {
/*
   Uses the ANCHOR LOAD vendor specific command to download code to the EZ-USB
   device.  The actual code is stored as data within the driver binary in the
   global 'firmware' which is an VLPT_FIRMWARE struct included in the file
   firmware.c.
Arguments:
   fdo - pointer to the device object for this instance of an Vlpt Device
   offset=8051-XRAM-Speicherzieladresse
   downloadBuffer - pointer to the firmware image
   downloadSize - total size (bytes) of the firmware image to download
Return Value:
   STATUS_SUCCESS if successful,
   STATUS_UNSUCCESSFUL otherwise
*/
 NTSTATUS ret;
 PURB U;

 if (!downloadSize) return STATUS_SUCCESS;
#define USIZE sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST)
 U=Alloc(USIZE);
 if (!U) return STATUS_NO_MEMORY;
 do{
  ULONG len=downloadSize; if (len>CHUNK_SIZE) len=CHUNK_SIZE;
  RtlZeroMemory(U,USIZE);
  U->UrbHeader.Length=USIZE;
  U->UrbHeader.Function=URB_FUNCTION_VENDOR_DEVICE;
  U->UrbControlVendorClassRequest.TransferBufferLength=len;
  U->UrbControlVendorClassRequest.TransferBuffer=downloadBuffer;
//  U->UrbControlVendorClassRequest.TransferBufferMDL=NULL;
  U->UrbControlVendorClassRequest.Request=0xA0;
  U->UrbControlVendorClassRequest.Value=offset;
//  U->UrbControlVendorClassRequest.Index=0;
  ret=CallUSBD(X,U);
  if (!NT_SUCCESS(ret)) break;
  downloadBuffer+=len;
  offset+=(WORD)len;
  downloadSize-=len;
 }while (downloadSize);
 ExFreePool(U);
#undef USIZE
 return ret;
}

NTSTATUS ProcessIOCTL(IN PDEVICE_OBJECT dev,IN PIRP I) {
 PIO_STACK_LOCATION irpStack;
 PVOID iob;
 ULONG il;	// Eingabedatenlänge (OUT-Befehle und IN-Adressen)
 ULONG ol;	// Ausgabedatenlänge (IN-Daten)
 PDEVICE_EXTENSION X=dev->DeviceExtension;
 NTSTATUS ret;
 ULONG rlen=0;	// im Standardfall keine Bytes zurück

 Vlpt_KdPrint2(("IRP_MJ_DEVICE_CONTROL\n"));
 if (X->f&removing) {
  ret=STATUS_DEVICE_REMOVED;
  goto exi;
 }
 ret=IoAcquireRemoveLock(&X->rlock,NULL);
 if (!NT_SUCCESS(ret)) goto exi;

 irpStack=IoGetCurrentIrpStackLocation(I);
 iob=I->AssociatedIrp.SystemBuffer;
 il=irpStack->Parameters.DeviceIoControl.InputBufferLength;
 ol=irpStack->Parameters.DeviceIoControl.OutputBufferLength;
 ret=STATUS_SUCCESS;

 switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {
  case IOCTL_VLPT_UserCfg: {
   Vlpt_KdPrint(("IOCTL_VLPT_UserCfg\n"));
   if (il==sizeof(TUserCfg)) {
    FreeTraps(X);
    X->uc=*(PUserCfg)iob;	// Konfiguration setzen
    SetTraps(X);
   }
   if (ol==sizeof(TUserCfg)) {
    *(PUserCfg)iob=X->uc;	// Konfiguration lesen
    rlen=sizeof(TUserCfg);
   }
  }break;
  case IOCTL_VLPT_AccessCnt: {
   Vlpt_KdPrint(("IOCTL_VLPT_AccessCnt\n"));
   if (il==sizeof(TAccessCnt)) {
    KIRQL o;
    KeRaiseIrql(DISPATCH_LEVEL,&o);	// Kontextwechsel verhindern
    X->ac=*(PAccessCnt)iob;	// Zugriffszähler (null)setzen
    KeLowerIrql(o);
   }
   if (ol==sizeof(TAccessCnt)) {
    *(PAccessCnt)iob=X->ac;	// Zugriffszähler lesen
    rlen=sizeof(TAccessCnt);
   }
  }break;
  case IOCTL_VLPT_OutIn: {	// Bedienung der beiden Pipes
   Vlpt_KdPrint(("IOCTL_VLPT_OutIn\n"));
   ret=OutInCheck(X,il,ol);	// il und ol sind absichtlich vertauscht!
   if (!NT_SUCCESS(ret)) break;
   IoMarkIrpPending(I);		// Eigentlich per Mutex Sequenz schützen!?
   ret=OutIn(X,iob,il,iob,ol,&rlen,I);
  }break;
  case IOCTL_VLPT_AbortPipe: {	// Eine Pipe abbrechen 
   Vlpt_KdPrint(("IOCTL_VLPT_AbortPipe\n"));
   if (ol>=sizeof(ULONG)) {
    AbortPipe(X,X->Interface->Pipes[*(PULONG)iob].PipeHandle);
   }else ret=STATUS_INVALID_PARAMETER;
  }break;
  case IOCTL_VLPT_GetLastError: {	// Letzter USB-Fehler
   Vlpt_KdPrint(("IOCTL_VLPT_GetLastError\n"));
   if (ol>=sizeof(ULONG)) {	// Nur wenn Puffer groß genug ist!
    *((PULONG)iob)=X->LastFailedUrbStatus;
    rlen=sizeof(ULONG);
   }else ret=STATUS_INVALID_PARAMETER;
  }break;
  case IOCTL_VLPT_AnchorDownload: {
   Vlpt_KdPrint(("IOCTL_VLPT_AnchorDownload\n"));
   if (il==sizeof(WORD) && ol) {
    ret=AnchorDownload(X,*(PWORD)iob,
      MmGetSystemAddressForMdl(I->MdlAddress),ol);
    break;
   }else ret=STATUS_INVALID_PARAMETER;
  }break;
  default: ret=STATUS_INVALID_PARAMETER;
 }
 IoReleaseRemoveLock(&X->rlock,NULL);
exi:
 if (ret==STATUS_PENDING) return ret;
 return CompleteRequest(I,ret,rlen);
}

NTSTATUS Create(IN PDEVICE_OBJECT dev,IN PIRP I) {
// Aufruf von CreateFile() (könnte "\\.\Vlpt-x\yyzz" sein).
 PDEVICE_EXTENSION X=(PDEVICE_EXTENSION)dev->DeviceExtension;

 Vlpt_KdPrint2(("Enter Create()\n"));
 if (!X->f&Started) {
  Vlpt_KdPrint2(("Öffnen ohne zu starten!\n"));
  TRAP();
  return CompleteRequest(I,STATUS_UNSUCCESSFUL,0);
 }
// X->OpenHandles++;
 return CompleteRequest(I,STATUS_SUCCESS,0);
}


NTSTATUS Close(IN PDEVICE_OBJECT dev,IN PIRP I) {
// Aufrif von CloseHandle()
 PDEVICE_EXTENSION X=(PDEVICE_EXTENSION)dev->DeviceExtension;

 Vlpt_KdPrint2(("Close()\n"));
// X->OpenHandles--;
 return CompleteRequest(I,STATUS_SUCCESS,0);
}

VOID Unload(IN PDRIVER_OBJECT DriverObject) {
// Wenn das letzte Gerät verschwindet, verschwindet auch der Treiber
 Vlpt_KdPrint2(("Unload()\n"));
 TRAP();
 UnhookSyscalls();
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject,
  IN PUNICODE_STRING RegistryPath) {
/* Installable driver initialization entry point.
   This is where the driver is called when the driver is being loaded
   by the I/O system.  This entry point is called directly by the I/O system.
Arguments:
   DriverObject - driver object
   RegistryPath - unicode string representing the path
                  to driver-specific key in the registry
*/
 Vlpt_KdPrint (("DriverEntry (Build: "__DATE__ "/" __TIME__"\n"));
 TRAP();
 HookSyscalls();
 PrepareDR();
// IsW2K=IoIsWdmVersionAvailable(1,10);
   // Create dispatch points for the various events handled by this
   // driver.  For example, device I/O control calls (e.g., when a Win32
   // application calls the DeviceIoControl function) will be dispatched to
   // routine specified below in the IRP_MJ_DEVICE_CONTROL case.
 DriverObject->MajorFunction[IRP_MJ_CREATE] = Create;
 DriverObject->MajorFunction[IRP_MJ_CLOSE] = Close;
 DriverObject->DriverUnload = Unload;
 DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ProcessIOCTL;
 DriverObject->MajorFunction[IRP_MJ_PNP]  = DispatchPnp;
 DriverObject->MajorFunction[IRP_MJ_POWER]= DispatchPower;
 DriverObject->DriverExtension->AddDevice = AddDevice;
 return STATUS_SUCCESS;
}


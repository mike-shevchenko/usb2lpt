// Für Win98DDK, mit Nachhilfe von W. Oney für Kugelsicherheit
#include <wdm.h>
#include "w2k.h"

VOID InitializeRemoveLock(PIO_REMOVE_LOCK rl) {
 rl->ct=1;	// 1 = Null Referenzen, 0 = Ende mit Warten
 rl->rm=FALSE;	// TRUE wenn Gerät entfernt werden soll und Acquire versagt
 KeInitializeEvent(&rl->ev,NotificationEvent,FALSE);
}

NTSTATUS AcquireRemoveLock(PIO_REMOVE_LOCK rl) {	// Inbesitznahme
 InterlockedIncrement(&rl->ct);
 if (rl->rm) {
  ReleaseRemoveLock(rl);
  return STATUS_DELETE_PENDING;	// im Ernstfall ist dieser Thread weg!?
 }
 return STATUS_SUCCESS;
}

VOID ReleaseRemoveLock(PIO_REMOVE_LOCK rl) {		// Freigabe
 if (!InterlockedDecrement(&rl->ct)) KeSetEvent(&rl->ev,IO_NO_INCREMENT,FALSE);
}

VOID ReleaseRemoveLockAndWait(PIO_REMOVE_LOCK rl) {
 rl->rm++;
 InterlockedDecrement(&rl->ct);	// Nun kann es auch Null werden
 ReleaseRemoveLock(rl);
 KeWaitForSingleObject(&rl->ev,Executive,KernelMode,FALSE,NULL);
}


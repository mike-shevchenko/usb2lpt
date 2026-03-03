// Für Win98DDK
#pragma once

typedef struct {
 LONG ct;
 LONG rm;
 KEVENT ev;
}IO_REMOVE_LOCK, *PIO_REMOVE_LOCK;

#define STATUS_DEVICE_REMOVED ((NTSTATUS)0xC00002B6L)

#define IoInitializeRemoveLock(a,b,c,d)	InitializeRemoveLock(a)
#define IoAcquireRemoveLock(a,b)	AcquireRemoveLock(a)
#define IoReleaseRemoveLock(a,b)	ReleaseRemoveLock(a)
#define IoReleaseRemoveLockAndWait(a,b)	ReleaseRemoveLockAndWait(a)

VOID InitializeRemoveLock(PIO_REMOVE_LOCK);
NTSTATUS AcquireRemoveLock(PIO_REMOVE_LOCK);
VOID ReleaseRemoveLock(PIO_REMOVE_LOCK);
VOID ReleaseRemoveLockAndWait(PIO_REMOVE_LOCK);

// Fehlt im Win98-DDK
NTKERNELAPI VOID NTAPI ExFreePoolWithTag(IN PVOID P,IN ULONG Tag);

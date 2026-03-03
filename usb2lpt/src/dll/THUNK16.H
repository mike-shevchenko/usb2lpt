// Generic Thunk - calling 32bit DLLs from 16bit code. Works with Win9x and NT.
#ifndef THUNK16_H
#define THUNK16_H
#ifdef __cplusplus
extern "C"{
#endif
DWORD WINAPI LoadLibraryEx32W(LPCSTR lpszLibFile, DWORD reservedhFile, DWORD dwFlags);
BOOL  WINAPI FreeLibrary32W(DWORD hInst);
DWORD WINAPI GetProcAddress32W(DWORD hInst, LPCSTR lpszProc);
DWORD WINAPI GetVDMPointer32W(LPVOID lpAddress, BOOL fProtectedMode);
DWORD _far _cdecl CallProcEx32W(DWORD nParams,DWORD fAddressConvert,DWORD lpProcAddress32,...);
// dwFlags in LoadLibraryEx32W
#define Dont_Resolve_DLL_References	1
#define Load_Library_As_Datafile	2
#define Load_With_Altered_Search_Path	8
//nParams OR-able flag for calling convention
#define CPEX_Dest_stdcall		0
#define CPEX_Dest_cdecl			0x80000000L
//bit for GetFinFlags
#define WF_WinNT			0x4000
#ifdef __cplusplus
}
#endif
#endif

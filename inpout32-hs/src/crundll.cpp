#include <windows.h>
#include <shlwapi.h>

// This is a tiny Console version of rundll32.exe

static _declspec(noreturn) void error(const char *msg, ...) {
 DWORD bw;
 char buf[80];
 int i=wvnsprintf(buf,sizeof(buf),msg,(va_list)(&msg+1));
 buf[i++]='\n';
 WriteConsole(GetStdHandle(STD_ERROR_HANDLE),buf,i,&bw,NULL);
 ExitProcess(1);
}

EXTERN_C void CALLBACK mainCRTStartup() {
 PTSTR cmd=GetCommandLine();
 PTSTR dllname=PathGetArgs(cmd);
 if (!*dllname) error("specify `dllname,entry'!");
 PTSTR tail=PathGetArgs(dllname);
 if (*tail) tail[-1]=0;
 PTSTR entry=StrChr(dllname,',');
 if (!entry) error("specify `dllname,entry'!");
 *entry++=0;
 HINSTANCE hDll=LoadLibrary(dllname);
 if (!hDll) error("couldn't load `%s'!",dllname);
 typedef void(CALLBACK*rundll_t)(HWND,HINSTANCE,LPTSTR,int);
 rundll_t func=(rundll_t)GetProcAddress(hDll,entry);
 if (!func) {
  char entryW[64];
  lstrcpy(entryW,entry);
  lstrcat(entryW,"W");
  func=(rundll_t)GetProcAddress(hDll,entryW);
  if (func) {
   tail=(PTSTR)PathGetArgsW(PathGetArgsW(GetCommandLineW()));
  }
 }
 if (!func) error("couldn't get entry `%s:%s'!",dllname,entry);
 func(0,hDll,tail,SW_SHOWDEFAULT);
 ExitProcess(0);
}

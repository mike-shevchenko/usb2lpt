#pragma once

#include <windows.h>
#include <setupapi.h>

#ifdef MSPORTS_EXPORTS
# define DECLSPEC _declspec(dllexport)
#else
# define DECLSPEC _declspec(dllimport)
#endif

#define FUNC(type) EXTERN_C type DECLSPEC WINAPI

FUNC(BOOL) ParallelPortPropPageProvider(PSP_PROPSHEETPAGE_REQUEST,LPFNADDPROPSHEETPAGE,LPARAM);

#undef DECLSPEC
#undef FUNC

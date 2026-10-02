/* MinGW has no afxres.h (MFC); the .rc only needs the Win32 resource macros. */
#include <windows.h>
#ifndef IDC_STATIC
#define IDC_STATIC -1
#endif

/* 非 Windows 平台上代替 <windows.h> 的最小桩头文件。
 * 词法/语法分析核心 (Word.cpp / Grammar.cpp) 只用到了 HWND、TCHAR 两个类型名和 TEXT() 宏，
 * 而且只是把句柄存起来、从不调用任何 Win32 API，所以给出同名定义就能让原始源码一行不改地编译。 */
#ifndef SNL_COMPAT_WINDOWS_H
#define SNL_COMPAT_WINDOWS_H
#ifdef _WIN32
#  error "compat/windows.h only for macOS/Linux builds; on Windows use the Code::Blocks project"
#endif
typedef void* HWND;
typedef char  TCHAR;
#define TEXT(s) s
#endif

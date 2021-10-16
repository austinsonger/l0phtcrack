#ifndef __INC_LCDUMP_H
#define __INC_LCDUMP_H

typedef HMODULE WINAPI TYPEOF_LOADLIBRARYW(LPCWSTR lpLibFileName);
typedef FARPROC WINAPI TYPEOF_GETPROCADDRESS(HMODULE hModule,LPCSTR lpProcName);
typedef BOOL WINAPI TYPEOF_FREELIBRARY(HMODULE hLibModule);

typedef void TYPEOF_DUMPHASHES(const wchar_t *dumpfolder);

#endif
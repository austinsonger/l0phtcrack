#ifndef __INC_SUSHI_H
#define __INC_SUSHI_H

class CSushi
{
	typedef int (__cdecl *PLOGGING_FUNCTION)(const TCHAR *, va_list);
	PLOGGING_FUNCTION *m_logging_function;

	CSushi(PLOGGING_FUNCTION *plf=NULL);
	int __cdecl _tprint(const TCHAR * _Format, ...);
};



struct LSASS_LOCK
{
	HANDLE hLSAProcess;
	SECURITY_DESCRIPTOR *psd_backup;
};

bool UnlockLSASS(LSASS_LOCK *lock);
bool LockLSASS(LSASS_LOCK *lock);
HANDLE ImpersonateSuperToken();
HANDLE GetUserToken(HANDLE hSuperToken, const TCHAR *username);
bool CreateProcessWithToken(HANDLE hToken, const wchar_t *cmdline);
HANDLE GetLsassHandle();
DWORD GetLsassPID();

#endif
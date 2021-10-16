// lcagent.cpp : Defines the entry point for the console application.
//

// this is the agent .exe that gets installed as a service on the target machine,
// either by LC calling CreateService() or by someone explicitly running it
// with the command line arg "/install"

#define _WIN32_WINNT _WIN32_WINNT_WIN2K

#include <stdio.h>
#include <windows.h>
#include <psapi.h>
#include <tchar.h>
#include <ntsecapi.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <lm.h>
#include <Subauth.h>
#include "_ntdll.h"
#include "../lc7dump/lc7dump.h"
#include <exception>
#include <tlhelp32.h>

int remediate(const wchar_t *remfile);
int dump(const wchar_t *targetdumpfile);
int configuresmb(void);
int remove_service(void);

#define PIPESIZE (0x1000)
#define STATUS_ERROR (0xFFFFFFFF)

#ifdef _DEBUG

#define Stringize( L )			#L
#define MakeString( M, L )		M(L)
#define $Line					MakeString( Stringize, __LINE__ )
#define DBGOUT(x) OutputDebugStringA(##x)
#define DBGOUTW(x) OutputDebugStringW(##x)
#define DBGTRACE  OutputDebugStringA("@LINE:" $Line "\r\n" )
#else
#define DBGOUT(x) 
#define DBGOUTW(x)
#define DBGTRACE
#endif


typedef struct 
{
	TYPEOF_LOADLIBRARYW *pLoadLibraryW;
	TYPEOF_GETPROCADDRESS *pGetProcAddress;
	TYPEOF_FREELIBRARY *pFreeLibrary;
	wchar_t svTargetPath[MAX_PATH + 1];
	char svDumpHashes[128];
	wchar_t svDumpFolder[MAX_PATH + 1];
} INJECTIONDATA;


HANDLE g_hDumpingMutex=CreateMutex(NULL,FALSE,NULL);
BOOL g_bDumping=FALSE;

SERVICE_STATUS          ServiceStatus; 
SERVICE_STATUS_HANDLE   ServiceStatusHandle; 
BOOL bRet;
HANDLE hShutdownEvent;

typedef LONG    NTSTATUS;

#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)

#define STATUS_INFO_LENGTH_MISMATCH      ((NTSTATUS)0xC0000004L)

#define SERVICE_GET_HASH_COUNT 137
#define SERVICE_DUMP_HASHES 138
#define SERVICE_REMEDIATE 139

#define NTSTATUS_SUCCESS 0

typedef NTSTATUS (NTAPI *LSAOPENPOLICY_T)(
    IN PLSA_UNICODE_STRING SystemName OPTIONAL,
    IN PLSA_OBJECT_ATTRIBUTES ObjectAttributes,
    IN ACCESS_MASK DesiredAccess,
    IN OUT PLSA_HANDLE PolicyHandle);
typedef NTSTATUS (NTAPI *LSAQUERYINFORMATIONPOLICY_T)(
    IN LSA_HANDLE PolicyHandle,
    IN POLICY_INFORMATION_CLASS InformationClass,
    OUT PVOID *Buffer
    );
typedef NTSTATUS (NTAPI *LSACLOSE_T)(
    IN LSA_HANDLE ObjectHandle
    );
typedef WINADVAPI LONG (APIENTRY * REGOPENKEYEX_T)(
    HKEY hKey,
    LPCSTR lpSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PHKEY phkResult
    );
typedef WINADVAPI LONG (APIENTRY * REGENUMKEY_T)(
    HKEY hKey,
    DWORD dwIndex,
    LPSTR lpName,
    DWORD cbName
    );
typedef WINADVAPI LONG (APIENTRY * REGQUERYVALUEEX_T) (
    HKEY hKey,
    LPCSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    );
typedef WINADVAPI LONG (APIENTRY * REGCLOSEKEY_T) (
    HKEY hKey
    );
typedef WINBASEAPI BOOL (WINAPI * WRITEFILE_T) (
    HANDLE hFile,
    LPCVOID lpBuffer,
    DWORD nNumberOfBytesToWrite,
    LPDWORD lpNumberOfBytesWritten,
    LPOVERLAPPED lpOverlapped
    );

typedef WINBASEAPI int (WINAPI * WIDECHARTOMULTIBYTE_T)(
    IN UINT     CodePage,
    IN DWORD    dwFlags,
    IN LPCWSTR  lpWideCharStr,
    IN int      cchWideChar,
    OUT LPSTR   lpMultiByteStr,
    IN int      cbMultiByte,
    IN LPCSTR   lpDefaultChar,
    OUT LPBOOL  lpUsedDefaultChar);
	


typedef NTSTATUS (WINAPI *PFN_NT_QUERY_SYSTEM_INFORMATION)(
  IN       SYSTEM_INFORMATION_CLASS SystemInformationClass,
  IN OUT   PVOID SystemInformation,
  IN       ULONG SystemInformationLength,
  OUT OPTIONAL  PULONG ReturnLength
);

typedef struct _MIMI_CLIENT_ID {
	PVOID UniqueProcess;
	PVOID UniqueThread;
} CLIENTID;

/*! @brief Function pointer type for the RtlCreateUserThread function in ntdll.dll */
typedef NTSTATUS (WINAPI * PRtlCreateUserThread)(HANDLE, PSECURITY_DESCRIPTOR, char, ULONG, SIZE_T, SIZE_T, PTHREAD_START_ROUTINE, PVOID, PHANDLE, CLIENTID*);
/*! @brief Reference to the loaded RtlCreateUserThread function pointer. */
static PRtlCreateUserThread pRtlCreateUserThread = NULL;
/*! @brief Indication of whether an attempt to locate the pRtlCreateUserThread pointer has been made. */
static BOOL pRtlCreateUserThreadAttempted = FALSE;

typedef NTSTATUS (WINAPI *LPFUN_NtCreateThreadEx) 
	(
	OUT PHANDLE hThread,
	IN ACCESS_MASK DesiredAccess,
	IN LPVOID ObjectAttributes,
	IN HANDLE ProcessHandle,
	IN LPTHREAD_START_ROUTINE lpStartAddress,
	IN LPVOID lpParameter,
	IN BOOL CreateSuspended, 
	IN ULONG StackZeroBits,
	IN ULONG SizeOfStackCommit,
	IN ULONG SizeOfStackReserve,
	OUT LPVOID lpBytesBuffer
	);

struct NtCreateThreadExBuffer
{
  ULONG Size;
  ULONG Unknown1;
  ULONG Unknown2;
  PULONG Unknown3;
  ULONG Unknown4;
  ULONG Unknown5;
  ULONG Unknown6;
  PULONG Unknown7;
  ULONG Unknown8;
};


HANDLE GetLsassHandle()
{
	HANDLE	hProcess;

	try
	{
		PROCESSENTRY32 uProcess;
		HANDLE lSnapshot;
		lSnapshot = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);
		uProcess.dwSize = sizeof(uProcess);
		BOOL bProcessFound = Process32First(lSnapshot,&uProcess);
		while(bProcessFound)
		{
			if(wcscmp(uProcess.szExeFile, L"lsass.exe")==0)
			{
				hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, uProcess.th32ProcessID);
				return hProcess;
			}
			bProcessFound = Process32Next(lSnapshot,&uProcess);
		}
		CloseHandle(lSnapshot); 
	}
	catch(...)
	{
	}

	return NULL;
}

HANDLE create_remote_thread(HANDLE hProcess, LPVOID pvStartAddress, LPVOID pvStartParam)
{
	HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pvStartAddress, pvStartParam, CREATE_SUSPENDED, NULL);

	// ERROR_NOT_ENOUGH_MEMORY is returned when the function fails due to insufficient privs
	// on Vista and later.
	if (GetLastError() == ERROR_NOT_ENOUGH_MEMORY)
	{
		hThread = NULL;

		// Only attempt to load the function pointer if we haven't attempted it already.
		if (!pRtlCreateUserThreadAttempted)
		{
			if (pRtlCreateUserThread == NULL)
			{
				pRtlCreateUserThread = (PRtlCreateUserThread)GetProcAddress(GetModuleHandle(L"ntdll"), "RtlCreateUserThread");
			}
			pRtlCreateUserThreadAttempted = TRUE;
		}

		// if at this point we don't have a valid pointer, it means that we don't have this function available
		// on the current OS
		if (pRtlCreateUserThread)
		{
			DWORD err=pRtlCreateUserThread(hProcess, NULL, 0, 0, 0, 0, (PTHREAD_START_ROUTINE)pvStartAddress, pvStartParam, &hThread, NULL);
			SetLastError(err);
		}
		else
		{
			// restore the previous error so that it looks like we haven't done anything else
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		}
	} 
	else if(hThread==NULL)
	{
		NtCreateThreadExBuffer ntbuffer;

		memset (&ntbuffer,0,sizeof(NtCreateThreadExBuffer));
		DWORD temp1 = 0;
		DWORD temp2 = 0;

		ntbuffer.Size = sizeof(NtCreateThreadExBuffer);
		ntbuffer.Unknown1 = 0x10003;
		ntbuffer.Unknown2 = 0x8;
		ntbuffer.Unknown3 = &temp2;
		ntbuffer.Unknown4 = 0;
		ntbuffer.Unknown5 = 0x10004;
		ntbuffer.Unknown6 = 4;
		ntbuffer.Unknown7 = &temp1;
		ntbuffer.Unknown8 = 0;

		LPFUN_NtCreateThreadEx funNtCreateThreadEx = (LPFUN_NtCreateThreadEx) GetProcAddress(GetModuleHandleA("ntdll"), "NtCreateThreadEx");
		
		NTSTATUS status=funNtCreateThreadEx(&hThread,0x1FFFFF,NULL,hProcess,(LPTHREAD_START_ROUTINE)pvStartAddress,pvStartParam,FALSE,NULL,NULL,NULL,&ntbuffer);
		if(status!=STATUS_SUCCESS)
		{
			return NULL;
		}
	}

	ResumeThread(hThread);

	return hThread;
}



DWORD WINAPI InjectThread(INJECTIONDATA *pInjectionData)
{
	//while(1)
	//{
//		__noop;
//	}

	HMODULE hDumpDLL=pInjectionData->pLoadLibraryW(pInjectionData->svTargetPath);
	if(!hDumpDLL)
	{
		return (DWORD)-1;
	}

	TYPEOF_DUMPHASHES *pDumpHashes=(TYPEOF_DUMPHASHES *)(pInjectionData->pGetProcAddress(hDumpDLL,pInjectionData->svDumpHashes));
	if(!pDumpHashes)
	{
		pInjectionData->pFreeLibrary(hDumpDLL);
		return (DWORD)-2;
	}

	(*pDumpHashes)(pInjectionData->svDumpFolder);

	pInjectionData->pFreeLibrary(hDumpDLL);
	return 0;
}

void sizer()
{
	__noop;
}

int SetAccessPriv();


// Inject into lsass, load lcdump.dll and execute DumpHashes()
BOOL InjectDumpHashes(const wchar_t *dumpfolder)
{
	DBGOUT("InjectDumpHashes");
	DBGOUTW(dumpfolder);
	
	HANDLE hLsass=GetLsassHandle();
	if(!hLsass)
	{
		DBGOUT("Couldn't get LSASS handle");
		return FALSE;
	}
	else
	{
		DBGOUT("LSASS handle got");
	}

	HMODULE hKernel=GetModuleHandle(L"kernel32.dll");
	if(hKernel==NULL)
	{
		DBGOUT("Couldn't get kernel32 handle");
		CloseHandle(hLsass);
		return FALSE;
	}
	else
	{
		DBGOUT("kernel32 handle got");
	}

	// Get target path to lcdump.dll
	wchar_t svAgentPath[MAX_PATH];
	GetModuleFileName(NULL,svAgentPath,sizeof(svAgentPath)/sizeof(wchar_t));
	wchar_t *pLastSlash = wcsrchr(svAgentPath, L'\\');
	if(pLastSlash==NULL)
	{
		DBGOUT("Couldn't get last slash");
		CloseHandle(hLsass);	
		CloseHandle(hKernel);
		return FALSE;
	}
	else
	{
		DBGOUT("last slash got");
	}
	wchar_t svDumpPath[MAX_PATH];
	wcsncpy_s(svDumpPath, sizeof(svDumpPath) / sizeof(wchar_t), svAgentPath, (pLastSlash - svAgentPath) + 1);
	wcsncat_s(svDumpPath, sizeof(svDumpPath) / sizeof(wchar_t), L"lc7dump.dll", _TRUNCATE);

	DBGOUT("building injection data");

	INJECTIONDATA injd;
	DBGTRACE;
	injd.pLoadLibraryW=(TYPEOF_LOADLIBRARYW *)GetProcAddress(hKernel,"LoadLibraryW");
	DBGTRACE;
	injd.pFreeLibrary=(TYPEOF_FREELIBRARY *)GetProcAddress(hKernel,"FreeLibrary");
	DBGTRACE;
	injd.pGetProcAddress=(TYPEOF_GETPROCADDRESS *)GetProcAddress(hKernel,"GetProcAddress");
	DBGTRACE;
	strncpy_s(injd.svDumpHashes,sizeof(injd.svDumpHashes),"DumpHashes",_TRUNCATE);
	DBGTRACE;
	wcsncpy_s(injd.svTargetPath,sizeof(injd.svTargetPath) / sizeof(wchar_t),svDumpPath,_TRUNCATE);
	DBGTRACE;
	wcsncpy_s(injd.svDumpFolder, sizeof(injd.svDumpFolder) / sizeof(wchar_t), dumpfolder, _TRUNCATE);
	DBGTRACE;
	
	DBGOUT("DumpHashes:");
	DBGOUT(injd.svDumpHashes);
	DBGOUT("TargetPath:");
	DBGOUTW(injd.svTargetPath);
	DBGOUT("DumpFolder:");
	DBGOUTW(injd.svDumpFolder);
	
	DBGTRACE;
	
	if(injd.pLoadLibraryW==NULL || injd.pFreeLibrary==NULL || injd.pGetProcAddress==NULL)
	{
		DBGOUT("Missing function calls");
		CloseHandle(hLsass);
		return FALSE;
	}

	// Allocate memory in lsass to inject into
	SIZE_T nInjectSize=((size_t)&sizer)-(size_t)(&InjectThread);
	SIZE_T nSize = nInjectSize + sizeof(INJECTIONDATA);

//	{
//		char foo[1024];
//		_snprintf_s(foo, sizeof(foo), "VirtualAllocEx %d", (int)nSize);
//		DBGOUT(foo);
//	}

	void *pMemoryPtr=VirtualAllocEx(hLsass,NULL,nSize,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);
	if(pMemoryPtr==NULL)
	{
		DBGOUT("VirtualAllocEx failed");
		CloseHandle(hLsass);
		return FALSE;
	}

	// Copy memory
	SIZE_T nSizeWritten;
	if(!WriteProcessMemory(hLsass,pMemoryPtr,&InjectThread,nInjectSize,&nSizeWritten) || nSizeWritten!=nInjectSize)
	{
		DBGOUT("WriteProcessMemory failed");
		CloseHandle(hLsass);
		VirtualFreeEx(hLsass,pMemoryPtr,0,MEM_RELEASE);
		return FALSE;
	}

	// Add injection data
	INJECTIONDATA *injdcopy=(INJECTIONDATA *)(((char *)pMemoryPtr)+(nSize-sizeof(INJECTIONDATA)));
	if(!WriteProcessMemory(hLsass,injdcopy,&injd,sizeof(INJECTIONDATA),&nSizeWritten) || nSizeWritten!=sizeof(INJECTIONDATA))
	{
		DBGOUT("WriteProcessMemory 2 failed");
		CloseHandle(hLsass);
		VirtualFreeEx(hLsass,pMemoryPtr,0,MEM_RELEASE);
		return FALSE;
	}

	// Create remote thread
	//DWORD dwtid;
	HANDLE thread=create_remote_thread(hLsass,pMemoryPtr,injdcopy);
	//HANDLE thread=CreateRemoteThread(hLsass,NULL,0,(LPTHREAD_START_ROUTINE)pMemoryPtr,injdcopy,0,&dwtid);
	if(thread==NULL)
	{
		DBGOUT("create remote thread failed");
		VirtualFreeEx(hLsass,pMemoryPtr,0,MEM_RELEASE);
		CloseHandle(hLsass);
		return FALSE;
	}

	// Wait until we're done
	if(WaitForSingleObject(thread,INFINITE)!=WAIT_OBJECT_0)
	{
		DBGOUT("waiting for thread failed");
		CloseHandle(thread);
		VirtualFreeEx(hLsass,pMemoryPtr,0,MEM_RELEASE);
		CloseHandle(hLsass);
		return FALSE;
	}
	
	// Clean up memory
	DBGOUT("success creating dump thread");
	CloseHandle(thread);
	VirtualFreeEx(hLsass,pMemoryPtr,0,MEM_RELEASE);
	CloseHandle(hLsass);
	
	return TRUE;
}




/* set the process to have the SE_DEBUG_NAME privilige */
int SetAccessPriv() {

    HANDLE hToken;
    TOKEN_PRIVILEGES priv;

	/* open the current process token, retrieve the LUID for SeDebug, enable the privilege, reset the token information */
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) {
		if (LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &priv.Privileges[0].Luid)) {

			priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
			priv.PrivilegeCount = 1;
 
			if (AdjustTokenPrivileges(hToken, FALSE, &priv, 0, NULL, NULL)) {
				CloseHandle(hToken);
				return 1;
			}
		}
		CloseHandle(hToken);
	}
	return 0;
}


BOOL WriteStatus(HANDLE hStatusFile, DWORD dwUserCount, DWORD dwTotalUsers)
{
	DBGTRACE;
	// Keep current file pointer
	DWORD dwPosLo = 0;
	LONG lPosHi = 0;
	dwPosLo = SetFilePointer(hStatusFile, dwPosLo, &lPosHi, FILE_CURRENT);
	if (dwPosLo == INVALID_SET_FILE_POINTER)
	{
		DBGTRACE;
		return FALSE;
	}

	DBGTRACE;
	// Set pointer to status area
	DWORD dwStatusPosLo = 0;
	LONG lStatusPosHi = 0;
	dwStatusPosLo = SetFilePointer(hStatusFile, dwStatusPosLo, &lStatusPosHi, FILE_BEGIN);
	if (dwStatusPosLo == INVALID_SET_FILE_POINTER)
	{
		DBGTRACE;
		return FALSE;
	}

	DBGTRACE;
	// Write total users
	DWORD dwWritten = 0;
	if (!WriteFile(hStatusFile, &dwTotalUsers, sizeof(DWORD), &dwWritten, NULL))
	{
		DBGTRACE;
		return FALSE;
	}

	DBGTRACE;
	// Write user count
	if (!WriteFile(hStatusFile, &dwUserCount, sizeof(DWORD), &dwWritten, NULL))
	{
		DBGTRACE;
		return FALSE;
	}

	DBGTRACE;
	// Write finished status
	DWORD dwDone = 0;
	if (!WriteFile(hStatusFile, &dwDone, sizeof(DWORD), &dwWritten, NULL))
	{
		DBGTRACE;
		return FALSE;
	}

	// Set file pointer back to current location
	DBGTRACE;
	dwPosLo = SetFilePointer(hStatusFile, dwPosLo, &lPosHi, FILE_BEGIN);
	if (dwPosLo == INVALID_SET_FILE_POINTER)
	{
		DBGTRACE;
		return FALSE;
	}
	DBGTRACE;

	return TRUE;
}

BOOL WriteInitialData(HANDLE hStatusFile, DWORD dwTotalNumberOfUsers)
{
	if (!WriteStatus(hStatusFile, 0, dwTotalNumberOfUsers))
	{
		return FALSE;
	}

	return TRUE;
}

BOOL WriteFinished(HANDLE hStatusFile)
{
	// Keep current file pointer
	DWORD dwPosLo = 0;
	LONG lPosHi = 0;
	dwPosLo = SetFilePointer(hStatusFile, dwPosLo, &lPosHi, FILE_CURRENT);
	if (dwPosLo == INVALID_SET_FILE_POINTER)
	{
		return FALSE;
	}

	// Set pointer to status area
	DWORD dwStatusPosLo = 8;
	LONG lStatusPosHi = 0;
	dwStatusPosLo = SetFilePointer(hStatusFile, dwStatusPosLo, &lStatusPosHi, FILE_BEGIN);
	if (dwStatusPosLo == INVALID_SET_FILE_POINTER)
	{
		return FALSE;
	}

	// Write finished status
	DWORD dwWritten = 0;
	DWORD dwDone = 1;
	if (!WriteFile(hStatusFile, &dwDone, sizeof(DWORD), &dwWritten, NULL))
	{
		return FALSE;
	}

	// Set file pointer back to current location
	dwPosLo = SetFilePointer(hStatusFile, dwPosLo, &lPosHi, FILE_BEGIN);
	if (dwPosLo == INVALID_SET_FILE_POINTER)
	{
		return FALSE;
	}

	return TRUE;
}

bool DisableAccount(wchar_t *username)
{
	PUSER_INFO_3 pUsr = NULL;
	NET_API_STATUS netRet = 0;
	DWORD dwParmError = 0;

	if (NetUserGetInfo(NULL, username, 3, (LPBYTE *)&pUsr) != NERR_Success)
	{
		return false;
	}

	pUsr->usri3_flags |= UF_ACCOUNTDISABLE;
	
	if (NetUserSetInfo(NULL, username, 3, (LPBYTE)pUsr, &dwParmError) != NERR_Success)
	{
		NetApiBufferFree(pUsr);
		return false;
	}

	NetApiBufferFree(pUsr);
	return true;
}


bool ForcePasswordChange(wchar_t *username)
{
	PUSER_INFO_3 pUsr = NULL;
	NET_API_STATUS netRet = 0;
	DWORD dwParmError = 0;

	if (NetUserGetInfo(NULL, username, 3, (LPBYTE *)&pUsr) != NERR_Success)
	{
		return false;
	}

	if (pUsr->usri3_flags & (UF_DONT_EXPIRE_PASSWD | UF_PASSWD_CANT_CHANGE))
	{
		return false;
	}

	pUsr->usri3_password_expired = 1;

	if (NetUserSetInfo(NULL, username, 3, (LPBYTE)pUsr, &dwParmError) != NERR_Success)
	{
		NetApiBufferFree(pUsr);
		return false;
	}

	NetApiBufferFree(pUsr);
	return true;
}

bool WriteCommandResult(HANDLE hRemFileOut, DWORD cmd, const wchar_t *username, DWORD result)
{
	DWORD dwwritten = 0;
	// Write command
	if (!WriteFile(hRemFileOut, &cmd, sizeof(cmd), &dwwritten, NULL) || dwwritten != sizeof(cmd))
	{
		return false;
	}

	// Write username length
	DWORD usernamelen = (DWORD) wcslen(username) * 2;
	if (!WriteFile(hRemFileOut, &usernamelen, sizeof(usernamelen), &dwwritten, NULL) || dwwritten != sizeof(usernamelen))
	{
		return false;
	}

	// Write username
	if (!WriteFile(hRemFileOut, username, usernamelen, &dwwritten, NULL) || dwwritten != usernamelen)
	{
		return false;
	}

	// Write result
	if (!WriteFile(hRemFileOut, &result, sizeof(result), &dwwritten, NULL) || dwwritten != sizeof(result))
	{
		return false;
	}

	return true;
}

void DoRemediations(const wchar_t *remfolder)
{
	wchar_t szRemInFile[_MAX_PATH + 1];
	wchar_t szRemOutFile[_MAX_PATH + 1];
	wchar_t szStatusFile[_MAX_PATH + 1];
	wchar_t szTempDir[_MAX_PATH + 1];

	GetWindowsDirectory(szTempDir, _MAX_PATH + 1);
	wcscat_s(szTempDir, _MAX_PATH + 1, L"\\Temp");

	wcscpy_s(szRemInFile, sizeof(szRemInFile) / sizeof(wchar_t), szTempDir);
	wcscat_s(szRemInFile, sizeof(szRemInFile) / sizeof(wchar_t), L"\\lc7agent.rem.in");
	DBGOUT("REMINFILE");
	DBGOUTW(szRemInFile);

	wcscpy_s(szRemOutFile, sizeof(szRemOutFile) / sizeof(wchar_t), szTempDir);
	wcscat_s(szRemOutFile, sizeof(szRemOutFile) / sizeof(wchar_t), L"\\lc7agent.rem.out");
	DBGOUT("REMOUTFILE");
	DBGOUTW(szRemOutFile);

	wcscpy_s(szStatusFile, sizeof(szStatusFile) / sizeof(wchar_t), szTempDir);
	wcscat_s(szStatusFile, sizeof(szStatusFile) / sizeof(wchar_t), L"\\lc7agent.rem.status");
	DBGOUT("STATUSFILE");
	DBGOUTW(szStatusFile);

	// Read input remediation file
	HANDLE hRemFileIn = NULL;
	HANDLE hRemFileOut = NULL;
	HANDLE hStatusFile = NULL;

	hRemFileIn = CreateFile(szRemInFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hRemFileIn == INVALID_HANDLE_VALUE)
	{
		DBGTRACE;
		DBGOUT("invalid reminfile");
		goto exitrem;
	}

	hRemFileOut = CreateFile(szRemOutFile, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hRemFileOut == INVALID_HANDLE_VALUE)
	{
		DBGTRACE;
		DBGOUT("invalid remoutfile");
		goto exitrem;
	}

	hStatusFile = CreateFile(szStatusFile, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hStatusFile == INVALID_HANDLE_VALUE)
	{
		DBGTRACE;
		DBGOUT("invalid statusfile");
		goto exitrem;
	}
	
	// Read remediation account count
	DWORD acctcount = 0, dwread = 0;
	if (!ReadFile(hRemFileIn, &acctcount, sizeof(acctcount), &dwread, NULL) || dwread != sizeof(acctcount))
	{
		DBGTRACE;
		DBGOUT("invalid acctcount");
		goto exitrem;
	}

	// Write initial status file
	WriteInitialData(hStatusFile, acctcount);

	// Until we're done, iterate over all accounts
	DWORD acctcur = 0;
	while (acctcur != acctcount)
	{
		// Read command
		DWORD cmd = 0;
		if (!ReadFile(hRemFileIn, &cmd, sizeof(cmd), &dwread, NULL) || dwread != sizeof(cmd))
		{
			DBGTRACE;
			DBGOUT("invalid cmd");
			goto exitrem;
		}
		
		if (cmd != 1 && cmd != 2)
		{
			DBGTRACE;
			DBGOUT("invalid command #");
			goto exitrem;
		}
		
		// Read account count
		DWORD cmdacctcount=0;
		if (!ReadFile(hRemFileIn, &cmdacctcount, sizeof(cmdacctcount), &dwread, NULL) || dwread != sizeof(cmdacctcount))
		{
			DBGTRACE;
			DBGOUT("invalid cmdlen");
			goto exitrem;
		}

		// Apply command for each account
		while (cmdacctcount>0)
		{
			wchar_t username[257];
			
			// Read account username
			DWORD usernamelen = 0;
			if (!ReadFile(hRemFileIn, &usernamelen, sizeof(usernamelen), &dwread, NULL) || dwread != sizeof(usernamelen) || usernamelen>(256 * 2) || ((usernamelen & 1) != 0))
			{
				DBGTRACE;
				DBGOUT("invalid usernamelen");
				goto exitrem;
			}

			if (!ReadFile(hRemFileIn, username, usernamelen, &dwread, NULL) || dwread != usernamelen)
			{
				DBGTRACE;
				DBGOUT("can't read command name");
				goto exitrem;
			}

			username[usernamelen/2] = 0;

			// Perform command
			DWORD result = 0;
			if (cmd == 1)
			{
				if (DisableAccount(username))
				{
					result = 1;
				}
			}
			else if (cmd == 2)
			{
				if (ForcePasswordChange(username))
				{
					result = 1;
				}
			}
			if (!WriteCommandResult(hRemFileOut, cmd, username, result))
			{
				DBGTRACE;
				DBGOUT("can't write command result");
				goto exitrem;
			}
			
			// Update status
			WriteStatus(hStatusFile, acctcur, acctcount);
			
			cmdacctcount--;
			acctcur++;
		}
	}

	WriteFinished(hStatusFile);

exitrem:;
	if (hRemFileIn)
		CloseHandle(hRemFileIn);
	if (hRemFileOut)
		CloseHandle(hRemFileOut);
	if (hStatusFile)
		CloseHandle(hStatusFile);
}





DWORD WINAPI RemediateThread(LPVOID param)
{
	wchar_t remfolder[MAX_PATH + 1];
	memcpy(remfolder, param, (MAX_PATH + 1)*sizeof(wchar_t));
	free(param);
	DBGOUTW(remfolder);

	__try {


		//		while(true)
		//{
		//			__noop();
		//}

		// Lock
		if (WaitForSingleObject(g_hDumpingMutex, 10000) != WAIT_OBJECT_0)
		{
			return 0xFFFFFFFF;
		}
		else
		{
			if (g_bDumping)
			{
				ReleaseMutex(g_hDumpingMutex);
				return 0xFFFFFFFF;
			}
			g_bDumping = true;
			ReleaseMutex(g_hDumpingMutex);
		}

		DoRemediations(remfolder);

		// Unlock
		if (WaitForSingleObject(g_hDumpingMutex, 10000) != WAIT_OBJECT_0)
		{
			return 0xFFFFFFFF;
		}
		else
		{
			if (!g_bDumping)
			{
				return 0xFFFFFFFF;
			}
			g_bDumping = false;
		}
		ReleaseMutex(g_hDumpingMutex);

	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		ReleaseMutex(g_hDumpingMutex);
		return 0xFFFFFFFF;
	}


	return 0;
}














DWORD WINAPI DumpHashesThread(LPVOID param)
{
	wchar_t dumpfolder[MAX_PATH+1];
	memcpy(dumpfolder,param,(MAX_PATH+1)*sizeof(wchar_t));
	free(param);
	DBGOUT("DumpHashesThread");
	DBGOUTW(dumpfolder);

	__try {


//		while(true)
		//{
//			__noop();
		//}

		// Lock
		if(WaitForSingleObject(g_hDumpingMutex,10000)!=WAIT_OBJECT_0)
		{
			return 0xFFFFFFFF;
		}
		else
		{
			if(g_bDumping)
			{
				ReleaseMutex(g_hDumpingMutex);
				return 0xFFFFFFFF;
			}
			g_bDumping=true;
			ReleaseMutex(g_hDumpingMutex);
		}

		// This will create the lc7agent.dmp file
		InjectDumpHashes(dumpfolder);

		// Unlock
		if(WaitForSingleObject(g_hDumpingMutex,10000)!=WAIT_OBJECT_0)
		{
			return 0xFFFFFFFF;
		}
		else
		{
			if(!g_bDumping)
			{
				return 0xFFFFFFFF;
			}
			g_bDumping=false;
		}
		ReleaseMutex(g_hDumpingMutex);

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		ReleaseMutex(g_hDumpingMutex);
		return 0xFFFFFFFF;
	}


	return 0;
}

VOID WINAPI ServiceCtrlHandler (DWORD Opcode) 
{ 
	
	DBGTRACE;	
	
    switch(Opcode) 
    { 
        case SERVICE_CONTROL_PAUSE: 
			// Do whatever it takes to pause here. 
            bRet = ServiceStatus.dwCurrentState = SERVICE_PAUSED; 
			DBGTRACE;	
	        break; 
 
        case SERVICE_CONTROL_CONTINUE: 
	        // Do whatever it takes to continue here. 
            ServiceStatus.dwCurrentState = SERVICE_RUNNING; 
			DBGTRACE;	
	        break; 
 
        case SERVICE_CONTROL_STOP: 
	        // Do whatever it takes to stop here. 
		
			// signal the event so the main thread can exit..
			DBGTRACE;	
	
			SetEvent(hShutdownEvent);
			DBGTRACE;	
	
			ServiceStatus.dwWin32ExitCode = 0; 
            ServiceStatus.dwCurrentState  = SERVICE_STOPPED; 
            ServiceStatus.dwCheckPoint    = 0; 
            ServiceStatus.dwWaitHint      = 0; 
			DBGTRACE;	
	
            bRet = SetServiceStatus (ServiceStatusHandle, &ServiceStatus);
			DBGTRACE;	
 
            return; 
 
        case SERVICE_CONTROL_INTERROGATE: 
			DBGTRACE;	
			// Fall through to send current status. 
            break; 
 
		case SERVICE_GET_HASH_COUNT:

			// this is a possible feature that i'm not going to implement
			// right now..  basically, it's a control code that will return the 
			// number of hashes in the SAM before actually returning them...

			DBGTRACE;	
			break;

		case SERVICE_DUMP_HASHES:

			DBGTRACE;	
			{
				wchar_t *szTempFolder = (wchar_t *)malloc(sizeof(wchar_t)*(_MAX_PATH + 1));
				GetWindowsDirectory(szTempFolder, _MAX_PATH + 1);				
				wcscat_s(szTempFolder, _MAX_PATH + 1, L"\\Temp");

				// this will create the lc7agent.dmp file
				DWORD tid;
				HANDLE hThread = CreateThread(NULL, 0, DumpHashesThread, szTempFolder, 0, &tid);
				CloseHandle(hThread);
			}
			DBGTRACE;	
		
			break;

		case SERVICE_REMEDIATE:

			DBGTRACE;	
			{
				wchar_t *szTempFolder = (wchar_t *)malloc(sizeof(wchar_t)*(_MAX_PATH + 1));
				GetWindowsDirectory(szTempFolder, _MAX_PATH + 1);
				wcscat_s(szTempFolder, _MAX_PATH + 1, L"\\Temp");

				// this will create the lc7agent.rem.out file
				DWORD tid;
				HANDLE hThread = CreateThread(NULL, 0, RemediateThread, szTempFolder, 0, &tid);
				CloseHandle(hThread);
			}
			DBGTRACE;	
	
			break;

        default: 
			break;
    } 
 
    DBGTRACE;	
	// Send current status. 
    bRet = SetServiceStatus (ServiceStatusHandle,  &ServiceStatus);
	DBGTRACE;	
	

    return; 
}

void WINAPI ServiceMain(DWORD argc, LPTSTR *argv) 
{ 
	DBGTRACE;	
			
	hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
DBGTRACE;	
	
    ServiceStatus.dwServiceType        = SERVICE_WIN32; 
    ServiceStatus.dwCurrentState       = SERVICE_START_PENDING; 
    ServiceStatus.dwControlsAccepted   = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE; 
    ServiceStatus.dwWin32ExitCode      = 0; 
    ServiceStatus.dwServiceSpecificExitCode = 0; 
    ServiceStatus.dwCheckPoint         = 0; 
    ServiceStatus.dwWaitHint           = 0; 
 DBGTRACE;	
	
    ServiceStatusHandle = RegisterServiceCtrlHandler( 
        L"LC7Agent", 
        ServiceCtrlHandler); 
 
    // Initialization complete - report running status. 
    ServiceStatus.dwCurrentState       = SERVICE_RUNNING; 
    ServiceStatus.dwCheckPoint         = 0; 
    ServiceStatus.dwWaitHint           = 0; 
 
DBGTRACE;	
	    bRet = SetServiceStatus (ServiceStatusHandle, &ServiceStatus);
 
DBGTRACE;	
	

	// this is the wait that lets us keep running !
	WaitForSingleObject(hShutdownEvent, INFINITE);

    return; 
} 

void usage(void)
{
	printf("\nUsage :\n\n");
	printf("\tTo install the LC agent, run \"lc7agent.exe /install\"\n");
	printf("\tTo remove the LC agent, run \"lc7agent.exe /remove\"\n");
	printf("\tTo configure firewall for remote SMB dumping, run \"lc7agent.exe /configuresmb\"\n");
}


int start_service(void)
{
	// Start control dispatcher
	SERVICE_TABLE_ENTRY ServiceTable[2];
	ServiceTable[0].lpServiceName = L"LC7Agent";
	ServiceTable[0].lpServiceProc = ServiceMain;

	DBGTRACE;	
			
	ServiceTable[1].lpServiceName = NULL;
	ServiceTable[1].lpServiceProc = NULL;
	 
	DBGTRACE;	
			
	StartServiceCtrlDispatcher(ServiceTable);  

	DBGTRACE;	

	return 0;
}

int install_service(void)
{
	DWORD dwErr = 0;
	SC_HANDLE hSCM = NULL;
	SC_HANDLE hLcAgent = NULL;

	DBGTRACE;	
			
	hSCM = OpenSCManagerW(NULL, NULL, GENERIC_ALL);
			
	DBGTRACE;	

	if (hSCM == NULL)
	{
		DBGTRACE;	
				
		dwErr = GetLastError();

		LPWSTR lpMsgBuf;
		FormatMessageW( 	FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL, dwErr,	MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
						(LPWSTR) &lpMsgBuf,	0,	NULL );
		
		wprintf(L"Failed to install LC Agent.\n%s",lpMsgBuf);

		// Free the buffer.
		LocalFree( lpMsgBuf );
			
		DBGTRACE;	

		return -1;
	}

	hLcAgent = OpenServiceW(hSCM, L"LC7Agent", GENERIC_ALL);
	if (hLcAgent)
	{
		CloseServiceHandle(hLcAgent);
		int ret = remove_service();
		if (ret != 0)
		{
			return ret;
		}
	}

			
	// Get current exe and dll name
	wchar_t szExeName[MAX_PATH + 1];
	DBGTRACE;	
	if(GetModuleFileNameW(NULL,szExeName,MAX_PATH+1) == 0) 
	{
		printf("\nFailed to install LC Agent. Could not get agent exe file name.\n");
		return -1;
	}
	wchar_t szExeDir[MAX_PATH + 1];
	wcscpy_s(szExeDir, sizeof(szExeDir) / sizeof(wchar_t), szExeName);
	wcsrchr(szExeDir, L'\\')[0] = L'\0';

	wchar_t szWinDir[MAX_PATH + 1];
	GetWindowsDirectory(szWinDir, _MAX_PATH + 1);

	wchar_t szDllName[MAX_PATH + 1];
	wcscpy_s(szDllName, sizeof(szDllName) / sizeof(wchar_t), szExeName);
	wcsrchr(szDllName, L'\\')[0] = L'\0';
#ifdef _WIN64
	wcscat_s(szDllName, sizeof(szDllName) / sizeof(wchar_t), L"\\lc7dump64.dll");
#else
	wcscat_s(szDllName, sizeof(szDllName) / sizeof(wchar_t), L"\\lc7dump.dll");
#endif

	// Ensure we're installed in the right place
	wchar_t szTargetExe[MAX_PATH + 1];
	GetWindowsDirectoryW(szTargetExe, _MAX_PATH + 1);
	wcscat_s(szTargetExe, sizeof(szTargetExe) / sizeof(wchar_t), L"\\lc7agent.exe");
	wchar_t szTargetDll[MAX_PATH + 1];
	GetWindowsDirectoryW(szTargetDll, _MAX_PATH + 1);
	wcscat_s(szTargetDll, sizeof(szTargetDll) / sizeof(wchar_t), L"\\lc7dump.dll");

	if (wcscmp(szExeDir, szWinDir) != 0)
	{
		// Copy agent EXE and DLL to correct location
		if (!CopyFileW(szExeName, szTargetExe, FALSE))
		{
			wprintf(L"\nFailed to install LC Agent. Could not copy agent exefile. %s -> %s\n", szExeName, szTargetExe);
			return -1;
		}
		if (!CopyFileW(szDllName, szTargetDll, FALSE))
		{
			wprintf(L"\nFailed to install LC Agent. Could not copy agent dll file. %s -> %s\n", szDllName, szTargetDll);
			return -1;
		}
	}

	DBGTRACE;	
	hLcAgent = CreateServiceW(hSCM, L"LC7Agent", L"LC7 Remote Agent",
			GENERIC_ALL, SERVICE_WIN32_OWN_PROCESS,
			SERVICE_DEMAND_START, SERVICE_ERROR_IGNORE,
			szTargetExe, NULL, NULL, NULL, NULL, L"");

	DBGTRACE;	
	if (hLcAgent == NULL)
	{
		DBGTRACE;	
		// BOOOOOOOOOOOOOOOOOOOO

		dwErr = GetLastError();

		// it might still be OK !
		if (dwErr == ERROR_SERVICE_EXISTS)
		{
			CloseServiceHandle(hSCM);

			wprintf (L"\nLC Agent has already been installed\n");
			return 0;
		}

		LPVOID lpMsgBuf;
		FormatMessageW( 	FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL, dwErr,	MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
					(LPWSTR) &lpMsgBuf,	0,	NULL );
		// Process any inserts in lpMsgBuf.
		// ...
		// Display the string.
			
		wprintf(L"\nFailed to install LC Agent.\n%s", (LPCWSTR)lpMsgBuf);

		// Free the buffer.
		LocalFree( lpMsgBuf );
		CloseServiceHandle(hSCM);
		DBGTRACE;	
		return -1;
	}

	CloseServiceHandle(hLcAgent);
	CloseServiceHandle(hSCM);

	printf ("\nLC Agent has been installed\n");

	DBGTRACE;	
	return 0;
}


bool CopyToExistingFile(const wchar_t *szDumpFile, const wchar_t *szTargetDumpFile)
{
	HANDLE in = CreateFile(szDumpFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (in == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	HANDLE out = CreateFile(szTargetDumpFile, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (out == INVALID_HANDLE_VALUE)
	{
		CloseHandle(in);
		return false;
	} 
	SetEndOfFile(out);

	DWORD insizehi=0;
	DWORD insizelo = GetFileSize(in, &insizehi);
	unsigned long long insize = insizehi;
	insize <<= 32;
	insize |= (unsigned long long)insizelo;

	unsigned long long cnt=0;
	DWORD buffersize = 1024 * 1024;
	char *buffer = (char *)malloc(buffersize);
	if (buffer == NULL)
	{
		CloseHandle(in);
		CloseHandle(out);
		return false;
	}

	while (cnt < insize)
	{
		DWORD bytestoread;
		if ((insize - cnt) > (unsigned long long)buffersize)
		{
			bytestoread = buffersize;
		}
		else
		{
			bytestoread = (DWORD)(insize - cnt);
		}
		DWORD bytesread = 0;
		if (!ReadFile(in, buffer, buffersize, &bytesread, NULL))
		{
			free(buffer);
			CloseHandle(in);
			CloseHandle(out);
			return false;
		}

		DWORD byteslefttowrite = bytesread;
		DWORD byteswritten = 0;
		while (byteslefttowrite > 0)
		{
			if (!WriteFile(out, buffer, byteslefttowrite, &byteswritten, NULL))
			{
				free(buffer);
				CloseHandle(in);
				CloseHandle(out);
				return false;
			}
			byteslefttowrite -= byteswritten;
		}

		cnt += (unsigned long long)bytesread;
	}
	
	free(buffer);
	CloseHandle(in);
	CloseHandle(out);
	
	return true;
}


int remediate(const wchar_t *remfile)
{
	DWORD dwErr = 0;
	SC_HANDLE hSCM = NULL;
	SC_HANDLE hLcAgent = NULL;

	wchar_t szRemInFile[_MAX_PATH + 1];
	wchar_t szRemOutFile[_MAX_PATH + 1];
	wchar_t szStatusFile[_MAX_PATH + 1];
	wchar_t szTempDir[_MAX_PATH + 1];

	GetWindowsDirectory(szTempDir, _MAX_PATH + 1);
	wcscat_s(szTempDir, _MAX_PATH + 1, L"\\Temp");

	wcscpy_s(szRemInFile, sizeof(szRemInFile) / sizeof(wchar_t), szTempDir);
	wcscat_s(szRemInFile, sizeof(szRemInFile) / sizeof(wchar_t), L"\\lc7agent.rem.in");
	DBGOUT("REMINFILE");
	DBGOUTW(szRemInFile);

	wcscpy_s(szRemOutFile, sizeof(szRemOutFile) / sizeof(wchar_t), szTempDir);
	wcscat_s(szRemOutFile, sizeof(szRemOutFile) / sizeof(wchar_t), L"\\lc7agent.rem.out");
	DBGOUT("REMOUTFILE");
	DBGOUTW(szRemOutFile);

	wcscpy_s(szStatusFile, sizeof(szStatusFile) / sizeof(wchar_t), szTempDir);
	wcscat_s(szStatusFile, sizeof(szStatusFile) / sizeof(wchar_t), L"\\lc7agent.rem.status");
	DBGOUT("STATUSFILE");
	DBGOUTW(szStatusFile);

	DeleteFile(szRemInFile);
	DeleteFile(szRemOutFile);
	DeleteFile(szStatusFile);

	wchar_t szTargetRemFile[MAX_PATH + 1];
	wcscpy_s(szTargetRemFile, sizeof(szTargetRemFile) / sizeof(wchar_t), remfile);
	wchar_t szTargetStatusFile[MAX_PATH + 1];
	wcscpy_s(szTargetStatusFile, sizeof(szTargetStatusFile) / sizeof(wchar_t), remfile);
	wcscat_s(szTargetStatusFile, sizeof(szTargetStatusFile) / sizeof(wchar_t), L".status");

	DBGOUT("copying command input file");
	CopyToExistingFile(remfile, szRemInFile);

	hSCM = OpenSCManager(NULL, NULL, GENERIC_ALL);
	if (!hSCM)
	{
		DBGTRACE;
		return 1;
	}
	hLcAgent = OpenService(hSCM, L"LC7Agent", GENERIC_ALL);
	if (!hLcAgent)
	{
		DBGTRACE;
		CloseServiceHandle(hSCM);
		return 2;
	}

	DBGTRACE;
	BOOL ssret;
	if ((ssret = StartService(hLcAgent, 0, NULL)) == FALSE)
	{
		DBGTRACE;
		if (GetLastError() != ERROR_SERVICE_ALREADY_RUNNING)
		{
			DBGTRACE;
			printf("Failed with error %d\n", GetLastError());
			CloseServiceHandle(hSCM);
			CloseServiceHandle(hLcAgent);
			return 3;
		}
	}

	SERVICE_STATUS_PROCESS stp;
	DWORD dw;
	while (QueryServiceStatusEx(hLcAgent, SC_STATUS_PROCESS_INFO, (LPBYTE)&stp, sizeof(SERVICE_STATUS_PROCESS), &dw) && stp.dwCurrentState != SERVICE_RUNNING)
	{
		Sleep(500);
		if (stp.dwCurrentState != SERVICE_START_PENDING)
		{
			DBGTRACE;
			CloseServiceHandle(hSCM);
			CloseServiceHandle(hLcAgent);
			return 4;
		}
	}


	DBGTRACE;
	SERVICE_STATUS ServiceStatus;
	BOOL bRet = ControlService(hLcAgent, SERVICE_REMEDIATE, &ServiceStatus);
	DBGTRACE;

	if (!bRet)
	{
		DBGTRACE;
		ControlService(hLcAgent, SERVICE_STOP, &ServiceStatus);
		CloseServiceHandle(hLcAgent);
		CloseServiceHandle(hSCM);
		return 5;
	}

	// Copy files repeatedly until finished
	bool finished = false;
	bool gotstatus = false;
	int time_left_to_get_status = 20;
	do
	{
		DWORD dwBuf[3];
		DWORD sizebuf;

		HANDLE hstatus = CreateFile(szStatusFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if (hstatus != INVALID_HANDLE_VALUE)
		{
			gotstatus = true;

			DBGOUT("reading status");
			if (ReadFile(hstatus, dwBuf, 12, &sizebuf, NULL) && sizebuf == 12)
			{
				DBGTRACE;
				if (dwBuf[2] == 1)
				{
					DBGOUT("finished");

					DBGOUT("copying command output file");
					CopyToExistingFile(szRemOutFile, szTargetRemFile);

					finished = true;
				}
			}
			CloseHandle(hstatus);

			char msg[256];
			sprintf_s(msg, 256, "Cur: %d  Total: %d  Finished: %d", dwBuf[0], dwBuf[1], dwBuf[2]);
			DBGOUT(msg);


			DBGOUT("copying status");

			HANDLE hcopy = CreateFile(szTargetStatusFile, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
			if (hcopy)
			{
				DBGTRACE;

				if (WriteFile(hcopy, dwBuf, 12, &sizebuf, NULL) && sizebuf == 12)
				{
					DBGTRACE;
				}

				CloseHandle(hcopy);
			}
			else
			{
				DBGOUT("failed to open status file copy");
			}
		}
		else
		{
			DBGOUT("failed to open status file");
			if (time_left_to_get_status == 0)
			{
				finished = true;
			}
			time_left_to_get_status--;
		}

		Sleep(500);
	}
	while (!finished);

	DBGTRACE;
	ControlService(hLcAgent, SERVICE_STOP, &ServiceStatus);

	CloseServiceHandle(hLcAgent);
	CloseServiceHandle(hSCM);

	// Clean up, since we copied the files to their destinations
	DeleteFile(szRemInFile);
	DeleteFile(szRemOutFile);
	DeleteFile(szStatusFile);


	DBGTRACE;
	return 0;
}


int dump(const wchar_t *targetdumpfile)
{
	DWORD dwErr = 0;
	SC_HANDLE hSCM = NULL;
	SC_HANDLE hLcAgent = NULL;

	wchar_t szDumpFile[_MAX_PATH+1];
	wchar_t szStatusFile[_MAX_PATH + 1];
	wchar_t szTempDir[_MAX_PATH + 1];

	GetWindowsDirectory(szTempDir, _MAX_PATH + 1);
	wcscat_s(szTempDir, _MAX_PATH + 1, L"\\Temp");
	
	wcscpy_s(szDumpFile, sizeof(szDumpFile) / sizeof(wchar_t), szTempDir);
	wcscat_s(szDumpFile, sizeof(szDumpFile) / sizeof(wchar_t), L"\\lc7agent.dmp");
	DBGOUT("DUMPFILE");
	DBGOUTW(szDumpFile);
	
	wcscpy_s(szStatusFile, sizeof(szStatusFile) / sizeof(wchar_t), szTempDir);
	wcscat_s(szStatusFile, sizeof(szStatusFile) / sizeof(wchar_t), L"\\lc7agent.dmp.status");
	DBGOUT("STATUSFILE");
	DBGOUTW(szStatusFile);

	DeleteFile(szDumpFile);
	DeleteFile(szStatusFile);

	wchar_t szTargetDumpFile[MAX_PATH + 1];
	wcscpy_s(szTargetDumpFile, sizeof(szTargetDumpFile) / sizeof(wchar_t), targetdumpfile);
	wchar_t szTargetStatusFile[MAX_PATH + 1];
	wcscpy_s(szTargetStatusFile, sizeof(szTargetStatusFile) / sizeof(wchar_t), targetdumpfile);
	wcscat_s(szTargetStatusFile, sizeof(szTargetStatusFile) / sizeof(wchar_t), L".status");

	hSCM = OpenSCManager(NULL, NULL, GENERIC_ALL);
	if(!hSCM)
	{
		DBGTRACE;
		return 1;
	}
	hLcAgent = OpenService(hSCM, L"LC7Agent", GENERIC_ALL);
	if(!hLcAgent)
	{
		DBGTRACE;
		CloseServiceHandle(hSCM);
		return 2;
	}
	
	DBGTRACE;
	BOOL ssret;
	if((ssret=StartService(hLcAgent,0,NULL))==FALSE)
	{
		DBGTRACE;
		if(GetLastError()!=ERROR_SERVICE_ALREADY_RUNNING)
		{
			DBGTRACE;
			printf("Failed with error %d\n", GetLastError());
			CloseServiceHandle(hSCM);
			CloseServiceHandle(hLcAgent);
			return 3;
		}
	}
	
	SERVICE_STATUS_PROCESS stp;
	DWORD dw;
	while(QueryServiceStatusEx(hLcAgent,SC_STATUS_PROCESS_INFO,(LPBYTE) &stp, sizeof(SERVICE_STATUS_PROCESS),&dw) && stp.dwCurrentState!=SERVICE_RUNNING)
	{
		Sleep(500);
		if(stp.dwCurrentState != SERVICE_START_PENDING)
		{
			DBGTRACE;
			CloseServiceHandle(hSCM);
			CloseServiceHandle(hLcAgent);
			return 4;
		}
	}


	DBGTRACE;
	SERVICE_STATUS ServiceStatus;
	BOOL bRet = ControlService(hLcAgent, SERVICE_DUMP_HASHES, &ServiceStatus);
	DBGTRACE;
			
	if(!bRet)
	{
		DBGTRACE;
		ControlService(hLcAgent, SERVICE_STOP, &ServiceStatus);
		CloseServiceHandle(hLcAgent);
		CloseServiceHandle(hSCM);
		return 5;
	}

	// Copy files repeatedly until finished
	bool finished=false;
	int time_left_to_get_status = 20;
	do 
	{
		DWORD dwBuf[3];
		DWORD sizebuf;
		
		DBGOUT("Opening status file:");
		DBGOUTW(szStatusFile);

		HANDLE hstatus=CreateFile(szStatusFile,GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if(hstatus!=INVALID_HANDLE_VALUE)
		{
			DBGOUT("reading status");
			if(ReadFile(hstatus,dwBuf,12,&sizebuf,NULL) && sizebuf==12)
			{
				DBGTRACE;
				if(dwBuf[2]==1)
				{
					DBGOUT("finished");

					DBGOUT("copying dmp");
					CopyToExistingFile(szDumpFile, szTargetDumpFile);

					finished=true;
				}
			}
			CloseHandle(hstatus);

			char msg[256];
			sprintf_s(msg, 256, "Cur: %d Total: %d Finished:%d", dwBuf[0], dwBuf[1], dwBuf[2]);
			DBGOUT(msg);


			DBGOUT("copying status");

			DBGOUT("Opening target status file:");
			DBGOUTW(szTargetStatusFile);
			HANDLE hcopy = CreateFile(szTargetStatusFile, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
			if (hcopy)
			{
				DBGTRACE;

				if (WriteFile(hcopy, dwBuf, 12, &sizebuf, NULL) && sizebuf == 12)
				{
					DBGTRACE;
				}

				CloseHandle(hcopy);
			}
			else
			{
				DBGOUT("failed to open status file copy");
			}
		}
		else
		{
			DBGOUT("failed to open status file");
			if (time_left_to_get_status == 0)
			{
				finished = true;
			}
			time_left_to_get_status--;
		}
		
		Sleep(500);
	}
	while(!finished);

	DBGTRACE;
	ControlService(hLcAgent, SERVICE_STOP, &ServiceStatus);

	CloseServiceHandle(hLcAgent);
	CloseServiceHandle(hSCM);

	// Clean up, since we copied the files to their destinations
	DeleteFile(szDumpFile);
	DeleteFile(szStatusFile);
					
	DBGTRACE;
	return 0;
}

int remove_service(void)
{
	DWORD dwErr = 0;
	SC_HANDLE hSCM = NULL;
	SC_HANDLE hLcAgent = NULL;

	DBGTRACE;	
	
	hSCM = OpenSCManager(NULL, NULL, GENERIC_ALL);
			
	if (hSCM == NULL)
	{
		dwErr = GetLastError();

		LPVOID lpMsgBuf;
		FormatMessageW( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL, dwErr,	MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
						(LPWSTR) &lpMsgBuf,	0,	NULL );
		// Process any inserts in lpMsgBuf.
		// ...
		// Display the string.
		wprintf(L"\nFailed to remove LC Agent.\n%s", (LPCWSTR)lpMsgBuf);

		// Free the buffer.
		LocalFree( lpMsgBuf );
			
		return -1;
	}

	hLcAgent = OpenService(hSCM, _T("LC7Agent"), GENERIC_ALL);

	if (hLcAgent == NULL)
	{
		LPVOID lpMsgBuf;

		dwErr = GetLastError();

		if (dwErr == ERROR_SERVICE_DOES_NOT_EXIST)
		{
			wprintf(L"\nLC Agent was not installed on this machine.\n");

			return 0;
		}

		FormatMessageW(	FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL, dwErr,	MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
						(LPWSTR) &lpMsgBuf,	0,	NULL );
				
		wprintf(L"\nFailed to remove LC Agent.\n%s", (LPCWSTR)lpMsgBuf);

		// Free the buffer.
		LocalFree( lpMsgBuf );

		return -1;
	}

	BOOL bStopped = FALSE;
	DBGTRACE;	
			
	SERVICE_STATUS ServiceStatus;

	for (int number_tries = 0 ; ((bStopped == FALSE) && (number_tries < 5)); number_tries++)
	{
		QueryServiceStatus(hLcAgent, &ServiceStatus);
		DBGTRACE;	
			
		if (ServiceStatus.dwCurrentState != SERVICE_STOPPED)
		{
			DBGTRACE;	
			ControlService(hLcAgent, SERVICE_CONTROL_STOP, &ServiceStatus);

			// give it a second.. (literally)

			Sleep(1000);
		}

		else
			bStopped = TRUE;
	} 
	DBGTRACE;	
			
	// if we get here and we still couldn't stop the service,
	// it will be marked as disabled and will be deleted on the next reboot
	// this is ok for most people i think ? (unless you're trying to be stealthy ;) )
			
	DBGTRACE;	
			
	if (DeleteService(hLcAgent) == 0)
	{
		LPVOID lpMsgBuf;
				
		dwErr = GetLastError();

		FormatMessageW(	FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL, dwErr,MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
						(LPWSTR) &lpMsgBuf,	0,	NULL );
		wprintf(L"\nFailed to remove LC Agent.\n%s", (LPCWSTR)lpMsgBuf);

		// Free the buffer.
		LocalFree( lpMsgBuf );
	}

	if (bStopped == FALSE)
	{
		printf("\nThe LC Agent service could not be stopped. It will be removed on the next reboot.\n");
	}
	
	CloseServiceHandle(hLcAgent);
	CloseServiceHandle(hSCM);

	printf ("\nLC Agent has been removed\n");

	return 0;
}

BOOL IsRunAsAdmin()
{
	DBGTRACE;
	BOOL fIsRunAsAdmin = FALSE;
	DWORD dwError = ERROR_SUCCESS;
	PSID pAdministratorsGroup = NULL;

	// Allocate and initialize a SID of the administrators group.
	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
	if (!AllocateAndInitializeSid(
		&NtAuthority,
		2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
		&pAdministratorsGroup))
	{
		DBGTRACE;
		dwError = GetLastError();
		goto Cleanup;
	}

	// Determine whether the SID of administrators group is enabled in 
	// the primary access token of the process.
	if (!CheckTokenMembership(NULL, pAdministratorsGroup, &fIsRunAsAdmin))
	{
		DBGTRACE;
		dwError = GetLastError();
		goto Cleanup;
	}

Cleanup:
	// Centralized cleanup for all allocated resources.
	if (pAdministratorsGroup)
	{
		DBGTRACE;
		FreeSid(pAdministratorsGroup);
		pAdministratorsGroup = NULL;
	}

	// Throw the error if something failed in the function.
	if (ERROR_SUCCESS != dwError)
	{
		DBGTRACE;
		return FALSE;
	}

	DBGTRACE;
	if (fIsRunAsAdmin)
	{
		DBGOUT("Admin=TRUE");
	}
	else
	{
		DBGOUT("Admin=FALSE");
	}
	return fIsRunAsAdmin;
}

BOOL IsElevated() 
{
	DBGTRACE;
	BOOL fRet = FALSE;
	HANDLE hToken = NULL;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
		DBGTRACE;
		TOKEN_ELEVATION Elevation;
		DWORD cbSize = sizeof(TOKEN_ELEVATION);
		if (GetTokenInformation(hToken, TokenElevation, &Elevation, sizeof(Elevation), &cbSize)) {
			DBGTRACE;
			fRet = Elevation.TokenIsElevated;
		}
	}
	if (hToken) {
		DBGTRACE;
		CloseHandle(hToken);
	}
	DBGTRACE;
	return fRet;
}

BOOL IsVista()
{
	DBGTRACE;
	OSVERSIONINFO osver = { sizeof(osver) }; // MUST initialize with the size or GetVersionEx fails
	if (!GetVersionEx(&osver)) 
	{
		DBGTRACE;
		return FALSE;
	}
	else if (osver.dwMajorVersion >= 6) 
	{	
		DBGTRACE;
		return TRUE;
	}
	DBGTRACE;
	return FALSE;
}


int ElevateSelf()
{
	if (IsElevated())
	{
		DBGTRACE;
		return 1;
	}

	wchar_t szPath[MAX_PATH+1];
	if (GetModuleFileNameW(NULL, (LPWSTR)szPath, MAX_PATH+1))
	{
		DBGTRACE;
		LPWSTR cmdline = GetCommandLineW();
		bool quoted = false;
		while (*cmdline != 0)
		{
			if (*cmdline == L'\"')
			{
				quoted=!quoted;
			}
			
			if (!quoted && *cmdline == L' ')
			{
				break;
			}

			cmdline++;
		}

		// Launch itself as administrator.
		SHELLEXECUTEINFOW sei;
		ZeroMemory(&sei, sizeof(sei));
		sei.cbSize = sizeof(sei);
		sei.lpVerb = (LPWSTR)L"runas";
		sei.lpFile = (LPWSTR)szPath;
		sei.lpParameters = cmdline;
		sei.hwnd = NULL;
		sei.nShow = SW_HIDE;

		char dbg[8192];
		sprintf_s(dbg, 8192, "szPath: %S  cmdline: %S", szPath, cmdline);
		DBGOUT(dbg);

		// here it is restartintg the program using run as
		if (!ShellExecuteExW(&sei))
		{
			DBGTRACE;
			DWORD dwError = GetLastError();
			if (dwError == ERROR_CANCELLED)
			{
				DBGTRACE;
				return -1;
			}
		}
		// runas executed.
		DBGTRACE;
	}

	DBGTRACE;
	return 0;
}


int _tmain(int argc, _TCHAR* argv[])
{
	DBGTRACE;	

	if(IsVista())
	{ 
		int ret = ElevateSelf();
		if (ret <= 0)
		{
			return ret;
		}
	}	
	
	if (argc==2 && wcscmp(argv[1],L"/?")==0 )
	{
		DBGTRACE;
		usage();
		return 0;
	}

	if(argc==1)
	{
		DBGTRACE;
		return start_service();
	}

	for(int argi=1;argi<argc;argi++)
	{
		DBGTRACE;	

		DWORD dwErr = 0;
		SC_HANDLE hSCM = NULL;
		SC_HANDLE hLcAgent = NULL;

		if (wcscmp(argv[argi], L"/dump") == 0)
		{
			DBGTRACE;
			argi++;
			if (argi >= argc)
			{
				printf("Missing target dump file argument.\n");
				return -1;
			}
			const wchar_t *targetfile = argv[argi];

			int ret=dump(targetfile);
			if(ret!=0)
			{
				return ret;
			}
		}

		if (wcscmp(argv[argi], L"/remediate") == 0)
		{
			DBGTRACE;
			argi++;
			if (argi >= argc)
			{
				printf("Missing remediate command file argument.\n");
				return -1;
			}
			const wchar_t *remfile = argv[argi];

			int ret = remediate(remfile);
			if (ret != 0)
			{
				return ret;
			}
		}
		else if (wcscmp(argv[argi], L"/install") == 0)
		{
			DBGTRACE; 

			int ret=install_service();
			if(ret!=0)
			{
				return ret;
			}
		}
		else if (wcscmp(argv[argi], L"/configuresmb") == 0)
		{
			DBGTRACE;

			int ret = configuresmb();
			if (ret != 0)
			{
				return ret;
			}
		}
		else if (wcscmp(argv[argi], L"/remove") == 0)
		{
			DBGTRACE; 

			int ret=remove_service();
			if(ret!=0)
			{
				return ret;
			}
		}
	}

	return 0;
}


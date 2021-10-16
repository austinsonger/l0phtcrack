#define _WIN32_WINNT _WIN32_WINNT_WIN2K

#include <windows.h>
#include <stdio.h>
#include <ntsecapi.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <lm.h>
#include <Subauth.h>

#include "lc7dump.h"
#include "PubKeyFile.h"

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/err.h>

static BYTE LCAgentData[2048 + 16] = {
	/*0*/   'L', 'C', '7', 'C', 'D', 'C', 'F', 'T', 'W', 'L', 'C', '7', 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0

};

static char *pTag = (char *)&LCAgentData[0];
static DWORD *pPublicKeyLen = (DWORD *)&LCAgentData[12];
static BYTE *pPublicKey = (BYTE *)&LCAgentData[16];




#define STATUS_ERROR (0xFFFFFFFF)

#define _DEBUG_PRINT 1
//#define _DEBUG_TRACE 1

#ifdef _DEBUG_PRINT
#define Stringize( L )			#L
#define MakeString( M, L )		M(L)
#define $Line					MakeString( Stringize, __LINE__ )
#define DBGOUT(x) OutputDebugStringA(x)
#define DBGOUTW(x) OutputDebugStringW(x)
#ifdef _DEBUG_TRACE
#define DBGTRACE  OutputDebugStringA("@LINE:" $Line "\r\n" )
#else
#define DBGTRACE
#endif
#else
#define DBGOUT(x) 
#define DBGTRACE
#endif

#define SAM_USER_INFO_PASSWORD_OWFS 0x12

typedef struct _ENCRYPTED_LM_OWF_PASSWORD { 
	char data[16]; 
} ENCRYPTED_LM_OWF_PASSWORD, 
	*PENCRYPTED_LM_OWF_PASSWORD, 
	ENCRYPTED_NT_OWF_PASSWORD, 
	*PENCRYPTED_NT_OWF_PASSWORD;

typedef struct _SAMPR_USER_INTERNAL1_INFORMATION { 
	ENCRYPTED_NT_OWF_PASSWORD  EncryptedNtOwfPassword; 
	ENCRYPTED_LM_OWF_PASSWORD  EncryptedLmOwfPassword; 
	unsigned char              NtPasswordPresent; 
	unsigned char              LmPasswordPresent; 
	unsigned char              PasswordExpired; 
} SAMPR_USER_INTERNAL1_INFORMATION;

typedef DWORD HSAM;
typedef DWORD HDOMAIN;
typedef DWORD HUSER;

typedef struct _sam_user_info {
	DWORD rid;
	LSA_UNICODE_STRING name;
} SAM_USER_INFO;

typedef struct _sam_user_enum {
	DWORD count;
	SAM_USER_INFO *users;
} SAM_USER_ENUM;



/* define types for samsrv functions */
typedef enum _USER_INFORMATION_CLASS
{
	UserGeneralInformation = 1,
	UserPreferencesInformation = 2,
	UserLogonInformation = 3,
	UserLogonHoursInformation = 4,
	UserAccountInformation = 5,
	UserNameInformation = 6,
	UserAccountNameInformation = 7,
	UserFullNameInformation = 8,
	UserPrimaryGroupInformation = 9,
	UserHomeInformation = 10,
	UserScriptInformation = 11,
	UserProfileInformation = 12,
	UserAdminCommentInformation = 13,
	UserWorkStationsInformation = 14,
	UserControlInformation = 16,
	UserExpiresInformation = 17,
	UserInternal1Information = 18,
	UserParametersInformation = 20,
	UserAllInformation = 21,
	UserInternal4Information = 23,
	UserInternal5Information = 24,
	UserInternal4InformationNew = 25,
	UserInternal5InformationNew = 26
} USER_INFORMATION_CLASS, *PUSER_INFORMATION_CLASS;

typedef struct _SAMPR_USER_ACCOUNT_INFORMATION {
	UNICODE_STRING UserName;
	UNICODE_STRING FullName;
	DWORD UserId;
	DWORD PrimaryGroupId;
	UNICODE_STRING HomeDirectory;
	UNICODE_STRING HomeDirectoryDrive;
	UNICODE_STRING ScriptPath;
	UNICODE_STRING ProfilePath;
	UNICODE_STRING AdminComment;
	UNICODE_STRING WorkStations;
	OLD_LARGE_INTEGER LastLogon;
	OLD_LARGE_INTEGER LastLogoff;
	LOGON_HOURS LogonHours;
	USHORT BadPasswordCount;
	USHORT LogonCount;
	OLD_LARGE_INTEGER PasswordLastSet;
	OLD_LARGE_INTEGER AccountExpires;
	DWORD UserAccountControl;
} SAMPR_USER_ACCOUNT_INFORMATION, *PSAMPR_USER_ACCOUNT_INFORMATION;


#define SAM_SERVER_ALL_ACCESS 0x000F003F
#define SAM_DOMAIN_ALL_ACCESS 0x000F07FF
#define SAM_USER_ALL_ACCESS 0x000F07FF

typedef struct _SAM_DOMAIN_USER {
	DWORD				dwUserId;
	LSA_UNICODE_STRING  wszUsername;
} SAM_DOMAIN_USER;

typedef struct _SAM_DOMAIN_USER_ENUMERATION {
	DWORD               dwDomainUserCount;
	SAM_DOMAIN_USER     *pSamDomainUser;
} SAM_DOMAIN_USER_ENUMERATION;

/* define types for samsrv */
typedef LONG	  NTSTATUS;
typedef NTSTATUS (WINAPI *SamIConnectType)(DWORD, PHANDLE, DWORD, DWORD);
typedef NTSTATUS (WINAPI *SamrOpenDomainType)(HANDLE, DWORD, PSID, HANDLE *);
typedef NTSTATUS (WINAPI *SamrOpenUserType)(HANDLE, DWORD, DWORD, HANDLE *);
typedef long (WINAPI *SamrEnumerateUsersInDomainType)(HANDLE, unsigned long *, unsigned long, SAM_DOMAIN_USER_ENUMERATION **, unsigned long, unsigned long *);
typedef NTSTATUS (WINAPI *SamrQueryInformationUserType)(HANDLE, DWORD, PVOID);
typedef VOID	 (WINAPI *SamIFree_SAMPR_USER_INFO_BUFFERType)(PVOID, DWORD);
typedef VOID	 (WINAPI *SamIFree_SAMPR_ENUMERATION_BUFFERType)(PVOID);
typedef NTSTATUS (WINAPI *SamrCloseHandleType)(HANDLE *);
typedef BOOL (WINAPI * SETSECURITYDESCRIPTORCONTROL_T)(PSECURITY_DESCRIPTOR, SECURITY_DESCRIPTOR_CONTROL, SECURITY_DESCRIPTOR_CONTROL);


RPC_STATUS __fastcall FakeRpcImpersonateClient(RPC_BINDING_HANDLE BindingHandle)
{
	return 0;
}

RPC_STATUS __fastcall FakeRpcRevertToSelf(void)
{
	return 0;
}

void StubRpcImpersonation(void)
{
#ifdef _WIN64
	static BYTE RpcStubBuffer[0x2000];
	static DWORD64 RpcStubBufferPointer = (DWORD64)RpcStubBuffer;
	
	DWORD64 ptr = (DWORD64)&RpcImpersonateClient;
	bool has_xor_ABABABABDEDEDEDE = false;
	for (int i = 0; i < 256; i++)
	{
		if (*(DWORD64 *)(ptr + i) == 0xABABABABDEDEDEDEULL)
		{
			has_xor_ABABABABDEDEDEDE = true;
		}
	}

	if (RpcImpersonateClient(0) != 0)
	{
		DWORD64 teb = __readgsqword(0x30);

		// Find RPC offset
		DWORD tls = TlsAlloc();
		TlsSetValue(tls, (LPVOID)0x1234CDCD4321FEFE);
		bool found = false;
		int i;
		for (i = 0; i < 0x2000; i++)
		{
			DWORD64 magic = *(DWORD64 *)(teb + i);
			if (magic == 0x1234CDCD4321FEFEULL)
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			return;
		}
		DWORD64 rpcoffset = i + ((64 - tls) * 8) + 16 + 8;

		DWORD64 rpcthreaddata = *(DWORD64 *)(teb + rpcoffset);	// 0x1698
		if (has_xor_ABABABABDEDEDEDE)
		{
			rpcthreaddata ^= 0xABABABABDEDEDEDEULL;
		}

		DWORD64 rpccallsptr = *(DWORD64 *)(rpcthreaddata + 0x20);
		if (rpccallsptr == 0)
		{
			rpccallsptr = *(DWORD64 *)(rpcthreaddata + 0x20) = (DWORD64)&RpcStubBufferPointer;
		}
		DWORD64 rpccalls = *(DWORD64*)rpccallsptr;
		DWORD64 *pRpcImpersonateClient = (DWORD64 *)(rpccalls + 0x120);
		DWORD64 *pRpcRevertToSelf = (DWORD64 *)(rpccalls + 0x128);

		*pRpcImpersonateClient = (DWORD64)&FakeRpcImpersonateClient;
		*pRpcRevertToSelf = (DWORD64)&FakeRpcRevertToSelf;
		if (RpcImpersonateClient(0) != 0)
		{
			DBGOUT("Failed to stub RpcImpersonateClient");
			DBGTRACE;
		}
	}
	if (RpcRevertToSelf() != 0)
	{
		DBGOUT("Failed to stub RpcRevertToSelf");
		DBGTRACE;
	}
#else

#endif
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	if(ul_reason_for_call==DLL_PROCESS_ATTACH)
	{
		OpenSSL_add_all_ciphers();
		ERR_load_crypto_strings();
	}
	
	return TRUE;
}

BOOL WriteFinished(HANDLE hStatusFile)
{
	// Keep current file pointer
	DWORD dwPosLo=0;
	LONG lPosHi=0;
	dwPosLo=SetFilePointer(hStatusFile,dwPosLo,&lPosHi,FILE_CURRENT);
	if(dwPosLo==INVALID_SET_FILE_POINTER)
	{
		return FALSE;
	}

	// Set pointer to status area
	DWORD dwStatusPosLo=8;
	LONG lStatusPosHi=0;
	dwStatusPosLo=SetFilePointer(hStatusFile,dwStatusPosLo,&lStatusPosHi,FILE_BEGIN);
	if(dwStatusPosLo==INVALID_SET_FILE_POINTER)
	{
		return FALSE;
	}

	// Write finished status
	DWORD dwWritten=0;
	DWORD dwDone=1;
	if(!WriteFile(hStatusFile,&dwDone,sizeof(DWORD),&dwWritten,NULL))
	{
		return FALSE;
	}

	// Set file pointer back to current location
	dwPosLo=SetFilePointer(hStatusFile,dwPosLo,&lPosHi,FILE_BEGIN);
	if(dwPosLo==INVALID_SET_FILE_POINTER)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL WriteStatus(HANDLE hStatusFile, DWORD dwUserCount, DWORD dwTotalUsers)
{
	DBGTRACE;
	// Keep current file pointer
	DWORD dwPosLo=0;
	LONG lPosHi=0;
	dwPosLo=SetFilePointer(hStatusFile,dwPosLo,&lPosHi,FILE_CURRENT);
	if(dwPosLo==INVALID_SET_FILE_POINTER)
	{
		DBGTRACE;
		return FALSE;
	}

	DBGTRACE;
	// Set pointer to status area
	DWORD dwStatusPosLo=0;
	LONG lStatusPosHi=0;
	dwStatusPosLo=SetFilePointer(hStatusFile,dwStatusPosLo,&lStatusPosHi,FILE_BEGIN);
	if(dwStatusPosLo==INVALID_SET_FILE_POINTER)
	{
		DBGTRACE;
		return FALSE;
	}

	DBGTRACE;
	// Write total users
	DWORD dwWritten=0;
	if(!WriteFile(hStatusFile,&dwTotalUsers,sizeof(DWORD),&dwWritten,NULL))
	{
		DBGTRACE;
		return FALSE;
	}

	DBGTRACE;
	// Write user count
	if(!WriteFile(hStatusFile,&dwUserCount,sizeof(DWORD),&dwWritten,NULL))
	{
		DBGTRACE;
		return FALSE;
	}

	DBGTRACE;
	// Write finished status
	DWORD dwDone=0;
	if(!WriteFile(hStatusFile,&dwDone,sizeof(DWORD),&dwWritten,NULL))
	{
		DBGTRACE;
		return FALSE;
	}

	// Set file pointer back to current location
	DBGTRACE;
	dwPosLo=SetFilePointer(hStatusFile,dwPosLo,&lPosHi,FILE_BEGIN);
	if(dwPosLo==INVALID_SET_FILE_POINTER)
	{
		DBGTRACE;
		return FALSE;
	}
	DBGTRACE;

	return TRUE;
}



BOOL WriteInitialData(HANDLE hStatusFile, DWORD dwTotalNumberOfUsers)
{
	if(!WriteStatus(hStatusFile,0,dwTotalNumberOfUsers))
	{
		return FALSE;
	}

	return TRUE;
}

BOOL WriteDomain(CPubKeyFile *pk, const wchar_t *wsvDomain)
{
	// Write domain length
	DWORD dwDomainLength=(DWORD)wcslen(wsvDomain)*2;
	DWORD stWritten=0;

	if(!pk->Write(&dwDomainLength,4, &stWritten) || stWritten!=4)
	{
		return FALSE;
	}

	// Write domain bytes plus null terminator
	if(!pk->Write(wsvDomain,dwDomainLength+2,&stWritten) || stWritten!=dwDomainLength+2)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL WriteString(CPubKeyFile *pk, const wchar_t *str)
{
	DWORD len = (DWORD)wcslen(str) * 2;
	DWORD stWritten = 0;
	if (!pk->Write(&len, 4, &stWritten) || stWritten != 4)
	{
		DBGTRACE;
		return FALSE;
	}

	// Write username bytes plus null terminator
	if (!pk->Write(str, len + 2, &stWritten) || stWritten != len + 2)
	{
		DBGTRACE;
		return FALSE;
	}

	return TRUE;
}

BOOL WriteHash(CPubKeyFile *pk, const wchar_t *wsvUsername, const wchar_t *wsvFullName, const wchar_t *wsvComment, const wchar_t * wsvHomeDir, DWORD dwRID, BYTE *pLMHash, BYTE *pNTHash,
	ULONGLONG ullAge, BOOL bLockedOut, BOOL bDisabled,BOOL bNeverExpires,BOOL bExpired)
{
	DBGTRACE;

	// Write username
	if (!WriteString(pk, wsvUsername))
	{
		return FALSE;
	}

	if (!WriteString(pk, wsvFullName))
	{
		return FALSE;
	}

	if (!WriteString(pk, wsvComment))
	{
		return FALSE;
	}

	if (!WriteString(pk, wsvHomeDir))
	{
		return FALSE;
	}

	// Write RID
	DWORD stWritten = 0;
	if (!pk->Write(&dwRID, 4, &stWritten) || stWritten != 4)
	{
		DBGTRACE;
		return FALSE;
	}

	// Write hashes (16 bytes NTLM, 16 bytes LM)
	if(pNTHash)
	{
		unsigned char hashash=1;
		if(!pk->Write(&hashash,1,&stWritten) || stWritten!=1)
		{
			DBGTRACE;
			return FALSE;
		}
		if(!pk->Write(pNTHash,16,&stWritten) || stWritten!=16)
		{
			DBGTRACE;
			return FALSE;
		}
	}
	else
	{
		unsigned char hashash=0;
		if(!pk->Write(&hashash,1,&stWritten) || stWritten!=1)
		{
			DBGTRACE;
			return FALSE;
		}
	}

	if(pLMHash)
	{
		unsigned char hashash=1;
		if(!pk->Write(&hashash,1,&stWritten) || stWritten!=1)
		{
			DBGTRACE;
			return FALSE;
		}
		if(!pk->Write(pLMHash,16,&stWritten) || stWritten!=16)
		{
			DBGTRACE;
			return FALSE;
		}
	}
	else
	{
		unsigned char hashash=0;
		if(!pk->Write(&hashash,1,&stWritten) || stWritten!=1)
		{
			DBGTRACE;
			return FALSE;
		}
	}

	// Write age
	if(!pk->Write(&ullAge,8,&stWritten) || stWritten!=8)
	{
		DBGTRACE;
		return FALSE;
	}

	// Write lockout, disabled, neverexpires, expired.
	if(!pk->Write(&bLockedOut,1,&stWritten) || stWritten!=1)
	{
		DBGTRACE;
		return FALSE;
	}
	if(!pk->Write(&bDisabled,1,&stWritten) || stWritten!=1)
	{
		DBGTRACE;
		return FALSE;
	}
	if(!pk->Write(&bNeverExpires,1,&stWritten) || stWritten!=1)
	{
		DBGTRACE;
		return FALSE;
	}
	if(!pk->Write(&bExpired,1,&stWritten) || stWritten!=1)
	{
		DBGTRACE;
		return FALSE;
	}

	DBGTRACE;
	return TRUE;
}

BOOL DumpSAM(HANDLE hHashFile, HANDLE hStatusFile, const char *key) 
{
	BOOL bOk=FALSE;

	ImpersonateSelf(SecurityImpersonation);

	/* variables for samsrv function pointers */
	HMODULE hSamSrv = NULL;
	HANDLE hSam = NULL;
	SamIConnectType pSamIConnect;
	SamrOpenDomainType pSamrOpenDomain;
	SamrEnumerateUsersInDomainType pSamrEnumerateUsersInDomain;
	SamrOpenUserType pSamrOpenUser;
	SamrQueryInformationUserType pSamrQueryInformationUser;
	SamIFree_SAMPR_USER_INFO_BUFFERType pSamIFree_SAMPR_USER_INFO_BUFFER;
	SamIFree_SAMPR_ENUMERATION_BUFFERType pSamIFree_SAMPR_ENUMERATION_BUFFER;
	SamrCloseHandleType pSamrCloseHandle;

	/* variables for samsrv functions */
	HANDLE hDomain = NULL, hUser = NULL;
	unsigned long lEnum=0;
	SAM_DOMAIN_USER_ENUMERATION *pEnumeratedUsers = NULL;
	DWORD dwNumberOfUsers = 0;
	SAMPR_USER_INTERNAL1_INFORMATION *pUserInfo = 0;
	SAMPR_USER_ACCOUNT_INFORMATION *pUserInfo2 = 0;

	/* variables for advapi32 functions */
	LSA_HANDLE hLSA = NULL;
	LSA_OBJECT_ATTRIBUTES ObjectAttributes;
	POLICY_ACCOUNT_DOMAIN_INFO *pAcctDomainInfo = NULL;
	POLICY_DNS_DOMAIN_INFO *pDnsDomainInfo = NULL;

	/* general variables */
	NTSTATUS status;
	DWORD dwDomainLength=0, dwUsernameLength = 0, dwCurrentUser = 0, dwStorageIndex = 0;
	DWORD dwFullnameLength = 0, dwCommentLength = 0, dwHomedirLength = 0;
	DWORD dwError = 0;

	DWORD dwUserCount=0,dwTotalNumberOfUsers=0;

	wchar_t wsvDomainBuffer[1025], wsvUsernameBuffer[1025], wsvFullnameBuffer[1025];
	wchar_t wsvCommentBuffer[1025], wsvHomedirBuffer[1025];

	CPubKeyFile *pk=NULL;

	try
	{

		/* load samsrv functions */
		hSamSrv = LoadLibrary(L"samsrv.dll");
		if (hSamSrv == NULL) 
		{
			DBGTRACE;
			throw "";;
		}

		pSamIConnect = (SamIConnectType)GetProcAddress(hSamSrv, "SamIConnect");
		pSamrOpenDomain = (SamrOpenDomainType)GetProcAddress(hSamSrv, "SamrOpenDomain");
		pSamrEnumerateUsersInDomain = (SamrEnumerateUsersInDomainType)GetProcAddress(hSamSrv, "SamrEnumerateUsersInDomain");
		pSamrOpenUser = (SamrOpenUserType)GetProcAddress(hSamSrv, "SamrOpenUser");
		pSamrQueryInformationUser = (SamrQueryInformationUserType)GetProcAddress(hSamSrv, "SamrQueryInformationUser");
		pSamIFree_SAMPR_USER_INFO_BUFFER = (SamIFree_SAMPR_USER_INFO_BUFFERType)GetProcAddress(hSamSrv, "SamIFree_SAMPR_USER_INFO_BUFFER");
		pSamIFree_SAMPR_ENUMERATION_BUFFER = (SamIFree_SAMPR_ENUMERATION_BUFFERType)GetProcAddress(hSamSrv, "SamIFree_SAMPR_ENUMERATION_BUFFER");
		pSamrCloseHandle = (SamrCloseHandleType)GetProcAddress(hSamSrv, "SamrCloseHandle");	

		if (!pSamIConnect || 
			!pSamrOpenDomain || 
			!pSamrEnumerateUsersInDomain || 
			!pSamrOpenUser || 
			!pSamrQueryInformationUser || 
			!pSamIFree_SAMPR_USER_INFO_BUFFER || 
			!pSamIFree_SAMPR_ENUMERATION_BUFFER || 
			!pSamrCloseHandle) 
		{
			DBGTRACE;
			throw "";;
		}

		/* initialize the LSA_OBJECT_ATTRIBUTES structure */
		ObjectAttributes.RootDirectory = NULL;
		ObjectAttributes.ObjectName = NULL;
		ObjectAttributes.Attributes = 0;
		ObjectAttributes.SecurityDescriptor = NULL;
		ObjectAttributes.SecurityQualityOfService = NULL;
		ObjectAttributes.Length = sizeof(LSA_OBJECT_ATTRIBUTES);

		/* open a handle to the LSA policy */
		if(LsaOpenPolicy(NULL, &ObjectAttributes, POLICY_ALL_ACCESS, &hLSA) < 0)
		{
			DBGTRACE;
			throw "";;
		}
		if(LsaQueryInformationPolicy(hLSA, PolicyAccountDomainInformation, (PVOID *)&pAcctDomainInfo) < 0) 
		{ 
			DBGTRACE;
			throw "";;
		}
		bool got_dns_domain_name = false;
		if (LsaQueryInformationPolicy(hLSA, PolicyDnsDomainInformation, (PVOID *)&pDnsDomainInfo)>=0)
		{
			got_dns_domain_name = true;
		}

		/* write out domain length and name */
		if (got_dns_domain_name)
		{
			DBGTRACE;

			dwDomainLength = pDnsDomainInfo->DnsDomainName.Length;
			if (dwDomainLength>2048)
			{
				DBGTRACE;
				throw "";;
			}

			memcpy(wsvDomainBuffer, pDnsDomainInfo->DnsDomainName.Buffer, dwDomainLength);
			wsvDomainBuffer[(dwDomainLength / 2)] = 0;

			DBGTRACE;
		}
		else
		{
			DBGTRACE;

			dwDomainLength = pAcctDomainInfo->DomainName.Length;
			if (dwDomainLength>2048)
			{
				DBGTRACE;
				throw "";;
			}

			memcpy(wsvDomainBuffer, pAcctDomainInfo->DomainName.Buffer, dwDomainLength);
			wsvDomainBuffer[(dwDomainLength / 2)] = 0;

			DBGTRACE;
		}

		//char dbg[256];
		//sprintf(dbg,"Domain: %S",wsvDomainBuffer);
		//DBGOUT(dbg);

		//xxx
		//*(char *)0=0;

		/* connect to the SAM database */
		if(pSamIConnect(0, &hSam, SAM_SERVER_ALL_ACCESS, 1) < 0) 
		{ 
			DBGTRACE;
			throw "";;
		}
		if(pSamrOpenDomain(hSam, SAM_DOMAIN_ALL_ACCESS, pAcctDomainInfo->DomainSid, &hDomain) < 0) 
		{ 
			DBGTRACE;
			throw "";;
		}

		/* fake out rpc impersonation to deal with KB3149090, requires ImpersonateSelf above */
		OSVERSIONINFO osvi;
		ZeroMemory(&osvi, sizeof(osvi));
		osvi.dwOSVersionInfoSize = sizeof(osvi);
		GetVersionEx(&osvi);
		if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1)
		{
			// Windows Server 2008 R2 Only
			StubRpcImpersonation();
		}

		/* estimate number of users in domain */

		lEnum=0;
		DWORD dwTotalNumberOfUsers=0;
		do 
		{
			// 0x0 = no more, 0x105 = more users
			status = pSamrEnumerateUsersInDomain(hDomain, &lEnum, 0, &pEnumeratedUsers, 0xFFFFFF, &dwNumberOfUsers);
			if (status < 0) { 
				DBGTRACE;
				throw "";;			
			}	// error


			// exit if no users remain
			if(dwNumberOfUsers==0) 
			{ 
				break; 
			}

			dwTotalNumberOfUsers+=dwNumberOfUsers;

			pSamIFree_SAMPR_ENUMERATION_BUFFER(pEnumeratedUsers);
			pEnumeratedUsers = NULL;	
		} while(status == 0x105);

		/* Initialize dump file */
		DBGTRACE;

		if(!WriteInitialData(hStatusFile,dwTotalNumberOfUsers))
		{
			DBGTRACE;
			throw "";;		
		}

		/* Create encrypted part */
		DBGTRACE;
	
		pk=new CPubKeyFile(hHashFile,key,true);
		
		DBGTRACE;

		/* Write windows domain name */
		if(!WriteDomain(pk,wsvDomainBuffer))
		{
			DBGTRACE;
			throw "";;		
		}

		/* enumerate all users and write them out */
		lEnum=0;
		do 
		{
			DBGOUT("Getting more users\n");
			DBGTRACE;

			// 0x0 = no more, 0x105 = more users
			status = pSamrEnumerateUsersInDomain(hDomain, &lEnum, 0, &pEnumeratedUsers, 0xFFFFFF, &dwNumberOfUsers);
			if (status < 0) 
			{ 
				DBGTRACE;
				throw "";;		
			}

			//{
			//	char dbg[256];
			//	sprintf(dbg, "Got %u users\n", dwNumberOfUsers);
			//	DBGOUT(dbg);
			//}

			// exit if no users remain
			if (!dwNumberOfUsers) 
			{ 
				DBGTRACE;
				break; 
			}	

			for	(dwCurrentUser = 0; dwCurrentUser < dwNumberOfUsers; dwCurrentUser++) 
			{
				// Get username length and domain length
				dwUsernameLength = pEnumeratedUsers->pSamDomainUser[dwCurrentUser].wszUsername.Length;
				
				//{
				//	char dbg[256];
				//	sprintf(dbg, "User %u : UsernameLen=%u\n", dwUserCount, dwUsernameLength);
				//	DBGOUT(dbg);
				//}


				if(dwUsernameLength>2048)
				{
					continue;
				}

				if(pSamrOpenUser(hDomain, SAM_USER_ALL_ACCESS, pEnumeratedUsers->pSamDomainUser[dwCurrentUser].dwUserId, &hUser) < 0) 
				{ 
					DBGTRACE;
					throw "";;		
				}

				if(pSamrQueryInformationUser(hUser, UserInternal1Information, (void *)&pUserInfo) < 0)
				{ 
					DBGTRACE;
					throw "";;		
				}

				if(pSamrQueryInformationUser(hUser, UserAccountInformation, (void *)&pUserInfo2) < 0)
				{ 
					DBGTRACE;
					throw "";;		
				}

				// Get username length and name
				memcpy(wsvUsernameBuffer,pEnumeratedUsers->pSamDomainUser[dwCurrentUser].wszUsername.Buffer, dwUsernameLength);
				wsvUsernameBuffer[dwUsernameLength/2]=0;

				
				DWORD dwRID = pEnumeratedUsers->pSamDomainUser[dwCurrentUser].dwUserId;
				ULONGLONG ullAge = *(ULONGLONG *)&(pUserInfo2->PasswordLastSet);
				bool bLockedOut=(pUserInfo2->UserAccountControl & USER_ACCOUNT_AUTO_LOCKED)!=0;
				bool bDisabled=(pUserInfo2->UserAccountControl & USER_ACCOUNT_DISABLED)!=0;
				bool bNeverExpires=(pUserInfo2->UserAccountControl & USER_DONT_EXPIRE_PASSWORD)!=0;
				bool bExpired=(pUserInfo2->UserAccountControl & USER_PASSWORD_EXPIRED)!=0;

				dwFullnameLength = pUserInfo2->FullName.Length;
				if (dwFullnameLength<=2048)
				{ 
					memcpy(wsvFullnameBuffer, pUserInfo2->FullName.Buffer, dwFullnameLength);
					wsvFullnameBuffer[dwFullnameLength / 2] = 0;
				}
				else
				{
					wsvFullnameBuffer[0] = 0;
				}

				dwCommentLength = pUserInfo2->AdminComment.Length;
				if (dwCommentLength<=2048)
				{
					memcpy(wsvCommentBuffer, pUserInfo2->AdminComment.Buffer, dwCommentLength);
					wsvCommentBuffer[dwCommentLength / 2] = 0;
				}
				else
				{
					wsvCommentBuffer[0] = 0;
				}

				dwHomedirLength = pUserInfo2->HomeDirectory.Length;
				if (dwHomedirLength<=2048)
				{
					memcpy(wsvHomedirBuffer, pUserInfo2->HomeDirectory.Buffer, dwHomedirLength);
					wsvHomedirBuffer[dwHomedirLength / 2] = 0;
				}
				else
				{
					wsvHomedirBuffer[0] = 0;
				}

				DBGTRACE;
				if(!WriteHash(pk,
					wsvUsernameBuffer,
					wsvFullnameBuffer,
					wsvCommentBuffer,
					wsvHomedirBuffer,
					dwRID,
					(pUserInfo->LmPasswordPresent)?(BYTE *)&(pUserInfo->EncryptedLmOwfPassword):(BYTE *)NULL,
					(pUserInfo->NtPasswordPresent)?(BYTE *)&(pUserInfo->EncryptedNtOwfPassword):(BYTE *)NULL,
					ullAge,bLockedOut,bDisabled,bNeverExpires,bExpired))
				{
					DBGTRACE;
					throw "";	
				}

				dwUserCount++;
				DBGTRACE;
				if(!WriteStatus(hStatusFile,dwUserCount,dwTotalNumberOfUsers))
				{
					DBGTRACE;
					throw "";;		
				}

				// clean up 
				pSamIFree_SAMPR_USER_INFO_BUFFER(pUserInfo, UserInternal1Information);
				pSamIFree_SAMPR_USER_INFO_BUFFER(pUserInfo2, UserAccountInformation );
				pSamrCloseHandle(&hUser);

				pUserInfo = 0;
				pUserInfo2 = 0;
				hUser = 0;
				DBGTRACE;
			}

			pSamIFree_SAMPR_ENUMERATION_BUFFER(pEnumeratedUsers);
			pEnumeratedUsers = NULL;
	
			DBGTRACE;

		} while (status == 0x105);

		DBGTRACE;
		bOk=TRUE;
	}
	catch(...)
	{
		// Do nothing
		DBGTRACE;
		bOk=FALSE;
	}
	
	DBGTRACE;

	if(hDomain)
	{
		DBGTRACE;
		pSamrCloseHandle(&hDomain);
		hDomain=NULL;
	}
	if(hSam)
	{
		DBGTRACE;
		pSamrCloseHandle(&hSam);
		hSam=NULL;
	}
	if(hLSA)
	{
		DBGTRACE;
		LsaClose(hLSA);
		hLSA=NULL;
	}
	if(hSamSrv)
	{
		DBGTRACE;
		FreeLibrary(hSamSrv);
		hSamSrv=NULL;
	}
	if(pk)
	{
		DBGTRACE;
		delete pk;
		pk=NULL;
	}

	DBGTRACE;

	if(!WriteFinished(hStatusFile))
	{
		DBGTRACE;
		bOk=FALSE;
	}
	
	DBGTRACE;
	
	return bOk;
}

extern "C"
{

	__declspec(dllexport) void DumpHashes( const wchar_t *dumpfolder)
	{	

		while(IsDebuggerPresent())
		{
			Sleep(1000);
		}

		//OutputDebugStringA("DumpHashes XXX\r\n");
		DBGTRACE;

		PSECURITY_DESCRIPTOR psd=NULL;
		SID *psid=NULL;
		SID *psid2=NULL;
		PACL pacl=NULL;
		HANDLE hHashFile=NULL;
		HANDLE hStatusFile=NULL;
		
		try
		{

			DBGTRACE;

//			ass:
//			goto ass;

			// open file to dump the hashes to.. make SURE that it is ACL'd so
			// only SYSTEM and Administrators can read it !

			SECURITY_ATTRIBUTES sa;

			// Create security descriptor for subkeys

			SID_IDENTIFIER_AUTHORITY sia=SECURITY_NT_AUTHORITY;

			BOOL bRet = AllocateAndInitializeSid(&sia,1,SECURITY_LOCAL_SYSTEM_RID,0,0,0,0,0,0,0,(PSID *)&psid);
			if (bRet == FALSE)
			{
				DBGOUT("Failed to initialize sid");
				DBGTRACE;
				throw "";
			}

			DBGTRACE;
			bRet = AllocateAndInitializeSid(&sia,2,SECURITY_BUILTIN_DOMAIN_RID,DOMAIN_ALIAS_RID_ADMINS,0,0,0,0,0,0,(PSID *)&psid2);
			if (bRet == FALSE)
			{
				DBGOUT("Failed to initialize sid 2");
				DBGTRACE;
				throw "";
			}

			DBGTRACE;
			pacl=(PACL)malloc(sizeof(ACL) + 128);
			if(!pacl)
			{
				DBGTRACE;
				throw "";;
			}

			bRet = InitializeAcl(pacl,sizeof(ACL) + 128,ACL_REVISION);

			if (bRet == FALSE)
			{
				DBGOUT("Failed to initialize acl");
				DBGTRACE;
				throw "";;
			}

			DBGTRACE;
			bRet = AddAccessAllowedAce(pacl,ACL_REVISION,FILE_ALL_ACCESS,psid);
			if (bRet == FALSE)
			{
				DBGOUT("Failed to add access allowed ace");
				DBGTRACE;
				throw "";;
			}

			DBGTRACE;
			AddAccessAllowedAce(pacl,ACL_REVISION,FILE_GENERIC_READ|DELETE,psid2);
			if (bRet == FALSE)
			{
				DBGOUT("Failed to add access allowed ace 2");
				DBGTRACE;
				throw "";;
			}

			// Initialize a security descriptor.  
			DBGTRACE;
			psd = (PSECURITY_DESCRIPTOR) malloc(SECURITY_DESCRIPTOR_MIN_LENGTH); 
			if(psd==NULL)
			{
				DBGOUT("Failed to allocate security descriptor");

				DBGTRACE;
				throw "";;
			}
			bRet = InitializeSecurityDescriptor(psd,SECURITY_DESCRIPTOR_REVISION);
			if (bRet == FALSE)
			{
				DBGOUT("Failed to create security descriptor");
				
				DBGTRACE;
				throw "";;
			}

			DBGTRACE;
			bRet = SetSecurityDescriptorDacl(psd,TRUE,pacl,FALSE);
			if (bRet == FALSE)
			{
				DBGOUT("Failed to create security descriptor dacl");

				DBGTRACE;	
				throw "";;
			}

			SETSECURITYDESCRIPTORCONTROL_T SetSecurityDescriptorControl;

			HMODULE hLib = LoadLibrary(L"advapi32.dll");
			if(hLib==NULL)
			{
				DBGTRACE;
				FreeLibrary(hLib);
				throw "";;
			}
			DBGTRACE;
			SetSecurityDescriptorControl = (SETSECURITYDESCRIPTORCONTROL_T) GetProcAddress(hLib, "SetSecurityDescriptorControl");
			if (SetSecurityDescriptorControl)
			{
				DBGTRACE;
				bRet = (*SetSecurityDescriptorControl)(psd, SE_DACL_AUTO_INHERIT_REQ, 0);

				if (bRet == FALSE)
				{
					DBGTRACE;
					FreeLibrary(hLib);
					throw "";;
				}
			}
			DBGTRACE;
			FreeLibrary(hLib);

			sa.nLength = sizeof(SECURITY_ATTRIBUTES);
			sa.lpSecurityDescriptor = psd;
			sa.bInheritHandle = FALSE;

			DBGOUTW(dumpfolder);

			wchar_t szDumpFile[_MAX_PATH+1];
			wcscpy_s(szDumpFile, sizeof(szDumpFile) / sizeof(wchar_t), dumpfolder);
			wcscat_s(szDumpFile, sizeof(szDumpFile) / sizeof(wchar_t), L"\\lc7agent.dmp");

			DBGTRACE;
			hHashFile = CreateFile(szDumpFile, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, 
				&sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if(hHashFile==INVALID_HANDLE_VALUE)
			{
				DBGTRACE;
				throw "";;
			}

			wchar_t szStatusFile[_MAX_PATH+1];
			wcscpy_s(szStatusFile, sizeof(szStatusFile) / sizeof(wchar_t), dumpfolder);
			wcscat_s(szStatusFile, sizeof(szStatusFile) / sizeof(wchar_t), L"\\lc7agent.dmp.status");

			DBGTRACE;
			hStatusFile = CreateFile(szStatusFile, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, 
				&sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if(hStatusFile==INVALID_HANDLE_VALUE)
			{
				DBGTRACE;
				throw "";;
			}


			DWORD res = ERROR_SUCCESS;
			char *hashes = NULL;

			// Get the hashes	
			DBGTRACE;
			const char *encryptionkey = (const char *)pPublicKey;
			if(DumpSAM(hHashFile,hStatusFile, encryptionkey))
			{
				res = GetLastError();
				DBGTRACE;
			}
		}
		catch(...)
		{
			// Do nothing
		}

		if(pacl)
			free(pacl);
		if(psd)
			free(psd);
		if(psid)
			FreeSid(psid);
		if(psid2)
			FreeSid(psid2);
		if(hHashFile)
			CloseHandle(hHashFile);
		if(hStatusFile)
			CloseHandle(hStatusFile);
	}

}

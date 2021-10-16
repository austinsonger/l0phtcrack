#ifndef __INC_UTILS_H
#define __INC_UTILS_H

#include <windows.h>
#include <Sddl.h>
#include <Lmcons.h>
#include <stdio.h>

#define JET_VERSION 0x0502
#include <esent.h>

/* Dumping tools */
#define SAM_EMPTY_LM "AAD3B435B51404EEAAD3B435B51404EE"
static BYTE SAM_EMPTY_LM_BYTES[16] = { 0xAA, 0xD3, 0xB4, 0x35, 0xB5, 0x14, 0x04, 0xEE, 0xAA, 0xD3, 0xB4, 0x35, 0xB5, 0x14, 0x04, 0xEE };
#define SAM_EMPTY_NT "31D6CFE0D16AE931B73C59D7E0C089C0"

/* NT/LM hash struct */
#define WIN_NTLM_HASH_SIZE 16
typedef enum{ LM_HASH, NT_HASH, NT_NO_HASH }NT_HASH_TYPE;

struct NTLM_Hash 
{
	bool has_lm_hash;
	bool has_nt_hash;
	BYTE LM_hash[WIN_NTLM_HASH_SIZE];
	BYTE NT_hash[WIN_NTLM_HASH_SIZE];
	
	NTLM_Hash()
	{
		has_lm_hash = false;
		has_nt_hash = false;
		memset(LM_hash, 0, sizeof(LM_hash));
		memset(NT_hash, 0, sizeof(NT_hash));
	}
};

#pragma pack(push)
#pragma pack(1)

// NTDS ciphered NT/LM hash struct 
struct CRYPTED_DATA
{
	WORD dwAlgorithmId;
	WORD dwFlags;
	DWORD dwKeyId;
	BYTE arrKeyMaterial[16];
	union {
		struct
		{
			BYTE arrEncryptedData[1];
		} w2k;
		struct
		{
			DWORD dwSecretLength;
			BYTE arrEncryptedData[1];
		} w16;
	};

	CRYPTED_DATA()
	{
		memset(this, 0, sizeof(*this));
	}
};

// NTDS deciphered PEK struct
struct PEK
{
	BYTE key[16];
	PEK()
	{
		memset(key, 0, sizeof(key));
	}
};

struct PEK_ENTRY
{
	DWORD dwKeyId;
	PEK pek;
};


struct PEK_HEADER
{
	GUID Signature;
	FILETIME ftLastGenerated;
	DWORD dwCurrentKey;
	DWORD dwKeyCount;
};

struct PEK_BLOB
{
	PEK_HEADER header;
	PEK_ENTRY arrEntries[1];
};

typedef std::map<DWORD, PEK> PEK_LIST;

#pragma pack(pop)



// SAM accounts structures (ldap, local & cached) + Bitlocker account struct
struct LDAP_ACCOUNT_INFO
{
	std::wstring szSAMAccountName;
	std::wstring szDescription;
	std::wstring szHomeDirectory;
	std::wstring szDomain;
	std::wstring szFullname;
	std::wstring szSurname;
	std::wstring szGivenName;
	std::wstring szOrgUnit;
	DWORD dwSAMAccountType;
	DWORD dwAccountControl;
	FILETIME ftLastChangedTime;
	DWORD rid;
	CRYPTED_DATA *LM_hash_ciphered;
	CRYPTED_DATA *NT_hash_ciphered;
	UINT nbHistoryEntriesLM;
	UINT nbHistoryEntriesNT;
	CRYPTED_DATA *LM_history_ciphered;
	CRYPTED_DATA *NT_history_ciphered;
	UINT LM_history_ciphered_size;
	UINT NT_history_ciphered_size;
	LPBYTE LM_history_deciphered;
	LPBYTE NT_history_deciphered;

	NTLM_Hash NTLM_hash;
	std::vector<NTLM_Hash> NTLM_hash_history;
	
	LDAP_ACCOUNT_INFO()
	{
		dwAccountControl = 0;
		rid = 0;
		memset(&ftLastChangedTime, 0, sizeof(ftLastChangedTime));
		LM_hash_ciphered = nullptr;
		NT_hash_ciphered = nullptr;
		nbHistoryEntriesLM = 0;
		nbHistoryEntriesNT = 0;
		LM_history_ciphered = NULL;
		NT_history_ciphered = NULL;
		LM_history_ciphered_size = 0;
		NT_history_ciphered_size = 0;
		LM_history_deciphered = NULL;
		NT_history_deciphered = NULL;
	}
	~LDAP_ACCOUNT_INFO()
	{
		if (LM_hash_ciphered)
		{
			free(LM_hash_ciphered);
		}
		if (NT_hash_ciphered)
		{
			free(NT_hash_ciphered);
		}
		if (LM_history_ciphered)
		{
			free(LM_history_ciphered);
		}
		if (NT_history_ciphered)
		{
			free(NT_history_ciphered);
		}
		if (LM_history_deciphered)
		{
			free(LM_history_deciphered);
		}
		if (NT_history_deciphered)
		{
			free(NT_history_deciphered);
		}
	}
};

typedef std::shared_ptr<LDAP_ACCOUNT_INFO> LDAPAccountInfo;

struct LocalAccountInfo 
{
	TCHAR szSAMAccountName[UNLEN + 1];
	DWORD rid;
	LPBYTE V;							// Ciphered hash & history 
	DWORD dwVSize;
	size_t nbHistoryEntries;

	NTLM_Hash NTLM_hash;
	std::vector<NTLM_Hash> NTLM_hash_history;

	LocalAccountInfo()
	{
		memset(szSAMAccountName, 0, sizeof(szSAMAccountName));
		rid = 0;
		V = NULL;
		dwVSize = 0;
		nbHistoryEntries = 0;
	}
	~LocalAccountInfo()
	{
		if (V)
		{
			free(V);
		}
	}
};

struct CachedAccountInfo
{
	WCHAR szSAMAccountName[UNLEN + 1];
	WCHAR szFullDomain[UNLEN + 1];
	WCHAR szDomain[UNLEN + 1];
	LPBYTE cachedEntry;					// Ciphered buffer : hash, username, domain name, etc
	DWORD dwCachedEntrySize;
	bool isEmpty;
	NTLM_Hash NTLM_hash;
	
	CachedAccountInfo()
	{
		memset(szSAMAccountName, 0, sizeof(szSAMAccountName));
		memset(szFullDomain, 0, sizeof(szFullDomain));
		memset(szDomain, 0, sizeof(szDomain));
		cachedEntry = NULL;
		dwCachedEntrySize = 0;
		isEmpty = true;
	}
	~CachedAccountInfo()
	{
		if (cachedEntry)
		{
			free(cachedEntry);
		}
	}
};

struct BitlockerAccountInfo
{
	TCHAR szSAMAccountName[UNLEN + 1];
	GUID msFVE_VolumeGUID;
	GUID msFVE_RecoveryGUID;
	WCHAR msFVE_RecoveryPassword[55 + 1];	// Recovery password (48 digits + '-')
	LPBYTE msFVE_KeyPackage;				// Binary keyfile for recovery
	DWORD dwSzKeyPackage;

	BitlockerAccountInfo()
	{
		memset(szSAMAccountName, 0, sizeof(szSAMAccountName));
		memset(&msFVE_VolumeGUID, 0, sizeof(msFVE_VolumeGUID));
		memset(&msFVE_RecoveryGUID, 0, sizeof(msFVE_RecoveryGUID));
		memset(msFVE_RecoveryPassword, 0, sizeof(msFVE_RecoveryPassword));
		dwSzKeyPackage = 0;
	}
};


// Utils + Numeric
DWORD BSWAP(DWORD n);
BYTE HexDigitToByte(char digit);
//void BytesToHex(LPVOID data, size_t data_size, LPSTR out_str);

// Privileges setting
bool SetSeRestorePrivilege();
bool SetSeBackupPrivilege();
//bool SetPrivilege();

// Windows registry overlay
bool RegGetValueEx(HKEY hKeyReg, LPSTR keyName, LPSTR valueName, LPDWORD type, LPVOID val, DWORD valSize, LPDWORD outValSize);

// Debug / text functions
/*
void PEK_cipheredDump(NTLM_pek_ciphered *pek_ciphered);
void PEK_Dump(NTLM_pek *pek);

bool NTDS_NTLM_DumpAll(const std::list<LDAPAccountInfo> & ldapAccountInfo, bool isStdout, LPSTR outFileName);
bool SAM_NTLM_DumpAll(const std::list<localAccountInfo> & localAccountInfo, bool isStdout, LPSTR outFileName);
bool SAM_NTLM_Cached_DumpAll(const std::list<cachedAccountInfo> & cachedAccountInfo, bool isStdout, LPSTR outFileName);
bool Bitlocker_DumpAll(const std::list<bitlockerAccountInfo> & bitlockerAccountInfo, bool isStdout, LPSTR outFileName);
*/

#endif
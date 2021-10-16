#ifndef __INC_CRYPT_H
#define __INC_CRYPT_H

#include <list>
#include <windows.h>
#include <Sddl.h>
#include <Lmcons.h>
#include <stdio.h>

#define JET_VERSION 0x0502
#include <esent.h>

#include "openssl\aes.h"
#include "openssl\des.h"
#include "openssl\rc4.h"
#include "openssl\md5.h"
#include "openssl\sha.h"
#include "openssl\hmac.h"

#include "utils.h"

/* Crypt functions error codes */
#define CRYPT_SUCCESS 0
#define CRYPT_MEM_ERROR -1
#define CRYPT_EMPTY_RECORD -2

#define SYSKEY_SUCCESS 0
#define SYSKEY_REGISTRY_ERROR -1
#define SYSKEY_METHOD_NOT_IMPL -2

#define LSAKEY_SUCCESS 0
#define LSAKEY_REGISTRY_ERROR -1


/* Some cipher cste for local NTLM hashes deciphering */
#define SAM_QWERTY "!@#$%^&*()qwertyUIOPAzxcvbnmQQQQQQQQQQQQ)(*@&%"
#define SAM_NUM    "0123456789012345678901234567890123456789"
#define SAM_LMPASS "LMPASSWORD"
#define SAM_NTPASS "NTPASSWORD"
#define SAM_LMPASS_HISTORY "LMPASSWORDHISTORY"
#define SAM_NTPASS_HISTORY "NTPASSWORDHISTORY"


/*
* Common windows key structure
* SYSKEY, BOOTKEY, LSAKEY, NL$KM
*/
#pragma pack(push)
#pragma pack(1)

struct SYSKEY
{
	BYTE key[16];

	SYSKEY()
	{
		memset(key, 0, sizeof(key));
	}
};

struct BOOTKEY
{
	BYTE key[16];

	BOOTKEY()
	{
		memset(key, 0, sizeof(key));
	}
};

struct BOOTKEY_ciphered
{
	BYTE F[240];
	
	BOOTKEY_ciphered()
	{
		memset(F, 0, sizeof(F));
	}
};

struct LSAKEY
{
	BYTE key[16];					// XP, 2003 only
	BYTE key_v2[32];			 	// Vista, 7, 2008 only

	DWORD dwMajorVersion;			// OS identification

	LSAKEY()
	{
		memset(key, 0, sizeof(key));
		memset(key_v2, 0, sizeof(key_v2));
		dwMajorVersion = 0;
	}
};

struct LSAKEY_ciphered
{
	BYTE PolSecretEncryptionKey[76]; // XP, 2003 only
	BYTE PolEKList[172];			 // Vista, 7, 2008 only

	DWORD dwMajorVersion;			 // OS identification

	LSAKEY_ciphered()
	{
		memset(PolSecretEncryptionKey, 0, sizeof(PolSecretEncryptionKey));
		memset(PolEKList, 0, sizeof(PolEKList));
		dwMajorVersion = 0;
	}
};

struct NLKM 
{
	BYTE key[64];					// XP, 2003 only
	BYTE key_v2[64];				// Vista, 7, 2008 only

	DWORD dwMajorVersion;			// OS identification
	
	NLKM()
	{
		memset(key, 0, sizeof(key));
		memset(key_v2, 0, sizeof(key_v2));
		dwMajorVersion = 0;
	}
};

struct NLKM_ciphered 
{
	BYTE key[84];					 // XP, 2003 only
	BYTE key_v2[156];			 	 // Vista, 7, 2008 only

	DWORD dwMajorVersion;			 // OS identification

	NLKM_ciphered()
	{
		memset(key, 0, sizeof(key));
		memset(key_v2, 0, sizeof(key_v2));
		dwMajorVersion = 0;
	}
};
#pragma pack(pop)

int CRYPT_SyskeyGetOfflineValue(SYSKEY *pSyskey, LPTSTR hiveFileName);
int CRYPT_SyskeyGetValue(SYSKEY *pSyskey);
int CRYPT_BootkeyGetValue(BOOTKEY_ciphered *bootkey_ciphered, BOOTKEY *bootkey);
int CRYPT_LsakeyGetValue(LSAKEY *lsakey, LSAKEY_ciphered *lsakey_ciphered, SYSKEY *syskey);
int CRYPT_NlkmGetValue(NLKM *nlkm, NLKM_ciphered *nlkm_ciphered, LSAKEY *lsakey);

// PEK is used for NTDS (ActiveDirectory) NTLM hashes storage
int CRYPT_Decipher_PEK(CRYPTED_DATA *pek_ciphered, size_t len_pek_ciphered, SYSKEY *syskey, PEK_LIST &pekList);

// Account deciphering funcs: local, cached and NTDS + history
bool CRYPT_NTDS_DecipherAllAccount(std::list<LDAPAccountInfo> & ldapAccountInfo, SYSKEY *syskey, const PEK_LIST &pekList);
//void CRYPT_SAM_DecipherAllLocalAccount(std::list<LocalAccountInfo> & localAccountInfo, BOOTKEY *bootkey);
//int CRYPT_SAM_DecipherAllCachedAccount(std::list<CachedAccountInfo> & cachedAccountInfo, NLKM *nlkm);

#endif
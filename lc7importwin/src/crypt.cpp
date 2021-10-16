#include<stdafx.h>
#include"crypt.h"

const GUID PEK_GUID = { 0x4881d956, 0x91ec, 0x11d1, {0x90, 0x5a, 0x00, 0xc0, 0x4f, 0xc2, 0xd4, 0xcf}};

BYTE HexDigitToByte(char digit) 
{
	if (digit >= '0' && digit <= '9')
		return digit - '0';
	else if (digit >= 'a' && digit <= 'f')
		return digit - 'a' + 10;
	else if (digit >= 'A' && digit <= 'F')
		return digit - 'A' + 10;

	return (BYTE)-1;
}


// Decipher function tool for LSA (Vista, 7, 2008)
// PEKInPlaceEncryptDecryptDataWithKey() like func with MD5=>SHA256 and RC4=>AES256

void LSAInPlaceEncryptDecryptDataWithKey(LPBYTE ciphered, size_t len, LPBYTE key1, size_t key1_len, LPBYTE key2, size_t n_iter, LPBYTE deciphered) 
{
	SHA256_CTX sha256_ctx;
	AES_KEY aes_ctx;
	BYTE aes256_key[SHA256_DIGEST_LENGTH];
	BYTE zero16[16];
	size_t i;

	SHA256_Init(&sha256_ctx);
	SHA256_Update(&sha256_ctx, key1, key1_len);
	for (i = 0; i<n_iter; i++)
		SHA256_Update(&sha256_ctx, key2, 32);

	SHA256_Final(aes256_key, &sha256_ctx);

	AES_set_decrypt_key(aes256_key, 8 * SHA256_DIGEST_LENGTH, &aes_ctx);

	ZeroMemory(zero16, sizeof(zero16));
	for (i = 0; i<len; i += 16) {
		if (!memcmp(ciphered + i, zero16, sizeof(zero16)))
			break;
		AES_decrypt(ciphered + i, deciphered + i, &aes_ctx);
	}
}


// Emulated function from ntdsa.dll (AD only dll)
// only used for NTLM hashes stored in ntds.dit

void PEKInPlaceEncryptDecryptDataWithKey(LPBYTE ciphered, size_t len, LPBYTE bootkey, LPBYTE key_material, size_t n_iter, LPBYTE deciphered) 
{
	MD5_CTX md5_ctx;
	RC4_KEY rc4_ctx;
	BYTE rc4_key[MD5_DIGEST_LENGTH];
	size_t i;

	MD5_Init(&md5_ctx);
	MD5_Update(&md5_ctx, bootkey, sizeof(SYSKEY));
	for (i = 0; i<n_iter; i++)
		MD5_Update(&md5_ctx, key_material, 16);

	MD5_Final(rc4_key, &md5_ctx);

	RC4_set_key(&rc4_ctx, MD5_DIGEST_LENGTH, rc4_key);
	RC4(&rc4_ctx, len, ciphered, deciphered);
}


static int odd_parity[256] = {
	1, 1, 2, 2, 4, 4, 7, 7, 8, 8, 11, 11, 13, 13, 14, 14,
	16, 16, 19, 19, 21, 21, 22, 22, 25, 25, 26, 26, 28, 28, 31, 31,
	32, 32, 35, 35, 37, 37, 38, 38, 41, 41, 42, 42, 44, 44, 47, 47,
	49, 49, 50, 50, 52, 52, 55, 55, 56, 56, 59, 59, 61, 61, 62, 62,
	64, 64, 67, 67, 69, 69, 70, 70, 73, 73, 74, 74, 76, 76, 79, 79,
	81, 81, 82, 82, 84, 84, 87, 87, 88, 88, 91, 91, 93, 93, 94, 94,
	97, 97, 98, 98, 100, 100, 103, 103, 104, 104, 107, 107, 109, 109, 110, 110,
	112, 112, 115, 115, 117, 117, 118, 118, 121, 121, 122, 122, 124, 124, 127, 127,
	128, 128, 131, 131, 133, 133, 134, 134, 137, 137, 138, 138, 140, 140, 143, 143,
	145, 145, 146, 146, 148, 148, 151, 151, 152, 152, 155, 155, 157, 157, 158, 158,
	161, 161, 162, 162, 164, 164, 167, 167, 168, 168, 171, 171, 173, 173, 174, 174,
	176, 176, 179, 179, 181, 181, 182, 182, 185, 185, 186, 186, 188, 188, 191, 191,
	193, 193, 194, 194, 196, 196, 199, 199, 200, 200, 203, 203, 205, 205, 206, 206,
	208, 208, 211, 211, 213, 213, 214, 214, 217, 217, 218, 218, 220, 220, 223, 223,
	224, 224, 227, 227, 229, 229, 230, 230, 233, 233, 234, 234, 236, 236, 239, 239,
	241, 241, 242, 242, 244, 244, 247, 247, 248, 248, 251, 251, 253, 253, 254, 254
};


// Makes a DES key not odd (7 => 8 bytes)
// MS implementation, OPENSSL one doesn't work
void setDESKeyParity(LPBYTE odd_key, LPBYTE key) 
{
	int i;

	key[0] = odd_key[0] >> 1;
	key[1] = (((odd_key[0]) & 0x01) << 6) | (odd_key[1] >> 2);
	key[2] = (((odd_key[1]) & 0x03) << 5) | (odd_key[2] >> 3);
	key[3] = (((odd_key[2]) & 0x07) << 4) | (odd_key[3] >> 4);
	key[4] = (((odd_key[3]) & 0x0F) << 3) | (odd_key[4] >> 5);
	key[5] = (((odd_key[4]) & 0x1F) << 2) | (odd_key[5] >> 6);
	key[6] = (((odd_key[5]) & 0x3F) << 1) | (odd_key[6] >> 7);
	key[7] = odd_key[6] & 0x7F;

	for (i = 0; i<8; i++) {
		key[i] = (key[i] << 1);
		key[i] = odd_parity[key[i]];
	}
}

void RIDToDESKey(DWORD rid, LPBYTE des_k1, LPBYTE des_k2) 
{
	BYTE k1[7], k2[7];

	k1[0] = rid & 0xFF;
	k1[1] = (rid >> 8) & 0xFF;
	k1[2] = (rid >> 16) & 0xFF;
	k1[3] = (rid >> 24) & 0xFF;
	k1[4] = k1[0];
	k1[5] = k1[1];
	k1[6] = k1[2];

	k2[0] = k1[3];
	k2[1] = k1[0];
	k2[2] = k1[1];
	k2[3] = k1[2];
	k2[4] = k2[0];
	k2[5] = k2[1];
	k2[6] = k2[2];

	setDESKeyParity(k1, des_k1);
	setDESKeyParity(k2, des_k2);
}

/*
// Get hidden syskey encoded bytes part in class string of a reg key
// (JD, Skew1, GBG, Data)

bool SyskeyGetClassBytes(HKEY hKeyReg, LPSTR keyName, LPSTR valueName, LPBYTE classBytes) 
{
	HKEY hKey, hSubKey;
	DWORD dwDisposition = 0, classSize;
	BYTE classStr[16];
	LONG ret;
	bool isSuccess = false;

	ret = RegCreateKeyExA(hKeyReg, keyName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_QUERY_VALUE, NULL, &hKey, &dwDisposition);

	if (ret != ERROR_SUCCESS)
		return false;
	else if (dwDisposition != REG_OPENED_EXISTING_KEY) {
		RegCloseKey(hKey);
		return false;
	}
	else {
		if (RegOpenKeyExA(hKey, valueName, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS) {
			classSize = 8 + 1;
			ret = RegQueryInfoKey(hSubKey, (LPTSTR)classStr, &classSize, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
			if ((ret == ERROR_SUCCESS) && (classSize == 8)) {
				classBytes[0] = (HexDigitToByte(classStr[0]) << 4) | HexDigitToByte(classStr[1]);
				classBytes[1] = (HexDigitToByte(classStr[2]) << 4) | HexDigitToByte(classStr[3]);
				classBytes[2] = (HexDigitToByte(classStr[4]) << 4) | HexDigitToByte(classStr[5]);
				classBytes[3] = (HexDigitToByte(classStr[6]) << 4) | HexDigitToByte(classStr[7]);
				isSuccess = true;
			}
			RegCloseKey(hSubKey);
		}
		RegCloseKey(hKey);
	}

	return isSuccess;
}

int CRYPT_SyskeyGetOfflineValue(SYSKEY *pSyskey, LPTSTR hiveFileName) 
{
	LONG errorCode;
	DWORD dwSecureBoot = 0;
	HKEY hkey;
	BYTE syskey[16];
	BYTE syskeyPerm[16] = { 0x8, 0x5, 0x4, 0x2, 0xb, 0x9, 0xd, 0x3, 0x0, 0x6, 0x1, 0xc, 0xe, 0xa, 0xf, 0x7 };
	int i;

	if ((errorCode = LoadHive(hiveFileName)) != ERROR_SUCCESS)
		return SYSKEY_REGISTRY_ERROR;

	if (!RegGetValueEx(HKEY_LOCAL_MACHINE, "OFFLINE_SYSTEM\\ControlSet001\\Control\\Lsa", "SecureBoot", NULL, &dwSecureBoot, sizeof(dwSecureBoot), NULL))
		return SYSKEY_REGISTRY_ERROR;

	if (dwSecureBoot != 1)
		return SYSKEY_METHOD_NOT_IMPL;

	if (!SyskeyGetClassBytes(HKEY_LOCAL_MACHINE, "OFFLINE_SYSTEM\\ControlSet001\\Control\\Lsa", "JD", syskey))
		return SYSKEY_REGISTRY_ERROR;

	if (!SyskeyGetClassBytes(HKEY_LOCAL_MACHINE, "OFFLINE_SYSTEM\\ControlSet001\\Control\\Lsa", "Skew1", syskey + 4))
		return SYSKEY_REGISTRY_ERROR;

	if (!SyskeyGetClassBytes(HKEY_LOCAL_MACHINE, "OFFLINE_SYSTEM\\ControlSet001\\Control\\Lsa", "GBG", syskey + 8))
		return SYSKEY_REGISTRY_ERROR;

	if (!SyskeyGetClassBytes(HKEY_LOCAL_MACHINE, "OFFLINE_SYSTEM\\ControlSet001\\Control\\Lsa", "Data", syskey + 12))
		return SYSKEY_REGISTRY_ERROR;


	for (i = 0; i<16; i++)
		pSyskey->key[i] = syskey[syskeyPerm[i]];

	UnloadHive();

	return SYSKEY_SUCCESS;
}

// Get syskey raw bytes (length=16)
// returns :
//  SYSKEY_SUCCESS => all is OK :)
//  SYSKEY_REGISTRY_ERROR => registry problems, permission, integrity...
//  SYSKEY_METHOD_NOT_IMPL => if syskey is not stored locally

int CRYPT_SyskeyGetValue(SYSKEY *pSyskey) 
{
	DWORD dwSecureBoot = 0;
	BYTE syskey[16];
	BYTE syskeyPerm[16] = { 0x8, 0x5, 0x4, 0x2, 0xb, 0x9, 0xd, 0x3, 0x0, 0x6, 0x1, 0xc, 0xe, 0xa, 0xf, 0x7 };
	int i;

	if (!RegGetValueEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Lsa", "SecureBoot", NULL, &dwSecureBoot, sizeof(dwSecureBoot), NULL))
		return SYSKEY_REGISTRY_ERROR;

	if (dwSecureBoot != 1)
		return SYSKEY_METHOD_NOT_IMPL;

	if (!SyskeyGetClassBytes(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Lsa", "JD", syskey))
		return SYSKEY_REGISTRY_ERROR;

	if (!SyskeyGetClassBytes(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Lsa", "Skew1", syskey + 4))
		return SYSKEY_REGISTRY_ERROR;

	if (!SyskeyGetClassBytes(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Lsa", "GBG", syskey + 8))
		return SYSKEY_REGISTRY_ERROR;

	if (!SyskeyGetClassBytes(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Lsa", "Data", syskey + 12))
		return SYSKEY_REGISTRY_ERROR;


	for (i = 0; i<16; i++)
		pSyskey->key[i] = syskey[syskeyPerm[i]];

	return SYSKEY_SUCCESS;
}


// Get bootkey raw bytes (length=32)
// returns :
//  SYSKEY_SUCCESS => all is OK :)
//  SYSKEY_REGISTRY_ERROR => registry problems, permission, integrity...
//  SYSKEY_METHOD_NOT_IMPL => if syskey is not stored locally

int CRYPT_BootkeyGetValue(BOOTKEY_ciphered *bootkey_ciphered, BOOTKEY *bootkey)
{
	BYTE rc4_key[MD5_DIGEST_LENGTH];
	MD5_CTX md5_ctx;
	RC4_KEY rc4_ctx;
	SYSKEY syskey;
	int retCode;

	if ((retCode = CRYPT_SyskeyGetValue(&syskey)) != SYSKEY_SUCCESS)
		return retCode;

	MD5_Init(&md5_ctx);
	MD5_Update(&md5_ctx, bootkey_ciphered->F + 0x70, 16);
	MD5_Update(&md5_ctx, SAM_QWERTY, strlen(SAM_QWERTY) + 1);
	MD5_Update(&md5_ctx, syskey.key, sizeof(syskey.key));
	MD5_Update(&md5_ctx, SAM_NUM, strlen(SAM_NUM) + 1);
	MD5_Final(rc4_key, &md5_ctx);

	RC4_set_key(&rc4_ctx, MD5_DIGEST_LENGTH, rc4_key);
	RC4(&rc4_ctx, sizeof(bootkey->key), bootkey_ciphered->F + 0x80, bootkey->key);

	return SYSKEY_SUCCESS;
}


// Get lsakey raw bytes (length=16)
// returns :
//  LSAKEY_SUCCESS => all is OK :)
//  LSAKEY_REGISTRY_ERROR => registry problems, permission, integrity...
int CRYPT_LsakeyGetValue(LSAKEY *lsakey, LSAKEY_ciphered *lsakey_ciphered, SYSKEY *syskey)
{
	BYTE buffer[48];
	BYTE buffer2[sizeof(lsakey_ciphered->PolEKList) - 60];

	lsakey->dwMajorVersion = lsakey_ciphered->dwMajorVersion;

	if (lsakey_ciphered->dwMajorVersion < 6) {

		PEKInPlaceEncryptDecryptDataWithKey(lsakey_ciphered->PolSecretEncryptionKey + 12,
			48,
			syskey->key,
			lsakey_ciphered->PolSecretEncryptionKey + 60,
			1000,
			buffer);
		memcpy(lsakey->key, buffer + 16, sizeof(lsakey->key));
	}
	else {

		LSAInPlaceEncryptDecryptDataWithKey(lsakey_ciphered->PolEKList + 60,
			sizeof(lsakey_ciphered->PolEKList) - 60,
			syskey->key,
			sizeof(syskey->key),
			lsakey_ciphered->PolEKList + 28,
			1000,
			buffer2);
		memcpy(lsakey->key_v2, buffer2 + 68, sizeof(lsakey->key_v2));
	}

	return LSAKEY_SUCCESS;
}


// Get NLKM raw bytes (length=64)
// returns :
//  LSAKEY_SUCCESS => all is OK :)
//  LSAKEY_REGISTRY_ERROR => registry problems, permission, integrity...

int CRYPT_NLKMGetValue(NLKM *nlkm, NLKM_ciphered *nlkm_ciphered, LSAKEY *lsakey) 
{
	DES_key_schedule des_key;
	BYTE deciphered_key[72], deciphered_data[80];
	BYTE des_key_bytes_odd[7], des_key_bytes[8];
	int i, j;

	if (nlkm_ciphered->dwMajorVersion >= 6) {
		LSAInPlaceEncryptDecryptDataWithKey(nlkm_ciphered->key_v2 + 60,
			sizeof(nlkm_ciphered->key_v2) - 60,
			lsakey->key_v2,
			sizeof(lsakey->key_v2),
			nlkm_ciphered->key_v2 + 28,
			1000,
			deciphered_data);
		memcpy(nlkm->key_v2, deciphered_data + 16, sizeof(nlkm->key_v2));
	}
	else {
		for (i = j = 0; i<72; i += 8) {
			memcpy(des_key_bytes_odd, lsakey->key + j, 7);
			setDESKeyParity(des_key_bytes_odd, des_key_bytes);

			DES_set_odd_parity((const_DES_cblock *)des_key_bytes);
			DES_set_key((const_DES_cblock *)des_key_bytes, &des_key);

			DES_ecb_encrypt((const_DES_cblock *)(nlkm_ciphered->key + 12 + i), (const_DES_cblock *)(deciphered_key + i), &des_key, DES_DECRYPT);

			j += 7;
			if ((j + 7)>sizeof(lsakey->key))
				j = sizeof(lsakey->key) - j;
		}

		if ((*(LPDWORD)deciphered_key) != sizeof(nlkm->key))
			return LSAKEY_REGISTRY_ERROR;

		memcpy(nlkm->key, deciphered_key + 8, sizeof(nlkm->key));
	}

	nlkm->dwMajorVersion = nlkm_ciphered->dwMajorVersion;

	return LSAKEY_SUCCESS;
}

*/

// Decipher PEK (from NTDS)
int CRYPT_Decipher_PEK(CRYPTED_DATA *pek_ciphered, size_t len_pek_ciphered, SYSKEY *syskey, PEK_LIST & pek_list)
{
	size_t outpeklen = len_pek_ciphered - 24;
	PEK_BLOB *outpek = (PEK_BLOB *)malloc(outpeklen);

	if (pek_ciphered->dwAlgorithmId == 2)
	{
		PEKInPlaceEncryptDecryptDataWithKey(pek_ciphered->w2k.arrEncryptedData,
			outpeklen,
			syskey->key,
			pek_ciphered->arrKeyMaterial,
			1000,
			(LPBYTE)outpek);
	}
	else if (pek_ciphered->dwAlgorithmId == 3)
	{
		AES_KEY aes_ctx;
		memset(&aes_ctx, 0, sizeof(aes_ctx));

		AES_set_decrypt_key(syskey->key, 16*8, &aes_ctx);
		AES_cbc_encrypt(pek_ciphered->w2k.arrEncryptedData, (LPBYTE)outpek, outpeklen, &aes_ctx, pek_ciphered->arrKeyMaterial, AES_DECRYPT);
	}
	else
	{
		return LSAKEY_REGISTRY_ERROR;
	}

	if (outpeklen < (32 + 20)) // must be at least one key
	{
		return LSAKEY_REGISTRY_ERROR;
	}
	size_t datalen = (outpek->header.dwKeyCount * 20) + 32;
	if (datalen > outpeklen) // don't read past the end
	{
		return LSAKEY_REGISTRY_ERROR; 
	}

	for (DWORD dwKeyNum=0;dwKeyNum<outpek->header.dwKeyCount;dwKeyNum++)
	{
		pek_list[outpek->arrEntries[dwKeyNum].dwKeyId] = outpek->arrEntries[dwKeyNum].pek;
	}

	return LSAKEY_SUCCESS;
}



// Hash deobfuscation for NTDS hash

void CRYPT_NTDS_Deobfuscate(LPBYTE ntlm_hash, LDAPAccountInfo ldapAccountEntry, LPBYTE deobfuscated)
{
	DES_key_schedule des_key1, des_key2;
	BYTE key1[8], key2[8];

	// Build DES keys from account SID 
	RIDToDESKey(ldapAccountEntry->rid, key1, key2);

	// DES deciphering
	DES_set_odd_parity((const_DES_cblock *)key1);
	DES_set_odd_parity((const_DES_cblock *)key2);
	DES_set_key((const_DES_cblock *)key1, &des_key1);
	DES_set_key((const_DES_cblock *)key2, &des_key2);

	DES_ecb_encrypt((const_DES_cblock *)ntlm_hash, (const_DES_cblock *)deobfuscated, &des_key1, DES_DECRYPT);
	DES_ecb_encrypt((const_DES_cblock *)(ntlm_hash + 8), (const_DES_cblock *)(deobfuscated + 8), &des_key2, DES_DECRYPT);
}


// Decipher one NTDS account hash

bool CRYPT_NTDS_DecipherAccount(LDAPAccountInfo ldapAccountEntry, SYSKEY *syskey, const PEK_LIST &pekList)
{
	BYTE LM_deciphered[WIN_NTLM_HASH_SIZE], NT_deciphered[WIN_NTLM_HASH_SIZE];
	UINT i;

	// Decipher with PEK
	if (ldapAccountEntry->LM_hash_ciphered)
	{
		DWORD lm_pekid = ldapAccountEntry->LM_hash_ciphered->dwKeyId;
		auto lmpekiter = pekList.find(lm_pekid);
		if (lmpekiter == pekList.end())
		{
			return false;
		}
		PEK lmpek = lmpekiter->second;

		PEKInPlaceEncryptDecryptDataWithKey(ldapAccountEntry->LM_hash_ciphered->w2k.arrEncryptedData,
			WIN_NTLM_HASH_SIZE,
			lmpek.key,
			ldapAccountEntry->LM_hash_ciphered->arrKeyMaterial,
			1,
			LM_deciphered);

		CRYPT_NTDS_Deobfuscate(LM_deciphered, ldapAccountEntry, ldapAccountEntry->NTLM_hash.LM_hash);
	}

	if (ldapAccountEntry->NT_hash_ciphered)
	{
		if (ldapAccountEntry->NT_hash_ciphered->dwAlgorithmId < 0x13)
		{
			DWORD nt_pekid = ldapAccountEntry->NT_hash_ciphered->dwKeyId;
			auto ntpekiter = pekList.find(nt_pekid);
			if (ntpekiter == pekList.end())
			{
				return false;
			}
			PEK ntpek = ntpekiter->second;

			PEKInPlaceEncryptDecryptDataWithKey(ldapAccountEntry->NT_hash_ciphered->w2k.arrEncryptedData,
				WIN_NTLM_HASH_SIZE,
				ntpek.key,
				ldapAccountEntry->NT_hash_ciphered->arrKeyMaterial,
				1,
				NT_deciphered);
		}
		else if (ldapAccountEntry->NT_hash_ciphered->dwAlgorithmId == 0x13)
		{
			DWORD nt_pekid = ldapAccountEntry->NT_hash_ciphered->dwKeyId;
			auto ntpekiter = pekList.find(nt_pekid);
			if (ntpekiter == pekList.end())
			{
				return false;
			}
			PEK ntpek = ntpekiter->second;

			AES_KEY aes_ctx;
			memset(&aes_ctx, 0, sizeof(aes_ctx));

			AES_set_decrypt_key(ntpek.key, 16 * 8, &aes_ctx);
			AES_cbc_encrypt(ldapAccountEntry->NT_hash_ciphered->w16.arrEncryptedData, NT_deciphered, WIN_NTLM_HASH_SIZE, &aes_ctx, ldapAccountEntry->NT_hash_ciphered->arrKeyMaterial, AES_DECRYPT);
		}
		else
		{
			return false;
		}
		CRYPT_NTDS_Deobfuscate(NT_deciphered, ldapAccountEntry, ldapAccountEntry->NTLM_hash.NT_hash);
	}

	// History
	if (ldapAccountEntry->nbHistoryEntriesLM)
	{
		DWORD lm_pekid = ldapAccountEntry->LM_history_ciphered->dwKeyId;
		auto lmpekiter = pekList.find(lm_pekid);
		if (lmpekiter == pekList.end())
		{
			return false;
		}
		PEK lmpek = lmpekiter->second;

		PEKInPlaceEncryptDecryptDataWithKey(ldapAccountEntry->LM_history_ciphered->w2k.arrEncryptedData,
			ldapAccountEntry->LM_history_ciphered_size - 24,
			lmpek.key,
			ldapAccountEntry->LM_history_ciphered->arrKeyMaterial,
			1,
			ldapAccountEntry->LM_history_deciphered);

		for (i = 0; i< ldapAccountEntry->nbHistoryEntriesLM; i++) 
		{
			if (((i)*WIN_NTLM_HASH_SIZE)<(ldapAccountEntry->LM_history_ciphered_size - 24))
				CRYPT_NTDS_Deobfuscate(ldapAccountEntry->LM_history_deciphered + i*WIN_NTLM_HASH_SIZE, ldapAccountEntry, ldapAccountEntry->NTLM_hash_history[i].LM_hash);
		}
	}
	if (ldapAccountEntry->nbHistoryEntriesNT)
	{
		if (ldapAccountEntry->NT_history_ciphered->dwAlgorithmId < 0x13)
		{
			DWORD nt_pekid = ldapAccountEntry->NT_history_ciphered->dwKeyId;
			auto ntpekiter = pekList.find(nt_pekid);
			if (ntpekiter == pekList.end())
			{
				return false;
			}
			PEK ntpek = ntpekiter->second;

			PEKInPlaceEncryptDecryptDataWithKey(ldapAccountEntry->NT_history_ciphered->w2k.arrEncryptedData,
				ldapAccountEntry->NT_history_ciphered_size - 24,
				ntpek.key,
				ldapAccountEntry->NT_history_ciphered->arrKeyMaterial,
				1,
				ldapAccountEntry->NT_history_deciphered);
		}
		else if (ldapAccountEntry->NT_history_ciphered->dwAlgorithmId == 0x13)
		{
			DWORD nt_pekid = ldapAccountEntry->NT_history_ciphered->dwKeyId;
			auto ntpekiter = pekList.find(nt_pekid);
			if (ntpekiter == pekList.end())
			{
				return false;
			}
			PEK ntpek = ntpekiter->second;

			AES_KEY aes_ctx;
			memset(&aes_ctx, 0, sizeof(aes_ctx));

			AES_set_decrypt_key(ntpek.key, 16 * 8, &aes_ctx);
			AES_cbc_encrypt(ldapAccountEntry->NT_history_ciphered->w16.arrEncryptedData, NT_deciphered, ldapAccountEntry->NT_history_ciphered_size - 24, &aes_ctx, ldapAccountEntry->NT_history_ciphered->arrKeyMaterial, AES_DECRYPT);
		}
		else
		{
			return false;
		}

		for (i = 0; i< ldapAccountEntry->nbHistoryEntriesNT; i++) 
		{
			if (((i)*WIN_NTLM_HASH_SIZE)<(ldapAccountEntry->NT_history_ciphered_size - 24))
				CRYPT_NTDS_Deobfuscate(ldapAccountEntry->NT_history_deciphered + i*WIN_NTLM_HASH_SIZE, ldapAccountEntry, ldapAccountEntry->NTLM_hash_history[i].NT_hash);
		}
	}

	return true;
}

// Decipher NTDS account hashes (AD)
bool CRYPT_NTDS_DecipherAllAccount(std::list<LDAPAccountInfo> &ldapAccountInfo, SYSKEY *syskey, const PEK_LIST &pekList) 
{
	for (LDAPAccountInfo currentAccount: ldapAccountInfo)
	{
		if(!CRYPT_NTDS_DecipherAccount(currentAccount, syskey, pekList))
		{
			return false;
		}
	}
	return true;
}

// Decipher one local account hash (LM or NT)
void CRYPT_SAM_Decipher(LPBYTE ntlm_hash, size_t nb_hash, DWORD rid, BOOTKEY *bootkey, LPSTR szPasswordCste, LPBYTE deciphered) 
{
	DES_key_schedule des_key1, des_key2;
	BYTE key1[8], key2[8];
	MD5_CTX md5_ctx;
	RC4_KEY rc4_ctx;
	BYTE rc4_key[MD5_DIGEST_LENGTH];
	BYTE tmp[24 * WIN_NTLM_HASH_SIZE]; // No more than 24 entries in hashes history
	int i;

	// Make hash from bootkey, RID and cst
	MD5_Init(&md5_ctx);
	MD5_Update(&md5_ctx, bootkey->key, sizeof(bootkey->key));
	MD5_Update(&md5_ctx, &rid, 4);
	MD5_Update(&md5_ctx, szPasswordCste, strlen(szPasswordCste) + 1);
	MD5_Final(rc4_key, &md5_ctx);

	RC4_set_key(&rc4_ctx, MD5_DIGEST_LENGTH, rc4_key);
	RC4(&rc4_ctx, nb_hash*WIN_NTLM_HASH_SIZE, ntlm_hash, tmp);

	// Build DES keys from account RID
	RIDToDESKey(rid, key1, key2);

	// DES deciphering
	DES_set_odd_parity((const_DES_cblock *)key1);
	DES_set_odd_parity((const_DES_cblock *)key2);
	DES_set_key((const_DES_cblock *)key1, &des_key1);
	DES_set_key((const_DES_cblock *)key2, &des_key2);

	for (i = 0; i<nb_hash; i++) 
	{
		DES_ecb_encrypt((const_DES_cblock *)(tmp + i*WIN_NTLM_HASH_SIZE), (const_DES_cblock *)(deciphered + i*WIN_NTLM_HASH_SIZE), &des_key1, DES_DECRYPT);
		DES_ecb_encrypt((const_DES_cblock *)(tmp + 8 + i*WIN_NTLM_HASH_SIZE), (const_DES_cblock *)(deciphered + 8 + i*WIN_NTLM_HASH_SIZE), &des_key2, DES_DECRYPT);
	}
}


// Decipher one local account hash + full history if asked
/*
void CRYPT_SAM_DecipherLocalAccount(LocalAccountInfo *localAccountEntry, BOOTKEY *bootkey) 
{
	BYTE lm_hash[WIN_NTLM_HASH_SIZE * 24], nt_hash[WIN_NTLM_HASH_SIZE * 24];
	BYTE deciphered_lm_hash[WIN_NTLM_HASH_SIZE * 24], deciphered_nt_hash[WIN_NTLM_HASH_SIZE * 24];	// No more than 24 entries in hashes history
	size_t hash_offset = *(LPWORD)(localAccountEntry->V + 0x9c) + 0xCC + 4;
	size_t hash_history_offset;
	UINT i;

	memset(lm_hash, 0, sizeof(lm_hash));
	memset(nt_hash, 0, sizeof(nt_hash));

	// Get current hash
	if (((*((LPDWORD)(localAccountEntry->V + hash_offset))) & 0xFFFFFF00) != 0x10000) 
	{ 
		// Is LM hash?
		memcpy(lm_hash, localAccountEntry->V + hash_offset, WIN_NTLM_HASH_SIZE);
		localAccountEntry->NTLM_hash.has_lm_hash = true;
		hash_offset += WIN_NTLM_HASH_SIZE + 4;
	}
	else 
	{
		hash_offset += 4;
	}

	if (((*((LPDWORD)(localAccountEntry->V + hash_offset))) & 0xFFFFFF00) != 0x10000) 
	{ 
		// Is it NT or not password protected?
		memcpy(nt_hash, localAccountEntry->V + hash_offset, WIN_NTLM_HASH_SIZE);
		localAccountEntry->NTLM_hash.has_nt_hash = true;
		hash_offset += WIN_NTLM_HASH_SIZE + 4;
	}
	else 
	{
		hash_offset += 4;
	}

	// Decipher current hash
	if (localAccountEntry->NTLM_hash.has_lm_hash)
	{
		CRYPT_SAM_Decipher(lm_hash, 1, localAccountEntry->rid, bootkey, SAM_LMPASS, localAccountEntry->NTLM_hash.LM_hash);
	}
	if (localAccountEntry->NTLM_hash.has_nt_hash)
	{
		CRYPT_SAM_Decipher(nt_hash, 1, localAccountEntry->rid, bootkey, SAM_NTPASS, localAccountEntry->NTLM_hash.NT_hash);
	}


	// Decipher history if there is
	// xxx: this is wrong, look into it
	if (localAccountEntry->NTLM_hash_history.size()!=0)
	{
		hash_history_offset = hash_offset;

		// Fix localAccountEntry->nbHistoryEntries -
		// (Strange MS behavior) for mixed LM / NTLM in history
		while (((*((LPDWORD)(localAccountEntry->V + hash_offset))) & 0xFFFFFF00) != 0x10000)
			hash_offset += 4;
		localAccountEntry->nbHistoryEntries = (hash_offset - hash_history_offset) / WIN_NTLM_HASH_SIZE;

		memcpy(nt_hash, localAccountEntry->V + hash_history_offset, localAccountEntry->nbHistoryEntries*WIN_NTLM_HASH_SIZE);
		CRYPT_SAM_Decipher(nt_hash, localAccountEntry->nbHistoryEntries, localAccountEntry->rid, bootkey, SAM_NTPASS_HISTORY, deciphered_nt_hash);

		if ((hash_history_offset + 4 + (localAccountEntry->nbHistoryEntries * WIN_NTLM_HASH_SIZE * 2)) <= localAccountEntry->dwVSize) 
		{
			memcpy(lm_hash, localAccountEntry->V + hash_history_offset + (localAccountEntry->nbHistoryEntries*WIN_NTLM_HASH_SIZE) + 4, localAccountEntry->nbHistoryEntries*WIN_NTLM_HASH_SIZE);
			CRYPT_SAM_Decipher(lm_hash, localAccountEntry->nbHistoryEntries, localAccountEntry->rid, bootkey, SAM_LMPASS_HISTORY, deciphered_lm_hash);
		}
		else
		{
			for (i = 0; i<localAccountEntry->nbHistoryEntries; i++)
				memcpy(deciphered_lm_hash + i*WIN_NTLM_HASH_SIZE, SAM_EMPTY_LM_BYTES, WIN_NTLM_HASH_SIZE);
		}

		for (i = 0; i<localAccountEntry->nbHistoryEntries; i++) 
		{
			memcpy(localAccountEntry->NTLM_hash_history[i].NT_hash, deciphered_nt_hash + i*WIN_NTLM_HASH_SIZE, WIN_NTLM_HASH_SIZE);
			memcpy(localAccountEntry->NTLM_hash_history[i].LM_hash, deciphered_lm_hash + i*WIN_NTLM_HASH_SIZE, WIN_NTLM_HASH_SIZE);
		}
	}
}

// Decipher local account hashes
void CRYPT_SAM_DecipherAllLocalAccount(std::list<LocalAccountInfo> &localAccountInfo, BOOTKEY *bootkey)
{
	for (LocalAccountInfo &currentAccount : localAccountInfo)
	{
		CRYPT_SAM_DecipherLocalAccount(&currentAccount, bootkey);
	}
}


// Decipher one cached domain account hash
int CRYPT_SAM_DecipherCachedAccount(CachedAccountInfo *cachedAccountEntry, NLKM *nlkm) 
{
	DWORD dwSAMAccountNameSize, dwDomainSize, dwDomainFullSize;
	BYTE hmac_msg[16], rc4_key[MD5_DIGEST_LENGTH], aes_iv[16];
	LPBYTE deciphered_data, pCurElt;
	RC4_KEY rc4_ctx;
	AES_KEY aes_ctx;

	dwSAMAccountNameSize = *(LPWORD)cachedAccountEntry->cachedEntry;

	if (!dwSAMAccountNameSize || (cachedAccountEntry->dwCachedEntrySize <= 96)) 
	{
		cachedAccountEntry->isEmpty = true;
		return CRYPT_EMPTY_RECORD;
	}
	else {
		dwDomainSize = *(LPWORD)(cachedAccountEntry->cachedEntry + 2);
		dwDomainFullSize = *(LPWORD)(cachedAccountEntry->cachedEntry + 60);
		memcpy(hmac_msg, cachedAccountEntry->cachedEntry + 64, sizeof(hmac_msg));

		if (!(deciphered_data = (LPBYTE)malloc(cachedAccountEntry->dwCachedEntrySize - 96))) {
			return CRYPT_MEM_ERROR;
		}

		if (nlkm->dwMajorVersion<6) {
			HMAC(EVP_md5(), nlkm->key, sizeof(nlkm->key), hmac_msg, sizeof(hmac_msg), rc4_key, NULL);
			RC4_set_key(&rc4_ctx, MD5_DIGEST_LENGTH, rc4_key);
			RC4(&rc4_ctx, cachedAccountEntry->dwCachedEntrySize - 96, cachedAccountEntry->cachedEntry + 96, deciphered_data);
		}
		else {
			AES_set_decrypt_key(nlkm->key_v2, 128, &aes_ctx);
			memcpy(aes_iv, cachedAccountEntry->cachedEntry + 64, sizeof(aes_iv));
			AES_cbc_encrypt(cachedAccountEntry->cachedEntry + 96, deciphered_data, cachedAccountEntry->dwCachedEntrySize - 96, &aes_ctx, aes_iv, AES_DECRYPT);
		}

		cachedAccountEntry->NTLM_hash.hash_type = NT_HASH;
		memcpy(cachedAccountEntry->NTLM_hash.NT_hash, deciphered_data, WIN_NTLM_HASH_SIZE);
		pCurElt = deciphered_data + 0x48;
		memcpy(cachedAccountEntry->szSAMAccountName, pCurElt, dwSAMAccountNameSize);
		pCurElt += (dwSAMAccountNameSize + (((dwSAMAccountNameSize >> 1) % 2) << 1));
		memcpy(cachedAccountEntry->szDomain, pCurElt, dwDomainSize);
		pCurElt += (dwDomainSize + (((dwDomainSize >> 1) % 2) << 1));
		memcpy(cachedAccountEntry->szFullDomain, pCurElt, dwDomainFullSize);

		free(deciphered_data);
	}

	return CRYPT_SUCCESS;
}



// Decipher all cached domain account hashes
int CRYPT_SAM_DecipherAllCachedAccount(std::list<CachedAccountInfo> & cachedAccountInfo, NLKM *nlkm) {

	int retCode, nbNonEmpty = 0;

	for (CachedAccountInfo & currentAccount : cachedAccountInfo)
	{
		retCode = CRYPT_SAM_DecipherCachedAccount(&currentAccount, nlkm);
		if (retCode == CRYPT_MEM_ERROR)
			break;
		else if (retCode == CRYPT_SUCCESS)
			nbNonEmpty++;
	} 

	return nbNonEmpty ? CRYPT_SUCCESS : CRYPT_EMPTY_RECORD;
}
*/
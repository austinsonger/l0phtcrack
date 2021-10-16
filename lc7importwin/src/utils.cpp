#include "utils.h"

// BSWAP a DWORD
DWORD BSWAP(DWORD n) 
{
	return _byteswap_ulong(n);
}


// Adjust token privilege with specific privilege
bool SetPrivilege(const wchar_t *szPrivilege) 
{
	TOKEN_PRIVILEGES tp;
	HANDLE hToken;
	LUID luid;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
		printf("OpenProcessToken() error: 0x%08X\n", GetLastError());
		return false;
	}

	if (!LookupPrivilegeValueW(NULL, szPrivilege, &luid))
		return false;

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (!AdjustTokenPrivileges(hToken, false, &tp, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL)) {
		printf("AdjustTokenPrivileges() error: 0x%08X\n\n", GetLastError());
		return false;
	}

	CloseHandle(hToken);

	return true;
}


// Adjust token privilege with seRestorePrivilege
bool SetSeRestorePrivilege() {
	return SetPrivilege(SE_RESTORE_NAME);
}

/*
* Adjust token privilege with seBackupPrivilege
*/
bool SetSeBackupPrivilege() {
	return SetPrivilege(SE_BACKUP_NAME);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//														REGISTRY FUNC
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
* Query value in registry (generic)
*/
bool RegGetValueEx(HKEY hKeyReg, LPSTR keyName, LPSTR valueName, LPDWORD type, LPVOID val, DWORD valSize, LPDWORD outValSize) {
	HKEY hKey;
	DWORD dwDisposition = 0, dwValueSize;
	LONG ret;

	ret = RegCreateKeyExA(hKeyReg, keyName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_QUERY_VALUE, NULL, &hKey, &dwDisposition);

	if ((ret == ERROR_SUCCESS) && (dwDisposition == REG_OPENED_EXISTING_KEY)) {
		dwValueSize = valSize;
		ret = RegQueryValueExA(hKey, valueName, NULL, type, (LPBYTE)val, &dwValueSize);
		if (outValSize && (ret == ERROR_SUCCESS))
			*outValSize = dwValueSize;
		RegCloseKey(hKey);

		if (!valSize)
			return (ret == ERROR_SUCCESS);

		return (ret == ERROR_SUCCESS) && (dwValueSize == valSize);
	}

	return false;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//														DUMPING FUNC
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
// Raw dump a ciphered PEK struct
void PEK_cipheredDump(s_NTLM_pek_ciphered *pek_ciphered) {
	TCHAR szPEKCiphered[256];

	BytesToHex(pek_ciphered, sizeof(s_NTLM_pek_ciphered), szPEKCiphered);
	printf("Ciphered PEK = %s\n", szPEKCiphered);
}


// Raw dump a PEK struct
void PEK_Dump(s_NTLM_pek *pek) {
	TCHAR szPEK[256];

	BytesToHex(pek->decipher_key2, sizeof(pek->decipher_key2), szPEK);
	printf("PEK = %s\n", szPEK);
}


// Dump to text one entry of a ll_ldapAccountInfo linked list
// Format : John The Ripper

bool NTDS_NTLM_DumpJohn(s_ldapAccountInfo *ldapAccountEntry, LPSTR szOut) {
	TCHAR szLM[64], szNT[256];
	UINT i;

	if (ldapAccountEntry->NTLM_hash.hash_type == NT_NO_HASH)
		wsprintf(szOut, "%ls:%d:%s:%s:::\r\n", ldapAccountEntry->szSAMAccountName, ldapAccountEntry->rid, SAM_EMPTY_LM, SAM_EMPTY_NT);
	else {
		BytesToHex(ldapAccountEntry->NTLM_hash.NT_hash, WIN_NTLM_HASH_SIZE, szNT);

		if (ldapAccountEntry->NTLM_hash.hash_type == LM_HASH) {
			BytesToHex(ldapAccountEntry->NTLM_hash.LM_hash, WIN_NTLM_HASH_SIZE, szLM);
			wsprintf(szOut, "%ls:%d:%s:%s:::\r\n", ldapAccountEntry->szSAMAccountName, ldapAccountEntry->rid, szLM, szNT);
		}
		else if (ldapAccountEntry->NTLM_hash.hash_type == NT_HASH)
			wsprintf(szOut, "%ls:%d:%s:%s:::\r\n", ldapAccountEntry->szSAMAccountName, ldapAccountEntry->rid, SAM_EMPTY_LM, szNT);
	}

	if (ldapAccountEntry->nbHistoryEntries) {
		for (i = 0; i<ldapAccountEntry->nbHistoryEntries; i++) {
			BytesToHex(ldapAccountEntry->NTLM_hash_history[i].NT_hash, WIN_NTLM_HASH_SIZE, szNT);
			BytesToHex(ldapAccountEntry->NTLM_hash_history[i].LM_hash, WIN_NTLM_HASH_SIZE, szLM);
			wsprintf(szOut + lstrlen(szOut), "%ls_hist%d:%d:%s:%s:::\r\n", ldapAccountEntry->szSAMAccountName, i, ldapAccountEntry->rid, szLM, szNT);
		}
	}

	return true;
}


// Dump to text one entry of a ll_ldapAccountInfo linked list
// Format : L0phtCrack
bool NTDS_NTLM_DumpLc(s_ldapAccountInfo *ldapAccountEntry, LPSTR szOut) {
	TCHAR szLM[256], szNT[256];
	UINT i;

	if (ldapAccountEntry->NTLM_hash.hash_type == NT_NO_HASH)
		wsprintf(szOut, "%ls:\"\":\"\":%s:%s\r\n", ldapAccountEntry->szSAMAccountName, SAM_EMPTY_LM, SAM_EMPTY_NT);
	else {
		BytesToHex(ldapAccountEntry->NTLM_hash.NT_hash, WIN_NTLM_HASH_SIZE, szNT);
		if (ldapAccountEntry->NTLM_hash.hash_type == LM_HASH) {
			BytesToHex(ldapAccountEntry->NTLM_hash.LM_hash, WIN_NTLM_HASH_SIZE, szLM);
			wsprintf(szOut, "%ls:\"\":\"\":%s:%s\r\n", ldapAccountEntry->szSAMAccountName, szLM, szNT);
		}
		else if (ldapAccountEntry->NTLM_hash.hash_type == NT_HASH)
			wsprintf(szOut, "%ls:\"\":\"\":%s:%s\r\n", ldapAccountEntry->szSAMAccountName, SAM_EMPTY_LM, szNT);
	}

	if (ldapAccountEntry->nbHistoryEntries) {
		for (i = 0; i<ldapAccountEntry->nbHistoryEntries; i++) {
			BytesToHex(ldapAccountEntry->NTLM_hash_history[i].NT_hash, WIN_NTLM_HASH_SIZE, szNT);
			BytesToHex(ldapAccountEntry->NTLM_hash_history[i].LM_hash, WIN_NTLM_HASH_SIZE, szLM);
			wsprintf(szOut + lstrlen(szOut), "%ls:\"\":\"\":%s:%s\r\n", ldapAccountEntry->szSAMAccountName, szLM, szNT);
		}
	}

	return true;
}


// Dump a ll_ldapAccountInfo linked list
// (SAMAccoutnName,deciphered NT, deciphered LM)
bool NTDS_NTLM_DumpAll(ll_ldapAccountInfo ldapAccountInfo, NT_DUMP_TYPE dump_type, bool isStdout, LPSTR outFileName) {
	ll_ldapAccountInfo currentAccount = ldapAccountInfo;
	TCHAR szHashLine[4096];
	DWORD dwNbWritten, count = 0;
	HANDLE hFile;
	bool ret;

	if (!currentAccount)
		return false;

	if (!isStdout) {
		if ((hFile = CreateFile(outFileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
			return false;
	}
	else
		puts(SZ_DUMP_BEGIN);

	do{
		if (dump_type == NTDUMP_JOHN)
			ret = NTDS_NTLM_DumpJohn(&currentAccount->info, szHashLine);
		else if (dump_type == NTDUMP_LC)
			ret = NTDS_NTLM_DumpLc(&currentAccount->info, szHashLine);
		if (isStdout)
			printf(szHashLine);
		else {
			WriteFile(hFile, szHashLine, lstrlen(szHashLine), &dwNbWritten, NULL);
		}
		if (ret)
			count++;
		currentAccount = currentAccount->next;
	} while (currentAccount);

	if (!isStdout)
		CloseHandle(hFile);
	else
		puts(SZ_DUMP_END);

	if (isStdout)
		printf("\n%d dumped accounts\n\n", count);
	else
		printf("\n%d dumped accounts to %s\n\n", count, outFileName);

	return true;
}


// Dump to text one entry of a ll_localAccountInfo linked list
// Format : John The Ripper
void SAM_NTLM_DumpJohn(s_localAccountInfo *localAccountEntry, LPSTR szOut) {
	TCHAR szLM[64], szNT[256];
	UINT i;

	BytesToHex(localAccountEntry->NTLM_hash.NT_hash, WIN_NTLM_HASH_SIZE, szNT);

	if (localAccountEntry->NTLM_hash.hash_type == LM_HASH) {
		BytesToHex(localAccountEntry->NTLM_hash.LM_hash, WIN_NTLM_HASH_SIZE, szLM);
		wsprintf(szOut, "%s:%d:%s:%s:::\r\n", localAccountEntry->szSAMAccountName, localAccountEntry->rid, szLM, szNT);
	}
	else if (localAccountEntry->NTLM_hash.hash_type == NT_HASH)
		wsprintf(szOut, "%s:%d:%s:%s:::\r\n", localAccountEntry->szSAMAccountName, localAccountEntry->rid, SAM_EMPTY_LM, szNT);
	else if (localAccountEntry->NTLM_hash.hash_type == NT_NO_HASH)
		wsprintf(szOut, "%s:%d:%s:%s:::\r\n", localAccountEntry->szSAMAccountName, localAccountEntry->rid, SAM_EMPTY_LM, SAM_EMPTY_NT);

	if (localAccountEntry->NTLM_hash_history) {
		for (i = 0; i<localAccountEntry->nbHistoryEntries; i++) {
			BytesToHex(localAccountEntry->NTLM_hash_history[i].NT_hash, WIN_NTLM_HASH_SIZE, szNT);
			BytesToHex(localAccountEntry->NTLM_hash_history[i].LM_hash, WIN_NTLM_HASH_SIZE, szLM);
			wsprintf(szOut + lstrlen(szOut), "%s_hist%d:%d:%s:%s:::\r\n", localAccountEntry->szSAMAccountName, i, localAccountEntry->rid, szLM, szNT);
		}
	}
}


// Dump to text one entry of a ll_localAccountInfo linked list
// Format : L0phtCrack
void SAM_NTLM_DumpLc(s_localAccountInfo *localAccountEntry, LPSTR szOut) {
	TCHAR szLM[256], szNT[256];
	UINT i;

	BytesToHex(localAccountEntry->NTLM_hash.NT_hash, WIN_NTLM_HASH_SIZE, szNT);

	if (localAccountEntry->NTLM_hash.hash_type == LM_HASH) {
		BytesToHex(localAccountEntry->NTLM_hash.LM_hash, WIN_NTLM_HASH_SIZE, szLM);
		wsprintf(szOut, "%s:\"\":\"\":%s:%s\r\n", localAccountEntry->szSAMAccountName, szLM, szNT);
	}
	else if (localAccountEntry->NTLM_hash.hash_type == NT_HASH)
		wsprintf(szOut, "%s:\"\":\"\":%s:%s\r\n", localAccountEntry->szSAMAccountName, SAM_EMPTY_LM, szNT);
	else if (localAccountEntry->NTLM_hash.hash_type == NT_NO_HASH)
		wsprintf(szOut, "%s:\"\":\"\":%s:%s\r\n", localAccountEntry->szSAMAccountName, SAM_EMPTY_LM, SAM_EMPTY_NT);

	if (localAccountEntry->NTLM_hash_history) {
		for (i = 0; i<localAccountEntry->nbHistoryEntries; i++) {
			BytesToHex(localAccountEntry->NTLM_hash_history[i].NT_hash, WIN_NTLM_HASH_SIZE, szNT);
			BytesToHex(localAccountEntry->NTLM_hash_history[i].LM_hash, WIN_NTLM_HASH_SIZE, szLM);
			wsprintf(szOut + lstrlen(szOut), "%s_hist%d:\"\":\"\":%s:%s\r\n", localAccountEntry->szSAMAccountName, i, szLM, szNT);
		}
	}
}


// Dump to text a ll_localAccountInfo linked list
// (SAMAccoutnName,deciphered NT, deciphered LM)
bool SAM_NTLM_DumpAll(ll_localAccountInfo localAccountInfo, NT_DUMP_TYPE dump_type, bool isStdout, LPSTR outFileName) {
	ll_localAccountInfo currentAccount = localAccountInfo;
	TCHAR szHashLine[4096];
	DWORD dwNbWritten, count = 0;
	HANDLE hFile;

	if (!currentAccount)
		return false;

	if (!isStdout) {
		if ((hFile = CreateFile(outFileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
			return false;
	}
	else
		puts(SZ_DUMP_BEGIN);

	do{
		if (dump_type == NTDUMP_JOHN)
			SAM_NTLM_DumpJohn(&currentAccount->info, szHashLine);
		else if (dump_type == NTDUMP_LC)
			SAM_NTLM_DumpLc(&currentAccount->info, szHashLine);
		if (isStdout)
			printf(szHashLine);
		else {
			WriteFile(hFile, szHashLine, lstrlen(szHashLine), &dwNbWritten, NULL);
		}
		currentAccount = currentAccount->next;
		count++;
	} while (currentAccount);

	if (!isStdout)
		CloseHandle(hFile);
	else
		puts(SZ_DUMP_END);

	if (isStdout)
		printf("\n%d dumped accounts\n\n", count);
	else
		printf("\n%d dumped accounts to %s\n\n", count, outFileName);

	return true;
}



// Dump to text a ll_cachedAccountInfo linked list
// (SAMAccoutnName,deciphered NT)
bool SAM_NTLM_Cached_DumpAll(ll_cachedAccountInfo cachedAccountInfo, NT_DUMP_TYPE dump_type, bool isStdout, LPSTR outFileName) {
	ll_cachedAccountInfo currentAccount = cachedAccountInfo;
	TCHAR szHashLine[1024], szNT[128];
	DWORD dwNbWritten, count = 0;
	HANDLE hFile;

	if (!isStdout) {
		if ((hFile = CreateFile(outFileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
			return false;
	}
	else
		puts(SZ_DUMP_BEGIN);

	do{
		if (!currentAccount->info.isEmpty) {
			BytesToHex(currentAccount->info.NTLM_hash.NT_hash, WIN_NTLM_HASH_SIZE, szNT);
			sprintf_s(szHashLine, 1024, "Cached Entry\r\n\tUsername: %ls\r\n\tDomain: %ls\r\n\tFull Domain: %ls\r\n\tNT hash: %s\r\n",
				currentAccount->info.szSAMAccountName,
				currentAccount->info.szDomain,
				currentAccount->info.szFullDomain,
				szNT);
			count++;
			if (isStdout)
				puts(szHashLine);
			else
				WriteFile(hFile, szHashLine, lstrlen(szHashLine), &dwNbWritten, NULL);
		}
		currentAccount = currentAccount->next;
	} while (currentAccount);

	if (!isStdout)
		CloseHandle(hFile);
	else
		puts(SZ_DUMP_END);

	if (isStdout)
		printf("\n%d dumped accounts\n\n", count);
	else
		printf("\n%d dumped accounts to %s\n\n", count, outFileName);

	return true;
}


// Dump key-package struct to a keyfile
bool BitLocker_DumpKeyPackage(s_bitlockerAccountInfo *bitlockerAccountEntry, LPWSTR outFileName) {
	HANDLE hFile;
	DWORD dwNbWritten;

	if ((hFile = CreateFileW(outFileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
		return false;

	WriteFile(hFile, bitlockerAccountEntry->msFVE_KeyPackage, bitlockerAccountEntry->dwSzKeyPackage, &dwNbWritten, NULL);

	CloseHandle(hFile);

	return dwNbWritten == bitlockerAccountEntry->dwSzKeyPackage;
}


// Dump to text one entry of a ll_bitlockerAccountInfo linked 
void Bitlocker_Dump(s_bitlockerAccountInfo *bitlockerAccountEntry, LPSTR szOut) {
	WCHAR szVolumeGUID[128], szRecoveryGUID[128];
	WCHAR szKeyPackageFileName[MAX_PATH + 1];

	StringFromGUID2(bitlockerAccountEntry->msFVE_VolumeGUID, szVolumeGUID, sizeof(szVolumeGUID));
	StringFromGUID2(bitlockerAccountEntry->msFVE_RecoveryGUID, szRecoveryGUID, sizeof(szRecoveryGUID));

	RtlZeroMemory(szKeyPackageFileName, sizeof(szKeyPackageFileName));
	RtlMoveMemory(szKeyPackageFileName, szRecoveryGUID + 1, 2 * (lstrlenW(szRecoveryGUID) - 2));
	lstrcatW(szKeyPackageFileName, L".pk");

	if (!BitLocker_DumpKeyPackage(bitlockerAccountEntry, szKeyPackageFileName))
		lstrcpynW(szKeyPackageFileName, L"(Error while saving)", MAX_PATH);

	sprintf_s(szOut, 2048, "Bitlocker entry\r\n\tVolume GUID: %ls\r\n\tRecovery GUID: %ls\r\n\tRecovery password: %ls\r\n\tKey-package: saved to binary file %ls\r\n",
		szVolumeGUID, szRecoveryGUID,
		bitlockerAccountEntry->msFVE_RecoveryPassword,
		szKeyPackageFileName);

}



// Dump a ll_bitlockerAccountInfo linked list
// (msFVE_VolumeGUID, msFVE_RecoveryGUID, msFVE_RecoveryPassword, msFVE_KeyPackage)
bool Bitlocker_DumpAll(ll_bitlockerAccountInfo bitlockerAccountInfo, bool isStdout, LPSTR outFileName) {
	ll_bitlockerAccountInfo currentAccount = bitlockerAccountInfo;
	TCHAR szBitlockerEntry[2048];
	DWORD dwNbWritten, count = 0;
	HANDLE hFile;

	if (!currentAccount)
		return false;

	if (!isStdout) {
		if ((hFile = CreateFile(outFileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
			return false;
	}
	else
		puts(SZ_DUMP_BEGIN);

	do{
		Bitlocker_Dump(&currentAccount->info, szBitlockerEntry);
		count++;
		if (isStdout)
			printf(szBitlockerEntry);
		else
			WriteFile(hFile, szBitlockerEntry, lstrlen(szBitlockerEntry), &dwNbWritten, NULL);
		currentAccount = currentAccount->next;
	} while (currentAccount);

	if (!isStdout)
		CloseHandle(hFile);
	else
		puts(SZ_DUMP_END);

	if (isStdout)
		printf("\n%d dumped entries\n\n", count);
	else
		printf("\n%d dumped entries to %s\n\n", count, outFileName);

	return true;
}
*/
#ifndef __INC_NTDS_H
#define __INC_NTDS_H

#include <list>
#include <map>
#include <windows.h>
#include <Sddl.h>
#include <Lmcons.h>
#include <stdio.h>

#define JET_VERSION 0x0502
#include <esent.h>

#include "crypt.h"

// NTDS objects table
#define NTDS_TBL_OBJ "datatable"

// NTDS interesting account attributes
// Many more at: https://github.com/yosqueoy/ditsnap/blob/master/ditsnap_exe/TableListView.cpp
#define ATT_SAM_ACCOUNT_NAME	"ATTm590045"
#define ATT_SAM_ACCOUNT_TYPE	"ATTj590126"
#define ATT_OBJECT_SID			"ATTr589970"
#define ATT_LM_HASH				"ATTk589879"
#define ATT_NT_HASH				"ATTk589914"
#define ATT_PEK					"ATTk590689"
#define ATT_LM_HASH_HISTORY		"ATTk589984"
#define ATT_NT_HASH_HISTORY		"ATTk589918"

#define ATT_SURNAME				"ATTm4"
#define ATT_GIVEN_NAME			"ATTm42"
#define ATT_ORG_UNIT			"ATTm11"
#define ATT_DESCRIPTION			"ATTm13"
#define ATT_DISPLAY_NAME		"ATTm131085"
#define ATT_COMMENT				"ATTm131153"
#define ATT_DOMAIN				"ATTm1376281"		
#define ATT_PRINCIPAL			"ATTm590480"
#define ATT_HOME_DIRECTORY		"ATTm589868"
#define ATT_USER_ACCOUNT_CONTROL "ATTj589832"
#define ATT_LAST_LOGON_INDEX "ATTq589876"
#define ATT_LAST_LOGON_TIME_STAMP_INDEX "ATTq591520"
#define ATT_ACCOUNT_EXPIRES_INDEX "ATTq589983"
#define ATT_PASSWORD_LAST_SET_INDEX "ATTq589920"
#define ATT_BAD_PWD_TIME_INDEX "ATTq589873"
#define ATT_LOGON_COUNT_INDEX "ATTj589993"
#define ATT_BAD_PWD_COUNT_INDEX "ATTj589836"

#define ATT_BITLOCKER_MSFVE_KEY_PACKAGE			"ATTk591823"
#define ATT_BITLOCKER_MSFVE_RECOVERY_GUID		"ATTk591789"
#define ATT_BITLOCKER_MSFVE_RECOVERY_PASSWORD	"ATTm591788"
#define ATT_BITLOCKER_MSFVE_VOLUME_GUID			"ATTk591822"

// SAM account types (NTDS)
#define SAM_GROUP_OBJECT				0x10000000
#define SAM_NON_SECURITY_GROUP_OBJECT	0x10000001
#define SAM_ALIAS_OBJECT				0x20000000
#define SAM_NON_SECURITY_ALIAS_OBJECT	0x20000001
#define SAM_USER_OBJECT					0x30000000
#define SAM_MACHINE_ACCOUNT				0x30000001
#define SAM_TRUST_ACCOUNT				0x30000002

#define UAC_DISABLED					0x00000002
#define UAC_LOCKED_OUT					0x00000010 
#define UAC_NO_PASSWORD_REQUIRED		0x00000020 
#define UAC_CANT_CHANGE_PASSWORD		0x00000040
#define UAC_NORMAL_ACCOUNT				0x00000200
#define UAC_INTERDOMAIN_TRUST_ACCOUNT	0x00000800
#define UAC_WORKSTATION_TRUST_ACCOUNT	0x00001000
#define UAC_SERVER_TRUST_ACCOUNT		0x00002000
#define UAC_PASSWORD_NEVER_EXPIRES		0x00010000
#define UAC_MSN_LOGON_ACCOUNT			0x00020000
#define UAC_SMART_CARD_REQUIRED			0x00040000
#define UAC_PASSWORD_EXPIRED			0x00800000

// NTDS parser structure definitions
#define ID_SAM_ACCOUNT_NAME		0
#define ID_SAM_ACCOUNT_TYPE		1
#define ID_OBJECT_SID			2
#define ID_LM_HASH				3
#define ID_NT_HASH				4
#define ID_PEK					5
#define ID_LM_HASH_HISTORY		6
#define ID_NT_HASH_HISTORY		7
#define ID_USER_ACCOUNT_CONTROL	8
#define ID_LAST_SET_TIME		9
#define ID_DESCRIPTION			10
#define ID_HOME_DIRECTORY		11
#define ID_DOMAIN				12
#define ID_SURNAME				13
#define ID_GIVEN_NAME			14
#define ID_ORG_UNIT				15
#define ID_DNT_COL				16
#define ID_ANCESTORS_COL		17

#define ID_MSFVE_KEY_PACKAGE		0
#define ID_MSFVE_RECOVERY_GUID		1
#define ID_MSFVE_RECOVERY_PASSWORD	2
#define ID_MSFVE_VOLUME_GUID		3

// Error codes for NTDS functions
#define NTDS_SUCCESS 0
#define NTDS_BAD_RECORD -1
#define NTDS_API_ERROR -2
#define NTDS_MEM_ERROR -3
#define NTDS_EMPTY_ERROR -4

class NTDS
{
private:
	JET_INSTANCE m_instance;
	JET_SESID m_sesid;
	JET_DBID m_dbid;
	QString m_ntdsfilename;
	JET_COLUMNDEF m_columndef[18];
	bool m_with_history;
	bool m_include_machine_accounts;
	bool m_using_recovery;
	QString m_recovery_tempdir;
	std::map<DWORD, std::wstring> m_dnt_to_domain;

protected:

	void Reset();
	QString ParseJetError(JET_ERR jet_err);

	JET_ERR GetRecord(JET_TABLEID tableid, JET_COLUMNID columnid, LPBYTE val, ULONG *val_size);
	int ParseDomainRecords(JET_TABLEID tableid);
	int NTLM_ParseSAMRecord(JET_TABLEID tableid, LDAPAccountInfo ldapAccountEntry);
	int NTLM_ParsePEKRecord(JET_TABLEID tableid, CRYPTED_DATA **pek_ciphered, size_t *len_pek_ciphered);
	int Bitlocker_ParseRecord(JET_TABLEID tableid, BitlockerAccountInfo *bitlockerAccountEntry);

public:

	NTDS(bool with_history, bool include_machine_accounts);
	~NTDS();

	bool Init(QString & error);
	bool Close(QString & error);
	bool RecoverDatabase(QString in_ntds, QString & recovered_ntds, QString &error);
	bool OpenDatabase(QString strNTDSPath, QString & error);
	bool CloseDatabase(QString & error);
	int NTLM_ParseDatabase(std::list<LDAPAccountInfo> & ldapAccountInfo, CRYPTED_DATA **pek_ciphered, size_t *len_pek_ciphered, QString & error);
	int Bitlocker_ParseDatabase(std::list<BitlockerAccountInfo> & bitlockerAccountInfo, QString & error);
};

#endif

#include<stdafx.h>
#include"ntds.h"

NTDS::NTDS(bool with_history, bool include_machine_accounts)
{
	m_with_history = with_history;
	m_include_machine_accounts = include_machine_accounts;

	Reset();
}

NTDS::~NTDS()
{
	QString error;
	Close(error);
}

void NTDS::Reset()
{
	m_instance = NULL;
	m_sesid = NULL;
	m_dbid = NULL;
	memset(m_columndef, 0, sizeof(m_columndef));
	m_using_recovery = false;
	m_recovery_tempdir = "";
	m_ntdsfilename = "";
	m_dnt_to_domain.clear();
}

QString NTDS::ParseJetError(JET_ERR jet_err) 
{
	char szErrString[1024];
	JET_ERR jetErr = jet_err;
	memset(szErrString, 0, sizeof(szErrString));
	JetGetSystemParameter(m_instance, m_sesid, JET_paramErrorToString, (JET_API_PTR *)&jetErr, szErrString, sizeof(szErrString));
	return QString::fromLatin1(szErrString);
}

bool NTDS::Init(QString &error) 
{
	if (m_instance)
	{
		error = "NTDS already initialized";
		return false;
	}
	JET_ERR jet_err;
	
	if ((jet_err = JetSetSystemParameter(&m_instance, JET_sesidNil, JET_paramDatabasePageSize, 8192, NULL)) != JET_errSuccess)
	{
		error = ParseJetError(jet_err);
		return false;
	}

	if ((jet_err = JetSetSystemParameter(&m_instance, JET_sesidNil, JET_paramRecovery, NULL, "Off")) != JET_errSuccess)
	{
		error = ParseJetError(jet_err);
		return false;
	}

	QString instancename = QString("LC7-JET-") + QUuid::createUuid().toString();
	if ((jet_err = JetCreateInstance(&m_instance, instancename.toLocal8Bit().constData())) != JET_errSuccess)
	{
		error = ParseJetError(jet_err);
		return false;
	}
	
	if ((jet_err = JetInit(&m_instance)) != JET_errSuccess)
	{
		error = ParseJetError(jet_err);
		return false;
	}

	return true;
}

bool NTDS::Close(QString &error)
{
	if (!m_instance)
	{
		error = "NTDS already terminated";
		return false;
	}

	JET_ERR jet_err;
	if((jet_err = JetTerm(m_instance)) != JET_errSuccess)
	{
		error = ParseJetError(jet_err);
		return false;
	}

	if (m_using_recovery)
	{
		QDir(m_recovery_tempdir).removeRecursively();
	}
	
	Reset();
	return true;
}

bool NTDS::RecoverDatabase(QString in_ntds, QString & recovered_ntds, QString &error)
{
	m_recovery_tempdir = g_pLinkage->NewTemporaryDir();
	QString newdit = QDir(m_recovery_tempdir).absoluteFilePath("recovery.dit");
	QFile::copy(in_ntds, newdit);

	recovered_ntds = newdit;
	
	if (m_instance)
	{
		error = "NTDS already initialized";
		return false;
	}
	JET_ERR jet_err;

	if ((jet_err = JetSetSystemParameter(&m_instance, JET_sesidNil, JET_paramDatabasePageSize, 8192, NULL)) != JET_errSuccess)
	{
		error = ParseJetError(jet_err);
		return false;
	}

	if ((jet_err = JetSetSystemParameter(&m_instance, JET_sesidNil, JET_paramRecovery, NULL, "On")) != JET_errSuccess)
	{
		error = ParseJetError(jet_err);
		return false;
	}

	if ((jet_err = JetSetSystemParameter(&m_instance, JET_sesidNil, JET_paramCircularLog, 1, NULL)) != JET_errSuccess)
	{
		error = ParseJetError(jet_err);
		return false;
	}

	if ((jet_err = JetSetSystemParameter(&m_instance, JET_sesidNil, JET_paramLogFilePath, NULL, QDir::toNativeSeparators(m_recovery_tempdir).toLocal8Bit().constData())) != JET_errSuccess)
	{
		error = ParseJetError(jet_err);
		return false;
	}

	if ((jet_err = JetSetSystemParameter(&m_instance, JET_sesidNil, JET_paramSystemPath, NULL, QDir::toNativeSeparators(m_recovery_tempdir).toLocal8Bit().constData())) != JET_errSuccess)
	{
		error = ParseJetError(jet_err);
		return false;
	}

	if ((jet_err = JetSetSystemParameter(&m_instance, JET_sesidNil, JET_paramAlternateDatabaseRecoveryPath, NULL, QDir::toNativeSeparators(m_recovery_tempdir).toLocal8Bit().constData())) != JET_errSuccess)
	{
		error = ParseJetError(jet_err);
		return false;
	}
	
	if ((jet_err = JetSetSystemParameter(&m_instance, JET_sesidNil, JET_paramBaseName, NULL, "EDB")) != JET_errSuccess)
	{
		error = ParseJetError(jet_err);
		return false;
	}

	QString instancename = QString("LC7-RECOVER-") + QUuid::createUuid().toString();
	if ((jet_err = JetCreateInstance(&m_instance, instancename.toLocal8Bit().constData())) != JET_errSuccess)
	{
		error = ParseJetError(jet_err);
		return false;
	}

	if ((jet_err = JetInit(&m_instance)) != JET_errSuccess)
	{
		error = ParseJetError(jet_err);
		return false;
	}

	jet_err = JetBeginSession(m_instance, &m_sesid, NULL, NULL);
	if (jet_err != JET_errSuccess) {
		error = ParseJetError(jet_err);
		JetEndSession(m_sesid, 0);
		JetTerm(m_instance);
		return false;
	}

	jet_err = JetAttachDatabase(m_sesid, newdit.toLocal8Bit(), 0);
	if (jet_err != JET_errSuccess)
	{
		error = ParseJetError(jet_err);
		JetEndSession(m_sesid, 0);
		JetTerm(m_instance);
		return false;
	}

	jet_err = JetDetachDatabase(m_sesid, newdit.toLocal8Bit());
	if (jet_err != JET_errSuccess)
	{
		error = ParseJetError(jet_err);
		JetEndSession(m_sesid, 0);
		JetTerm(m_instance);
		return false;
	}

	jet_err = JetEndSession(m_sesid, 0);
	if (jet_err != JET_errSuccess)
	{
		error = ParseJetError(jet_err);
		JetTerm(m_instance);
		return false;
	}

	JetTerm(m_instance);

	m_using_recovery = true;
	return true;
}

bool NTDS::OpenDatabase(QString strNTDSPath, QString & error) 
{
	JET_DBID dbID = JET_dbidNil;
	JET_ERR jet_err;
	
	m_ntdsfilename = strNTDSPath;

	jet_err = JetBeginSession(m_instance, &m_sesid, NULL, NULL);
	if (jet_err != JET_errSuccess) {
		error = ParseJetError(jet_err);
		JetEndSession(m_sesid, 0);
		return false;
	}

	jet_err = JetAttachDatabase(m_sesid, m_ntdsfilename.toLocal8Bit(), JET_bitDbReadOnly);
	if (jet_err != JET_errSuccess) 
	{
#ifdef ATTEMPT_RECOVERY
		if (jet_err == JET_errDatabaseDirtyShutdown)
		{
			JetEndSession(m_sesid, 0);
			
			if (!Close(error))
			{
				return false;
			}

			if (!RecoverDatabase(strNTDSPath, m_ntdsfilename, error))
			{
				return false;
			}

			if (!Init(error))
			{
				return false;
			}

			jet_err = JetBeginSession(m_instance, &m_sesid, NULL, NULL);
			if (jet_err != JET_errSuccess) {
				error = ParseJetError(jet_err);
				JetEndSession(m_sesid, 0);
				return false;
			}

			jet_err = JetAttachDatabase(m_sesid, m_ntdsfilename.toLocal8Bit(), JET_bitDbReadOnly);
			if (jet_err != JET_errSuccess)
			{
				error = ParseJetError(jet_err);
				JetEndSession(m_sesid, 0);
				return false;
			}
		}
		else
		{
			error = ParseJetError(jet_err);
			JetEndSession(m_sesid, 0);
			return false;
		}
#else
		error = ParseJetError(jet_err) + "\n\nNTDS.DIT was not copied correctly. Refer to the documentation on the proper way to copy the NTDS.DIT file.";
		JetEndSession(m_sesid, 0);
		return false;
#endif
	}

	jet_err = JetOpenDatabase(m_sesid, m_ntdsfilename.toLocal8Bit(), NULL, &m_dbid, JET_bitDbReadOnly);
	if (jet_err != JET_errSuccess) {
		error = ParseJetError(jet_err);
		JetEndSession(m_sesid, 0);
		return false;
	}

	return true;
}

bool NTDS::CloseDatabase(QString &error)
{
	JET_ERR jet_err;

	jet_err = JetCloseDatabase(m_sesid, m_dbid, 0);
	if (jet_err != JET_errSuccess)
	{
		error = ParseJetError(jet_err);
		return false;
	}

	jet_err = JetDetachDatabase(m_sesid, m_ntdsfilename.toLocal8Bit());
	if (jet_err != JET_errSuccess) 
	{
		error = ParseJetError(jet_err);
		return false;
	}

	jet_err = JetEndSession(m_sesid, 0);
	if (jet_err != JET_errSuccess) 
	{
		error = ParseJetError(jet_err);
		return false;
	}

	return true;
}

JET_ERR NTDS::GetRecord(JET_TABLEID tableid, JET_COLUMNID columnid, LPBYTE val, ULONG *val_size)
{
	JET_ERR jet_err;
	if (val)
	{
		memset(val, 0, *val_size);
	}
	jet_err = JetRetrieveColumn(m_sesid, tableid, columnid, val, *val_size, val_size, 0, NULL);
	return jet_err;
}

int NTDS::ParseDomainRecords(JET_TABLEID tableid)
{
	unsigned long attributeSize;
	BYTE attributeVal[1024];
	JET_ERR jet_err;

	// Get DNT_col
	DWORD dnt;
	attributeSize = sizeof(attributeVal);
	jet_err = GetRecord(tableid, m_columndef[ID_DNT_COL].columnid, attributeVal, &attributeSize);
	if ((jet_err != JET_errSuccess) || attributeSize != sizeof(DWORD))
	{
		return NTDS_BAD_RECORD;
	}
	dnt = *LPDWORD(attributeVal);

	// Get domain components
	attributeSize = sizeof(attributeVal);
	jet_err = GetRecord(tableid, m_columndef[ID_DOMAIN].columnid, attributeVal, &attributeSize);
	if ((jet_err == JET_errSuccess) && attributeSize)
	{
		m_dnt_to_domain[dnt] = std::wstring((const wchar_t *)attributeVal, attributeSize / sizeof(wchar_t));
	}
	return NTDS_SUCCESS;
}

int NTDS::NTLM_ParseSAMRecord(JET_TABLEID tableid, LDAPAccountInfo ldapAccountEntry)
{
	unsigned long attributeSize;
	BYTE attributeVal[1024];
	JET_ERR jet_err;

	// Browse per SAM account type
	attributeSize = sizeof(attributeVal);
	jet_err = GetRecord(tableid, m_columndef[ID_SAM_ACCOUNT_TYPE].columnid, attributeVal, &attributeSize);
	if ((jet_err != JET_errSuccess) || (attributeSize != sizeof(ldapAccountEntry->dwSAMAccountType)))
	{
		return NTDS_BAD_RECORD;
	}

	ldapAccountEntry->dwSAMAccountType = *(LPDWORD)attributeVal;

	if ((ldapAccountEntry->dwSAMAccountType != SAM_USER_OBJECT) &&
		(ldapAccountEntry->dwSAMAccountType != SAM_MACHINE_ACCOUNT) &&
		(ldapAccountEntry->dwSAMAccountType != SAM_TRUST_ACCOUNT))
	{
		return NTDS_BAD_RECORD;
	}
	if (!m_include_machine_accounts)
	{
		if (ldapAccountEntry->dwSAMAccountType != SAM_USER_OBJECT)
		{
			return NTDS_BAD_RECORD;
		}
	}

	// Get SAM account name
	attributeSize = sizeof(attributeVal);
	jet_err = GetRecord(tableid, m_columndef[ID_SAM_ACCOUNT_NAME].columnid, attributeVal, &attributeSize);
	if ((!attributeSize) || (jet_err != JET_errSuccess))
	{
		return NTDS_BAD_RECORD;
	}

	ldapAccountEntry->szSAMAccountName = std::wstring((const wchar_t *)attributeVal, attributeSize / sizeof(wchar_t));

	// Get user account control
	attributeSize = sizeof(attributeVal);
	jet_err = GetRecord(tableid, m_columndef[ID_USER_ACCOUNT_CONTROL].columnid, attributeVal, &attributeSize);
	if ((jet_err != JET_errSuccess) || (attributeSize != sizeof(DWORD)))
	{
		return NTDS_BAD_RECORD;
	}
	ldapAccountEntry->dwAccountControl = *(LPDWORD)attributeVal;

	// Get description
	attributeSize = sizeof(attributeVal);
	jet_err = GetRecord(tableid, m_columndef[ID_DESCRIPTION].columnid, attributeVal, &attributeSize);
	if (attributeSize && jet_err == JET_errSuccess)
	{
		ldapAccountEntry->szDescription = std::wstring((const wchar_t *)attributeVal, attributeSize / sizeof(wchar_t));
	}
	
	// Get home directory
	attributeSize = sizeof(attributeVal);
	jet_err = GetRecord(tableid, m_columndef[ID_HOME_DIRECTORY].columnid, attributeVal, &attributeSize);
	if (attributeSize && jet_err == JET_errSuccess)
	{
		ldapAccountEntry->szHomeDirectory = std::wstring((const wchar_t *)attributeVal, attributeSize / sizeof(wchar_t));
	}

	// Get first name
	attributeSize = sizeof(attributeVal);
	jet_err = GetRecord(tableid, m_columndef[ID_SURNAME].columnid, attributeVal, &attributeSize);
	if (attributeSize && jet_err == JET_errSuccess)
	{
		ldapAccountEntry->szSurname = std::wstring((const wchar_t *)attributeVal, attributeSize / sizeof(wchar_t));
	}

	// Get given name
	attributeSize = sizeof(attributeVal);
	jet_err = GetRecord(tableid, m_columndef[ID_GIVEN_NAME].columnid, attributeVal, &attributeSize);
	if (attributeSize && jet_err == JET_errSuccess)
	{
		ldapAccountEntry->szGivenName = std::wstring((const wchar_t *)attributeVal, attributeSize / sizeof(wchar_t));
	}

	// Get org unit
	attributeSize = sizeof(attributeVal);
	jet_err = GetRecord(tableid, m_columndef[ID_ORG_UNIT].columnid, attributeVal, &attributeSize);
	if (attributeSize && jet_err == JET_errSuccess)
	{
		ldapAccountEntry->szOrgUnit = std::wstring((const wchar_t *)attributeVal, attributeSize / sizeof(wchar_t));
	}

	// Get last changed time
	attributeSize = sizeof(attributeVal);
	jet_err = GetRecord(tableid, m_columndef[ID_LAST_SET_TIME].columnid, attributeVal, &attributeSize);
	if (attributeSize == sizeof(FILETIME) && jet_err == JET_errSuccess)
	{
		ldapAccountEntry->ftLastChangedTime = *(LPFILETIME)attributeVal;
	}
	
	// Get LM hash
	attributeSize = sizeof(attributeVal);
	jet_err = GetRecord(tableid, m_columndef[ID_LM_HASH].columnid, attributeVal, &attributeSize);
	if ((jet_err == JET_errSuccess) && (attributeSize >= WIN_NTLM_HASH_SIZE+24))
	{
		ldapAccountEntry->LM_hash_ciphered = (CRYPTED_DATA*)malloc(attributeSize);
		memcpy(ldapAccountEntry->LM_hash_ciphered, attributeVal, attributeSize);
		ldapAccountEntry->NTLM_hash.has_lm_hash = true;
	}
	
	// Get NT hash
	attributeSize = sizeof(attributeVal);
	jet_err = GetRecord(tableid, m_columndef[ID_NT_HASH].columnid, attributeVal, &attributeSize);
	if ((jet_err == JET_errSuccess) && (attributeSize >= WIN_NTLM_HASH_SIZE + 24))
	{
		ldapAccountEntry->NT_hash_ciphered = (CRYPTED_DATA*)malloc(attributeSize);
		memcpy(ldapAccountEntry->NT_hash_ciphered, attributeVal, attributeSize);
		ldapAccountEntry->NTLM_hash.has_nt_hash = true;
	}

	// Get Ancestors
	attributeSize = sizeof(attributeVal);
	jet_err = GetRecord(tableid, m_columndef[ID_ANCESTORS_COL].columnid, attributeVal, &attributeSize);
	if ((jet_err == JET_errSuccess) && attributeSize)
	{
		DWORD *pdw = (DWORD *)attributeVal;
		for (int i = 0; i < attributeSize / sizeof(DWORD); i++)
		{
			DWORD pdnt = pdw[i];
			auto iter = m_dnt_to_domain.find(pdnt);
			if (iter != m_dnt_to_domain.end())
			{
				ldapAccountEntry->szDomain = iter->second;
			}
		}
	}

	if (m_with_history) 
	{
		// Get LM hash history
		jet_err = GetRecord(tableid, m_columndef[ID_LM_HASH_HISTORY].columnid, NULL, &attributeSize);
		if (jet_err == JET_errSuccess && attributeSize) 
		{
			ldapAccountEntry->LM_history_ciphered = (CRYPTED_DATA*)malloc(attributeSize);
			ldapAccountEntry->LM_history_deciphered = (LPBYTE)malloc(attributeSize);
			
			if (!ldapAccountEntry->LM_history_ciphered || !ldapAccountEntry->LM_history_deciphered)
				return NTDS_MEM_ERROR;

			jet_err = GetRecord(tableid, m_columndef[ID_LM_HASH_HISTORY].columnid, (LPBYTE)ldapAccountEntry->LM_history_ciphered, &attributeSize);
			if (jet_err != JET_errSuccess)
				return NTDS_API_ERROR;

			ldapAccountEntry->LM_history_ciphered_size = attributeSize;
			ldapAccountEntry->nbHistoryEntriesLM = (attributeSize - 24) / WIN_NTLM_HASH_SIZE;
		}

		// Get NT hash history 
		jet_err = GetRecord(tableid, m_columndef[ID_NT_HASH_HISTORY].columnid, NULL, &attributeSize);
		if (jet_err == JET_errSuccess && attributeSize) 
		{
			ldapAccountEntry->NT_history_ciphered = (CRYPTED_DATA*)malloc(attributeSize);
			ldapAccountEntry->NT_history_deciphered = (LPBYTE)malloc(attributeSize);

			if (!ldapAccountEntry->NT_history_ciphered || !ldapAccountEntry->NT_history_deciphered)
				return NTDS_MEM_ERROR;

			jet_err = GetRecord(tableid, m_columndef[ID_NT_HASH_HISTORY].columnid, (LPBYTE)ldapAccountEntry->NT_history_ciphered, &attributeSize);
			if (jet_err != JET_errSuccess)
				return NTDS_API_ERROR;

			ldapAccountEntry->NT_history_ciphered_size = attributeSize;
			ldapAccountEntry->nbHistoryEntriesNT = (attributeSize - 24) / WIN_NTLM_HASH_SIZE; 
		}
	}
	else
	{
		ldapAccountEntry->nbHistoryEntriesLM = 0;
		ldapAccountEntry->nbHistoryEntriesNT = 0;
	}

	// Get RID
	attributeSize = sizeof(attributeVal);
	jet_err = GetRecord(tableid, m_columndef[ID_OBJECT_SID].columnid, attributeVal, &attributeSize);
	if (jet_err != JET_errSuccess)
	{
		return NTDS_BAD_RECORD;
	}

	ldapAccountEntry->rid = BSWAP(*LPDWORD(attributeVal + attributeSize - sizeof(ldapAccountEntry->rid)));
	
	return NTDS_SUCCESS;
}

int NTDS::NTLM_ParsePEKRecord(JET_TABLEID tableid, CRYPTED_DATA **pek_ciphered,size_t *len_pek_ciphered) 
{
	unsigned long attributeSize;
	BYTE attributeVal[1024];
	JET_ERR jet_err;

	attributeSize = sizeof(attributeVal);

	jet_err = GetRecord(tableid, m_columndef[ID_PEK].columnid, attributeVal, &attributeSize);
	if (jet_err != JET_errSuccess)
	{
		*len_pek_ciphered = 0;
		*pek_ciphered = nullptr;
		return NTDS_BAD_RECORD;
	}
	if (attributeSize < sizeof(32+16+8))
	{
		*len_pek_ciphered = 0;
		*pek_ciphered = nullptr;
		return NTDS_BAD_RECORD;
	}

	*len_pek_ciphered = (size_t)attributeSize;
	*pek_ciphered = (CRYPTED_DATA *)malloc(*len_pek_ciphered);
	if (!*pek_ciphered)
	{
		*len_pek_ciphered = 0;
		return NTDS_MEM_ERROR;
	}

	memcpy(*pek_ciphered, attributeVal, *len_pek_ciphered);
	if ((*pek_ciphered)->dwAlgorithmId != 2 && (*pek_ciphered)->dwAlgorithmId != 3)
	{
		*len_pek_ciphered = 0;
		free(*pek_ciphered);
		*pek_ciphered = nullptr;
		return NTDS_BAD_RECORD;
	}

	return NTDS_SUCCESS;
}

int NTDS::NTLM_ParseDatabase(std::list<LDAPAccountInfo> &ldapAccountInfo, CRYPTED_DATA **pek_ciphered, size_t *len_pek_ciphered, QString & error)
{
	JET_TABLEID tableid;
	JET_ERR jet_err;
	int success = NTDS_SUCCESS;
	int retCode;

	jet_err = JetOpenTable(m_sesid, m_dbid, NTDS_TBL_OBJ, NULL, 0, JET_bitTableReadOnly | JET_bitTableSequential, &tableid);
	if (jet_err != JET_errSuccess) 
	{
		error = ParseJetError(jet_err);
		return NTDS_API_ERROR;
	}

	// Get attributes identifiers 
	jet_err = JetGetTableColumnInfo(m_sesid, tableid, ATT_SAM_ACCOUNT_NAME, &m_columndef[ID_SAM_ACCOUNT_NAME], sizeof(JET_COLUMNDEF), JET_ColInfo);
	jet_err |= JetGetTableColumnInfo(m_sesid, tableid, ATT_OBJECT_SID, &m_columndef[ID_OBJECT_SID], sizeof(JET_COLUMNDEF), JET_ColInfo);
	jet_err |= JetGetTableColumnInfo(m_sesid, tableid, ATT_LM_HASH, &m_columndef[ID_LM_HASH], sizeof(JET_COLUMNDEF), JET_ColInfo);
	jet_err |= JetGetTableColumnInfo(m_sesid, tableid, ATT_NT_HASH, &m_columndef[ID_NT_HASH], sizeof(JET_COLUMNDEF), JET_ColInfo);
	jet_err |= JetGetTableColumnInfo(m_sesid, tableid, ATT_PEK, &m_columndef[ID_PEK], sizeof(JET_COLUMNDEF), JET_ColInfo);
	jet_err |= JetGetTableColumnInfo(m_sesid, tableid, ATT_SAM_ACCOUNT_TYPE, &m_columndef[ID_SAM_ACCOUNT_TYPE], sizeof(JET_COLUMNDEF), JET_ColInfo);
	jet_err |= JetGetTableColumnInfo(m_sesid, tableid, ATT_LM_HASH_HISTORY, &m_columndef[ID_LM_HASH_HISTORY], sizeof(JET_COLUMNDEF), JET_ColInfo);
	jet_err |= JetGetTableColumnInfo(m_sesid, tableid, ATT_NT_HASH_HISTORY, &m_columndef[ID_NT_HASH_HISTORY], sizeof(JET_COLUMNDEF), JET_ColInfo);
	jet_err |= JetGetTableColumnInfo(m_sesid, tableid, ATT_USER_ACCOUNT_CONTROL, &m_columndef[ID_USER_ACCOUNT_CONTROL], sizeof(JET_COLUMNDEF), JET_ColInfo);
	jet_err |= JetGetTableColumnInfo(m_sesid, tableid, ATT_PASSWORD_LAST_SET_INDEX, &m_columndef[ID_LAST_SET_TIME], sizeof(JET_COLUMNDEF), JET_ColInfo);
	jet_err |= JetGetTableColumnInfo(m_sesid, tableid, ATT_DESCRIPTION, &m_columndef[ID_DESCRIPTION], sizeof(JET_COLUMNDEF), JET_ColInfo);
	jet_err |= JetGetTableColumnInfo(m_sesid, tableid, ATT_HOME_DIRECTORY, &m_columndef[ID_HOME_DIRECTORY], sizeof(JET_COLUMNDEF), JET_ColInfo);
	jet_err |= JetGetTableColumnInfo(m_sesid, tableid, ATT_DOMAIN, &m_columndef[ID_DOMAIN], sizeof(JET_COLUMNDEF), JET_ColInfo);
	jet_err |= JetGetTableColumnInfo(m_sesid, tableid, ATT_SURNAME, &m_columndef[ID_SURNAME], sizeof(JET_COLUMNDEF), JET_ColInfo);
	jet_err |= JetGetTableColumnInfo(m_sesid, tableid, ATT_GIVEN_NAME, &m_columndef[ID_GIVEN_NAME], sizeof(JET_COLUMNDEF), JET_ColInfo);
	jet_err |= JetGetTableColumnInfo(m_sesid, tableid, ATT_ORG_UNIT, &m_columndef[ID_ORG_UNIT], sizeof(JET_COLUMNDEF), JET_ColInfo);
	jet_err |= JetGetTableColumnInfo(m_sesid, tableid, "DNT_col", &m_columndef[ID_DNT_COL], sizeof(JET_COLUMNDEF), JET_ColInfo);

	if (jet_err != JET_errSuccess) 
	{
		error = ParseJetError(jet_err);
		JetCloseTable(m_sesid, tableid);
		return NTDS_API_ERROR;
	}

	// Parse datatable for ciphered PEK and domain information
	jet_err = JetMove(m_sesid, tableid, JET_MoveFirst, 0);
	bool got_pek = false;
	do{
		if (!got_pek && NTLM_ParsePEKRecord(tableid, pek_ciphered, len_pek_ciphered) == NTDS_SUCCESS)
		{
			got_pek = true;
		}
		
		ParseDomainRecords(tableid);

	} while (JetMove(m_sesid, tableid, JET_MoveNext, 0) == JET_errSuccess);

	if (!got_pek)
	{
		error = "Unable to decrypt NTDS.DIT, missing PEK";
		JetCloseTable(m_sesid, tableid);
		return NTDS_BAD_RECORD;
	}

	// Parse datatable for SAM accounts
	jet_err = JetMove(m_sesid, tableid, JET_MoveFirst, 0);
	do{
		LDAPAccountInfo ldapAccountEntry = std::make_shared<LDAP_ACCOUNT_INFO>();
		retCode = NTLM_ParseSAMRecord(tableid, ldapAccountEntry);
		if (retCode == NTDS_SUCCESS)
		{
			ldapAccountInfo.push_back(ldapAccountEntry);
		}
		else if (retCode == NTDS_MEM_ERROR) {
			error = "Memory error.";
			jet_err = JetCloseTable(m_sesid, tableid);
			free(*pek_ciphered);
			*pek_ciphered = nullptr;
			return retCode;
		}

	} while (JetMove(m_sesid, tableid, JET_MoveNext, 0) == JET_errSuccess);

	if (ldapAccountInfo.size()==0)
		success = NTDS_EMPTY_ERROR;

	jet_err = JetCloseTable(m_sesid, tableid);
	if (jet_err != JET_errSuccess) {
		error = ParseJetError(jet_err);
		free(*pek_ciphered);
		*pek_ciphered = nullptr;
		return NTDS_API_ERROR;
	}

	return success;
}



// Try to parse a bitlocker record
// Parsing is done through Volume GUID (TODO: per machine name)

int NTDS::Bitlocker_ParseRecord(JET_TABLEID tableid, BitlockerAccountInfo *bitlockerAccountEntry) 
{
	unsigned long attributeSize;
	BYTE attributeVal[2048];
	JET_ERR jet_err;

	memset(bitlockerAccountEntry, 0, sizeof(BitlockerAccountInfo));

	// Parse per Volume GUID
	attributeSize = sizeof(attributeVal);
	jet_err = GetRecord(tableid, m_columndef[ID_MSFVE_VOLUME_GUID].columnid, attributeVal, &attributeSize);
	if ((!attributeSize) || (jet_err != JET_errSuccess) || (attributeSize != sizeof(bitlockerAccountEntry->msFVE_VolumeGUID)))
	{
		return NTDS_BAD_RECORD;
	}

	memcpy(&bitlockerAccountEntry->msFVE_VolumeGUID, attributeVal, attributeSize);

	// Get recovery GUID
	attributeSize = sizeof(attributeVal);
	jet_err = GetRecord(tableid, m_columndef[ID_MSFVE_RECOVERY_GUID].columnid, attributeVal, &attributeSize);
	if ((jet_err == JET_errSuccess) && (attributeSize == sizeof(bitlockerAccountEntry->msFVE_RecoveryGUID))) 
	{
		memcpy(&bitlockerAccountEntry->msFVE_RecoveryGUID, attributeVal, attributeSize);
	}

	// Get recovery password
	attributeSize = sizeof(attributeVal);
	jet_err = GetRecord(tableid, m_columndef[ID_MSFVE_RECOVERY_PASSWORD].columnid, attributeVal, &attributeSize);
	if ((jet_err == JET_errSuccess) && (attributeSize == (sizeof(bitlockerAccountEntry->msFVE_RecoveryPassword) - 2))) 
	{
		memcpy(&bitlockerAccountEntry->msFVE_RecoveryPassword, attributeVal, attributeSize);
	}

	// Get key package
	attributeSize = sizeof(attributeVal);
	jet_err = GetRecord(tableid, m_columndef[ID_MSFVE_KEY_PACKAGE].columnid, attributeVal, &attributeSize);
	if ((jet_err == JET_errSuccess) && attributeSize) 
	{
		bitlockerAccountEntry->msFVE_KeyPackage = (LPBYTE)malloc(attributeSize);
		if (!bitlockerAccountEntry->msFVE_KeyPackage)
		{
			return NTDS_MEM_ERROR;
		}
		memcpy(bitlockerAccountEntry->msFVE_KeyPackage, attributeVal, attributeSize);
		bitlockerAccountEntry->dwSzKeyPackage = attributeSize;
	}

	return NTDS_SUCCESS;
}



// Parse NTDS file database and extract bitlocker related attributes
// (Key package, recovery password, recovery guid, volume id)

int NTDS::Bitlocker_ParseDatabase(std::list<BitlockerAccountInfo> & bitlockerAccountInfo, QString & error)
{
	BitlockerAccountInfo bitlockerAccountEntry;
	JET_TABLEID tableid;
	JET_ERR jet_err;
	int success = NTDS_SUCCESS;
	int retCode;

	jet_err = JetOpenTable(m_sesid, m_dbid, NTDS_TBL_OBJ, NULL, 0, JET_bitTableReadOnly | JET_bitTableSequential, &tableid);
	if (jet_err != JET_errSuccess) 
	{
		error = ParseJetError(jet_err);
		return NTDS_API_ERROR;
	}

	// Get attributes identifiers
	jet_err = JetGetTableColumnInfo(m_sesid, tableid, ATT_BITLOCKER_MSFVE_KEY_PACKAGE, &m_columndef[ID_MSFVE_KEY_PACKAGE], sizeof(JET_COLUMNDEF), JET_ColInfo);
	jet_err |= JetGetTableColumnInfo(m_sesid, tableid, ATT_BITLOCKER_MSFVE_RECOVERY_GUID, &m_columndef[ID_MSFVE_RECOVERY_GUID], sizeof(JET_COLUMNDEF), JET_ColInfo);
	jet_err |= JetGetTableColumnInfo(m_sesid, tableid, ATT_BITLOCKER_MSFVE_RECOVERY_PASSWORD, &m_columndef[ID_MSFVE_RECOVERY_PASSWORD], sizeof(JET_COLUMNDEF), JET_ColInfo);
	jet_err |= JetGetTableColumnInfo(m_sesid, tableid, ATT_BITLOCKER_MSFVE_VOLUME_GUID, &m_columndef[ID_MSFVE_VOLUME_GUID], sizeof(JET_COLUMNDEF), JET_ColInfo);

	if (jet_err != JET_errSuccess) 
	{
		error = ParseJetError(jet_err);
		JetCloseTable(m_sesid, tableid);
		return NTDS_API_ERROR;
	}

	// Parse datatable for Bitlocker accounts 
	jet_err = JetMove(m_sesid, tableid, JET_MoveFirst, 0);
	do {
		retCode = Bitlocker_ParseRecord(tableid, &bitlockerAccountEntry);
		if (retCode == NTDS_SUCCESS) 
		{
			bitlockerAccountInfo.push_back(bitlockerAccountEntry);
		}
		else if (retCode == NTDS_MEM_ERROR)
		{
			return retCode;
		}
	} while (JetMove(m_sesid, tableid, JET_MoveNext, 0) == JET_errSuccess);

	if (bitlockerAccountInfo.size()==0)
	{
		success = NTDS_EMPTY_ERROR;
	}

	jet_err = JetCloseTable(m_sesid, tableid);
	if (jet_err != JET_errSuccess) 
	{
		error = ParseJetError(jet_err);
		return NTDS_API_ERROR;
	}

	return success;
}

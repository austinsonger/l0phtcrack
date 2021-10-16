#include"stdafx.h"
#include"ntreg.h"
#include"crypt.h"
//#include"slow_md5.h"
#include"slow_des.h"
//#include"slow_rc4.h"
#include"ntds.h"
#include<memory>

/////////////////////////////////////////////////////////////////////////////////////////////////////////

NTDSImporter::NTDSImporter(ILC7AccountList *accountlist, ILC7CommandControl *ctrl)
{
	TR;
	if (ctrl)
		m_ctrl = ctrl->GetSubControl("NTDS Importer: ");
	else
		m_ctrl = nullptr;

	m_accountlist = accountlist;
	m_account_limit = 0;
	m_with_history = false;
	m_include_machine_accounts = false;
}

NTDSImporter::~NTDSImporter()
{
	TR;
	if (m_ctrl)
		m_ctrl->ReleaseSubControl();
}

void NTDSImporter::SetAccountLimit(quint32 alim)
{
	m_account_limit = alim;
}

void NTDSImporter::SetWithHistory(bool with_history)
{
	m_with_history = with_history;
}

void NTDSImporter::SetIncludeMachineAccounts(bool include_machine_accounts)
{
	m_include_machine_accounts = include_machine_accounts;
}


bool NTDSImporter::DoImport(QString ntds_filename, QString system_filename, QString & error, bool & cancelled)
{
	TR;
	
	UpdateStatus("Importing from NTDS.DIT file...", 0, true);
	
	CRYPTED_DATA *_PEK_ciphered = nullptr;
	PEK_LIST _PEK_LIST;
	SYSKEY _SYSKEY;
	//BOOTKEY_ciphered _BOOTKEY_ciphered;
	//BOOTKEY _BOOTKEY;
	//LSAKEY_ciphered _LSAKEY_ciphered;
	//LSAKEY _LSAKEY;
	//NLKM_ciphered _NLKM_ciphered;
	//NLKM _NLKM;


	struct hive *system_hive = open_hive(QDir::toNativeSeparators(system_filename).toLocal8Bit().constData(), HMODE_RO);
	if (system_hive == NULL)
	{
		error = "Couldn't open SYSTEM file. Possibly improper format, access denied or file inexistant.";
		return false;
	}

	size_t bootkeylen;
	unsigned char *bootkey = GetBootKey(system_hive, bootkeylen);
	if (!bootkey)
	{
		error = "Unable to get SYSKEY from SYSTEM hive.";
		close_hive(system_hive);
		return false;
	}

	memcpy(_SYSKEY.key, bootkey, sizeof(_SYSKEY.key));

	free(bootkey);
	close_hive(system_hive);



	NTDS ntds(m_with_history, m_include_machine_accounts);
	if (!ntds.Init(error))
	{
		error = "Unable to initialize NTDS";
		return false;
	}
	
	if (!ntds.OpenDatabase(QDir::toNativeSeparators(ntds_filename).toLocal8Bit().constData(), error))
	{
		return false;
	}
	
	std::list<LDAPAccountInfo> ldapAccountDatabase;
	size_t lenPekCiphered;
	if (ntds.NTLM_ParseDatabase(ldapAccountDatabase, &_PEK_ciphered, &lenPekCiphered, error) != NTDS_SUCCESS)
	{
		QString cderr;

		ntds.CloseDatabase(cderr);
		return false;
	}

	if (CRYPT_Decipher_PEK(_PEK_ciphered, lenPekCiphered, &_SYSKEY, _PEK_LIST) != LSAKEY_SUCCESS)
	{
		error = "Unable to decipher PEK";

		QString cderr;
		ntds.CloseDatabase(cderr);
		return false;
	}
	if (!CRYPT_NTDS_DecipherAllAccount(ldapAccountDatabase, &_SYSKEY, _PEK_LIST))
	{
		error = "Unable to decipher accounts";

		QString cderr;
		ntds.CloseDatabase(cderr);
		return false;
	}
	
	if (!ntds.CloseDatabase(error))
	{
		return false;
	}

	int account_count = (int)ldapAccountDatabase.size();

	bool success = true;

	m_accountlist->Acquire();
	int starting_count = m_accountlist->GetAccountCount();
	
	int imported_count = 0;
	if (m_ctrl)
		m_ctrl->UpdateCurrentProgressBar(0);

	for(LDAPAccountInfo & currentAccount : ldapAccountDatabase) 
	{
		if (!AddEntry(currentAccount, error))
		{
			success = false;
			break;
		}
		else
		{
			imported_count++;
		}

		if ((imported_count % 100) == 0)
		{
			quint32 complete = (quint32)(imported_count * 100) / account_count;
			QString str = QString("%1 users imported out of %2").arg(imported_count).arg(account_count);
			UpdateStatus(str, complete, false);
		}

		if (m_account_limit && imported_count >= m_account_limit)
		{
			break;
		}
	}

	QString str = QString("%1 users imported out of %2").arg(imported_count).arg(account_count);
	UpdateStatus(str, 100, true);

	// add endbatch, and remove account_count-imported_count
	if (imported_count < account_count)
	{
		std::set<int> positions;
		for (int x = (starting_count + imported_count); x<(starting_count + account_count); x++)
		{
			positions.insert(x);
		}
		m_accountlist->RemoveAccounts(positions);
	}
	m_accountlist->Release();

	return success;
}




static QString convert_string(unsigned char *V, quint32 offset, quint32 length)
{
	const ushort *w = (const ushort *)(V + offset);
	QString str = QString::fromUtf16(w, length / 2);

	return str;
}

bool NTDSImporter::AddEntry(LDAPAccountInfo ldapAccountInfo, QString &error)
{
	LC7Account acct;

	if (ldapAccountInfo->NTLM_hash.has_lm_hash)
	{
		LC7Hash hash;
		hash.hashtype = FOURCC(HASHTYPE_LM);
		hash.crackstate = 0;
		hash.cracktime = 0;

		QByteArray hbytes((const char *)ldapAccountInfo->NTLM_hash.LM_hash, 16);

		hash.hash = hbytes.toHex().toUpper();

		if (hash.hash == "AAD3B435B51404EEAAD3B435B51404EE")
		{
			hash.crackstate = CRACKSTATE_CRACKED;
			hash.cracktype = "No Password";
		}
		else if (hash.hash.startsWith("AAD3B435B51404EE"))
		{
			hash.crackstate = CRACKSTATE_FIRSTHALF_CRACKED;
		}
		else if (hash.hash.endsWith("AAD3B435B51404EE"))
		{
			hash.crackstate = CRACKSTATE_SECONDHALF_CRACKED;
		}

		acct.hashes.append(hash);
	}

	if (ldapAccountInfo->NTLM_hash.has_nt_hash)
	{
		LC7Hash hash;
		hash.hashtype = FOURCC(HASHTYPE_NT);
		hash.crackstate = 0;
		hash.cracktime = 0;

		QByteArray hbytes((const char *)ldapAccountInfo->NTLM_hash.NT_hash, 16);

		hash.hash = hbytes.toHex().toUpper();

		if (hash.hash == "31D6CFE0D16AE931B73C59D7E0C089C0")
		{
			hash.crackstate = CRACKSTATE_CRACKED;
			hash.cracktype = "No Password";
		}

		acct.hashes.append(hash);
	}

	acct.userid = QString("%1").arg(ldapAccountInfo->rid);

	acct.username = QString::fromStdWString(ldapAccountInfo->szSAMAccountName);
	acct.domain = QString::fromStdWString(ldapAccountInfo->szDomain);
	
	quint64 filetime_lastchanged = (((quint64)ldapAccountInfo->ftLastChangedTime.dwHighDateTime) << 32) | ((quint64)ldapAccountInfo->ftLastChangedTime.dwLowDateTime);
	
	QString strfullname = QString::fromStdWString(ldapAccountInfo->szGivenName) + " " + QString::fromStdWString(ldapAccountInfo->szSurname);
	strfullname = strfullname.trimmed();
	QString strcomment = QString::fromStdWString(ldapAccountInfo->szDescription);
	QString orgunit = QString::fromStdWString(ldapAccountInfo->szOrgUnit);
	QString strhomedir = QString::fromStdWString(ldapAccountInfo->szHomeDirectory);

	acct.lastchanged = (filetime_lastchanged > 0) ? (filetime_lastchanged / 10000000ULL - 11644473600ULL) : 0;
	acct.lockedout = (ldapAccountInfo->dwAccountControl & UAC_LOCKED_OUT) != 0;
	acct.disabled = (ldapAccountInfo->dwAccountControl & UAC_DISABLED) != 0;
	acct.neverexpires = (ldapAccountInfo->dwAccountControl & UAC_PASSWORD_NEVER_EXPIRES) != 0;
	acct.mustchange = (ldapAccountInfo->dwAccountControl & UAC_PASSWORD_EXPIRED) != 0;
	acct.machine = "";
	acct.userinfo = QString("%1 %2 %3").arg(strfullname).arg(strcomment).arg(strhomedir).trimmed();
	acct.remediations = -1;

	if (!m_accountlist->AppendAccount(acct))
	{
		error = "Account limit reached, upgrade your license to import more accounts.";
		return false;
	}

	return true;
}


void NTDSImporter::UpdateStatus(QString statustext, quint32 cur, bool statuslog)
{
	TR;
	if (m_ctrl)
	{
		m_ctrl->SetStatusText(statustext);
		m_ctrl->UpdateCurrentProgressBar(cur);
		if (statuslog)
		{
			m_ctrl->AppendToActivityLog(statustext + "\n");
		}
	}
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////

// Permutation matrix for boot key
static BYTE p[] = { 0x8, 0x5, 0x4, 0x2, 0xb, 0x9, 0xd, 0x3,
0x0, 0x6, 0x1, 0xc, 0xe, 0xa, 0xf, 0x7 };

// Constants for NTDS decrypt algorithm
static const char *aqwerty = "!@#$%^&*()qwertyUIOPAzxcvbnmQQQQQQQQQQQQ)(*@&%";
static const char *anum = "0123456789012345678901234567890123456789";
static const char *antpassword = "NTPASSWORD";
static const char *almpassword = "LMPASSWORD";

static BYTE empty_lm[] = { 0xAA, 0xD3, 0xB4, 0x35, 0xB5, 0x14, 0x04, 0xEE, 0xAA, 0xD3, 0xB4, 0x35, 0xB5, 0x14, 0x04, 0xEE };
static BYTE empty_nt[] = { 0x31, 0xD6, 0xCF, 0xE0, 0xD1, 0x6A, 0xE9, 0x31, 0xB7, 0x3C, 0x59, 0xD7, 0xE0, 0xC0, 0x89, 0xC0 };

// sid_to_key1: Function to convert the RID to the first decrypt key.
static void sid_to_key1(unsigned long sid, unsigned char deskey[8])
{
	char s[7];

	s[0] = (unsigned char)(sid & 0xFF);
	s[1] = (unsigned char)((sid >> 8) & 0xFF);
	s[2] = (unsigned char)((sid >> 16) & 0xFF);
	s[3] = (unsigned char)((sid >> 24) & 0xFF);
	s[4] = s[0];
	s[5] = s[1];
	s[6] = s[2];

	slowdes_str_to_key(s, deskey);
}

// sid_to_key2: Function to convert the RID to the second decrypt key.
static void sid_to_key2(unsigned long sid, unsigned char deskey[8])
{
	char s[7];

	s[0] = (unsigned char)((sid >> 24) & 0xFF);
	s[1] = (unsigned char)(sid & 0xFF);
	s[2] = (unsigned char)((sid >> 8) & 0xFF);
	s[3] = (unsigned char)((sid >> 16) & 0xFF);
	s[4] = s[0];
	s[5] = s[1];
	s[6] = s[2];

	slowdes_str_to_key(s, deskey);
}

quint32 NTDSImporter::FindControlSet(struct hive *system_hive)
{
	int ofs_selectkey = trav_path(system_hive, 0, "Select", TPF_NK);
	if (!ofs_selectkey)
	{
		return 1;
	}
	ofs_selectkey += 4;

	int type = get_val_type(system_hive, ofs_selectkey, "Current", TPF_VK);
	if (type == -1 || type != REG_DWORD) {
		return 1;
	}

	quint32 len = get_val_len(system_hive, ofs_selectkey, "Current", TPF_VK);
	if (len != sizeof(quint32)) {
		return 1;
	}

	quint32 *data = (quint32 *)get_val_data(system_hive, ofs_selectkey, "Current", 0, TPF_VK);
	if (!data) {
		return 1;
	}

	return *data;
}

unsigned char *NTDSImporter::GetBootKey(struct hive *system_hive, size_t &bootkeylen)
{
	quint32 cs = FindControlSet(system_hive);

	QString control_set=QString("ControlSet%1\\Control\\Lsa").arg(cs,3,10,QChar('0'));
	int ofs_lsa = trav_path(system_hive, 0, control_set.toLatin1().constData(), TPF_NK);
	if (!ofs_lsa)
	{
		return 0;
	}
	ofs_lsa += 4;

	const char *lsa_keys[] = { "JD", "Skew1", "GBG", "Data" };
	unsigned char *bootkey = NULL;
	bootkeylen = 0;
	for (int i = 0; i<_countof(lsa_keys); i++)
	{
		struct keyval *kv = get_class(system_hive, ofs_lsa, lsa_keys[i]);

		const ushort *classstr = (const ushort *)&(kv->data);
		QByteArray strClass = QString::fromUtf16(classstr).toLatin1();
		QByteArray databuf = QByteArray::fromHex(strClass);

		size_t oldbootkeylen = bootkeylen;
		bootkeylen += databuf.size();
		bootkey = (unsigned char *)realloc(bootkey, bootkeylen);
		memcpy(bootkey + oldbootkeylen, databuf.constData(), databuf.size());

		free(kv);
	}

	unsigned char *bootkey_scrambled = (unsigned char *)malloc(bootkeylen);
	for (size_t i = 0; i<bootkeylen; i++)
	{
		bootkey_scrambled[i] = bootkey[p[i]];
	}

	free(bootkey);

	return bootkey_scrambled;

}




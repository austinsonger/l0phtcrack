#include"stdafx.h"
#include"ntreg.h"
#include"crypt.h"
#include"slow_des.h"
#include"slow_rc4.h"
//#include"slow_md5.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////

SAMImporter::SAMImporter(ILC7AccountList *accountlist, ILC7CommandControl *ctrl)
{
	TR;
	if (ctrl)
		m_ctrl = ctrl->GetSubControl("SAM Importer: ");
	else
		m_ctrl = nullptr;

	m_accountlist = accountlist;
	m_account_limit = 0;

	slowdes_init();
}

SAMImporter::~SAMImporter()
{
	TR;
	if (m_ctrl)
		m_ctrl->ReleaseSubControl();
}

void SAMImporter::SetAccountLimit(quint32 alim)
{
	m_account_limit = alim;
}


bool SAMImporter::DoImport(QString sam_filename, QString system_filename, QString & error, bool & cancelled)
{
	TR;

	HKEY sam_start_key = HKEY_LOCAL_MACHINE;

	struct hive *sam_hive = open_hive(QDir::toNativeSeparators(sam_filename).toLocal8Bit().constData(), HMODE_RO);
	if (sam_hive == NULL)
	{
		error = "Couldn't open SAM file. Possibly improper format, access denied or file inexistant.";
		return false;
	}

	struct hive *system_hive = open_hive(QDir::toNativeSeparators(system_filename).toLocal8Bit().constData(), HMODE_RO);
	if (system_hive == NULL)
	{
		error = "Couldn't open SYSTEM file. Possibly improper format, access denied or file inexistant.";
		close_hive(sam_hive);
		return false;
	}

	size_t bootkeylen;
	unsigned char *bootkey = GetBootKey(system_hive, bootkeylen);
	if (!bootkey)
	{
		close_hive(sam_hive);
		close_hive(system_hive);
		return false;
	}

	quint8 hbootkey[16];
	if (!GetHBootKey(sam_hive, bootkey, bootkeylen, hbootkey))
	{
		free(bootkey);
		close_hive(sam_hive);
		close_hive(system_hive);
		return false;
	}
	free(bootkey);

	int ofs_userskey = trav_path(sam_hive, 0, "\\SAM\\Domains\\Account\\Users", TPF_NK);
	if (!ofs_userskey)
	{
		error="Couldn't open \\SAM\\Domains\\Account\\Users in SAM file. Possibly improper format.";
		close_hive(sam_hive);
		close_hive(system_hive);
		return false;
	}
	ofs_userskey += 4;

	// Enumerate the SAM entries
	struct nk_key *key = (struct nk_key *)(sam_hive->buffer + ofs_userskey);
	if (key->id != 0x6b6e) {
		error="Error: Not a 'nk' node!\n";
		close_hive(sam_hive);
		close_hive(system_hive);
		return false;
	}

	if (key->no_subkeys == 0)
	{
		close_hive(sam_hive);
		close_hive(system_hive);
		return true;
	}

	UpdateStatus("Importing from SAM file...", 0, true);

	int account_count = key->no_subkeys;

	bool success = true;

	m_accountlist->Acquire();
	int starting_count = m_accountlist->GetAccountCount();
	
	int imported_count = 0;
	if (m_ctrl)
		m_ctrl->UpdateCurrentProgressBar(0);

	struct ex_data ex;
	int count = 0, countri = 0;
	while ((ex_next_n(sam_hive, ofs_userskey, &count, &countri, &ex) > 0))
	{
		if (m_ctrl && m_ctrl->StopRequested())
		{
			cancelled = true;
			break;
		}


		quint32 rid = strtoul(ex.name, 0, 16);

		// Hack as we know there is a Names key here
		if (rid != 0)
		{
			if (AddEntry(ex.nkoffs + 4, rid, sam_hive, hbootkey, error) == 0)
			{
				success = false;
				break;
			}
			else
			{
				imported_count++;
			}
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

	close_hive(sam_hive);
	close_hive(system_hive);

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
	if (!offset || !length)
	{
		return QString();
	}

	const ushort *w = (const ushort *)(V + offset);
	QString str = QString::fromUtf16(w, length / 2);

	return str;
}

static QByteArray convert_byte_array(unsigned char *V, quint32 offset, quint32 length)
{
	if (!offset || !length)
	{
		return QByteArray();
	}

	const char *b = (const char *)(V + offset);
	QByteArray ba = QByteArray(b, length);

	return ba;
}


#define ACB_DISABLED   0x0001  /* Act disabled */
#define ACB_HOMDIRREQ  0x0002  /* Home directory required */
#define ACB_PWNOTREQ   0x0004  /* User password not req */
#define ACB_TEMPDUP    0x0008  /* Temporary duplicate account?? */
#define ACB_NORMAL     0x0010  /* Normal user account */
#define ACB_MNS        0x0020  /* MNS logon user account */
#define ACB_DOMTRUST   0x0040  /* Interdomain trust account */
#define ACB_WSTRUST    0x0080  /* Workstation trust account */

#define ACB_SVRTRUST   0x0100  /*  Server trust account */
#define ACB_PWNOEXP    0x0200  /* User password does not expire */
/* Seems not to be used on failed console logins at least */
#define ACB_AUTOLOCK   0x0400  /* Account auto locked */
#define ACB_ENC_TXT_PWD_ALLOWED 0x00000800 /* 1 = Encryped text password is allowed */
#define ACB_SMARTCARD_REQUIRED 0x00001000 /* 1 = Smart Card required */
#define ACB_TRUSTED_FOR_DELEGATION 0x00002000 /* 1 = Trusted for Delegation */
#define ACB_NOT_DELEGATED 0x00004000 /* 1 = Not delegated */
#define ACB_USE_DES_KEY_ONLY 0x00008000 /* 1 = Use DES key only */
#define ACB_DONT_REQUIRE_PREAUTH 0x00010000 /* 1 = Preauth not required */
#define ACB_PWEXPIRED			0x00020000  /* 1 = Password is expired */
#define ACB_NO_AUTH_DATA_REQD		0x00080000  /* 1 = No authorization data required */

struct SAM_HASH_V1 {
	quint16 pekid;
	quint16 rev;
	quint8 data[1];
};

struct SAM_HASH_V2 {
	quint16 pekid;
	quint16 rev;
	quint32 offset;
	quint8 salt[16];
	quint8 data[1];
};

bool SAMImporter::AddEntry(int ofs_entrykey, quint32 rid, struct hive *sam_hive, quint8 hbootkey[16], QString &error)
{
	int vtype = get_val_type(sam_hive, ofs_entrykey, "V", 0);
	if (vtype == -1)
	{
		error = "Registry format error";
		return false;
	}
	int vlen = get_val_len(sam_hive, ofs_entrykey, "V", 0);
	if (vlen == -1)
	{
		error = "Registry format error";
		return false;
	}
	unsigned char *V = (unsigned char *)get_val_data(sam_hive, ofs_entrykey, "V", vtype, 0);
	if (V == NULL)
	{
		error = "Registry format error";
		return false;
	}

	quint32 name_offset = *(quint32 *)(V + 0xC) + 0xCC;
	quint32 name_length = *(quint32 *)(V + 0x10);
	quint32 fullname_offset = *(quint32 *)(V + 0x18) + 0xCC;
	quint32 fullname_length = *(quint32 *)(V + 0x1C);
	quint32 comment_offset = *(quint32 *)(V + 0x24) + 0xCC;
	quint32 comment_length = *(quint32 *)(V + 0x28);
	quint32 homedir_offset = *(quint32 *)(V + 0x48) + 0xCC;
	quint32 homedir_length = *(quint32 *)(V + 0x4C);

	QString strname = convert_string(V, name_offset, name_length);
	QString strfullname = convert_string(V, fullname_offset, fullname_length);
	QString strcomment = convert_string(V, comment_offset, comment_length);
	QString strhomedir = convert_string(V, homedir_offset, homedir_length);
	
	quint32 lmhash_offset = *(quint32 *)(V + 0x9c) + 0xCC;
	quint32 lmhash_length = *(quint32 *)(V + 0xA0);
	quint32 nthash_offset = *(quint32 *)(V + 0xA8) + 0xCC;
	quint32 nthash_length = *(quint32 *)(V + 0xAC);
	
	QByteArray lmhash = convert_byte_array(V, lmhash_offset, lmhash_length);
	QByteArray nthash = convert_byte_array(V, nthash_offset, nthash_length);
	
	quint8 enc_lm_hash[16];
	bool lm_exists = DecryptHash(rid, lmhash, enc_lm_hash, hbootkey, false);
	
	quint8 enc_nt_hash[16];
	bool nt_exists = DecryptHash(rid, nthash, enc_nt_hash, hbootkey, true);

	// Get F Bits
	int ftype = get_val_type(sam_hive, ofs_entrykey, "F", 0);
	if (ftype == -1)
	{
		error = "Registry format error";
		return false;
	}
	int flen = get_val_len(sam_hive, ofs_entrykey, "F", 0);
	if (flen == -1)
	{
		error = "Registry format error";
		return false;
	}
	unsigned char *F = (unsigned char *)get_val_data(sam_hive, ofs_entrykey, "F", ftype, 0);
	if (F == NULL)
	{
		error = "Registry format error";
		return false;
	}

	quint16 bits = *(quint16 *)(F + 0x38);

	LC7Account acct;

	if (lm_exists)
	{
		LC7Hash hash;
		hash.hashtype = FOURCC(HASHTYPE_LM);
		hash.crackstate = 0;
		hash.cracktime = 0;

		QByteArray hbytes((const char *)enc_lm_hash, 16);

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

	if (nt_exists)
	{
		LC7Hash hash;
		hash.hashtype = FOURCC(HASHTYPE_NT);
		hash.crackstate = 0;
		hash.cracktime = 0;

		QByteArray hbytes((const char *)enc_nt_hash, 16);

		hash.hash = hbytes.toHex().toUpper();

		if (hash.hash == "31D6CFE0D16AE931B73C59D7E0C089C0")
		{
			hash.crackstate = CRACKSTATE_CRACKED;
			hash.cracktype = "No Password";
		}

		acct.hashes.append(hash);
	}

	acct.userid = QString("%1").arg(rid);

	acct.username = strname;
	acct.domain = "";
	
	quint64 filetime_lastchanged = *(quint64 *)(F + 0x18);
	quint64 filetime_expires = *(quint64 *)(F + 0x20);

	acct.lastchanged = (filetime_lastchanged > 0) ? (filetime_lastchanged / 10000000ULL - 11644473600ULL) : 0;
	acct.lockedout = (bits & ACB_AUTOLOCK) != 0;
	acct.disabled = (bits & ACB_DISABLED)!=0;
	acct.neverexpires = (bits & ACB_PWNOEXP) != 0;
	acct.mustchange = (bits & ACB_PWEXPIRED) != 0;
	acct.machine = "";
	acct.userinfo = QString("%1 %2 %3").arg(strfullname).arg(strcomment).arg(strhomedir);

	/* This is for account expiration, not password expiration
	if (filetime_expires == 0)
	{

	}
	else if (filetime_expires == 0x7FFFFFFFFFFFFFFFULL)
	{

	}
	else
	{
		time_t expiretime = (filetime_expires / 10000000ULL - 11644473600ULL);
		time_t now = QDateTime::currentDateTimeUtc().toTime_t();

		if (expiretime <= now)
		{
			acct.expired = true;
		}
	}
	*/

	acct.remediations = -1;

	if (!m_accountlist->AppendAccount(acct))
	{
		error = "Account limit reached, upgrade your license to import more accounts.";
		return false;
	}

	return true;
}


void SAMImporter::UpdateStatus(QString statustext, quint32 cur, bool statuslog)
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

// Constants for SAM decrypt algorithm
static const char *aqwerty = "!@#$%^&*()qwertyUIOPAzxcvbnmQQQQQQQQQQQQ)(*@&%";
static const char *anum = "0123456789012345678901234567890123456789";
static const char *antpassword = "NTPASSWORD";
static const char *almpassword = "LMPASSWORD";

static BYTE empty_lm[] = { 0xAA, 0xD3, 0xB4, 0x35, 0xB5, 0x14, 0x04, 0xEE, 0xAA, 0xD3, 0xB4, 0x35, 0xB5, 0x14, 0x04, 0xEE };
static BYTE empty_nt[] = { 0x31, 0xD6, 0xCF, 0xE0, 0xD1, 0x6A, 0xE9, 0x31, 0xB7, 0x3C, 0x59, 0xD7, 0xE0, 0xC0, 0x89, 0xC0 };

// rid_to_key1: Function to convert the RID to the first decrypt key.
static void rid_to_key1(uint32_t rid, unsigned char deskey[8])
{
	char s[7];

	s[0] = (unsigned char)(rid & 0xFF);
	s[1] = (unsigned char)((rid >> 8) & 0xFF);
	s[2] = (unsigned char)((rid >> 16) & 0xFF);
	s[3] = (unsigned char)((rid >> 24) & 0xFF);
	s[4] = s[0];
	s[5] = s[1];
	s[6] = s[2];

	slowdes_str_to_key(s, deskey);
}

// rid_to_key2: Function to convert the RID to the second decrypt key.
static void rid_to_key2(uint32_t rid, unsigned char deskey[8])
{
	char s[7];

	s[0] = (unsigned char)((rid >> 24) & 0xFF);
	s[1] = (unsigned char)(rid & 0xFF);
	s[2] = (unsigned char)((rid >> 8) & 0xFF);
	s[3] = (unsigned char)((rid >> 16) & 0xFF);
	s[4] = s[0];
	s[5] = s[1];
	s[6] = s[2];

	slowdes_str_to_key(s, deskey);
}

bool SAMImporter::DecryptHash(quint32 rid, QByteArray hash_data, quint8 out_hash[16], quint8 *hbootkey, bool isNT)
{
	if (hash_data.size()<=4)
	{
		return false;
	}

	quint8 obfkey[16];

	SAM_HASH_V1 *sam_hash = (SAM_HASH_V1 *)hash_data.data();
	if (sam_hash->rev == 1)
	{
		if (hash_data.length() != 20)
		{
			m_ctrl->AppendToActivityLog(QString("Rev 1 hash found with unsupported length %1 for rid %2").arg(hash_data.length(), rid));
			return false;
		}

		MD5_CTX md5;
		MD5_Init(&md5);
		MD5_Update(&md5, hbootkey, 16);
		MD5_Update(&md5, &rid, sizeof(rid));
		MD5_Update(&md5, isNT ? antpassword : almpassword, strlen(isNT ? antpassword : almpassword) + 1);
		quint8 rc4_key[16];
		MD5_Final(rc4_key, &md5);

		struct rc4_state rc4;
		rc4_init(&rc4, rc4_key, 16);
		rc4_crypt(&rc4, sam_hash->data, obfkey, 16);
	}
	else if (sam_hash->rev == 2)
	{
		SAM_HASH_V2 * sam_hash_v2 = (SAM_HASH_V2 *)sam_hash;
		if (sam_hash_v2->offset == 0)
		{
			return false;
		}

		if (sam_hash_v2->offset < 16)
		{
			m_ctrl->AppendToActivityLog(QString("Unsupported salt length %1 found for rid %2").arg(sam_hash_v2->offset, rid));
			return false;
		}
		
		if (hash_data.length() != 56)
		{
			m_ctrl->AppendToActivityLog(QString("Rev 2 hash found with unsupported length %1 for rid %2").arg(hash_data.length(), rid));
			return false;
		}

		AES_KEY aes_ctx;
		memset(&aes_ctx, 0, sizeof(aes_ctx));

		AES_set_decrypt_key(hbootkey, 16 * 8, &aes_ctx);
		AES_cbc_encrypt(sam_hash_v2->data, obfkey, 16, &aes_ctx, sam_hash_v2->salt, AES_DECRYPT);
	}
	else
	{
		m_ctrl->AppendToActivityLog(QString("Unsupported hash revision %1 found for rid %2").arg(sam_hash->rev, rid));
		return false;
	}

	BYTE deskey1[8], deskey2[8];
	rid_to_key1(rid, deskey1);
	rid_to_key2(rid, deskey2);

	SLOWDES_CTX fdctx;
	slowdes_setkey(&fdctx, deskey1);
	slowdes_dedes(&fdctx, obfkey);
	slowdes_setkey(&fdctx, deskey2);
	slowdes_dedes(&fdctx, obfkey + 8);

	memcpy(out_hash, obfkey, 16);

	return true;
}




quint32 SAMImporter::FindControlSet(struct hive *system_hive)
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

unsigned char *SAMImporter::GetBootKey(struct hive *system_hive, size_t &bootkeylen)
{
	quint32 cs = FindControlSet(system_hive);

	QString control_set = QString("ControlSet%1\\Control\\Lsa").arg(cs, 3, 10, QChar('0'));
	int ofs_lsa = trav_path(system_hive, 0, control_set.toLatin1().constData(), TPF_NK);
	if (!ofs_lsa)
	{
		return 0;
	}
	ofs_lsa += 4;

	const char *lsa_keys[] = { "JD", "Skew1", "GBG", "Data" };
	unsigned char *bootkey = NULL;
	bootkeylen = 0;
	for (int i = 0; i < _countof(lsa_keys); i++)
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
	for (size_t i = 0; i < bootkeylen; i++)
	{
		bootkey_scrambled[i] = bootkey[p[i]];
	}

	free(bootkey);

	return bootkey_scrambled;

	/*
	root = get_root(sysaddr)
	if not root: return None

	lsa = open_key(root, lsa_base)
	if not lsa: return None

	bootkey = ""

	for lk in lsa_keys:
	key = open_key(lsa, [lk])
	class_data = sysaddr.read(key.Class.value, key.ClassLength.value)
	bootkey += class_data.decode('utf-16-le').decode('hex')

	bootkey_scrambled = ""
	for i in range(len(bootkey)):
	bootkey_scrambled += bootkey[p[i]]

	return bootkey_scrambled
	*/
}

struct SAM_KEY_DATA_V1 {
	quint32 rev;
	quint32 len;
	quint8 salt[16];
	quint8 key[16];
	quint8 md5[16];
	quint8 _unknown[8];
};

struct SAM_KEY_DATA_V2 {
	quint32 rev; 
	quint32 len;
	quint32 checklen;
	quint32 datalen;
	quint8 salt[16];
	BYTE data[1];
} SAM_KEY_DATA_AES, *PSAM_KEY_DATA_AES;

enum class DOMAIN_SERVER_ROLE : int
{
	Backup = 2,
	Primary = 3
};

enum class DOMAIN_SERVER_ENABLE_STATE : int
{
	Enabled = 1,
	Disabled
};

struct DOMAIN_ACCOUNT_F {
	quint16 rev;
	quint16 _unknown0;
	quint32 _unknown1;
	quint64 creation_time;
	quint64 modification_count;
	quint64 max_age;
	quint64 min_age;
	quint64 force_logoff;
	quint64 lockout_duration;
	quint64 lockout_window;
	quint64 modification_count_at_promotion;
	quint32 rid;
	quint32 properties;
	quint16 min_len;
	quint16 history_len;
	quint16 lockout_threshold;
	DOMAIN_SERVER_ENABLE_STATE server_state;
	DOMAIN_SERVER_ROLE server_role;
	quint32 uas_compatibility_required;
	quint32 _unknown2;
	SAM_KEY_DATA_V1 keys1;
	SAM_KEY_DATA_V1 keys2;
	quint32 _unknown3;
	quint32 _unknown4;
};

bool SAMImporter::GetHBootKey(struct hive *sam_hive, unsigned char *bootkey, size_t bootkeylen, quint8 hbootkey[16])
{
	int ofs_account = trav_path(sam_hive, 0, "\\SAM\\Domains\\Account", TPF_NK);
	if (!ofs_account)
	{
		return false;
	}
	ofs_account += 4;

	int type = get_val_type(sam_hive, ofs_account, "F", 0);
	if (type == -1)
	{
		return false;
	}
	int len = get_val_len(sam_hive, ofs_account, "F", 0);
	if (len == -1)
	{
		return false;
	}
	DOMAIN_ACCOUNT_F *F = (DOMAIN_ACCOUNT_F *)get_val_data(sam_hive, ofs_account, "F", type, 0);
	if (F == NULL)
	{
		return false;
	}

	// check revision
	if (F->rev == 2)
	{
		if (F->keys1.rev == 1)
		{
			MD5_CTX md5;
			MD5_Init(&md5);
			MD5_Update(&md5, &(F->keys1.salt), 16);
			MD5_Update(&md5, aqwerty, strlen(aqwerty) + 1);
			MD5_Update(&md5, bootkey, bootkeylen);
			MD5_Update(&md5, anum, strlen(anum) + 1);

			unsigned char rc4_key[16];
			MD5_Final(rc4_key, &md5);

			struct rc4_state rc4;
			rc4_init(&rc4, rc4_key, 16);
			rc4_crypt(&rc4, (unsigned char *)&(F->keys1.key), hbootkey, 16);

			return true;
		}
		else
		{
			m_ctrl->AppendToActivityLog(QString("Unknown key revision %1 for V2 F account with rid %2").arg(F->keys1.rev).arg(F->rid));
			return false;
		}
	}
	else if (F->rev == 3)
	{
		if (F->keys1.rev == 2)
		{
			SAM_KEY_DATA_V2 *keys1_v2 = (SAM_KEY_DATA_V2 *)&(F->keys1);

			AES_KEY aes_ctx;
			memset(&aes_ctx, 0, sizeof(aes_ctx));

			AES_set_decrypt_key(bootkey, 16 * 8, &aes_ctx);
			AES_cbc_encrypt(keys1_v2->data, hbootkey, 16, &aes_ctx, keys1_v2->salt, AES_DECRYPT);

			return true;
		}
		else
		{
			m_ctrl->AppendToActivityLog(QString("Unknown key revision %1 for V3 F account with rid %2").arg(F->keys1.rev).arg(F->rid));
			return false;
		}
	}
	
	m_ctrl->AppendToActivityLog(QString("Unknown F account revision with rid %2").arg(F->rev).arg(F->rid));
	return false;
}



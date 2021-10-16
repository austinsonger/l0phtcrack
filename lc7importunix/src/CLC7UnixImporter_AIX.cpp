#include"stdafx.h"

static QDateTime janfirst1970 = QDateTime(QDate(1970, 1, 1), QTime(0, 0, 0));


static bool check_aix_bool(QString str, bool *ok)
{
	if (str.toLower() == "yes" || str.toLower() == "true" || str.toLower() == "always")
	{
		if (ok)
		{
			*ok = true;
		}
		return true;
	}
	else if (str.toLower() == "no" || str.toLower() == "false" || str.toLower() == "never")
	{
		if (ok)
		{
			*ok = true;
		}
		return false;
	}
	if (ok)
	{
		*ok = false;
	}
	return false;
}

static qint64 read_aix_date(QString str, bool *ok)
{
	*ok = false;

	if (str == "0")
	{
		*ok = true;
		return 0;
	}

	QDateTime dt = QDateTime::fromString(str, "MMddHHmmssyy");
	if (!dt.isValid())
	{
		dt = QDateTime::fromString(str, "MMddHHmmyy");
		if (!dt.isValid())
		{
			*ok = false;
			return 0;
		}
	}
		 
	QString dbgdate = dt.toString();
	OutputDebugString(dbgdate.toStdWString().c_str());

	if (dt.date().year() < 1970)
	{
		dt = dt.addYears(100);
	}

	dbgdate = dt.toString();
	OutputDebugString(dbgdate.toStdWString().c_str());

	*ok = true;
	return (qint64)janfirst1970.secsTo(dt);
}

CLC7UnixImporter_AIX::CLC7UnixImporter_AIX(ILC7AccountList *accountlist, ILC7CommandControl *ctrl) : CLC7UnixImporter(accountlist, ctrl)
{
}

bool CLC7UnixImporter_AIX::ParseStanzaFile(QString filename, QMap<QString, QMap<QString, QString> > & stanzas, QString &error)
{
	TR;
	QFile pwf(filename);
	if (!pwf.open(QIODevice::ReadOnly))
	{
		error = "Unable to open file";
		return false;
	}

	{
		QTextStream ts(&pwf);

		QString currentstanza;
		while (!ts.atEnd())
		{
			if (ts.status() != QTextStream::Ok)
			{
				error = "Error reading file";
				return false;
			}

			QString line = ts.readLine().trimmed();

			if (line.size() == 0)
			{
				// xxx: technically this ends a stanza, but we're trying to be permissive.
				continue;
			}

			if (line.startsWith("#") || line.startsWith("*"))
			{
				continue;
			}

			int colonpos = line.indexOf(":");
			int equalpos = line.indexOf("=");

			if (colonpos != -1 && (equalpos == -1 || colonpos < equalpos))
			{
				// Is start of new stanza
				currentstanza = line.left(colonpos);
				if (!stanzas.contains(currentstanza))
				{
					stanzas[currentstanza] = QMap<QString, QString>();
				}
			}
			if (equalpos != -1 && (colonpos == -1 || equalpos < colonpos))
			{
				// Is key/value line of stanza
				if (currentstanza.isNull() || currentstanza.isEmpty())
				{
					error = "Invalid stanza format.";
					return false;
				}
				QString key = line.left(equalpos).trimmed();
				QString value = line.mid(equalpos + 1).trimmed();

				// Correct action is to only accept the first value, per AIX documentation
				if (!stanzas[currentstanza].contains(key))
				{
					stanzas[currentstanza][key] = value;
				}
			}
		}
		pwf.seek(0);
	}

	return true;
}


bool CLC7UnixImporter_AIX::CheckHashType(QByteArray field, FOURCC &hashtype, QByteArray & hash, bool &disabled, bool &lockedout)
{
	TR;
	
	lockedout = false;
	disabled = false;
	if (field == "*")
	{
		disabled = true;
		return true;
	}
	else
	{
		hash = field;
	}
	if (hash.isEmpty())
	{
		return false;
	}

	if (hash.size() == 13)
	{
		hashtype = FOURCC(HASHTYPE_UNIX_DES);
		return true;
	}
	else if (hash.startsWith("{smd5}"))
	{
		hashtype = FOURCC(HASHTYPE_AIX_MD5);
		return true;
	}
	else if (hash.startsWith("{ssha1}"))
	{
		hashtype = FOURCC(HASHTYPE_AIX_SHA1);
		return true;
	}
	else if (hash.startsWith("{ssha256}"))
	{
		hashtype = FOURCC(HASHTYPE_AIX_SHA256);
		return true;
	}
	else if (hash.startsWith("{ssha512}"))
	{
		hashtype = FOURCC(HASHTYPE_AIX_SHA512);
		return true;
	}
	
	return false;
}

bool CLC7UnixImporter_AIX::GetPasswdHashTypes(QString filename, QList<FOURCC> & hashtypes)
{
	TR;
	QList<QList<QString>> lines;
	QSet<FOURCC> htypeset;
	QString error;
	if (!ParseColonFile(filename, 7, lines, error))
	{
		return false;
	}

	foreach(QStringList fields, lines)
	{
		FOURCC htype;
		QByteArray hash;
		bool disabled = false;
		bool lockedout = false;

		if (!CheckHashType(fields[1].toLatin1(), htype, hash, disabled, lockedout))
		{
			continue;
		}

		htypeset.insert(htype);
	}

	hashtypes = htypeset.toList();

	return true;
}


bool CLC7UnixImporter_AIX::GetAIXSecurityHashTypes(QString filename, QList<FOURCC> & hashtypes)
{
	TR;
	QMap<QString, QMap<QString, QString> > stanzas;
	QSet<FOURCC> htypeset;
	QString error;
	if (!ParseStanzaFile(filename, stanzas, error))
	{
		return false;
	}

	foreach(QString username, stanzas.keys())
	{

		// Skip default stanza
		if (username == "default")
		{
			continue;
		}

		FOURCC htype;
		QByteArray hash;
		bool disabled = false;
		bool lockedout = false;

		if (!CheckHashType(stanzas[username]["password"].toLatin1(), htype, hash, disabled, lockedout))
		{
			continue;
		}

		htypeset.insert(htype);
	}

	hashtypes = htypeset.toList();

	return true;
}

bool CLC7UnixImporter_AIX::VerifyAIXStanzaFile(QString filename)
{
	TR;
	QMap<QString, QMap<QString, QString> > stanzas;
	QSet<FOURCC> htypeset;
	QString error;
	if (!ParseStanzaFile(filename, stanzas, error))
	{
		return false;
	}

	return true;
}


bool CLC7UnixImporter_AIX::ImportAIXSecurityPasswdFile(QString filename, QList<ACCT> & accts, QString &error, bool &cancelled)
{
	// Build merge map
	QMap<QString, ACCT *> accts_by_name;
	for (QList<ACCT>::iterator it = accts.begin(); it != accts.end(); it++)
	{
		if (it->valid_fields & FIELD_USERNAME)
		{
			accts_by_name[it->username] = &(*it);
		}
	}

	UpdateStatus("Importing /etc/security/passwd file", 0, true);

	QMap<QString, QMap<QString, QString>> stanzas;
	if (!ParseStanzaFile(filename, stanzas, error))
	{
		return false;
	}
	if (stanzas.size() == 0)
	{
		error = "File is empty or is not /etc/security/passwd format file";
		return false;
	}

	UpdateStatus("Loading accounts", 0, true);

	int count = stanzas.size();
	int imported = 0;

	foreach(QString stanzakey, stanzas.keys())
	{
		if (m_ctrl->StopRequested())
		{
			cancelled = true;
			return true;
		}
		QMap<QString, QString> stanza = stanzas[stanzakey];

		bool ok;

		ACCT acct;
		acct.valid_fields = 0;

		acct.valid_fields |= FIELD_USERNAME;
		acct.username = stanzakey;

		acct.valid_fields |= FIELD_PASSWORD;
		acct.password = stanza["password"].toLatin1();

		ok = true;
		acct.last_changed_date = stanza["lastupdate"].toUInt(&ok);
		if (ok)
			acct.valid_fields |= FIELD_LAST_CHANGED_DATE;

		if (stanza.contains("expires"))
		{
			ok = true;
			acct.account_expired_date = read_aix_date(stanza["expires"], &ok);
			if (ok)
			{
				acct.valid_fields |= FIELD_ACCOUNT_EXPIRED_DATE;
			}
		}

		if (stanza.contains("flags") && stanza["flags"].contains("ADMCHG"))
		{
			acct.valid_fields |= FIELD_ADMCHG;
			acct.admchg = true;
		}

		ACCT *pacct = accts_by_name[acct.username];
		if (pacct)
		{
			MergeAccounts(pacct, &acct);
		}
		else
		{
			accts.append(acct);
		}
		imported++;

		if (imported % 10 == 0)
		{
			m_ctrl->UpdateCurrentProgressBar(imported * 100 / count);
		}
	}

	m_ctrl->UpdateCurrentProgressBar(100);

	return true;
}


bool CLC7UnixImporter_AIX::ImportAIXSecurityUserFile(QString filename, QList<ACCT> & accts, QString &error, bool &cancelled)
{
	// Build merge map
	QMap<QString, ACCT *> accts_by_name;
	for (QList<ACCT>::iterator it = accts.begin(); it != accts.end(); it++)
	{
		if (it->valid_fields & FIELD_USERNAME)
		{
			accts_by_name[it->username] = &(*it);
		}
	}

	UpdateStatus("Importing /etc/security/user file", 0, true);

	QMap<QString, QMap<QString, QString>> stanzas;
	if (!ParseStanzaFile(filename, stanzas, error))
	{
		return false;
	}
	if (stanzas.size() == 0)
	{
		error = "File is empty or is not /etc/security/user format file";
		return false;
	}

	UpdateStatus("Loading accounts", 0, true);

	int count = stanzas.size();
	int imported = 0;

	QMap<QString, QString> default_stanza = stanzas["default"];


	foreach(QString stanzakey, stanzas.keys())
	{

		// Skip default stanza
		if (stanzakey == "default")
		{
			continue;
		}

		if (m_ctrl->StopRequested())
		{
			cancelled = true;
			return true;
		}
		QMap<QString, QString> stanza = stanzas[stanzakey];

		bool ok;

		ACCT acct;
		acct.valid_fields = 0;

		acct.valid_fields |= FIELD_USERNAME;
		acct.username = stanzakey;

		if (((acct.valid_fields & FIELD_LOCKED_OUT) == 0) && (stanza.contains("account_locked") || default_stanza.contains("account_locked")))
		{
			ok = true;
			acct.locked_out = check_aix_bool(stanza["account_locked"], &ok);
			if (!ok)
			{
				ok = true;
				acct.locked_out = check_aix_bool(default_stanza["account_locked"], &ok);
			}
			if (ok)
			{
				acct.valid_fields |= FIELD_LOCKED_OUT;
			}
		}

		if (((acct.valid_fields & FIELD_ACCOUNT_EXPIRED_DATE) == 0) && (stanza.contains("expires") || default_stanza.contains("expires")))
		{
			ok = true;
			acct.account_expired_date = read_aix_date(stanza["expires"], &ok);
			if (!ok)
			{
				ok = true;
				acct.account_expired_date = read_aix_date(default_stanza["expires"], &ok);
				if (acct.account_expired_date == 0)
				{
					ok = false;
				}
			}
			if (ok)
			{
				acct.valid_fields |= FIELD_ACCOUNT_EXPIRED_DATE;
			}
		}

		ok = true;
		if (((acct.valid_fields & FIELD_PASSWORD_MAX_AGE) ==0 ) && (stanza.contains("maxage") || default_stanza.contains("maxage")))
		{
			ok = true;
			acct.password_max_age = stanza["maxage"].toUInt(&ok);
			if (!ok)
			{
				ok = true;
				acct.password_max_age = default_stanza["maxage"].toUInt(&ok);
				if (acct.password_max_age == 0)
				{
					ok = false;
				}
			}
			if (ok)
			{
				acct.valid_fields |= FIELD_PASSWORD_MAX_AGE;
			}
		}

		ok = true;
		if (((acct.valid_fields & FIELD_WARNING_DAYS_FOR_PASSWORD_EXPIRE) == 0) && (stanza.contains("pwdwarntime") || default_stanza.contains("pwdwarntime")))
		{
			ok = true;
			acct.warning_days_for_password_expire = stanza["pwdwarntime"].toUInt(&ok);
			if (!ok)
			{
				ok = true;
				acct.warning_days_for_password_expire = default_stanza["pwdwarntime"].toUInt(&ok);
				if (acct.warning_days_for_password_expire == 0)
				{
					ok = false;
				}
			}
			if (ok)
			{
				acct.valid_fields |= FIELD_WARNING_DAYS_FOR_PASSWORD_EXPIRE;
			}
		}

		ACCT *pacct = accts_by_name[acct.username];
		if (pacct)
		{
			MergeAccounts(pacct, &acct);
		}
		else
		{
			accts.append(acct);
		}
		imported++;

		if (imported % 10 == 0)
		{
			m_ctrl->UpdateCurrentProgressBar(imported * 100 / count);
		}
	}

	m_ctrl->UpdateCurrentProgressBar(100);

	return true;
}

void CLC7UnixImporter_AIX::ConvertAcctToLC7Account(const ACCT &acct, LC7Account &lc7account)
{
	lc7account.flags = 0;
	if (acct.valid_fields & FIELD_USERNAME)
	{
		lc7account.username = acct.username;
	}
	if ((acct.valid_fields & FIELD_UID) && (acct.valid_fields & FIELD_GID))
	{
		lc7account.userid = QString("%1:%2").arg(acct.uid).arg(acct.gid);
	}
	else if (acct.valid_fields & FIELD_UID)
	{
		lc7account.userid = QString("%1").arg(acct.uid);
	}
	else if (acct.valid_fields & FIELD_GID)
	{
		lc7account.userid = QString(":%1").arg(acct.gid);
	}

	lc7account.userinfo = "";
	if (acct.valid_fields & FIELD_USERINFO)
	{
		lc7account.userinfo += QString("%1%2").arg((lc7account.userinfo.length() > 0) ? " " : "").arg(acct.userinfo);
	}
	if (acct.valid_fields & FIELD_HOMEDIR)
	{
		lc7account.userinfo += QString("%1%2").arg((lc7account.userinfo.length() > 0) ? " " : "").arg(acct.homedir);
	}
	if (acct.valid_fields & FIELD_SHELL)
	{
		lc7account.userinfo += QString("%1%2").arg((lc7account.userinfo.length() > 0) ? " " : "").arg(acct.shell);
	}

	bool disabled = false;
	bool lockedout = false;
	if (acct.valid_fields & FIELD_PASSWORD)
	{
		FOURCC hashtype;
		QByteArray hash;
		CheckHashType(acct.password, hashtype, hash, disabled, lockedout); // Must succeed as this was checked earlier

		if (hashtype != 0)
		{
			LC7Hash lc7hash;
			lc7hash.hashtype = hashtype;
			lc7hash.hash = hash;
			if (hash.isEmpty())
			{
				lc7hash.crackstate = CRACKSTATE_CRACKED;
				lc7hash.cracktype = "No Password";
				lc7hash.cracktime = 0;
			}
			else
			{
				lc7hash.crackstate = CRACKSTATE_NOT_CRACKED;
				lc7hash.cracktime = 0;
				lc7hash.cracktype = QString();
			}
			lc7account.hashes.append(lc7hash);
		}
	}

	lc7account.lastchanged = 0;
	if (acct.valid_fields & FIELD_LAST_CHANGED_DATE)
	{
		lc7account.lastchanged = janfirst1970.addSecs(acct.last_changed_date).toTime_t();
	}

	lc7account.lockedout = lockedout;
	if (acct.valid_fields & FIELD_LOCKED_OUT)
	{
		lc7account.lockedout = acct.locked_out;
	}

	lc7account.disabled = disabled;
	if (acct.valid_fields & FIELD_ACCOUNT_EXPIRED_DATE)
	{
		if (acct.account_expired_date != 0)
		{
			if (janfirst1970.addSecs(acct.account_expired_date) < QDateTime::currentDateTime())
			{
				lc7account.disabled = true;
			}
		}
	}

	lc7account.neverexpires = true;
	lc7account.mustchange = false;
	if (acct.valid_fields & FIELD_PASSWORD_MAX_AGE && acct.valid_fields & FIELD_LAST_CHANGED_DATE)
	{
		if (acct.password_max_age != 0)
		{
			lc7account.neverexpires = false;
			if (janfirst1970.addSecs(acct.last_changed_date).addDays(acct.password_max_age * 7) < QDateTime::currentDateTime())
			{
				lc7account.mustchange = true;
			}
		}
	}

	if (acct.valid_fields & FIELD_ADMCHG)
	{
		lc7account.mustchange = true;
	}

	lc7account.remediations = -1;
}


///////////////////


CLC7UnixImporter_AIX_PASSWDSECURITYPASSWDSECURITYUSER::CLC7UnixImporter_AIX_PASSWDSECURITYPASSWDSECURITYUSER(ILC7AccountList *accountlist, ILC7CommandControl *ctrl) : CLC7UnixImporter_AIX(accountlist, ctrl)
{
}

QString CLC7UnixImporter_AIX_PASSWDSECURITYPASSWDSECURITYUSER::name()
{
	return "aixpasswdsecuritypasswdsecurityuser";
}

QString CLC7UnixImporter_AIX_PASSWDSECURITYPASSWDSECURITYUSER::desc()
{
	return "AIX: /etc/passwd, /etc/security/passwd, & /etc/security/user";
}

QStringList CLC7UnixImporter_AIX_PASSWDSECURITYPASSWDSECURITYUSER::filetypes()
{
	return QString("/etc/passwd;/etc/security/passwd;/etc/security/user").split(";");
}

bool CLC7UnixImporter_AIX_PASSWDSECURITYPASSWDSECURITYUSER::CheckHashTypes(QStringList filenames, QList<bool> &filevalid, QList<FOURCC> & hashtypes)
{
	TR;

	filevalid.clear();
	filevalid.append(false);
	filevalid.append(false);
	filevalid.append(false);

	if (filenames.size() == 0)
	{
		return false;
	}
	QList<QList<QString>> lines;
	QString error;
	if (ParseColonFile(filenames[0], 7, lines, error))
	{
		filevalid[0] = true;
	}

	if (filenames.size() == 1)
	{
		return false;
	}
	if (GetAIXSecurityHashTypes(filenames[1], hashtypes))
	{
		filevalid[1] = true;
	}

	if (filenames.size() == 2)
	{
		return false;
	}
	if (VerifyAIXStanzaFile(filenames[2]))
	{
		filevalid[2] = true;
	}

	return filevalid[0] && filevalid[1] && filevalid[2];
}

bool CLC7UnixImporter_AIX_PASSWDSECURITYPASSWDSECURITYUSER::DoImport(QList<LC7Account> &lc7accounts, QStringList filenames, FOURCC hashtype, QString & error, bool & cancelled)
{
	TR;
	cancelled = false;

	QList<ACCT> accts;
	if (!ImportPasswdFile(filenames[0], accts, error, cancelled))
	{
		return false;
	}
	if (cancelled)
	{
		return true;
	}

	if (!ImportAIXSecurityPasswdFile(filenames[1], accts, error, cancelled))
	{
		return false;
	}
	if (cancelled)
	{
		return true;
	}

	if (!ImportAIXSecurityUserFile(filenames[2], accts, error, cancelled))
	{
		return false;
	}
	if (cancelled)
	{
		return true;
	}

	foreach(ACCT acct, accts)
	{
		LC7Account lc7acct;

		ConvertAcctToLC7Account(acct, lc7acct);

		if (lc7acct.hashes.size() == 1 && lc7acct.hashes[0].hashtype != hashtype)
		{
			continue;
		}

		lc7accounts.append(lc7acct);
	}

	return true;
}

/////////////////////

CLC7UnixImporter_AIX_PASSWDSECURITYPASSWD::CLC7UnixImporter_AIX_PASSWDSECURITYPASSWD(ILC7AccountList *accountlist, ILC7CommandControl *ctrl) : CLC7UnixImporter_AIX(accountlist, ctrl)
{
}

QString CLC7UnixImporter_AIX_PASSWDSECURITYPASSWD::name()
{
	return "aixpasswdsecuritypasswd";
}

QString CLC7UnixImporter_AIX_PASSWDSECURITYPASSWD::desc()
{
	return "AIX: /etc/passwd & /etc/security/passwd";
}

QStringList CLC7UnixImporter_AIX_PASSWDSECURITYPASSWD::filetypes()
{
	return QString("/etc/passwd;/etc/security/passwd").split(";");
}

bool CLC7UnixImporter_AIX_PASSWDSECURITYPASSWD::CheckHashTypes(QStringList filenames, QList<bool> &filevalid, QList<FOURCC> & hashtypes)
{
	TR;

	filevalid.clear();
	filevalid.append(false);
	filevalid.append(false);

	if (filenames.size() == 0)
	{
		return false;
	}
	QList<QList<QString>> lines;
	QString error;
	if (ParseColonFile(filenames[0], 7, lines, error))
	{
		filevalid[0] = true;
	}

	if (filenames.size() == 1)
	{
		return false;
	}
	if (GetAIXSecurityHashTypes(filenames[1], hashtypes))
	{
		filevalid[1] = true;
	}

	return filevalid[0] && filevalid[1];
}


bool CLC7UnixImporter_AIX_PASSWDSECURITYPASSWD::DoImport(QList<LC7Account> &lc7accounts, QStringList filenames, FOURCC hashtype, QString & error, bool & cancelled)
{
	TR;
	cancelled = false;

	QList<ACCT> accts;
	if (!ImportPasswdFile(filenames[0], accts, error, cancelled))
	{
		return false;
	}
	if (cancelled)
	{
		return true;
	}

	if (!ImportAIXSecurityPasswdFile(filenames[1], accts, error, cancelled))
	{
		return false;
	}
	if (cancelled)
	{
		return true;
	}

	foreach(ACCT acct, accts)
	{
		LC7Account lc7acct;

		ConvertAcctToLC7Account(acct, lc7acct);

		if (lc7acct.hashes.size() == 1 && lc7acct.hashes[0].hashtype != hashtype)
		{
			continue;
		}

		lc7accounts.append(lc7acct);
	}

	return true;
}



/////////////////////

CLC7UnixImporter_AIX_SECURITYPASSWD::CLC7UnixImporter_AIX_SECURITYPASSWD(ILC7AccountList *accountlist, ILC7CommandControl *ctrl) : CLC7UnixImporter_AIX(accountlist, ctrl)
{
}

QString CLC7UnixImporter_AIX_SECURITYPASSWD::name()
{
	return "aixsecuritypasswd";
}

QString CLC7UnixImporter_AIX_SECURITYPASSWD::desc()
{
	return "AIX: /etc/security/passwd only";
}

QStringList CLC7UnixImporter_AIX_SECURITYPASSWD::filetypes()
{
	return QString("/etc/security/passwd").split(";");
}

bool CLC7UnixImporter_AIX_SECURITYPASSWD::CheckHashTypes(QStringList filenames, QList<bool> &filevalid, QList<FOURCC> & hashtypes)
{
	TR;

	filevalid.clear();
	filevalid.append(false);

	if (filenames.size() == 0)
	{
		return false;
	}

	if (GetAIXSecurityHashTypes(filenames[0], hashtypes))
	{
		filevalid[0] = true;
	}

	return filevalid[0];
}


bool CLC7UnixImporter_AIX_SECURITYPASSWD::DoImport(QList<LC7Account> &lc7accounts, QStringList filenames, FOURCC hashtype, QString & error, bool & cancelled)
{
	TR;
	cancelled = false;

	QList<ACCT> accts;
	if (!ImportAIXSecurityPasswdFile(filenames[0], accts, error, cancelled))
	{
		return false;
	}
	if (cancelled)
	{
		return true;
	}

	foreach(ACCT acct, accts)
	{
		LC7Account lc7acct;

		ConvertAcctToLC7Account(acct, lc7acct);

		if (lc7acct.hashes.size() == 1 && lc7acct.hashes[0].hashtype != hashtype)
		{
			continue;
		}

		lc7accounts.append(lc7acct);
	}

	return true;
}

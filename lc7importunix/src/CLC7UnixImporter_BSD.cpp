#include"stdafx.h"

static QDateTime janfirst1970 = QDateTime(QDate(1970, 1, 1), QTime(0, 0, 0));

CLC7UnixImporter_BSD::CLC7UnixImporter_BSD(ILC7AccountList *accountlist, ILC7CommandControl *ctrl) : CLC7UnixImporter(accountlist, ctrl)
{
}

bool CLC7UnixImporter_BSD::CheckHashType(QByteArray field, FOURCC &hashtype, QByteArray & hash, bool &disabled, bool &lockedout)
{
	TR;
	lockedout = false;
	disabled = false;
	if (field.startsWith("*LOCKED*")) // FreeBSD
	{
		lockedout = true;
		hash = field.mid(8);
	}
	else if (field.startsWith("*")) // OpenBSD
	{
		lockedout = true;
		hash = field.mid(1);
	}
	else
	{
		hash = field;
	}
	if (hash.isEmpty())
	{
		// Empty password matches any hash type
		hashtype = 0;
		return true;
	}

	if (hash.size() == 13)
	{
		hashtype = FOURCC(HASHTYPE_UNIX_DES);
		return true;
	}
	else if (hash.startsWith("$1") || hash.startsWith("$apr1"))
	{
		hashtype = FOURCC(HASHTYPE_UNIX_MD5);
		return true;
	}
	else if (hash.startsWith("$2"))
	{
		hashtype = FOURCC(HASHTYPE_UNIX_BLOWFISH);
		return true;
	}
	else if (hash.startsWith("$5"))
	{
		hashtype = FOURCC(HASHTYPE_UNIX_SHA256);
		return true;
	}
	else if (hash.startsWith("$6"))
	{
		hashtype = FOURCC(HASHTYPE_UNIX_SHA512);
		return true;
	}

	return false;
}

bool CLC7UnixImporter_BSD::GetPasswdHashTypes(QString filename, QList<FOURCC> & hashtypes)
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


bool CLC7UnixImporter_BSD::GetMasterPasswdHashTypes(QString filename, QList<FOURCC> & hashtypes)
{
	TR;
	QList<QList<QString>> lines;
	QSet<FOURCC> htypeset;
	QString error;
	if (!ParseColonFile(filename, 10, lines, error))
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



bool CLC7UnixImporter_BSD::ImportMasterPasswdFile(QString filename, QList<ACCT> & accts, QString &error, bool &cancelled)
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

	UpdateStatus("Importing master.passwd file", 0, true);

	QList<QList<QString>> lines;
	if (!ParseColonFile(filename, 10, lines, error))
	{
		return false;
	}
	if (lines.size() == 0)
	{
		error = "File is empty or is not master.passwd format file";
		return false;
	}

	UpdateStatus("Loading accounts", 0, true);

	int count = lines.size();
	int imported = 0;
	foreach(QStringList fields, lines)
	{
		if (m_ctrl->StopRequested())
		{
			cancelled = true;
			return true;
		}

		bool ok;

		ACCT acct;
		acct.valid_fields = 0;

		acct.valid_fields |= FIELD_USERNAME;
		acct.username = fields[0];

		acct.valid_fields |= FIELD_PASSWORD;
		acct.password = fields[1].toLatin1();

		ok = true;
		acct.uid = fields[2].toUInt(&ok);
		if (ok)
			acct.valid_fields |= FIELD_UID;

		ok = true;
		acct.gid = fields[3].toUInt(&ok);
		if (ok)
			acct.valid_fields |= FIELD_GID;

		ok = true;
		acct.password_expired_date = fields[5].toUInt(&ok);
		if (ok)
			acct.valid_fields |= FIELD_PASSWORD_EXPIRED_DATE;

		ok = true;
		acct.account_expired_date = fields[6].toUInt(&ok);
		if (ok)
			acct.valid_fields |= FIELD_ACCOUNT_EXPIRED_DATE;

		acct.userinfo = fields[7];
		if (!acct.userinfo.isEmpty())
		{
			acct.valid_fields |= FIELD_USERINFO;
		}
		acct.homedir = fields[8];
		if (!acct.homedir.isEmpty())
		{
			acct.valid_fields |= FIELD_HOMEDIR;
		}
		acct.shell = fields[9];
		if (!acct.shell.isEmpty())
		{
			acct.valid_fields |= FIELD_SHELL;
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


void CLC7UnixImporter_BSD::ConvertAcctToLC7Account(const ACCT &acct, LC7Account &lc7account)
{
	lc7account.flags = 0;

	if (acct.valid_fields & FIELD_USERNAME)
		lc7account.username = acct.username;

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

	if (acct.valid_fields & FIELD_PASSWORD_EXPIRED_DATE)
	{
		if (acct.password_expired_date != 0)
		{
			if (janfirst1970.addDays(acct.password_expired_date) <= QDateTime::currentDateTimeUtc())
			{
				lc7account.mustchange = true;
			}
		}
		else
		{
			lc7account.neverexpires = true;
		}
	}
	if (acct.valid_fields & FIELD_ACCOUNT_EXPIRED_DATE)
	{
		if (acct.account_expired_date != 0)
		{
			if (janfirst1970.addDays(acct.account_expired_date) <= QDateTime::currentDateTimeUtc())
			{
				lc7account.disabled = true;
			}
		}
	}
	if (disabled)
	{
		lc7account.disabled = true;
	}
	if (lockedout)
	{
		lc7account.lockedout = true;
	}

	lc7account.remediations = -1;
}


///////////////////


CLC7UnixImporter_BSD_PASSWDMASTERPASSWD::CLC7UnixImporter_BSD_PASSWDMASTERPASSWD(ILC7AccountList *accountlist, ILC7CommandControl *ctrl) : CLC7UnixImporter_BSD(accountlist, ctrl)
{
}

QString CLC7UnixImporter_BSD_PASSWDMASTERPASSWD::name()
{
	return "bsdpasswdmasterpasswd";
}

QString CLC7UnixImporter_BSD_PASSWDMASTERPASSWD::desc()
{
	return "BSD: /etc/passwd & /etc/master.passwd";
}

QStringList CLC7UnixImporter_BSD_PASSWDMASTERPASSWD::filetypes()
{
	return QString("/etc/passwd;/etc/master.passwd").split(";");
}

bool CLC7UnixImporter_BSD_PASSWDMASTERPASSWD::CheckHashTypes(QStringList filenames, QList<bool> &filevalid, QList<FOURCC> & hashtypes)
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
	if (GetMasterPasswdHashTypes(filenames[1], hashtypes))
	{
		filevalid[1] = true;
	}

	return filevalid[0] && filevalid[1];
}

bool CLC7UnixImporter_BSD_PASSWDMASTERPASSWD::DoImport(QList<LC7Account> &lc7accounts, QStringList filenames, FOURCC hashtype, QString & error, bool & cancelled)
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

	if (!ImportMasterPasswdFile(filenames[1], accts, error, cancelled))
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

CLC7UnixImporter_BSD_MASTERPASSWD::CLC7UnixImporter_BSD_MASTERPASSWD(ILC7AccountList *accountlist, ILC7CommandControl *ctrl) : CLC7UnixImporter_BSD(accountlist, ctrl)
{
}

QString CLC7UnixImporter_BSD_MASTERPASSWD::name()
{
	return "bsdmasterpasswd";
}

QString CLC7UnixImporter_BSD_MASTERPASSWD::desc()
{
	return "BSD: /etc/master.passwd only";
}

QStringList CLC7UnixImporter_BSD_MASTERPASSWD::filetypes()
{
	return QString("/etc/master.passwd").split(";");
}

bool CLC7UnixImporter_BSD_MASTERPASSWD::CheckHashTypes(QStringList filenames, QList<bool> &filevalid, QList<FOURCC> & hashtypes)
{
	TR;

	filevalid.clear();
	filevalid.append(false);

	if (filenames.size() == 0)
	{
		return false;
	}
	if (GetMasterPasswdHashTypes(filenames[0], hashtypes))
	{
		filevalid[0] = true;
	}

	return filevalid[0];
}

bool CLC7UnixImporter_BSD_MASTERPASSWD::DoImport(QList<LC7Account> &lc7accounts, QStringList filenames, FOURCC hashtype, QString & error, bool & cancelled)
{
	TR;
	cancelled = false;

	QList<ACCT> accts;
	if (!ImportMasterPasswdFile(filenames[0], accts, error, cancelled))
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

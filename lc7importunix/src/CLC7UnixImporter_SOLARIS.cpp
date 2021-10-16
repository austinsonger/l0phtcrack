#include"stdafx.h"

static QDateTime janfirst1970 = QDateTime(QDate(1970, 1, 1), QTime(0, 0, 0));

CLC7UnixImporter_SOLARIS::CLC7UnixImporter_SOLARIS(ILC7AccountList *accountlist, ILC7CommandControl *ctrl) : CLC7UnixImporter(accountlist, ctrl)
{
}

bool CLC7UnixImporter_SOLARIS::CheckHashType(QByteArray field, FOURCC &hashtype, QByteArray & hash, bool &disabled, bool &lockedout)
{
	TR;
	lockedout = false;
	disabled = false;
	if (field.startsWith("*LK*"))
	{
		lockedout = true;
		field = field.mid(4);
	}

	if (field == "UP" || field == "NL")
	{
		disabled = true;
	}
	else if (field == "NP" || field == "PS" || field == "UN")
	{
		hash = "";
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

bool CLC7UnixImporter_SOLARIS::GetPasswdHashTypes(QString filename, QList<FOURCC> & hashtypes)
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

bool CLC7UnixImporter_SOLARIS::GetShadowHashTypes(QString filename, QList<FOURCC> & hashtypes)
{
	TR;
	QList<QList<QString>> lines;
	QSet<FOURCC> htypeset;
	QString error;
	if (!ParseColonFile(filename, 9, lines, error))
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




void CLC7UnixImporter_SOLARIS::ConvertAcctToLC7Account(const ACCT &acct, LC7Account &lc7account)
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

	if (acct.valid_fields & FIELD_LAST_CHANGED_DATE)
	{
		if (acct.last_changed_date == 0)
		{	
			lc7account.mustchange = true;
		}
		else
		{
			lc7account.lastchanged = janfirst1970.addDays(acct.last_changed_date).toTime_t();		// Password last changed date (time64_t)
		}
	}
	else
	{
		lc7account.lastchanged = 0;
	}

	if (acct.valid_fields & FIELD_ACCOUNT_EXPIRED_DATE)
	{
		if (janfirst1970.addDays(acct.account_expired_date) <= QDateTime::currentDateTimeUtc())
		{
			lc7account.disabled = true;
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
	if (acct.valid_fields & FIELD_DAYS_LEFT_UNTIL_PASSWORD_EXPIRES)
	{
		if (acct.days_left_until_password_expires == 0)
		{
			lc7account.mustchange = true;
		}
	}
	else
	{
		lc7account.neverexpires = true;
	}

	lc7account.remediations = -1;
}

///////////////////

CLC7UnixImporter_SOLARIS_PASSWDSHADOW::CLC7UnixImporter_SOLARIS_PASSWDSHADOW(ILC7AccountList *accountlist, ILC7CommandControl *ctrl) : CLC7UnixImporter_SOLARIS(accountlist, ctrl)
{
}

QString CLC7UnixImporter_SOLARIS_PASSWDSHADOW::name()
{
	return "solarispasswdshadow";
}

QString CLC7UnixImporter_SOLARIS_PASSWDSHADOW::desc()
{
	return "Solaris: /etc/passwd & /etc/shadow";
}

QStringList CLC7UnixImporter_SOLARIS_PASSWDSHADOW::filetypes()
{
	return QString("/etc/passwd;/etc/shadow").split(";");
}

bool CLC7UnixImporter_SOLARIS_PASSWDSHADOW::CheckHashTypes(QStringList filenames, QList<bool> &filevalid, QList<FOURCC> & hashtypes)
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
	if (GetShadowHashTypes(filenames[1], hashtypes))
	{
		filevalid[1] = true;
	}

	return filevalid[0] && filevalid[1];
}

bool CLC7UnixImporter_SOLARIS_PASSWDSHADOW::DoImport(QList<LC7Account> &lc7accounts, QStringList filenames, FOURCC hashtype, QString & error, bool & cancelled)
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

	if (!ImportShadowFile(filenames[1], accts, error, cancelled))
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

CLC7UnixImporter_SOLARIS_SHADOW::CLC7UnixImporter_SOLARIS_SHADOW(ILC7AccountList *accountlist, ILC7CommandControl *ctrl) : CLC7UnixImporter_SOLARIS(accountlist, ctrl)
{
}

QString CLC7UnixImporter_SOLARIS_SHADOW::name()
{
	return "solarisshadow";
}

QString CLC7UnixImporter_SOLARIS_SHADOW::desc()
{
	return "Solaris: /etc/shadow only";
}

QStringList CLC7UnixImporter_SOLARIS_SHADOW::filetypes()
{
	return QString("/etc/shadow").split(";");
}

bool CLC7UnixImporter_SOLARIS_SHADOW::CheckHashTypes(QStringList filenames, QList<bool> &filevalid, QList<FOURCC> & hashtypes)
{
	TR;

	filevalid.clear();
	filevalid.append(false);

	if (filenames.size() == 0)
	{
		return false;
	}
	if (GetShadowHashTypes(filenames[0], hashtypes))
	{
		filevalid[0] = true;
	}

	return filevalid[0];
}

bool CLC7UnixImporter_SOLARIS_SHADOW::DoImport(QList<LC7Account> &lc7accounts, QStringList filenames, FOURCC hashtype, QString & error, bool & cancelled)
{
	TR;
	cancelled = false;

	QList<ACCT> accts;
	if (!ImportShadowFile(filenames[0], accts, error, cancelled))
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

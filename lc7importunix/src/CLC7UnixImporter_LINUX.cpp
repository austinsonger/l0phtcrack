#include"stdafx.h"

static QDateTime janfirst1970 = QDateTime(QDate(1970, 1, 1), QTime(0, 0, 0));

CLC7UnixImporter_LINUX::CLC7UnixImporter_LINUX(ILC7AccountList *accountlist, ILC7CommandControl *ctrl) : CLC7UnixImporter(accountlist, ctrl)
{
}

bool CLC7UnixImporter_LINUX::CheckHashType(QByteArray field, FOURCC &hashtype, QByteArray & hash, bool &disabled, bool &lockedout)
{
	TR;
	lockedout = false;
	disabled = false;
	while (field.startsWith("!"))
	{
		lockedout = true;
		field = field.mid(1);
	}
	if (field == "*")
	{
		lockedout = true;
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

bool CLC7UnixImporter_LINUX::GetPasswdHashTypes(QString filename, QList<FOURCC> & hashtypes)
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

bool CLC7UnixImporter_LINUX::GetShadowHashTypes(QString filename, QList<FOURCC> & hashtypes)
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




void CLC7UnixImporter_LINUX::ConvertAcctToLC7Account(const ACCT &acct, LC7Account &lc7account)
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
	bool lockedout=false;
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
		lc7account.lastchanged = janfirst1970.addDays(acct.last_changed_date).toTime_t();		// Password last changed date (time64_t)
	else
		lc7account.lastchanged = 0;

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
		if (acct.days_left_until_password_expires == 99999)
		{
			lc7account.neverexpires = true;
		}
	}

	lc7account.remediations = -1;
}

///////////////////

CLC7UnixImporter_LINUX_PASSWDSHADOW::CLC7UnixImporter_LINUX_PASSWDSHADOW(ILC7AccountList *accountlist, ILC7CommandControl *ctrl) : CLC7UnixImporter_LINUX(accountlist, ctrl)
{
}

QString CLC7UnixImporter_LINUX_PASSWDSHADOW::name()
{
	return "linuxpasswdshadow";
}

QString CLC7UnixImporter_LINUX_PASSWDSHADOW::desc()
{
	return "Linux: /etc/passwd & /etc/shadow";
}

QStringList CLC7UnixImporter_LINUX_PASSWDSHADOW::filetypes()
{
	return QString("/etc/passwd;/etc/shadow").split(";");
}

bool CLC7UnixImporter_LINUX_PASSWDSHADOW::CheckHashTypes(QStringList filenames, QList<bool> &filevalid, QList<FOURCC> & hashtypes)
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

bool CLC7UnixImporter_LINUX_PASSWDSHADOW::DoImport(QList<LC7Account> &lc7accounts, QStringList filenames, FOURCC hashtype, QString & error, bool & cancelled)
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

CLC7UnixImporter_LINUX_SHADOW::CLC7UnixImporter_LINUX_SHADOW(ILC7AccountList *accountlist, ILC7CommandControl *ctrl) : CLC7UnixImporter_LINUX(accountlist, ctrl)
{
}

QString CLC7UnixImporter_LINUX_SHADOW::name()
{
	return "linuxshadow";
}

QString CLC7UnixImporter_LINUX_SHADOW::desc()
{
	return "Linux: /etc/shadow only";
}

QStringList CLC7UnixImporter_LINUX_SHADOW::filetypes()
{
	return QString("/etc/shadow").split(";");
}

bool CLC7UnixImporter_LINUX_SHADOW::CheckHashTypes(QStringList filenames, QList<bool> &filevalid, QList<FOURCC> & hashtypes)
{
	TR;

	filevalid.clear();
	filevalid.append(false);
	
	if (filenames.size()==0)
	{
		return false;
	}

	if (GetShadowHashTypes(filenames[0], hashtypes))
	{
		filevalid[0] = true;
	}

	return filevalid[0];
}

bool CLC7UnixImporter_LINUX_SHADOW::DoImport(QList<LC7Account> &lc7accounts, QStringList filenames, FOURCC hashtype, QString & error, bool & cancelled)
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

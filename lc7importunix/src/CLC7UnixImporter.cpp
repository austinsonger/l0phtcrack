#include"stdafx.h"

CLC7UnixImporter::CLC7UnixImporter(ILC7AccountList *accountlist, ILC7CommandControl *ctrl)
{
	TR;
	m_accountlist = accountlist;
	if (ctrl)
		m_ctrl = ctrl->GetSubControl("Passwd Importer: ");
	else
		m_ctrl = nullptr;
}

CLC7UnixImporter::~CLC7UnixImporter()
{
	TR;
	if (m_ctrl)
		m_ctrl->ReleaseSubControl();
}

void CLC7UnixImporter::UpdateStatus(QString statustext, quint32 cur, bool statuslog)
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


void CLC7UnixImporter::MergeAccounts(ACCT *acct1, ACCT *acct2)
{
	if (acct2->valid_fields & FIELD_USERNAME)
		acct1->username = acct2->username;
	if (acct2->valid_fields & FIELD_PASSWORD)
		acct1->password = acct2->password;
	if (acct2->valid_fields & FIELD_UID)
		acct1->uid = acct2->uid;
	if (acct2->valid_fields & FIELD_GID)
		acct1->gid = acct2->gid;
	if (acct2->valid_fields & FIELD_USERINFO)
		acct1->userinfo = acct2->userinfo;
	if (acct2->valid_fields & FIELD_HOMEDIR)
		acct1->homedir = acct2->homedir;
	if (acct2->valid_fields & FIELD_SHELL)
		acct1->shell = acct2->shell;
	if (acct2->valid_fields & FIELD_LAST_CHANGED_DATE)
		acct1->last_changed_date = acct2->last_changed_date;
	if (acct2->valid_fields & FIELD_DAYS_LEFT_UNTIL_CHANGE_ALLOWED)
		acct1->days_left_until_change_allowed = acct2->days_left_until_change_allowed;
	if (acct2->valid_fields & FIELD_DAYS_LEFT_UNTIL_PASSWORD_EXPIRES)
		acct1->days_left_until_password_expires = acct2->days_left_until_password_expires;
	if (acct2->valid_fields & FIELD_WARNING_DAYS_FOR_PASSWORD_EXPIRE)
		acct1->warning_days_for_password_expire = acct2->warning_days_for_password_expire;
	if (acct2->valid_fields & FIELD_DISABLE_DAYS_AFTER_PASSWORD_EXPIRE)
		acct1->disable_days_after_password_expire = acct2->disable_days_after_password_expire;
	if (acct2->valid_fields & FIELD_PASSWORD_EXPIRED_DATE)
		acct1->password_expired_date = acct2->password_expired_date;
	if (acct2->valid_fields & FIELD_ACCOUNT_EXPIRED_DATE)
		acct1->account_expired_date = acct2->account_expired_date;
	if (acct2->valid_fields & FIELD_LOCKED_OUT)
		acct1->locked_out = acct2->locked_out;
	if (acct2->valid_fields & FIELD_PASSWORD_MAX_AGE)
		acct1->password_max_age = acct2->password_max_age;
	if (acct2->valid_fields & FIELD_ADMCHG)
		acct1->admchg = acct2->admchg;

	acct1->valid_fields |= acct2->valid_fields;
}

bool CLC7UnixImporter::CountLinesColonFile(QString filename, int fields, int & linecount, QString &error)
{
	TR;
	QFile pwf(filename);
	if (!pwf.open(QIODevice::ReadOnly))
	{
		error = "Unable to open file";
		return false;
	}

	// Get estimate
	int accountcount = 0;
	{
		QTextStream ts(&pwf);

		while (!ts.atEnd())
		{
			if (ts.status()!=QTextStream::Ok)
			{
				error = "Error reading file";
				return false;
			}
			QString line = ts.readLine();

			QStringList parts = line.split(":");
			if (parts.size() == fields)
			{
				accountcount++;
			}
		}
		pwf.seek(0);
	}

	return true;
}

bool CLC7UnixImporter::ParseColonFile(QString filename, int fields, QList<QList<QString>> & lines, QString &error)
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
				continue;
			}

			if (line.startsWith("#"))
			{
				continue;
			}

			QStringList parts = line.split(":");
			if (parts.size() == fields)
			{
				lines.append(parts);
			}
			else
			{
				error = "Invalid file format";
				return false;
			}

		}
		pwf.seek(0);
	}

	return true;
}


bool CLC7UnixImporter::ImportPasswdFile(QString filename, QList<ACCT> & accts, QString &error, bool &cancelled)
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

	UpdateStatus("Importing passwd file", 0, true);

	QList<QList<QString>> lines;
	if (!ParseColonFile(filename, 7, lines, error))
	{
		return false;
	}
	if (lines.size() == 0)
	{
		error = "File is empty or is not passwd format file";
		return false;
	}

	UpdateStatus("Loading accounts", 0, true);

	int count = lines.size();
	int imported = 0;
	foreach(QStringList fields, lines)
	{
		if (m_ctrl && m_ctrl->StopRequested())
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

		acct.valid_fields |= FIELD_USERINFO;
		acct.userinfo = fields[4];

		acct.valid_fields |= FIELD_HOMEDIR;
		acct.homedir = fields[5];

		acct.valid_fields |= FIELD_SHELL;
		acct.shell = fields[6];

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
			if (m_ctrl)
				m_ctrl->UpdateCurrentProgressBar(imported * 100 / count);
		}
	}

	if (m_ctrl)
		m_ctrl->UpdateCurrentProgressBar(100);

	return true;
}

bool CLC7UnixImporter::ImportShadowFile(QString filename, QList<ACCT> & accts, QString &error, bool &cancelled)
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

	UpdateStatus("Importing shadow file", 0, true);

	QList<QList<QString>> lines;
	if (!ParseColonFile(filename, 9, lines, error))
	{
		return false;
	}
	if (lines.size() == 0)
	{
		error = "File is empty or is not shadow format file";
		return false;
	}

	UpdateStatus("Loading accounts", 0, true);

	int count = lines.size();
	int imported = 0;
	foreach(QStringList fields, lines)
	{
		if (m_ctrl && m_ctrl->StopRequested())
		{
			cancelled = true;
			return true;
		}

		ACCT acct;
		acct.valid_fields = 0;

		acct.valid_fields |= FIELD_USERNAME;
		acct.username = fields[0];

		acct.valid_fields |= FIELD_PASSWORD;
		acct.password = fields[1].toLatin1();

		bool ok;

		ok = true;
		acct.last_changed_date = (fields[2].toInt(&ok));
		if (ok)
			acct.valid_fields |= FIELD_LAST_CHANGED_DATE;

		ok = true;
		acct.days_left_until_change_allowed = fields[3].toInt(&ok);
		if (ok)
			acct.valid_fields |= FIELD_DAYS_LEFT_UNTIL_CHANGE_ALLOWED;

		ok = true;
		acct.days_left_until_password_expires = fields[4].toInt(&ok);
		if (ok)
			acct.valid_fields |= FIELD_DAYS_LEFT_UNTIL_PASSWORD_EXPIRES;

		ok = true;
		acct.warning_days_for_password_expire = fields[5].toInt(&ok);
		if (ok)
			acct.valid_fields |= FIELD_WARNING_DAYS_FOR_PASSWORD_EXPIRE;

		ok = true;
		acct.disable_days_after_password_expire = fields[6].toInt(&ok);
		if (ok)
			acct.valid_fields |= FIELD_DISABLE_DAYS_AFTER_PASSWORD_EXPIRE;

		ok = true;
		acct.account_expired_date = fields[7].toInt(&ok);
		if (ok)
			acct.valid_fields |= FIELD_ACCOUNT_EXPIRED_DATE;

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
			if (m_ctrl)
				m_ctrl->UpdateCurrentProgressBar(imported * 100 / count);
		}
	}

	if (m_ctrl)
		m_ctrl->UpdateCurrentProgressBar(100);

	return true;
}



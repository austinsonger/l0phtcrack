#include<stdafx.h>

static QDateTime janfirst1970 = QDateTime(QDate(1970, 1, 1), QTime(0, 0, 0));

ShadowImporter::ShadowImporter(ILC7AccountList *accountlist, ILC7CommandControl *ctrl)
{TR;
	if (ctrl)
		m_ctrl = ctrl->GetSubControl("Shadow Importer: ");
	else
		m_ctrl = nullptr;

	m_accountlist = accountlist;
	m_account_limit = 0;

	m_pimp_oldpasswd = new CLC7UnixImporter_OLDPASSWD(accountlist, ctrl);
	m_pimp_linux_passwdshadow = new CLC7UnixImporter_LINUX_PASSWDSHADOW(accountlist, ctrl);
	m_pimp_linux_shadow = new CLC7UnixImporter_LINUX_SHADOW(accountlist, ctrl);
	m_pimp_solaris_passwdshadow = new CLC7UnixImporter_SOLARIS_PASSWDSHADOW(accountlist, ctrl);
	m_pimp_solaris_shadow = new CLC7UnixImporter_SOLARIS_SHADOW(accountlist, ctrl);;
	m_pimp_bsd_passwdmasterpasswd = new CLC7UnixImporter_BSD_PASSWDMASTERPASSWD(accountlist, ctrl);
	m_pimp_bsd_masterpasswd = new CLC7UnixImporter_BSD_MASTERPASSWD(accountlist, ctrl);
	m_pimp_aix_passwdsecuritypasswdsecurityuser = new CLC7UnixImporter_AIX_PASSWDSECURITYPASSWDSECURITYUSER(accountlist, ctrl);
	m_pimp_aix_passwdsecuritypasswd = new CLC7UnixImporter_AIX_PASSWDSECURITYPASSWD(accountlist, ctrl);
	m_pimp_aix_securitypasswd = new CLC7UnixImporter_AIX_SECURITYPASSWD(accountlist, ctrl);

	RegisterImporters();
}

ShadowImporter::~ShadowImporter()
{TR;
	delete m_pimp_oldpasswd;
	delete m_pimp_linux_passwdshadow;
	delete m_pimp_linux_shadow;
	delete m_pimp_solaris_passwdshadow;
	delete m_pimp_solaris_shadow;
	delete m_pimp_bsd_passwdmasterpasswd;
	delete m_pimp_bsd_masterpasswd;
	delete m_pimp_aix_passwdsecuritypasswdsecurityuser;
	delete m_pimp_aix_passwdsecuritypasswd;
	delete m_pimp_aix_securitypasswd;
	
	if (m_ctrl)
		m_ctrl->ReleaseSubControl();
}

void ShadowImporter::SetAccountLimit(quint32 alim)
{
	m_account_limit = alim;
}

void ShadowImporter::GetPasswdImporters(QList<ILC7UnixImporter *> & passwd_importers)
{
	passwd_importers = m_importers;
}

ILC7UnixImporter *ShadowImporter::GetPasswdImporter(QString name)
{
	return m_importers_by_name[name];
}


void ShadowImporter::RegisterImporter(ILC7UnixImporter *pimp)
{
	m_importers.append(pimp);
	m_importers_by_name[pimp->name()] = pimp;
}

void ShadowImporter::RegisterImporters()
{
	RegisterImporter(m_pimp_linux_passwdshadow);
	RegisterImporter(m_pimp_linux_shadow);
	RegisterImporter(m_pimp_solaris_passwdshadow);
	RegisterImporter(m_pimp_solaris_shadow);
	RegisterImporter(m_pimp_bsd_passwdmasterpasswd);
	RegisterImporter(m_pimp_bsd_masterpasswd);
	RegisterImporter(m_pimp_aix_passwdsecuritypasswdsecurityuser);
	RegisterImporter(m_pimp_aix_passwdsecuritypasswd);
	RegisterImporter(m_pimp_aix_securitypasswd);
	RegisterImporter(m_pimp_oldpasswd);
}

/////////////////////////////////


bool ShadowImporter::IncludedInFlags(LC7Account & acct, IMPORT_FLAGS flags)
{
	if (flags & exclude_expired)
	{
		if (acct.mustchange)
		{
			return false;
		}
	}
	if (flags & exclude_disabled)
	{
		if (acct.disabled)
		{
			return false;
		}
	}
	if (flags & exclude_lockedout)
	{
		if (acct.lockedout)
		{
			return false;
		}
	}

	return true;
}


bool ShadowImporter::DoImport(QString importer_name, QStringList filenames, FOURCC hashtype, IMPORT_FLAGS flags, QString & error, bool & cancelled)
{
	if (!m_importers_by_name.contains(importer_name))
	{
		return false;
	}

	ILC7UnixImporter *pimp = m_importers_by_name[importer_name];
	QList<LC7Account> lc7accts;
	if (!pimp->DoImport(lc7accts, filenames, hashtype, error, cancelled))
	{
		return false;
	}
	if (cancelled)
	{
		return true;
	}

	UpdateStatus("Importing accounts", 0);

	quint32 account_count = 0;
	foreach(LC7Account lc7acct, lc7accts)
	{
		if (IncludedInFlags(lc7acct, flags))
		{
			account_count++;
		}
	}

	if (account_count == 0)
	{
		error = "No accounts available with selected hash type.";
		return false;
	}

	m_accountlist->Acquire();
	size_t starting_count = m_accountlist->GetAccountCount();
	
	quint32 imported_count = 0;
	bool success = true;
	int remconfig = -1;

	foreach(LC7Account lc7acct, lc7accts)
	{
		if (m_ctrl && m_ctrl->StopRequested())
		{
			cancelled = true;
			break;
		}

		if (!IncludedInFlags(lc7acct, flags))
		{
			continue;
		}

		// Add remediation config
		if (remconfig == -1 && !m_remediations.isEmpty())
		{
			remconfig = m_accountlist->AddRemediations(m_remediations);
		}
		lc7acct.remediations = remconfig;

		if (!m_accountlist->AppendAccount(lc7acct))
		{
			error = "Account limit reached, upgrade your license to import more accounts.";
			success = false;
			break;
		}

		imported_count++;

		if ((imported_count % 10) == 0)
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
		for (quint32 x = (quint32)(starting_count + imported_count); x<(starting_count + account_count); x++)
		{
			positions.insert(x);
		}
		m_accountlist->RemoveAccounts(positions);
	}
	m_accountlist->Release();

	return success;

}

void ShadowImporter::SetRemediations(const LC7Remediations &remediations)
{
	m_remediations = remediations;
}

void ShadowImporter::UpdateStatus(QString statustext, quint32 cur, bool statuslog)
{TR;
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

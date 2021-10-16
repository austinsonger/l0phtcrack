#include<stdafx.h>

PWDumpImporter::PWDumpImporter(ILC7AccountList *accountlist, ILC7CommandControl *ctrl)
{TR;
	m_ctrl = ctrl ? ctrl->GetSubControl("PWDump Importer: ") : nullptr;
	m_accountlist = accountlist;
	m_account_limit = 0;

	CLC7ImportWinPlugin *winplugin = (CLC7ImportWinPlugin*)g_pLinkage->GetPluginRegistry()->FindPluginByID(UUID_IMPORTWINPLUGIN);
	m_parse_cr = winplugin->UseChallengeResponse();
}

PWDumpImporter::~PWDumpImporter()
{TR;
	if (m_ctrl)
		m_ctrl->ReleaseSubControl();
}

void PWDumpImporter::SetAccountLimit(quint32 alim)
{
	m_account_limit = alim;
}



static bool isHex(const QString &s)
{
	foreach(QChar c, s)
	{
		char x = c.toLatin1();
		if (!((x >= 'a' && x <= 'f') ||
			  (x >= 'A' && x <= 'F') ||
			  (x >= '0' && x <= '9')))
		{
			return false;
		}
	}
	return true;
}

static bool isNumber(const QString &s)
{
	foreach(QChar c, s)
	{
		char x = c.toLatin1();
		if (!(x >= '0' && x <= '9'))
		{
			return false;
		}
	}
	return true;
}

bool PWDumpImporter::ParseCR(QStringList parts, LC7Account & acct)
{
	if (!m_parse_cr)
	{
		return false;
	}

	int n = 0;
	if (parts.size() < 5)
		return false;

	bool foundhashes = false;
	int version = 0;
	bool lm = false;
	bool nt = false;
	int schal = -1;
	int cchal = -1;
	int lmresp = -1;
	int ntresp = -1;

	for (n = 0; n < parts.size()-2; n++)
	{
		// LMCRv1 only (challenge first)
		if (parts[n].size() == 16 && parts[n + 1].size() == 48 && (parts[n + 2].size() == 0 || parts[n + 2] == "000000000000000000000000000000000000000000000000" || parts[n + 2].startsWith("NO PASSWORD")) && isHex(parts[n]) && isHex(parts[n + 1]))
		{
			foundhashes = true;
			version = 1;
			lm = true;
			schal = n;
			lmresp = n + 1;
			break;
		}
		// NTLMCRv1 only (challenge first)
		if (parts[n].size() == 16 && (parts[n + 1].size() == 0 || parts[n + 1] == "000000000000000000000000000000000000000000000000" || parts[n + 1].startsWith("NO PASSWORD")) && parts[n + 2].size() == 48 && isHex(parts[n]) && isHex(parts[n + 2]))
		{
			foundhashes = true;
			version = 1;
			nt = true;
			schal = n;
			ntresp = n + 1;
			break;
		}
		// LMCRv1 only (challenge last)
		if (parts[n].size() == 48 && (parts[n + 1].size() == 0 || parts[n + 1] == "000000000000000000000000000000000000000000000000" || parts[n + 1].startsWith("NO PASSWORD")) && parts[n + 2].size() == 16 && isHex(parts[n]) && isHex(parts[n + 2]))
		{
			foundhashes = true;
			version = 1;
			lm = true;
			schal = n + 2;
			lmresp = n;
			break;
		}
		// NTLMCRv1 only (challenge last)
		if ((parts[n].size() == 0 || parts[n] == "000000000000000000000000000000000000000000000000" || parts[n].startsWith("NO PASSWORD")) && parts[n + 1].size() == 48 && parts[n + 2].size() == 16 && isHex(parts[n + 1]) && isHex(parts[n + 2]))
		{
			foundhashes = true;
			version = 1;
			nt = true;
			schal = n + 2;
			ntresp = n + 1;
			break;
		}
		// LMCRv1 + NTLMCRv1
		if (parts[n].size() == 16 && parts[n + 1].size() == 48 && parts[n + 2].size() == 48 && isHex(parts[n]) && isHex(parts[n + 1]) && isHex(parts[n + 2]))
		{
			foundhashes = true;
			version = 1;
			lm = true;
			nt = true;
			schal = n;
			lmresp = n + 1;
			ntresp = n + 2;
			break;
		}
		// LMCRv1 + NTLMCRv1 (challenge last)
		if (parts[n].size() == 48 && parts[n + 1].size() == 48 && parts[n + 2].size() == 16 && isHex(parts[n]) && isHex(parts[n + 1]) && isHex(parts[n + 2]))
		{
			foundhashes = true;
			version = 1;
			lm = true;
			nt = true;
			schal = n + 2;
			lmresp = n;
			ntresp = n + 1;
			break;
		}
		// LMCRv2
		if (parts[n].size() == 16 && parts[n + 1].size() == 32 && parts[n + 2].size() == 16 && isHex(parts[n]) && isHex(parts[n + 1]) && isHex(parts[n + 2]))
		{
			foundhashes = true;
			version = 2;
			lm = true;
			schal = n;
			lmresp = n + 1;
			cchal = n + 2;
			break;
		}

		// NTLMCRv2
		if (parts[n].size() == 16 && parts[n + 1].size() == 32 && parts[n + 2].size() > 16 && parts[n + 2].startsWith("01010000") && isHex(parts[n]) && isHex(parts[n + 1]) && isHex(parts[n + 2]))
		{
			foundhashes = true;
			version = 2;
			nt = true;
			schal = n;
			ntresp = n + 1;
			cchal = n + 2;
			break;
		}
	}

	if (!foundhashes)
	{
		return false;
	}

	if (parts[0].contains('\\'))
	{
		QStringList principal = parts[0].split('\\');
		acct.domain = principal[0];
		acct.username = principal[1];

		if (n == 2 && isNumber(parts[1]))
		{
			// principal, uid
			acct.userid = parts[1];
		}
		else if (n == 1)
		{
			// principal
		}
		else
		{
			return false;
		}
	}
	else if (n >= 3 && isNumber(parts[1]))
	{
		// username, uid, domain
		acct.username = parts[0];
		acct.userid = parts[1];
		acct.domain = parts[2];
	}
	else if (n >= 3 && isNumber(parts[2]))
	{
		// username, domain, uid
		acct.username = parts[0];
		acct.domain = parts[1];
		acct.userid = parts[2];
	}
	
	if (lm && version==1)
	{
		LC7Hash hash;
		hash.hashtype = FOURCC(HASHTYPE_LM_CHALRESP);
		hash.crackstate = 0;
		hash.cracktime = 0;
		hash.hash = (parts[schal] + ":" + parts[lmresp]).toLatin1();
		acct.hashes.append(hash);
	}
	if (lm && version==2)
	{
		LC7Hash hash;
		hash.hashtype = FOURCC(HASHTYPE_LM_CHALRESP_V2);
		hash.crackstate = 0;
		hash.cracktime = 0;
		hash.hash = (parts[schal] + ":" + parts[lmresp] + ":" + parts[cchal]).toLatin1();
		acct.hashes.append(hash);
	}
	if (nt && version == 1)
	{
		LC7Hash hash;
		hash.hashtype = FOURCC(HASHTYPE_NTLM_CHALRESP);
		hash.crackstate = 0;
		hash.cracktime = 0;
		hash.hash = (parts[schal] + ":" + parts[ntresp]).toLatin1();
		acct.hashes.append(hash);
	}
	if (nt && version == 2)
	{
		LC7Hash hash;
		hash.hashtype = FOURCC(HASHTYPE_NTLM_CHALRESP_V2);
		hash.crackstate = 0;
		hash.cracktime = 0;
		hash.hash = (parts[schal] + ":" + parts[ntresp] + ":" + parts[cchal]).toLatin1();
		acct.hashes.append(hash);
	}
	
	return true;
}

bool PWDumpImporter::ParseNT(QStringList parts, LC7Account & acct)
{
	if (parts.size() == 7)
	{
		QString username = parts[0];
		quint32 uid = parts[1].toUInt();
		QString lmhash = parts[2];
		QString nthash = parts[3];
		QString comment = parts[4];
		QString homedir = parts[5];

		if (lmhash.startsWith("NO PASSWORD*"))
		{
			lmhash = "";
		}
		if (nthash.startsWith("NO PASSWORD*"))
		{
			nthash = "";
		}


		if (lmhash.size() > 0)
		{
			LC7Hash hash;
			hash.hashtype = FOURCC(HASHTYPE_LM);
			hash.crackstate = 0;
			hash.cracktime = 0;
			hash.hash = lmhash.toLatin1();

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

		if (nthash.size() > 0)
		{
			LC7Hash hash;
			hash.hashtype = FOURCC(HASHTYPE_NT);
			hash.crackstate = 0;
			hash.cracktime = 0;
			hash.hash = nthash.toLatin1();

			if (hash.hash == "31D6CFE0D16AE931B73C59D7E0C089C0")
			{
				hash.crackstate = CRACKSTATE_CRACKED;
				hash.cracktype = "No Password";
			}

			acct.hashes.append(hash);
		}

		acct.userid = QString("%1").arg(uid);
		if (homedir.size() > 0)
		{
			acct.userinfo = QString("%1 %2").arg(comment).arg(homedir);
		}
		else
		{
			acct.userinfo = QString("%1").arg(comment);
		}

		if (username.split("\\").size() == 2)
		{
			acct.username = username.split("\\")[1];
			acct.domain = username.split("\\")[0];
		}
		else
		{
			acct.username = username;
			acct.domain = "";
		}

		return true;
	}

	return false;
}

bool PWDumpImporter::GetHashTypes(QString filename, QSet<fourcc> & hashtypes, QString & error)
{
	TR;
	
	QFile pwf(filename);
	if (!pwf.open(QIODevice::ReadOnly))
	{
		error = "Unable to open PWDump file.";
		return false;
	}

	bool success = true;

	QTextStream ts(&pwf);

	while (!ts.atEnd())
	{
		QString line = ts.readLine();

		QStringList parts = line.split(":");

		LC7Account acct;

		if (!ParseCR(parts, acct))
		{
			if (!ParseNT(parts, acct))
			{
				continue;
			}
		}
		foreach(const LC7Hash &hash, acct.hashes)
		{
			hashtypes.insert(hash.hashtype);
		}
	}

	return success;
}

bool PWDumpImporter::DoImport(QString filename, QString & error, bool & cancelled)
{TR;
	QFile pwf(filename);
	if (!pwf.open(QIODevice::ReadOnly))
	{
		error = "Unable to open PWDump file.";
		return false;
	}

	// Get estimate
	UpdateStatus("Estimating time to import...", 0, true);
	size_t account_count = 0;
	{
		QTextStream ts(&pwf);

		while (!ts.atEnd())
		{
			QString line = ts.readLine();
			account_count++;
		}
		pwf.seek(0);
	}

	UpdateStatus("Importing from PWDump file...",0,true);

	bool success = true;

	m_accountlist->Acquire();
	int starting_count = m_accountlist->GetAccountCount();

	QTextStream ts(&pwf);

	int imported_count = 0;
	if (m_ctrl)
		m_ctrl->UpdateCurrentProgressBar(0);
	
	bool iscr = false;
	bool isnt = false;

	while (!ts.atEnd())
	{
		if (m_ctrl && m_ctrl->StopRequested())
		{
			cancelled = true;
			break;
		}

		QString line = ts.readLine();

		QStringList parts = line.split(":");

		LC7Account acct;

		if (ParseCR(parts, acct))
		{
			if (isnt)
			{
				error = "Can not import pwdump files with both challenge response hashes and password hashes.";
				success = false;
				break;
			}
			iscr = true;
		}
		else if (ParseNT(parts,acct))
		{
			if (iscr)
			{
				error = "Can not import pwdump files with both challenge response hashes and password hashes.";
				success = false;
				break;
			}
			isnt = true;
		}
		else
		{
			continue;
		}
	
		acct.lastchanged = 0;
		acct.lockedout = 0;
		acct.disabled = 0;
		acct.neverexpires = 0;
		acct.mustchange = 0;
		acct.machine = "";
		acct.remediations = -1;

		if (!m_accountlist->AppendAccount(acct))
		{
			error = "Account limit reached, upgrade your license to import more accounts.";
			success = false;
			break;
		}

		imported_count++;

		if ((imported_count % 100) == 0)
		{
			quint32 complete = (quint32)(imported_count * 100) / (quint32)account_count;
			QString str = QString("%1 users imported out of %2").arg(imported_count).arg(account_count);
			UpdateStatus(str, complete, false);
		}

		// Cap account count
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


void PWDumpImporter::UpdateStatus(QString statustext, quint32 cur, bool statuslog)
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

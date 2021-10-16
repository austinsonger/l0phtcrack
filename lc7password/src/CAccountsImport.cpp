#include<stdafx.h>

CAccountsImport::CAccountsImport(ILC7AccountList *accountlist, ILC7CommandControl *ctrl)
{
	m_accountlist = accountlist;
	m_ctrl = ctrl;
	m_plink = GET_ILC7PASSWORDLINKAGE(g_pLinkage);
}

CAccountsImport::~CAccountsImport()
{
}

void CAccountsImport::setFilename(QString filename)
{
	m_filename = filename;
}

bool CAccountsImport::DoImport(QString &error, bool &cancelled)
{
	cancelled = false;

	QFile infile(m_filename);
	if (!infile.open(QIODevice::ReadOnly))
	{
		error = "Unable to open file for import.";
		return false;
	}

	m_ctrl->SetStatusText("Importing accounts");
	m_ctrl->UpdateCurrentProgressBar(0);

	m_accountlist->Acquire();
	/*
	int i, cnt = m_accountlist->GetAccountCount();
	for (i = 0; i < cnt; i++)
	{
		if (m_ctrl->StopRequested())
		{
			break;
		}

		if ((i % 100) == 0)
		{
			m_ctrl->SetStatusText(QString("Importing accounts (%1/%2)").arg(i).arg(cnt));
			m_ctrl->UpdateCurrentProgressBar((quint32)(i * 100 / cnt));
		}
	}
	*/
	m_ctrl->SetStatusText(QString("Imported accounts"));

	m_accountlist->Release();

	infile.close();

	m_ctrl->UpdateCurrentProgressBar(100);

	return true;
}

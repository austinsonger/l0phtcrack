#include"stdafx.h"
#include"lc7app.h"

CLC7Updater::CLC7Updater(ILC7Controller *ctrl) : QThread(NULL)
{
	m_ctrl = ctrl;
	m_manual_check = false;
}

CLC7Updater::~CLC7Updater()
{TR;

}

void CLC7Updater::setManualCheck()
{TR;
	m_manual_check = true;
}

void CLC7Updater::run()
{TR;
	bool manual_check = m_manual_check;
	m_manual_check = false;

#if (PLATFORM == PLATFORM_WIN64)
	#ifdef _DEBUG
		QString updatejson = "https://s3.amazonaws.com/updates.lc7/lc7/win64/release/current.json";
	#elif defined(BETA) && BETA
		QString updatejson = "https://s3.amazonaws.com/updates.lc7/lc7/win64/beta/current.json";
	#else
		QString updatejson = "https://s3.amazonaws.com/updates.lc7/lc7/win64/release/current.json";
	#endif
#elif (PLATFORM == PLATFORM_WIN32)
	#ifdef _DEBUG
		QString updatejson = "https://s3.amazonaws.com/updates.lc7/lc7/win32/release/current.json";
	#elif defined(BETA) && BETA
		QString updatejson = "https://s3.amazonaws.com/updates.lc7/lc7/win32/beta/current.json";
	#else
		QString updatejson = "https://s3.amazonaws.com/updates.lc7/lc7/win32/release/current.json";
	#endif
#else
	#error "Specify URL here"
#endif

	// Get current version
	m_current_version = VERSION_STRING;

	// Download update json 
	CLC7FileDownloader down(this);
	QString error;
	if (!down.download(updatejson, 20000, error))
	{
		if (manual_check)
		{
			m_ctrl->GetGUILinkage()->WarningMessage("L0phtCrack Update", QString("Updates are not available at this time due to the network being unresponsive. Please connect to the Internet or try again later. Error: %1").arg(error));
		}
		return;
	}

	QJsonDocument doc = QJsonDocument::fromJson(down.downloadedData());
	if (doc.isNull())
	{
		if (manual_check)
		{
			m_ctrl->GetGUILinkage()->ErrorMessage("L0phtCrack Update", "Unable to parse update. Notify support@l0phtcrack.com.");
		}
		return;
	}

	m_url = doc.object()["url"].toString();
	m_sha256digest = doc.object()["sha256digest"].toString();
	m_new_version = doc.object()["version"].toString();
	m_md5digest = doc.object()["md5digest"].toString();
	m_releasenotes = doc.object()["releasenotes"].toString();

#ifdef _WIN32
	if (GetKeyState(VK_LCONTROL) & 0x8000)
	{
		emit sig_doUpdate();
		return;
	}
#endif

	if (m_current_version == m_new_version)
	{
		if (manual_check)
		{
			m_ctrl->GetGUILinkage()->InfoMessage("L0phtCrack Update", "No updates are available. You have the latest version.");
		}
		return;
	}
	else if (!manual_check && 
		m_ctrl->GetSettings()->value("_update_:reminderversion").toString()==m_new_version &&
		(!m_ctrl->GetSettings()->contains("_update_:reminderdate") || (m_ctrl->GetSettings()->value("_update_:reminderdate").toDateTime() > QDateTime::currentDateTime())))
	{
		return;
	}
		
	emit sig_doUpdate();
}

#ifdef Q_OS_WIN 
static bool RunElevatedProcess(QString exefile)
{
	exefile = QDir::toNativeSeparators(exefile);

//	int result = (int)::ShellExecuteA(0, "open", exefile.toUtf8().constData(), 0, 0, SW_SHOWNORMAL);
//	if (SE_ERR_ACCESSDENIED == result)
//	{
		//// Requesting elevation
		intptr_t result = (intptr_t)::ShellExecuteA(0, "runas", exefile.toUtf8().constData(), 0, 0, SW_SHOWNORMAL);
	//}
	if (result <= 32)
	{
		return false;
	}
	return true;
}
#endif

void CLC7Updater::doUpdate()
{TR;
	CLC7UpdateDialog dlg(m_ctrl, m_url, m_current_version, m_new_version, m_releasenotes);
	m_ctrl->GetGUILinkage()->ShadeUI(true);
	int res = dlg.exec();
	m_ctrl->GetGUILinkage()->ShadeUI(false);

	if (res == CLC7UpdateDialog::Rejected)
	{
		if (dlg.getSkipThisUpdate())
		{
			m_ctrl->GetSettings()->setValue("_update_:reminderversion", m_new_version);
			m_ctrl->GetSettings()->remove("_update_:reminderdate");
		}
		else if (dlg.getRemindMeTomorrow())
		{
			m_ctrl->GetSettings()->setValue("_update_:reminderversion", m_new_version);
			m_ctrl->GetSettings()->setValue("_update_:reminderdate", QDateTime::currentDateTime().addDays(1));
		}

		return;
	}

	// Run updater
	QString updater = dlg.getFilePath();
#ifdef Q_OS_WIN
	if (!RunElevatedProcess(updater))
#else
	if (!QProcess::startDetached(updater, QStringList()))
#endif
	{
		m_ctrl->GetGUILinkage()->ErrorMessage("L0phtCrack Update", "Unable to perform update at this time. Update requires administrative privileges.");
		QFile::remove(updater);
		return;
	}

	g_jom.ReleaseChildProcesses();

	m_ctrl->GetGUILinkage()->Exit(false);

	return;
}
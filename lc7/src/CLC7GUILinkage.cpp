#include<stdafx.h>

#undef TR
#define TR


CLC7GUILinkage::CLC7GUILinkage(LC7Main *pMainWnd)
{TR;
	m_pMainWnd=pMainWnd;
	
	connect(this, &CLC7GUILinkage::sig_asyncProc, pMainWnd, &LC7Main::slot_asyncProc);
}

CLC7GUILinkage::~CLC7GUILinkage()
{TR;
}

ILC7Interface *CLC7GUILinkage::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7GUILinkage")
	{
		return this;
	}
	return NULL;
}

bool CLC7GUILinkage::AddMainMenuTab(QString strTabTitle, QString strInternalName, QWidget *pMainMenuTab)
{TR;
	return m_pMainWnd->AddMainMenuTab(strTabTitle, strInternalName, pMainMenuTab);
}

bool CLC7GUILinkage::RemoveMainMenuTab(QWidget *pMainMenuTab)
{TR;
	return m_pMainWnd->RemoveMainMenuTab(pMainMenuTab);
}

QWidget *CLC7GUILinkage::GetMainMenuTab(QString strInternalName)
{
	TR;
	return m_pMainWnd->GetMainMenuTab(strInternalName);
}


bool CLC7GUILinkage::SwitchToMainMenuTab(QString strInternalName)
{TR;
	return m_pMainWnd->SwitchToMainMenuTab(strInternalName);
}

void CLC7GUILinkage::AddTopMenuItem(ILC7Action *act)
{
	TR;
	m_pMainWnd->AddTopMenuItem(act);
}

void CLC7GUILinkage::RemoveTopMenuItem(ILC7Action *act)
{
	TR;
	m_pMainWnd->RemoveTopMenuItem(act);
}

void CLC7GUILinkage::AddStartupDialogItem(ILC7Action *act)
{
	TR;
	m_pMainWnd->AddStartupDialogItem(act);
}

void CLC7GUILinkage::RemoveStartupDialogItem(ILC7Action *act)
{
	TR;
	m_pMainWnd->RemoveStartupDialogItem(act);
}




QAbstractButton *CLC7GUILinkage::GetHelpButton(void)
{TR;
	return m_pMainWnd->GetHelpButton();
}

ILC7WorkQueueWidget *CLC7GUILinkage::GetWorkQueueWidget()
{TR;
	return m_pMainWnd->GetWorkQueueWidget();
}


void CLC7GUILinkage::ErrorMessage(QString title, QString msg)
{TR;
	QSemaphore s(1);
	s.acquire();

	bool ret;
	
	QList<QVariant> args;
	args.append(QVariant(title));
	args.append(QVariant(msg));
	args.append(QVariant((qulonglong)&ret));

	emit sig_asyncProc(&s, "ErrorMessage", args);

	s.acquire();
}

void CLC7GUILinkage::WarningMessage(QString title, QString msg)
{TR;
	QSemaphore s(1);
	s.acquire();

	bool ret;
	
	QList<QVariant> args;
	args.append(QVariant(title));
	args.append(QVariant(msg));
	args.append(QVariant((qulonglong)&ret));

	emit sig_asyncProc(&s, "WarningMessage", args);

	s.acquire();
}

void CLC7GUILinkage::InfoMessage(QString title, QString msg)
{TR;
	QSemaphore s(1);
	s.acquire();

	bool ret;
	
	QList<QVariant> args;
	args.append(QVariant(title));
	args.append(QVariant(msg));
	args.append(QVariant((qulonglong)&ret));

	emit sig_asyncProc(&s, "InfoMessage", args);

	s.acquire();
}

bool CLC7GUILinkage::YesNoBox(QString title, QString msg)
{TR;
	QSemaphore s(1);
	s.acquire();

	bool ret;
	
	QList<QVariant> args;
	args.append(QVariant(title));
	args.append(QVariant(msg));
	args.append(QVariant((qulonglong)&ret));

	emit sig_asyncProc(&s, "YesNoBox", args);
	s.acquire();

	return ret;
}


bool CLC7GUILinkage::YesNoBoxWithNeverAgain(QString title, QString msg, QString neveragainkey)
{
	TR;
	QSemaphore s(1);
	s.acquire();

	bool ret;

	QList<QVariant> args;
	args.append(QVariant(title));
	args.append(QVariant(msg));
	args.append(QVariant(neveragainkey));
	args.append(QVariant((qulonglong)&ret));

	emit sig_asyncProc(&s, "YesNoBoxWithNeverAgain", args);
	s.acquire();

	return ret;
}

bool CLC7GUILinkage::AskForPassword(QString title, QString msg, QString & passwd)
{TR;
	QSemaphore s(1);
	s.acquire();

	bool ret;

	QList<QVariant> args;
	args.append(QVariant(title));
	args.append(QVariant(msg));
	args.append(QVariant((qulonglong)&passwd));
	args.append(QVariant((qulonglong)&ret));

	emit sig_asyncProc(&s, "AskForPassword", args);
	s.acquire();

	return ret;
}

bool CLC7GUILinkage::AskForUsernameAndPassword(QString title, QString msg, QString &username, QString & passwd)
{
	QSemaphore s(1);
	s.acquire();

	bool ret;

	QList<QVariant> args;
	args.append(QVariant(title));
	args.append(QVariant(msg));
	args.append(QVariant((qulonglong)&username));
	args.append(QVariant((qulonglong)&passwd));
	args.append(QVariant((qulonglong)&ret));

	emit sig_asyncProc(&s, "AskForUsernameAndPassword", args);
	s.acquire();

	return ret;
}


bool CLC7GUILinkage::OpenFileDialog(QString title, QString starting_folder, QString filter, QString &filepath)
{TR;
	QSemaphore s(1);
	s.acquire();

	bool ret;

	QList<QVariant> args;
	args.append(QVariant(title));
	args.append(QVariant(starting_folder));
	args.append(QVariant(filter));
	args.append(QVariant((qulonglong)&filepath));
	args.append(QVariant((qulonglong)&ret));

	emit sig_asyncProc(&s, "OpenFileDialog", args);
	s.acquire();

	return ret;
}

bool CLC7GUILinkage::OpenFileDialogMultiple(QString title, QString starting_folder, QString filter, QStringList &paths)
{TR;
	QSemaphore s(1);
	s.acquire();

	bool ret;

	QList<QVariant> args;
	args.append(QVariant(title));
	args.append(QVariant(starting_folder));
	args.append(QVariant(filter));
	args.append(QVariant((qulonglong)&paths));
	args.append(QVariant((qulonglong)&ret));

	emit sig_asyncProc(&s, "OpenFileDialogMultiple", args);
	s.acquire();

	return ret;
}

bool CLC7GUILinkage::SaveFileDialog(QString title, QString starting_folder, QString filter, QString &filepath)
{TR;
	QSemaphore s(1);
	s.acquire();

	bool ret;

	QList<QVariant> args;
	args.append(QVariant(title));
	args.append(QVariant(starting_folder));
	args.append(QVariant(filter));
	args.append(QVariant((qulonglong)&filepath));
	args.append(QVariant((qulonglong)&ret));

	emit sig_asyncProc(&s, "SaveFileDialog", args);
	s.acquire();

	return ret;
}

bool CLC7GUILinkage::GetDirectoryDialog(QString title, QString starting_folder, QString &dirpath)
{
	TR;
	QSemaphore s(1);
	s.acquire();

	bool ret;

	QList<QVariant> args;
	args.append(QVariant(title));
	args.append(QVariant(starting_folder));
	args.append(QVariant((qulonglong)&dirpath));
	args.append(QVariant((qulonglong)&ret));

	emit sig_asyncProc(&s, "GetDirectoryDialog", args);
	s.acquire();

	return ret;
}


bool CLC7GUILinkage::RequestNewSession()
{
	TR;
	QSemaphore s(1);
	s.acquire();

	bool ret;

	QList<QVariant> args;
	args.append(QVariant((qulonglong)&ret));

	emit sig_asyncProc(&s, "RequestNewSession", args);
	s.acquire();

	return ret;
}

bool CLC7GUILinkage::RequestOpenSession(QString requested_session)
{TR;
	QSemaphore s(1);
	s.acquire();

	bool ret;

	QList<QVariant> args;
	args.append(requested_session);
	args.append(QVariant((qulonglong)&ret));

	emit sig_asyncProc(&s, "RequestOpenSession", args);
	s.acquire();

	return ret;
}



bool CLC7GUILinkage::RequestCloseSession(bool force, bool allow_startup_dialog)
{
	TR;
	QSemaphore s(1);
	s.acquire();

	bool ret;

	QList<QVariant> args;
	args.append(force);
	args.append(allow_startup_dialog);
	args.append(QVariant((qulonglong)&ret));

	emit sig_asyncProc(&s, "RequestCloseSession", args);
	s.acquire();

	return ret;
}


void CLC7GUILinkage::Callback(ILC7GUILinkage::GUITHREAD_CALLBACK_TYPE *callback, QList<QVariant> args)
{TR;
	QSemaphore s(1);
	s.acquire();

	args.prepend(QVariant((qulonglong)callback));

	emit sig_asyncProc(&s, "Callback", args);

	s.acquire();
}


void CLC7GUILinkage::AppendToActivityLog(QString text)
{
	m_pMainWnd->GetWorkQueueWidget()->AppendToActivityLog(text);
}

void CLC7GUILinkage::UpdateUI()
{
	m_pMainWnd->UpdateUILater();
}

void CLC7GUILinkage::ShadeUI(bool shade)
{
	QSemaphore s(1);
	s.acquire();

	QList<QVariant> args;
	args.append(shade);

	emit sig_asyncProc(&s, "ShadeUI", args);
	s.acquire();
}


void CLC7GUILinkage::Exit(bool warn)
{TR;
	//QSemaphore s(1);
	//s.acquire();

	QList<QVariant> args;
	args << warn;

	//emit sig_asyncProc(&s, "Exit", args);
	//s.acquire();

	emit sig_asyncProc(NULL, "Exit", args);
}


bool CLC7GUILinkage::PauseBackgroundSession(QString sessionfile)
{TR;
	QSemaphore s(1);
	s.acquire();

	bool ret;

	QList<QVariant> args;
	args.append(sessionfile);
	args.append(QVariant((qulonglong)&ret));

	emit sig_asyncProc(&s, "PauseBackgroundSession", args);

	s.acquire();

	return ret;
}

void CLC7GUILinkage::ShowStartupDialog(bool show)
{TR;
	QSemaphore s(1);
	s.acquire();

	QList<QVariant> args;
	args.prepend(QVariant(show));

	emit sig_asyncProc(&s, "ShowStartupDialog", args);

	s.acquire();
}



QWidget *CLC7GUILinkage::CreatePresetWidget(QWidget *page, QWidget *configwidget, QString preset_group)
{TR;
	return m_pMainWnd->CreatePresetWidget(page, configwidget, preset_group);
}


QMainWindow *CLC7GUILinkage::GetMainWindow()
{TR;
	return m_pMainWnd;
}

ILC7ProgressBox *CLC7GUILinkage::CreateProgressBox(QString title, QString status, UINT32 progresscur, UINT32 progressmax, bool can_cancel)
{TR;
	return new CLC7ProgressBox(title, status, progresscur, progressmax, can_cancel);
}

ILC7ColorManager *CLC7GUILinkage::GetColorManager()
{TR;
	return m_pMainWnd->GetColorManager();
}
#ifndef __INC_LC7APP_H
#define __INC_LC7APP_H

#include"LC7Main.h"
#include<QApplication>
#include"../../lc7core/include/ILC7Controller.h"
#include"CLC7GUILinkage.h"
#include"CLC7ColorManager.h"
#include"qtsingleapplication.h"
#include"CSystemOptions.h"
#include"CSystemPage.h"
#include"Attach.h"

#ifdef _WIN32

class JobObjectManager
{
private:
	HANDLE m_jobobject;
public:
	JobObjectManager();
	~JobObjectManager();
	void AddProcessToJob(HANDLE hProcess);
	void ReleaseChildProcesses();
};
extern JobObjectManager g_jom;

#endif

class CLC7App: public QtSingleApplication
{

private:
	bool m_debugEnabled;
	LC7Main *m_pMainWnd;
	CLC7GUILinkage *m_pGUILinkage;
	ILC7Controller *m_pCtrl;
	bool m_bControllerStarted;
	bool m_bControllerPluginsStarted;
	QLibrary m_CoreDLL;
	QString m_resume_override;
	
	CSystemPage *m_pSystemPage;
	CSystemOptions *m_pSystemColor;
	ILC7ActionCategory *m_pSystemCat;
	ILC7Action *m_pUISettingsAct;
	
	QCommandLineParser m_commandlineparser;

	Attach::Server m_attachserver;
	Attach::Client m_attachclient;

	static bool OnStartupCB(void *);
	bool OnStartup();

public:

	static CLC7App *getInstance();

	CLC7App(int argc, char *argv[]);
	virtual ~CLC7App();
	
	void ParseCommandLine(void);
	QCommandLineParser & GetCommandLineParser();

	QString GetResumeOverride(void);

	bool DoAttachManager(bool & normal_startup); 

	bool Initialize(void);
	int Execute(void);
	bool Terminate(void);

	void setDebugEnabled(bool enabled);
	bool debugEnabled();

	LC7Main *GetMainWindow(void);

	Attach::Server & GetAttachServer();
	Attach::Client & GetAttachClient();

	ILC7Controller *GetController();
};


#endif
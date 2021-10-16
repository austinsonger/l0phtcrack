#include<stdafx.h>
#include"LC7Main.h"

#ifdef _WIN32

JobObjectManager::JobObjectManager()
{
	// Create job object
	m_jobobject = CreateJobObject(NULL, NULL);
	if (m_jobobject != NULL)
	{
		// Configure all child processes associated with the job to terminate when the main process exits
		JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = { 0 };
		jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
		if (!SetInformationJobObject(m_jobobject, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli)))
		{
			CloseHandle(m_jobobject);
			m_jobobject = NULL;
		}

		AssignProcessToJobObject(m_jobobject, GetCurrentProcess());
	}
}

JobObjectManager::~JobObjectManager()
{
	if (m_jobobject)
	{
		CloseHandle(m_jobobject);
		m_jobobject = NULL;
	}
}

void JobObjectManager::AddProcessToJob(HANDLE hProcess)
{
	if (!m_jobobject)
		return;

	AssignProcessToJobObject(m_jobobject, hProcess);
}

void JobObjectManager::ReleaseChildProcesses()
{
	// Configure all child processes associated with the job to terminate when the main process exits
	JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = { 0 };
	jeli.BasicLimitInformation.LimitFlags = 0;
	if (!SetInformationJobObject(m_jobobject, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli)))
	{
		CloseHandle(m_jobobject);
		m_jobobject = NULL;
	}
}

JobObjectManager g_jom;

#endif




CLC7App *g_the_app=NULL;
extern QIcon readIcoFile(const QString & path);

void init_debug();
void enable_debug();
void disable_debug();

// Singleton
CLC7App *CLC7App::getInstance()
{TR;
	return g_the_app;
}

CLC7App::CLC7App(int argc, char *argv[]):QtSingleApplication(argc,argv)
{
	m_pMainWnd=NULL;
	m_pCtrl=NULL;
	m_bControllerStarted=false;
	m_pGUILinkage=NULL;
	
	m_pSystemPage=NULL;
	m_pSystemColor=NULL;
	m_pSystemCat=NULL;
	m_pUISettingsAct=NULL;

	m_debugEnabled = false;
	m_bControllerPluginsStarted = false;
	init_debug();

	g_the_app=this;
}

CLC7App::~CLC7App()
{// TR;

}


// Parse command line
void CLC7App::ParseCommandLine()
{TR;
	QStringList args = arguments();

	QCoreApplication::setOrganizationName("L0pht Holdings LLC");
	QCoreApplication::setOrganizationDomain("www.l0phtcrack.com");
	QCoreApplication::setApplicationName("L0phtCrack 7 (" PLATFORM_NAME ")");
	QCoreApplication::setApplicationVersion(VERSION_STRING);

	m_commandlineparser.setApplicationDescription("L0phtCrack 7");
	QCommandLineOption helpOption = m_commandlineparser.addHelpOption();
	QCommandLineOption versionOption = m_commandlineparser.addVersionOption();
	QCommandLineOption debugOption(QStringList() << "d" << "debug", "Turn on application debugging");
	m_commandlineparser.addOption(debugOption);
	QCommandLineOption taskOption(QStringList() << "task", "Start task <lc7task_path>", "lc7task_path");
	m_commandlineparser.addOption(taskOption);
	QCommandLineOption cleanupOption(QStringList() << "cleanup", "Remove installer at <installer_path>", "installer_path");
	m_commandlineparser.addOption(cleanupOption);
	QCommandLineOption resumeOption(QStringList() << "resume", "Resume session <lc7session_path>", "lc7session_path");
	m_commandlineparser.addOption(resumeOption);
	QCommandLineOption runOption(QStringList() << "r" << "run", "Run the queue after opening the session file");
	m_commandlineparser.addOption(runOption);
	m_commandlineparser.addPositionalArgument("session", "Session file to open");

	// Pass in arguments directly since without this, it seems to fail sometimes.
	m_commandlineparser.process(args);

	if (m_commandlineparser.isSet(helpOption)) 
	{
		m_commandlineparser.showHelp();
	}

	if (m_commandlineparser.isSet(versionOption))
	{
		m_commandlineparser.showHelp();
	}
#ifdef _DEBUG
	QStringList posargs = m_commandlineparser.positionalArguments();
	QString task = m_commandlineparser.value("task");
	bool debug = m_commandlineparser.isSet("debug");
	bool run = m_commandlineparser.isSet("run");
#endif
}

bool CLC7App::DoAttachManager(bool & normal_startup)
{TR;
	normal_startup = true;

	// If we are interactive, see if there's an attach server to 
	// attach to and ask if that's what the user wants to do.
	if (Attach::isInteractiveSession())
	{
		if (Attach::serverExists("LC7ATTACH"))
		{
			int res = QMessageBox::question(NULL, "Attach to running task?", "An LC7 background task is executing. Do you wish to attach to it?\n\n"
				"Pressing 'yes' will attach to the task running in the background. If you do this, you can not return it to the background when you're done and you must finish out the session.\n\n"
				"Pressing 'no' will create a new session. Creating a new session may operate slowly due to resources being consumed by the executing background task.", QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel);
			if (res == QMessageBox::Yes)
			{
				// Attach to interactive session
				
				/* XXX direct attach code
				if (!m_attachclient.connectToServer("LC7ATTACH"))
				{
					QMessageBox::critical(NULL, "Couldn't attach to running task", "Attaching to the running task has failed. You may not have permission to perform this action.");
					return false;
				}

				exec();

				normal_startup = false;
				*/

				/* Indirect attach code */
				QString resumefile;
				if (!m_attachclient.connectToServer("LC7ATTACH", resumefile))
				{
					QMessageBox::critical(NULL, "Couldn't attach to running task", "Attaching to the running task has failed. The task may not be interruptable at this time. Please try again later.");
					return false;
				}

				// This will override the command line to cause this to open and restart				
				m_resume_override = resumefile;
				
				return true;
			}
			else if (res == QMessageBox::Cancel)
			{
				return false;
			}
		}
	}
	else
	{
		// If we're not interactive then this window launches with an attachserver
		if (!m_attachserver.listen("LC7ATTACH"))
		{
			return false;
		}
	}
	
	return true;
}

static bool RemoveReadOnlyAttributeRecursive(const QString& directory)
{
	int failures = 0;
	QDirIterator it(directory, QDirIterator::NoIteratorFlags);
	while (it.hasNext())
	{
		QString fileName = it.next();
		if (fileName.endsWith("/.") || fileName.endsWith("/.."))
			continue;

		QFileInfo fileInfo(fileName);
		if (fileInfo.isDir())
		{
			if (!RemoveReadOnlyAttributeRecursive(fileName))
			{
				qDebug() << "ERROR: Failed to RemoveReadOnlyAttributeRecursive() " << fileName;
				failures++;
			}
		}
		else
		{
#ifdef Q_OS_WIN
			wchar_t* fileNamePtr = (wchar_t*)fileName.utf16();
			if (!SetFileAttributesW(fileNamePtr, GetFileAttributesW(fileNamePtr) & ~FILE_ATTRIBUTE_READONLY))
			{
				failures++;
			}
#endif
		}
	}

#ifdef Q_OS_WIN
	wchar_t* directoryPtr = (wchar_t*)directory.utf16();
	if (!SetFileAttributesW(directoryPtr, GetFileAttributesW(directoryPtr) & ~FILE_ATTRIBUTE_READONLY))
	{
		failures++;
	}
#endif

	return failures == 0 ? true : false;
}


static void RegisterFileAssociations(void)
{
#ifdef _WIN32
	HKEY hkey;

	std::wstring extension = L".lc7";
	std::wstring desc = L"LC7 Session";
	std::wstring app = L"\""+ QDir::toNativeSeparators(QCoreApplication::applicationFilePath()).toStdWString()+L"\" \"%1\"";
	std::wstring path = extension + L"\\shell\\open\\command\\";

	if (RegCreateKeyExW(HKEY_CLASSES_ROOT, extension.c_str(), 0, 0, 0, KEY_ALL_ACCESS, 0, &hkey, 0) != ERROR_SUCCESS)
	{
		return;
	}
	RegSetValueExW(hkey, L"", 0, REG_SZ, (BYTE*)desc.c_str(), sizeof(desc));
	RegCloseKey(hkey);
	
	if (RegCreateKeyExW(HKEY_CLASSES_ROOT, path.c_str(), 0, 0, 0, KEY_ALL_ACCESS, 0, &hkey, 0) != ERROR_SUCCESS)
	{
		return;
	}
	RegSetValueExW(hkey, L"", 0, REG_SZ, (BYTE*)app.c_str(),(DWORD) app.length()*sizeof(wchar_t));

	RegCloseKey(hkey);
#endif
}


bool CLC7App::Initialize()
{
	TR;

	QProcessEnvironment::systemEnvironment().insert("CUDA_CACHE_DISABLE", "1");

	// Register file associations
	RegisterFileAssociations();

	// Ensure temporary folder is gone
	QDir tempdir = QDir::temp();
	if (QFileInfo(tempdir.filePath("LC7_temp")).isDir())
	{
		if (tempdir.cd("LC7_temp"))
		{
			RemoveReadOnlyAttributeRecursive(tempdir.absolutePath());
			if(!tempdir.removeRecursively())
			{
				QMessageBox::critical(NULL, "Temporary directory is locked", "Can not start L0phtCrack, the temporary directory %TEMP%\\LC7_temp is locked. Ensure no LC7 process is running, and/or reboot to fix this issue.");
				return false;
			}
			tempdir.cdUp();
		}
	}

	// Raise main thread priority
	QThread::currentThread()->setPriority(QThread::Priority::HighPriority);

	// Use System Proxy
	QNetworkProxyFactory::setUseSystemConfiguration(true);

	// Change working directory to program directory
	QDir::setCurrent(QCoreApplication::applicationDirPath());

	setAttribute(Qt::AA_UseHighDpiPixmaps);
	setEffectEnabled(Qt::UI_AnimateMenu);
	setEffectEnabled(Qt::UI_FadeMenu);
	setEffectEnabled(Qt::UI_AnimateCombo);
	setEffectEnabled(Qt::UI_AnimateTooltip);
	setEffectEnabled(Qt::UI_FadeTooltip);
	setEffectEnabled(Qt::UI_AnimateToolBox);

	QIcon ico = readIcoFile(":/lc7/lc7.ico");
	setWindowIcon(ico);

	if (m_commandlineparser.isSet("debug") || QSettings().value("_debug_:debug_logging").toBool())
	{
		setDebugEnabled(true);
		enable_debug();
	}

	// Load core library
	m_CoreDLL.setFileName("lc7core");
	if (!m_CoreDLL.load())
	{
		return false;
	}

	// Create controller object
	TYPEOF_CreateLC7Controller *pCreateLC7Controller = (TYPEOF_CreateLC7Controller *)m_CoreDLL.resolve("CreateLC7Controller");
	if (!pCreateLC7Controller)
	{
		return false;
	}
	m_pCtrl = (*pCreateLC7Controller)();

	// Create main window
	m_pMainWnd = new LC7Main(m_pCtrl);

	setActivationWindow(m_pMainWnd);
	connect(this, SIGNAL(messageReceived(const QString &)), m_pMainWnd, SLOT(slot_singleApplicationMessageReceived(const QString &)));

	// Create ui-level linkage
	m_pGUILinkage = new CLC7GUILinkage(m_pMainWnd);
	m_pCtrl->SetGUILinkage(m_pGUILinkage);

	// Pass GUI Linkage to attachserver
	if (m_attachserver.isListening())
	{
		m_attachserver.setGUILinkage(m_pGUILinkage);
	}

	// Start up controller
	QStringList client_startup_modes;
	client_startup_modes.append("gui");
	if (!m_pCtrl->Startup(client_startup_modes))
	{
		return false;
	}
	m_bControllerStarted = true;

	// Decorate
	m_pMainWnd->Startup();

	// Plug in system page
	m_pSystemPage = new CSystemPage(0, m_pCtrl->GetLinkage(), m_pCtrl);
	m_pSystemColor = new CSystemOptions(m_pCtrl->GetLinkage());
	if (!m_pCtrl->AddComponent(m_pSystemColor))
	{
		return false;
	}
	if (!m_pGUILinkage->AddMainMenuTab("System/Settings", "system/settings", m_pSystemPage))
	{
		return false;
	}

	m_pSystemCat = m_pCtrl->CreateActionCategory("system", "System", "Global system options");
	m_pUISettingsAct = m_pSystemCat->CreateAction(m_pSystemColor->GetID(), "get_options", QStringList(),
		"UI Settings",
		"User interface settings");

	// Start up plugins
	if (!m_pCtrl->StartupPlugins())
	{
		return false;
	}
	m_bControllerPluginsStarted = true;

	// Flush default settings to registry
	m_pSystemPage->RefreshContent();
	
	// Select topmost tab
	m_pMainWnd->SwitchToFirstTab();

	// Show main window
	if ((m_commandlineparser.isSet("task") || (m_commandlineparser.isSet("run") && m_commandlineparser.positionalArguments().size()==1)) && Attach::isInteractiveSession())
	{
		// If this is a task, start in system tray instead
		m_pMainWnd->DoMinimizeToSystemTray();
		m_pMainWnd->DoSystemTrayMessage("An LC7 task is starting");
	}
	else
	{
		m_pMainWnd->showMaximized();
	}
	
	// Process command line
	QTimer::singleShot(0, m_pMainWnd, SLOT(slot_processCommandLine()));

	return true;
}

int CLC7App::Execute(void)
{TR;
	return exec();
}

bool CLC7App::Terminate(void)
{
	TR;

	if (m_pMainWnd)
	{
		m_pMainWnd->Shutdown();
	}


	if (m_bControllerPluginsStarted)
	{
		m_pCtrl->ShutdownPlugins();
	}

	if(m_pSystemCat)
	{
		if (m_pUISettingsAct)
		{
			m_pSystemCat->RemoveAction(m_pUISettingsAct);
			m_pUISettingsAct = NULL;
		}
		
		m_pCtrl->RemoveActionCategory(m_pSystemCat);
		m_pSystemCat=NULL;
	}

	if(m_pSystemColor)
	{
		m_pCtrl->RemoveComponent(m_pSystemColor);
		delete m_pSystemColor;
		m_pSystemColor=NULL;
	}

	if(m_pSystemPage)
	{
		m_pGUILinkage->RemoveMainMenuTab(m_pSystemPage);
		delete m_pSystemPage;
		m_pSystemPage=NULL;
	}
	
	// Shut down and remove controller
	if(m_bControllerStarted)
	{
		m_pCtrl->Shutdown();
	}

	// Exit main window
	if(m_pMainWnd)
	{
		delete m_pMainWnd;
		m_pMainWnd=NULL;
	}

	if(m_bControllerStarted)
	{
		delete m_pCtrl;
		m_pCtrl=NULL;
	}

	if(m_CoreDLL.isLoaded())
	{
		m_CoreDLL.unload();
	}

	// Remove ui-level linkage
	if(m_pGUILinkage)
	{
		delete m_pGUILinkage;
		m_pGUILinkage=NULL;
	}
	
	disable_debug();
	return true;
}

void CLC7App::setDebugEnabled(bool enabled)
{
	m_debugEnabled = enabled;
}

bool CLC7App::debugEnabled()
{
	return m_debugEnabled;
}

LC7Main *CLC7App::GetMainWindow(void)
{TR;
	return m_pMainWnd;
}

ILC7Controller *CLC7App::GetController()
{TR;
	return m_pCtrl;
}

QCommandLineParser & CLC7App::GetCommandLineParser()
{TR;
	return m_commandlineparser;
}

QString CLC7App::GetResumeOverride(void)
{TR;
	return m_resume_override;
}


Attach::Server & CLC7App::GetAttachServer()
{TR;
	return m_attachserver;
}

Attach::Client & CLC7App::GetAttachClient()
{TR;
	return m_attachclient;
}

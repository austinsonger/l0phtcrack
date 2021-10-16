#include <stdafx.h>
#include<io.h>
#include<sys/stat.h>
#include"keychain.h"
using namespace QKeychain;


#if defined(_WIN32) || defined(__CYGWIN__)
#ifdef __GNUC__
#define DECLSPEC_EXPORT __attribute__((dllexport))
#define DECLSPEC_IMPORT __attribute__((dllimport))
#else
#define DECLSPEC_EXPORT __declspec(dllexport)
#define DECLSPEC_IMPORT __declspec(dllimport)
#endif
#else
#if defined(__GNUC__) && __GNUC__ >= 4
#define DECLSPEC_EXPORT __attribute__ ((visibility("default")))
#define DECLSPEC_IMPORT
#else
#define DECLSPEC_EXPORT
#define DECLSPEC_IMPORT
#endif
#endif


extern "C"
{

	DECLSPEC_EXPORT ILC7Controller *CreateLC7Controller()
	{
		TR;
		return new CLC7Controller();
	}

}


CLC7Controller::CLC7Controller()
{
	m_settings = nullptr;
	m_pSecureStringSerializer = nullptr;
	m_cpuinformation = new CLC7CPUInformation();
	m_pGUILinkage = nullptr;
	m_single_workqueuefactory = nullptr;
	m_batch_workqueuefactory = nullptr;
	m_pLinkage = nullptr;
	m_session_opened = false;
	m_session_modified = false;

	m_pCoreSettings = nullptr;
	m_pSystemCat = nullptr;
	m_pCoreSettingsAct = nullptr;

	m_task_running = false;
	m_task_quit_on_finish = false;
	m_current_task = nullptr;

	m_single_workqueue = nullptr;
	m_batch_workqueue = nullptr;
	m_task_scheduler = nullptr;
	m_historical_data = nullptr;
	m_preset_manager = nullptr;
	m_plugin_registry = nullptr;
	m_thermal_watchdog = nullptr;
	m_system_monitor = nullptr;

	connect(&m_autosave_timer, &QTimer::timeout, this, &CLC7Controller::slot_autosaveTimer);
}

CLC7Controller::~CLC7Controller()
{
	TR;

	// Something should have closed the session before controller went away!
	Q_ASSERT(m_session_opened == false);
	Q_ASSERT(m_pCoreSettings == nullptr);
	Q_ASSERT(m_pSystemCat == nullptr);
	Q_ASSERT(m_pCoreSettingsAct == nullptr);
	Q_ASSERT(m_pSecureStringSerializer == nullptr);
	Q_ASSERT(m_task_scheduler == nullptr);
	Q_ASSERT(m_historical_data == nullptr);
	Q_ASSERT(m_preset_manager == nullptr);
	Q_ASSERT(m_plugin_registry == nullptr);
	Q_ASSERT(m_thermal_watchdog == nullptr);
	Q_ASSERT(m_system_monitor == nullptr);

	delete m_cpuinformation;
}

ILC7Interface *CLC7Controller::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Controller")
	{
		return this;
	}
	return NULL;
}

QStringList CLC7Controller::GetActiveClientModes()
{
	TR;
	return m_active_client_modes;
}


void CLC7Controller::CallSessionActivityNotifiers(ILC7Linkage::SESSION_ACTIVITY activity)
{
	TR;
	// Call null handlers (just notification, no object)
	QList<ActivityNotifier> notifiers = m_session_activity_notifiers.values(QUuid());
	foreach(ActivityNotifier notifier, notifiers)
	{
		(notifier.callback_object->*notifier.callback_function)(activity, NULL);
	}

	// Call session activity handlers (notify with session handler object)
	foreach(ILC7SessionHandler *handler, m_sessionhandlers)
	{
		QList<ActivityNotifier> notifiers = m_session_activity_notifiers.values(handler->GetId());
		foreach(ActivityNotifier notifier, notifiers)
		{
			(notifier.callback_object->*notifier.callback_function)(activity, handler);
		}
	}
}

void CLC7Controller::UpdateRecentSessionsList(QString sessionfile)
{
	TR;
	ILC7Settings *settings = GetSettings();
	QStringList recentsessionfiles = settings->value("_core_:recentsessionfiles", QStringList()).toStringList();
	recentsessionfiles.removeAll(sessionfile);
	recentsessionfiles.prepend(sessionfile);

	// Trim to 10 recent session files
	if (recentsessionfiles.size() == 11)
	{
		recentsessionfiles.removeLast();
	}

	settings->setValue("_core_:recentsessionfiles", recentsessionfiles);
}

QMap<QString, QVariant> *CLC7Controller::GetSessionState(void)
{
	TR;
	return &m_session_state;
}

bool CLC7Controller::IsAnyQueueRunning(void)
{
	TR;
	if (m_single_workqueue && m_single_workqueue->GetWorkQueueState() == ILC7WorkQueue::IN_PROGRESS)
	{
		return true;
	}
	if (m_batch_workqueue && m_batch_workqueue->GetWorkQueueState() == ILC7WorkQueue::IN_PROGRESS)
	{
		return true;
	}

	return false;
}

void CLC7Controller::NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler)
{
	TR;
	switch (activity)
	{
	case ILC7Linkage::SESSION_OPEN_POST:
	case ILC7Linkage::SESSION_NEW_POST:
		if (handler && handler->GetId() == BATCH_WORKQUEUE_HANDLER_ID)
		{
			m_batch_workqueue = (ILC7WorkQueue *)handler;
			m_batch_workqueue->AddQueueStateChangedListener(this, (void(QObject::*)()) &CLC7Controller::slot_batchWorkQueueStateChanged);
		}
		if (handler && handler->GetId() == SINGLE_WORKQUEUE_HANDLER_ID)
		{
			m_single_workqueue = (ILC7WorkQueue *)handler;
		}

		if (!handler)
		{
			// Start autosave timer
			StartAutosaveTimer();
		}

		break;
	case ILC7Linkage::SESSION_CLOSE_PRE:

		if (!handler)
		{
			// Stop autosave timer
			StopAutosaveTimer();
		}

		if (handler && handler->GetId() == BATCH_WORKQUEUE_HANDLER_ID)
		{
			m_batch_workqueue = nullptr;
		}
		if (handler && handler->GetId() == SINGLE_WORKQUEUE_HANDLER_ID)
		{
			m_single_workqueue = nullptr;
		}
		break;
	}
}



bool CLC7Controller::NewSession(void)
{
	TR;
	if (IsAnyQueueRunning())
	{
		return false;
	}
	if (m_session_opened)
	{
		return false;
	}

	// New
	m_session_state.clear();
	m_session_data.clear();
	m_session_partials.clear();

	foreach(QUuid handler_id, m_sessionhandlerfactory_by_id.keys())
	{
		ILC7SessionHandlerFactory *factory = m_sessionhandlerfactory_by_id[handler_id];
		ILC7SessionHandler *handler = factory->CreateSessionHandler(handler_id);

		m_sessionhandler_by_id[handler_id] = handler;
		m_sessionhandlers.append(handler);
	}

	// Post-new callbacks
	CallSessionActivityNotifiers(ILC7Linkage::SESSION_NEW_POST);

	m_session_path = "";
	m_session_opened = true;
	m_session_modified = false;

	m_pGUILinkage->GetWorkQueueWidget()->Reset();
	m_pGUILinkage->UpdateUI();

	return true;
}

bool CLC7Controller::CloseSession(void)
{
	TR;
	if (IsAnyQueueRunning())
	{
		return false;
	}

	if (!m_session_opened)
	{
		return false;
	}

	// Pre-close callbacks		
	CallSessionActivityNotifiers(ILC7Linkage::SESSION_CLOSE_PRE);

	// Close
	foreach(ILC7SessionHandler *handler, m_sessionhandlers)
	{
		QUuid handler_id = handler->GetId();
		ILC7SessionHandlerFactory *factory = m_sessionhandlerfactory_by_id[handler_id];
		factory->DestroySessionHandler(handler);
	}

	m_sessionhandler_by_id.clear();
	m_sessionhandlers.clear();
	m_session_path = "";
	m_session_data.clear();
	m_session_state.clear();
	m_session_partials.clear();
	m_session_opened = false;
	m_session_modified = false;

	m_pGUILinkage->GetWorkQueueWidget()->Reset();
	m_pGUILinkage->UpdateUI();

	return true;
}

bool CLC7Controller::IsSessionOpen(QString * sessionfile)
{
	if (!m_session_opened)
	{
		if (sessionfile)
		{
			*sessionfile = "";
		}
		return false;
	}

	if (sessionfile)
	{
		*sessionfile = m_session_path;
	}
	return true;
}

bool CLC7Controller::OpenSession(QString sessionfile)
{
	TR;
	if (IsAnyQueueRunning())
	{
		return false;
	}

	if (m_session_opened)
	{
		return false;
	}

	// Open session file
	QFile sf(sessionfile);
	if (!sf.open(QIODevice::ReadOnly))
	{
		return false;
	}
	QDataStream ds(&sf);

	if (!LoadSessionFromStream(ds))
	{
		return false;
	}

	// Post-open callbacks
	CallSessionActivityNotifiers(ILC7Linkage::SESSION_OPEN_POST);

	m_session_path = sessionfile;
	m_session_opened = true;
	m_session_modified = false;

	// Update recent files list
	UpdateRecentSessionsList(sessionfile);
	m_pGUILinkage->UpdateUI();

	// See if queue was running, if this was an autosave
	if (m_batch_workqueue->IsCheckpointed())
	{
		m_batch_workqueue->ResumeFromCheckpoint();
		m_batch_workqueue->StartRequest();
	}
	else if (m_single_workqueue->IsCheckpointed())
	{
		m_single_workqueue->ResumeFromCheckpoint();
		m_single_workqueue->StartRequest();
	}

	return true;
}

bool CLC7Controller::NewSessionFromData(QByteArray sessiondata)
{
	TR;
	if (!GetGUILinkage()->RequestCloseSession(false, false))
	{
		return false;
	}

	if (IsAnyQueueRunning())
	{
		return false;
	}

	if (m_session_opened)
	{
		return false;
	}

	QDataStream ds(&sessiondata, QIODevice::ReadOnly);

	if (!LoadSessionFromStream(ds))
	{
		return false;
	}

	// Post-open callbacks
	CallSessionActivityNotifiers(ILC7Linkage::SESSION_OPEN_POST);

	m_session_path = "";
	m_session_opened = true;
	m_session_modified = false;

	// Update recent files list
	m_pGUILinkage->UpdateUI();

	return true;
}


bool CLC7Controller::LoadSessionFromStream(QDataStream & ds)
{
	TR;
	m_pSecureStringSerializer->ClearCache();

	quint32 version;
	ds >> version;
	if (version != 2)
	{
		return false;
	}

	// Load session data
	ds >> m_session_data;

	// Load session state
	ds >> m_session_state;

	// Load widget state
	if (!m_pGUILinkage->GetWorkQueueWidget()->Load(ds))
	{
		return false;
	}

	// Get list of session handler uuids saved into session in order
	QList<QVariant> serialized_session_handler_ids = m_session_data["session_handler_ids"].toList();
	QList<QVariant> serialized_session_handler_bytearrays = m_session_data["session_handler_bytearrays"].toList();

	// Open
	QList<QVariant>::iterator iter_id = serialized_session_handler_ids.begin();
	QList<QVariant>::iterator iter_ba = serialized_session_handler_bytearrays.begin();
	for (; iter_id != serialized_session_handler_ids.end() && iter_ba != serialized_session_handler_bytearrays.end(); iter_id++, iter_ba++)
	{
		QUuid handler_id = iter_id->toUuid();
		QByteArray handler_data = iter_ba->toByteArray();

		// See if we have a matching handler factory
		ILC7SessionHandlerFactory *factory = m_sessionhandlerfactory_by_id[handler_id];
		bool partial = true;
		if (factory)
		{
			// Serialize the handler
			ILC7SessionHandler *handler = factory->CreateSessionHandler(handler_id);
			QDataStream ds(handler_data);
			if (handler->Load(ds))
			{
				partial = false;

				m_sessionhandler_by_id[handler_id] = handler;
				m_sessionhandlers.append(handler);
			}
		}
		if (partial)
		{
			// Save as a partial
			m_session_partials[handler_id] = handler_data;
		}
	}

	// Create any new handlers that aren't in the session
	foreach(QUuid handler_id, m_sessionhandlerfactory_by_id.keys())
	{
		if (!m_sessionhandler_by_id.contains(handler_id))
		{
			ILC7SessionHandlerFactory *factory = m_sessionhandlerfactory_by_id[handler_id];
			ILC7SessionHandler *handler = factory->CreateSessionHandler(handler_id);

			m_sessionhandler_by_id[handler_id] = handler;
			m_sessionhandlers.append(handler);
		}
	}

	if (ds.status() != QDataStream::Ok)
	{
		return false;
	}

	if (m_pSecureStringSerializer->DidCancel())
	{
		GetGUILinkage()->WarningMessage("No Password Specified", "This session was loaded without entering the appropriate credentials. Remediation and importing may not work as desired.");
	}

	return true;
}

bool CLC7Controller::SaveSession(QString sessionfile, bool as_copy, bool allow_insecure, bool allow_running)
{
	TR;
	CLC7SecureStringSerializerOptionWrapper sssow(m_pSecureStringSerializer, allow_insecure);

	if (as_copy && sessionfile == m_session_path)
	{
		return false;
	}

	if (!allow_running && IsAnyQueueRunning())
	{
		return false;
	}

	if (!m_session_opened)
	{
		return false;
	}

	// Open session file
	QFile sf(sessionfile);
	if (!sf.open(QIODevice::WriteOnly))
	{
		return false;
	}
	sf.resize(0);

	QDataStream ds(&sf);

	quint32 version = 2;
	ds << version;

	// Pre-open callbacks		
	CallSessionActivityNotifiers(ILC7Linkage::SESSION_SAVE_PRE);

	// Save
	QList<QVariant> serialized_session_handler_ids;
	QList<QVariant> serialized_session_handler_bytearrays;
	foreach(ILC7SessionHandler *handler, m_sessionhandlers)
	{
		QUuid handler_id = handler->GetId();
		QByteArray handler_data;
		QDataStream ds(&handler_data, QIODevice::WriteOnly);
		if (!handler->Save(ds))
		{
			return false;
		}
		serialized_session_handler_ids.append(handler_id);
		serialized_session_handler_bytearrays.append(handler_data);
	}

	// Add session partials
	foreach(QUuid handler_id, m_session_partials.keys())
	{
		serialized_session_handler_ids.append(handler_id);
		serialized_session_handler_bytearrays.append(m_session_partials[handler_id]);
	}

	m_session_data["session_handler_ids"] = serialized_session_handler_ids;
	m_session_data["session_handler_bytearrays"] = serialized_session_handler_bytearrays;

	// Save session data
	ds << m_session_data;

	// Save session state
	ds << m_session_state;

	// Save widget state
	if (!m_pGUILinkage->GetWorkQueueWidget()->Save(ds))
	{
		return false;
	}

	if (!as_copy)
	{
		m_session_modified = false;
		m_session_path = sessionfile;

		// Update recent files list
		UpdateRecentSessionsList(sessionfile);
	}

	// Post-save callbacks
	CallSessionActivityNotifiers(ILC7Linkage::SESSION_SAVE_POST);

	m_pGUILinkage->UpdateUI();

	return true;
}

bool CLC7Controller::RunTask(QString taskid, bool quit_on_finish)
{
	TR;
	if (!GetGUILinkage()->RequestCloseSession(false, false))
	{
		return false;
	}

	if (m_task_running || IsAnyQueueRunning() || IsSessionOpen())
	{
		return false;
	}

	ILC7Task *itask = m_task_scheduler->FindScheduledTaskById(taskid);
	if (!itask)
	{
		return false;
	}

	CLC7Task *task = (CLC7Task *)itask;

	QByteArray sessiondata = task->GetSessionData();

	if (!NewSessionFromData(sessiondata))
	{
		return false;
	}

	m_task_quit_on_finish = quit_on_finish;
	m_task_running = true;
	m_current_task = task;

	if (!m_batch_workqueue->StartRequest())
	{
		m_task_running = false;
		m_current_task = nullptr;
		return false;
	}

	return true;
}

bool CLC7Controller::IsTaskRunning(void)
{
	TR;
	return m_task_running;
}


bool CLC7Controller::RunManualTask(QString sessionfile, bool quit_on_finish)
{
	TR;
	if (!GetGUILinkage()->RequestCloseSession(false, false))
	{
		return false;
	}

	if (m_task_running || IsAnyQueueRunning() || IsSessionOpen())
	{
		return false;
	}
	
	if (!OpenSession(sessionfile))
	{
		return false;
	}

	m_task_quit_on_finish = quit_on_finish;
	m_task_running = true;
	m_current_task = nullptr;

	if (m_batch_workqueue->GetWorkQueueState() != ILC7WorkQueue::STATE::VALIDATED)
	{
		m_pGUILinkage->ErrorMessage("Queue Not Validated", QString("Queue is not validated. Session file must have validated queue before saving if you want to use '--run'."));

		CloseSession();

		m_task_running = false;
		return false;
	}

	if (!m_batch_workqueue->StartRequest())
	{
		m_task_running = false;

		CloseSession();

		return false;
	}

	return true;
}


void CLC7Controller::slot_doFinishTask(void)
{
	TR;
	if (!m_task_running)
	{
		Q_ASSERT(0);
		return;
	}
	if (!m_current_task || m_current_task->GetSaveTaskOutput())
	{
		// Get task save path
		QString taskoutdir = m_settings->value("_core_:task_output_directory").toString();
		QString taskoutname;
		if (m_current_task)
		{
			taskoutname = m_current_task->GetName();
		}
		else
		{
			taskoutname = QFileInfo(m_session_path).baseName();
		}

		taskoutname = taskoutname.remove("\\").remove("//").remove("*").remove("?").remove("..");
		QString filename = QDateTime::currentDateTime().toString("yyyyMMddhhmmss") + "_" + taskoutname + ".lc7";

		QDir d(taskoutdir);

		if (!d.mkpath("."))
		{
			m_pGUILinkage->ErrorMessage("Saving Task Output Error", QString("Unable to create directory: %1").arg(taskoutdir));
			return;
		}


		QString taskoutpath = d.absoluteFilePath(filename);

		if (!SaveSession(taskoutpath))
		{
			m_pGUILinkage->ErrorMessage("Saving Task Output Error", QString("Unable to save session output to: %1").arg(taskoutpath));
		}
	}

	if (m_current_task)
	{
		m_task_scheduler->CallTaskFinishedCallbacks(m_current_task);
	}

	bool quit = m_task_quit_on_finish;

	m_task_quit_on_finish = false;
	m_task_running = false;
	m_current_task = nullptr;

	if (quit)
	{
		CloseSession();

		m_pGUILinkage->Exit(true);
	}
}

void CLC7Controller::slot_batchWorkQueueStateChanged(void)
{
	TR;
	if (m_task_running)
	{
		if (m_batch_workqueue && (m_batch_workqueue->GetWorkQueueState() == ILC7WorkQueue::COMPLETE || m_batch_workqueue->GetWorkQueueState() == ILC7WorkQueue::FAIL))
		{
			QTimer::singleShot(0, this, &CLC7Controller::slot_doFinishTask);
		}
		else if (!m_batch_workqueue || m_batch_workqueue->GetWorkQueueState() != ILC7WorkQueue::IN_PROGRESS)
		{
			// Task is no longer running as a task.
			m_task_running = false;
			m_current_task = nullptr;
			m_task_quit_on_finish = false;
		}
	}
}

QMap<QUuid, QByteArray> CLC7Controller::GetSessionPartials()
{
	return m_session_partials;
}

bool CLC7Controller::IsSessionModified(void)
{
	return m_session_modified;
}

void CLC7Controller::ReportSessionModified(void)
{
	m_session_modified = true;
	m_pGUILinkage->UpdateUI();
}

void CLC7Controller::RegisterSessionHandlerFactory(QUuid handler_id, ILC7SessionHandlerFactory *factory)
{
	TR;
	m_sessionhandlerfactory_by_id[handler_id] = factory;
}

void CLC7Controller::UnregisterSessionHandlerFactory(QUuid handler_id)
{
	TR;
	m_sessionhandlerfactory_by_id.remove(handler_id);
}

ILC7SessionHandler *CLC7Controller::GetSessionHandler(QUuid handler_id)
{
	TR;
	return m_sessionhandler_by_id[handler_id];
}

void CLC7Controller::RegisterNotifySessionActivity(QUuid handler_id, QObject *callback_object, void (QObject::*callback_function)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))
{
	TR;
	ActivityNotifier an;
	an.callback_object = callback_object;
	an.callback_function = callback_function;
	m_session_activity_notifiers.insert(handler_id, an);
}

void CLC7Controller::UnregisterNotifySessionActivity(QUuid handler_id, QObject *callback_object, void (QObject::*callback_function)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))
{
	TR;
	ActivityNotifier an;
	an.callback_object = callback_object;
	an.callback_function = callback_function;
	m_session_activity_notifiers.remove(handler_id, an);
}

void CLC7Controller::SetGUILinkage(ILC7GUILinkage *guilinkage)
{
	TR;
	m_pGUILinkage = guilinkage;

	m_single_workqueuefactory = new CLC7WorkQueueFactory(this, false);
	m_batch_workqueuefactory = new CLC7WorkQueueFactory(this, true);

	RegisterSessionHandlerFactory(SINGLE_WORKQUEUE_HANDLER_ID, m_single_workqueuefactory);
	RegisterSessionHandlerFactory(BATCH_WORKQUEUE_HANDLER_ID, m_batch_workqueuefactory);
}
	
static bool RemoveReadOnlyAttribute(const QString & path)
{
#ifdef Q_OS_WIN
	wchar_t* pathptr = (wchar_t*)path.utf16();
	if (!SetFileAttributesW(pathptr, GetFileAttributesW(pathptr) & ~FILE_ATTRIBUTE_READONLY))
	{
		return false;
	}
#endif
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
			if(!RemoveReadOnlyAttribute(fileName))
			{
				failures++;
			}
		}
	}

	if (!RemoveReadOnlyAttribute(directory))
	{
		failures++;
	}

	return failures == 0 ? true : false;
}



bool CLC7Controller::Startup(QStringList active_client_modes)
{
#ifdef _WIN32
	// Create global mutexes
	m_compile_mutex = CreateMutex(NULL, FALSE, "LC7CompileMutex");
#endif



	int oldmode;
	_umask_s(0, &oldmode);

	m_active_client_modes = active_client_modes;

	// Set up temporary area
	QDir tempdir = QDir::temp();
	if (QFileInfo(tempdir.filePath("LC7_temp")).isDir())
	{
		return false;
	}
	tempdir.mkdir("LC7_temp");
	RemoveReadOnlyAttributeRecursive(tempdir.absolutePath());
	m_strTempRoot = tempdir.filePath("LC7_temp");

	// Set library paths	
	QString cwd = QCoreApplication::applicationDirPath();
	QStringList libpaths;
	libpaths += cwd;
	QDir dir(cwd);
	dir.cd("lcplugins");
	libpaths += dir.absolutePath();
	QCoreApplication::setLibraryPaths(libpaths);

	// Create all objects
	m_settings = new CLC7Settings();
	m_plugin_registry = new CLC7PluginRegistry(this);
	m_pLinkage = new CLC7Linkage(this);
	m_thermal_watchdog = new CLC7ThermalWatchdog(this);
	m_system_monitor = new CLC7SystemMonitor(this);
	m_task_scheduler = new CLC7TaskScheduler(this);
	m_historical_data = new CLC7HistoricalData(this);
	m_preset_manager = new CLC7PresetManager(this);
	m_pSecureStringSerializer = new CLC7SecureStringSerializer(this);
	m_pCoreSettings = new CLC7CoreSettings(m_pLinkage);

	// Initialize all objects
	if (!AddComponent(m_pCoreSettings))
	{
		return false;
	}

	m_pSystemCat = CreateActionCategory("system", "System", "Global system options");
	m_pCoreSettingsAct = m_pSystemCat->CreateAction(m_pCoreSettings->GetID(), "get_options", QStringList(),
		"LC7 Core Settings",
		"LC7 Core system settings");

	QString error;
	if (!m_task_scheduler->Initialize(error))
	{
		m_pGUILinkage->WarningMessage("Task Scheduler Unavailable",
			QString("The task scheduler function could not be initialized. You may not have permission to access the Task Scheduler, or it is disabled administratively. This functionality will be disabled."));
	}

	if (!m_historical_data->Initialize(error))
	{
		m_pGUILinkage->WarningMessage("Failed to initialize", QString("The historical data function could not be initialized.\n\nYou may need to reinstall L0phtCrack due to a corrupted installation.\n\n%1").arg(error));
	}

	bool presets_ok = true;
	if (!m_preset_manager->Initialize(error, presets_ok))
	{
		m_pGUILinkage->ErrorMessage("Failed to initialize", QString("The preset manager function could not be initialized.\n\nYou may need to reinstall L0phtCrack due to a corrupted installation.\n\n%1").arg(error));
		return false;
	}
	if (!presets_ok)
	{
		m_pGUILinkage->WarningMessage("Failed to load presets", QString("Presets could not be loaded. Presets will be reset to defaults."));
	}

	RegisterNotifySessionActivity(QUuid(), this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CLC7Controller::NotifySessionActivity);
	RegisterNotifySessionActivity(SINGLE_WORKQUEUE_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CLC7Controller::NotifySessionActivity);
	RegisterNotifySessionActivity(BATCH_WORKQUEUE_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CLC7Controller::NotifySessionActivity);

	m_thermal_watchdog->start();

	return true;
}


bool CLC7Controller::StartupPlugins(void)
{
	TR;
	QString error;
	QList<ILC7PluginLibrary *> failed_libraries;
	if (!m_plugin_registry->LoadPluginLibraries(failed_libraries, error))
	{
		m_pGUILinkage->ErrorMessage("Failed to initialize", QString("Error reading plugin libraries, system can not start. You may need to reinstall L0phtCrack due to a corrupted installation. %1").arg(error));
		return false;
	}

	if (failed_libraries.size() > 0)
	{
		QString message;
		foreach(ILC7PluginLibrary *lib, failed_libraries)
		{
			if (lib->IsSystemLibrary())
			{
				message.append(QString("%1: %2\n").arg(lib->GetDisplayName()).arg(lib->GetFailureReason()));
				m_pGUILinkage->ErrorMessage("System can not start up",
					QString("A critical system plugin failed to start:\n\n%1\nYour system may be incompatible with L0phtCrack 7 or your installation is corrupt. Reinstall L0phtCrack 7 and if that doesn't work, contact support@l0phtcrack.com.").arg(message));
				return false;
			}
			else
			{
				message.append(QString("%1: %2\n").arg(lib->GetDisplayName()).arg(lib->GetFailureReason()));
				m_plugin_registry->DisablePluginLibrary(lib->GetInternalName());
			}
		}

		m_pGUILinkage->WarningMessage("Plugin activation problems",
			QString("Some plugins failed to initialize:\n\n%1\nThese plugins have been disabled. You can re-enable them from the 'System' menu.").arg(message));
	}

	return true;
}

void CLC7Controller::ShutdownPlugins(void)
{
	TR;
	// Deactivate all plugins in reverse registration order
	m_plugin_registry->UnloadPluginLibraries();
}

void CLC7Controller::Shutdown(void)
{
	TR;
	// Stop thermal watchdog
	if (m_thermal_watchdog)
	{
		m_thermal_watchdog->requestInterruption();
		m_thermal_watchdog->wait();
		delete m_thermal_watchdog;
		m_thermal_watchdog = nullptr;
	}

	// Stop system monitor
	if (m_system_monitor)
	{
		delete m_system_monitor;
		m_system_monitor = nullptr;
	}


	// Unregister all notifiers
	UnregisterNotifySessionActivity(QUuid(), this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CLC7Controller::NotifySessionActivity);
	UnregisterNotifySessionActivity(SINGLE_WORKQUEUE_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CLC7Controller::NotifySessionActivity);
	UnregisterNotifySessionActivity(BATCH_WORKQUEUE_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CLC7Controller::NotifySessionActivity);

	// Terminate presets
	if (m_preset_manager)
	{
		m_preset_manager->Terminate();
		delete m_preset_manager;
		m_preset_manager = nullptr;
	}

	// Terminate historical data
	if (m_historical_data)
	{
		m_historical_data->Terminate();
		delete m_historical_data;
		m_historical_data = nullptr;
	}

	// Terminate task scheduler
	if (m_task_scheduler)
	{
		delete m_task_scheduler;
		m_task_scheduler = nullptr;
	}
	
	if (m_pSystemCat)
	{
		if (m_pCoreSettingsAct)
		{
			m_pSystemCat->RemoveAction(m_pCoreSettingsAct);
			m_pCoreSettingsAct = nullptr;
		}

		RemoveActionCategory(m_pSystemCat);
		m_pSystemCat = nullptr;
	}

	if (m_pSecureStringSerializer)
	{
		delete m_pSecureStringSerializer;
		m_pSecureStringSerializer = nullptr;
	}
	
	if (m_pCoreSettings)
	{
		RemoveComponent(m_pCoreSettings);
		delete m_pCoreSettings;
		m_pCoreSettings = nullptr;
	}

	if (m_plugin_registry)
	{
		delete m_plugin_registry;
		m_plugin_registry = nullptr;
	}

	if (m_settings)
	{
		delete m_settings;
		m_settings = nullptr;
	}

	if (m_single_workqueuefactory)
	{
		delete m_single_workqueuefactory;
		m_single_workqueuefactory = nullptr;
	}

	if (m_batch_workqueuefactory)
	{
		delete m_batch_workqueuefactory;
		m_batch_workqueuefactory = nullptr;
	}

	if (m_pLinkage)
	{
		delete m_pLinkage;
		m_pLinkage = nullptr;
	}

	m_pGUILinkage = nullptr;

	QDir temproot(m_strTempRoot);
	temproot.removeRecursively();

	if (m_task_running)
	{
		Q_ASSERT(0);
	}

#ifdef _WIN32
	// Destroy global mutex
	CloseHandle(m_compile_mutex);
#endif


}

ILC7GUILinkage *CLC7Controller::GetGUILinkage(void)
{
	return m_pGUILinkage;
}

ILC7Linkage *CLC7Controller::GetLinkage(void)
{
	return m_pLinkage;
}

ILC7HistoricalData *CLC7Controller::GetHistoricalData()
{
	return m_historical_data;
}

ILC7Settings *CLC7Controller::GetSettings()
{
	return m_settings;
}

ILC7CPUInformation *CLC7Controller::GetCPUInformation()
{
	return m_cpuinformation;
}

ILC7SystemMonitor *CLC7Controller::GetSystemMonitor()
{
	return m_system_monitor;
}

ILC7ThermalWatchdog *CLC7Controller::GetThermalWatchdog()
{
	return m_thermal_watchdog;
}


ILC7PluginRegistry *CLC7Controller::GetPluginRegistry()
{
	return m_plugin_registry;
}

bool CLC7Controller::AddComponent(ILC7Component *component)
{
	TR;
	if (m_components_by_uuid.contains(component->GetID()))
	{
		return false;
	}

	m_components_by_uuid.insert(component->GetID(), component);

	return true;
}

bool CLC7Controller::RemoveComponent(ILC7Component *component)
{
	TR;
	if (!m_components_by_uuid.contains(component->GetID()))
	{
		return false;
	}

	m_components_by_uuid.remove(component->GetID());

	return true;
}


ILC7Component *CLC7Controller::FindComponentByID(QUuid id)
{
	if (!m_components_by_uuid.contains(id))
	{
		return NULL;
	}

	return m_components_by_uuid[id];
}


ILC7TaskScheduler *CLC7Controller::GetTaskScheduler(void)
{
	return m_task_scheduler;
}

ILC7PresetManager *CLC7Controller::GetPresetManager(void)
{
	return m_preset_manager;
}

ILC7ActionCategory *CLC7Controller::CreateActionCategory(QString internal_name, QString name, QString desc, QIcon icon)
{
	TR;
	return m_top_level_action_category.CreateActionCategory(internal_name, name, desc, icon);
}

void CLC7Controller::RemoveActionCategory(ILC7ActionCategory *cat)
{
	TR;
	m_top_level_action_category.RemoveActionCategory(cat);
}

QList<ILC7ActionCategory *> CLC7Controller::GetActionCategories()
{
	return m_top_level_action_category.GetActionCategories();
}

bool CLC7Controller::SecureKeyExists(QString section, QString key)
{
	TR;
	QString error;
	QByteArray data;
	return SecureLoad(section, key, data, error);
}

bool CLC7Controller::SecureDelete(QString section, QString key, QString & error)
{
	TR;
	DeletePasswordJob job(section);
	job.setAutoDelete(false);
	job.setKey(section + "_" + key);
	job.setQueued(false);

	job.start();

	if (job.error()) {
		error = job.errorString();
		return false;
	}
	return true;
}


bool CLC7Controller::SecureStore(QString section, QString key, QString value, QString & error)
{
	TR;
	WritePasswordJob job(section);
	job.setAutoDelete(false);
	job.setKey(section + "_" + key);
	job.setTextData(value);
	job.setQueued(false);

	job.start();

	if (job.error()) {
		error = job.errorString();
		return false;
	}
	return true;
}


bool CLC7Controller::SecureStore(QString section, QString key, LC7SecureString value, QString & error)
{
	TR;
	WritePasswordJob job(section);
	job.setAutoDelete(false);
	job.setKey(section + "_" + key);
	job.setTextData(value.ToSerializedString());

	job.setQueued(false);

	job.start();

	if (job.error()) {
		error = job.errorString();
		return false;
	}
	return true;
}


bool CLC7Controller::SecureStore(QString section, QString key, QByteArray value, QString & error)
{
	TR;
	WritePasswordJob job(section);
	job.setAutoDelete(false);
	job.setKey(section + "_" + key);
	job.setBinaryData(value);
	job.setQueued(false);

	job.start();

	if (job.error()) {
		error = job.errorString();
		return false;
	}

	return true;
}


bool CLC7Controller::SecureLoad(QString section, QString key, QString &value, QString & error)
{
	TR;
	ReadPasswordJob job(section);
	job.setAutoDelete(false);
	job.setKey(section + "_" + key);
	job.setQueued(false);

	job.start();

	if (job.error()) {
		error = job.errorString();
		return false;
	}

	value = job.textData();

	return true;
}


bool CLC7Controller::SecureLoad(QString section, QString key, LC7SecureString &value, QString & error)
{
	TR;
	ReadPasswordJob job(section);
	job.setAutoDelete(false);
	job.setKey(section + "_" + key);
	job.setQueued(false);

	job.start();

	if (job.error()) {
		error = job.errorString();
		return false;
	}

	value = LC7SecureString::FromSerializedString(job.textData());

	return true;
}


bool CLC7Controller::SecureLoad(QString section, QString key, QByteArray &value, QString & error)
{
	TR;
	ReadPasswordJob job(section);
	job.setAutoDelete(false);
	job.setKey(section + "_" + key);
	job.setQueued(false);

	job.start();

	if (job.error()) {
		error = job.errorString();
		return false;
	}

	value = job.binaryData();

	return true;
}

QString CLC7Controller::NewTemporaryDir(void)
{
	TR;
	QDir temproot(m_strTempRoot);
	QString name = QUuid::createUuid().toString();
	temproot.mkdir(name);
	return temproot.filePath(name);
}

QString CLC7Controller::NewTemporaryFile(bool create, QString ext)
{
	TR;
	QDir temproot(m_strTempRoot);
	QString name = temproot.filePath(QUuid::createUuid().toString()) + ext;
	if (create)
	{
		QFile f(name);
		if (!f.open(QIODevice::WriteOnly))
		{
			return "";
		}
	}

	return name;
}

QString CLC7Controller::GetStartupDirectory()
{
	return qApp->applicationDirPath();
}

QString CLC7Controller::GetSessionsDirectory()
{
	QDir docsdir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
	QString default_sessions_directory = docsdir.absoluteFilePath("LC7 Sessions");

	QString sessions_directory = GetSettings()->value("_core_:sessions_directory", default_sessions_directory).toString();

	QDir(sessions_directory).mkpath(".");

	return sessions_directory;
}

QString CLC7Controller::GetReportsDirectory()
{
	QDir docsdir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
	QString default_reports_directory = docsdir.absoluteFilePath("LC7 Reports");

	QString reports_directory = GetSettings()->value("_core_:reports_directory", default_reports_directory).toString();

	QDir(reports_directory).mkpath(".");

	return reports_directory;
}

QString CLC7Controller::GetPluginsDirectory()
{
	return qApp->applicationDirPath() + "/lcplugins";
}

QString CLC7Controller::GetAppDataDirectory()
{
	QString appdata = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	QDir appdatadir(appdata);
	appdatadir.mkpath(".");
	return appdata;
}

QString CLC7Controller::GetPublicDataDirectory()
{
#ifdef _WIN32
	QString publicdata = QString::fromLocal8Bit(qgetenv("ALLUSERSPROFILE"));
#else
	QString publicdata = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
#endif
	QDir publicdatadir(publicdata);
	publicdatadir.cd("L0phtCrack 7");
	//publicdatadir.mkpath(".");
	return publicdatadir.absolutePath();
}

QString CLC7Controller::GetCacheDirectory()
{
	QString appdata = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
	QDir appdatadir(appdata);
	appdatadir.mkpath(".");
	return appdata;
}


QStringList CLC7Controller::GetRecentSessionsList()
{
	ILC7Settings *settings = GetSettings();
	QStringList recentsessionfiles = settings->value("_core_:recentsessionfiles", QStringList()).toStringList();
	return recentsessionfiles;
}

void CLC7Controller::StartAutosaveTimer()
{
	// Run autosave timer every minute
	m_last_autosave_file = "";
	m_autosave_counter = 0;
	m_autosave_timer.setInterval(1000 * 60);
	m_autosave_timer.start();
}

void CLC7Controller::StopAutosaveTimer()
{
	m_autosave_timer.stop();
	if (!m_last_autosave_file.isEmpty() && QFileInfo(m_last_autosave_file).isFile())
	{
		// Remove unused autosave file
		QFile::remove(m_last_autosave_file);
	}
	m_last_autosave_file = "";
}

bool CLC7Controller::IsAutoSaveAvailable()
{
	return GetAutoSavedSessions().size() > 0;
}

QFileInfoList CLC7Controller::GetAutoSavedSessions()
{
	QDir autosave_directory(GetAppDataDirectory());
	autosave_directory.mkdir("autosave");
	autosave_directory.cd("autosave");

	QFileInfoList entryinfolist = autosave_directory.entryInfoList(QStringList() << "*.lc7", QDir::Files, QDir::Time);

	foreach(QFileInfo fi, entryinfolist)
	{
		if (fi.fileName() == QFileInfo(m_last_autosave_file).fileName())
		{
			entryinfolist.removeOne(fi);
			break;
		}
	}

	return entryinfolist;

}

void CLC7Controller::slot_autosaveTimer()
{
	// This fires every minute. Keep the number of minutes since we started the session.
	m_autosave_counter++;

	// If we're not autosaving, punt
	if (!m_settings->value("_core_:autosave", true).toBool())
	{
		return;
	}

	// If we are autosaving, see if enough time has elapsed
	int autosave_time = m_settings->value("_core_:autosave_time", 5).toInt();
	if (m_autosave_counter < autosave_time)
	{
		return;
	}

	// Save the session to an autosave file
	QDir autosave_directory(GetAppDataDirectory());
	autosave_directory.mkdir("autosave");
	autosave_directory.cd("autosave");

	QDateTime now = QDateTime::currentDateTime();
	QString session_name = "Untitled";
	if (m_session_opened && !m_session_path.isEmpty())
	{
		session_name = QFileInfo(m_session_path).baseName();
	}
	QString autosave_file_name = QString("%1_Autosave_%2.lc7").arg(session_name).arg(now.toString("yyyyMMddhhmmss"));
	QString autosave_file_path = autosave_directory.absoluteFilePath(autosave_file_name);

	// Save it while running
	if (!SaveSession(autosave_file_path, true, false, true))
	{
		return;
	}

	// Remove previous autosave file
	QString old_autosave_file = m_last_autosave_file;
	m_last_autosave_file = autosave_file_path;
	if (!old_autosave_file.isEmpty() && QFileInfo(old_autosave_file).isFile())
	{
		QFile::remove(old_autosave_file);
	}

	// Reset counter so we can go again
	m_autosave_counter = 0;

	// Say we did it
	//	m_pGUILinkage->GetWorkQueueWidget()->AppendToActivityLog(QString("Session auto-saved to %1\n").arg(QDir::toNativeSeparators(autosave_file_name)));
}
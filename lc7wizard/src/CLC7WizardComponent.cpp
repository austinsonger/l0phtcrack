#include"stdafx.h"
#include"../lc7password/include/uuids.h"
#include"../lc7importwin/include/uuids.h"
#include"../lc7importunix/include/uuids.h"
#include"../lc7jtr/include/uuids.h"
#include"../lc7reports/include/uuids.h"

CLC7WizardComponent::CLC7WizardComponent()
{
	TR;
	g_pLinkage->RegisterNotifySessionActivity(BATCH_WORKQUEUE_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CLC7WizardComponent::NotifySessionActivity);
	m_batch_workqueue = NULL;
}

CLC7WizardComponent::~CLC7WizardComponent()
{	TR;
	g_pLinkage->UnregisterNotifySessionActivity(BATCH_WORKQUEUE_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CLC7WizardComponent::NotifySessionActivity);
}

ILC7Interface *CLC7WizardComponent::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Component")
	{
		return this;
	}
	return NULL;
}

QUuid CLC7WizardComponent::GetID()
{
	return UUID_LC7WIZARD;
}

bool CLC7WizardComponent::QueueBatchCommand(QUuid uuid, QMap<QString, QVariant> & config, QString &error)
{
	ILC7Component *comp = g_pLinkage->FindComponentByID(uuid);

	QStringList args;
	args << "queue";

	config["workqueue"] = (qulonglong)m_batch_workqueue;
	if (comp->ExecuteCommand("gui", args, config, error, NULL)!=ILC7Component::SUCCESS)
	{
		return false;
	}

	return true;
}

void CLC7WizardComponent::ShowHideColumn(QString colname, bool show)
{
	ILC7Settings *settings = g_pLinkage->GetSettings();
	QString name = QString(UUID_PASSWORDGUIPLUGIN.toString() + ":show_column_%1").arg(colname);
	settings->setValue(name, show);
}

bool CLC7WizardComponent::RunWizard(QMap<QString, QVariant> & config, bool & cancelled, QString & error)
{	
	TR;

	ILC7GUILinkage *guilink = g_pLinkage->GetGUILinkage();
	bool have_session = (m_batch_workqueue != NULL);
	if (have_session)
	{
		if (!guilink->RequestCloseSession(false, false))
		{
			cancelled = true;
			return true;
		}
	}

	cancelled = false;
	CLC7Wizard wizard;
	g_pLinkage->GetGUILinkage()->ShadeUI(true);
	int res = wizard.exec();
	g_pLinkage->GetGUILinkage()->ShadeUI(false);
	if (res != QDialog::Accepted)
	{
		cancelled = true;
		return true;
	}

	// Display Options
	bool display_passwords = wizard.field("report_display_passwords").toBool();
	bool display_hashes = wizard.field("report_display_hashes").toBool();

	ILC7PasswordLinkage *passlink = GET_ILC7PASSWORDLINKAGE(g_pLinkage);
	LC7HashType lc7hashtype;
	QString err;
	foreach(FOURCC hashtype, passlink->ListHashTypes())
	{
		QString out;
		if (!passlink->LookupHashType(hashtype, lc7hashtype, err))
		{
			out = "Unknown";
		}
		else
		{
			out = lc7hashtype.name;
		}
		ShowHideColumn(out + " Hash", display_hashes);
		ShowHideColumn(out + " Password", display_passwords);
	}

	// Create session (also updates shown columns)
	if (!guilink->RequestNewSession())
	{
		cancelled = true;
		return true;
	}

	// Add Import
	if (wizard.field("import_windows").toBool())
	{
		if (wizard.field("import_windows_local").toBool())
		{
			QMap<QString, QVariant> config = ((CLC7WizardPage *)wizard.page(CLC7Wizard::Page_Windows_Import_Local))->GetConfig();
			if (!QueueBatchCommand(UUID_IMPORTWINDOWSLOCALGUI, config, error))
			{
				return false;
			}
		}
		else if (wizard.field("import_windows_remote_smb").toBool())
		{
			QMap<QString, QVariant> config;
			config = ((CLC7WizardPage *)wizard.page(CLC7Wizard::Page_Windows_Import_Remote_SMB))->GetConfig();
			
			if (!QueueBatchCommand(UUID_IMPORTWINDOWSREMOTEGUI, config, error))
			{
				return false;
			}
		}
		else if (wizard.field("import_windows_pwdump").toBool())
		{
			QMap<QString, QVariant> config = ((CLC7WizardPage *)wizard.page(CLC7Wizard::Page_Windows_Import_PWDump))->GetConfig();
			
			if (!QueueBatchCommand(UUID_IMPORTPWDUMPGUI, config, error))
			{
				return false;
			}
		}
	}
	else if (wizard.field("import_unix").toBool())
	{
		if (wizard.field("import_unix_ssh").toBool())
		{
			QMap<QString, QVariant> config = ((CLC7WizardPage *)wizard.page(CLC7Wizard::Page_Unix_Import_SSH))->GetConfig();

			if (!QueueBatchCommand(UUID_IMPORTUNIXSSHGUI, config, error))
			{
				return false;
			}

		}
		else if (wizard.field("import_unix_shadow").toBool())
		{
			QMap<QString, QVariant> config = ((CLC7WizardPage *)wizard.page(CLC7Wizard::Page_Unix_Import_File))->GetConfig();

			if (!QueueBatchCommand(UUID_IMPORTSHADOWGUI, config, error))
			{
				return false;
			}
		}
	}

	// Add Audit
	{
		QMap<QString, QVariant> config;
		config["name"] = QString("User Info");
		config["jtr_mode"] = "single";

		QString disp;
		disp += config["name"].toString();
		config["display_string"] = disp;

		QueueBatchCommand(UUID_TECHNIQUEJTRSINGLEGUI, config, error);
	}

	if (wizard.field("audit_quick").toBool())
	{
		QMap<QString, QVariant> config;
		config["name"] = QString("%1:%2").arg("Dictionary").arg("Fast");
		config["jtr_mode"] = "wordlist";
		config["display_string"] = config["name"].toString();
		ILC7Preset *preset = g_pLinkage->GetPresetManager()->presetById(UUID_DICTIONARY_FAST);
		config.unite(preset->config().toMap());

		QueueBatchCommand(UUID_TECHNIQUEJTRDICTIONARYGUI, config, error);
	}
	else if (wizard.field("audit_common").toBool())
	{
		QMap<QString, QVariant> config;
		config["name"] = QString("%1:%2").arg("Dictionary").arg("Thorough");
		config["jtr_mode"] = "wordlist";
		config["display_string"] = config["name"].toString();
		ILC7Preset *preset = g_pLinkage->GetPresetManager()->presetById(UUID_DICTIONARY_THOROUGH);
		config.unite(preset->config().toMap());

		QueueBatchCommand(UUID_TECHNIQUEJTRDICTIONARYGUI, config, error);

		config.clear();
		config["name"] = QString("%1:%2").arg("Brute").arg("Fast");
		config["jtr_mode"] = "incremental";
		config["display_string"] = config["name"].toString();
		preset = g_pLinkage->GetPresetManager()->presetById(UUID_BRUTE_FAST);
		config.unite(preset->config().toMap());

		QueueBatchCommand(UUID_TECHNIQUEJTRBRUTEGUI, config, error);
	}
	else if (wizard.field("audit_thorough").toBool())
	{
		QMap<QString, QVariant> config;
		config["name"] = QString("%1:%2").arg("Dictionary").arg("Complex");
		config["jtr_mode"] = "wordlist";
		config["display_string"] = config["name"].toString();
		ILC7Preset *preset = g_pLinkage->GetPresetManager()->presetById(UUID_DICTIONARY_COMPLEX);
		config.unite(preset->config().toMap());

		QueueBatchCommand(UUID_TECHNIQUEJTRDICTIONARYGUI, config, error);

		config.clear();
		config["name"] = QString("%1:%2").arg("Brute").arg("Thorough");
		config["jtr_mode"] = "incremental";
		config["display_string"] = config["name"].toString();
		preset = g_pLinkage->GetPresetManager()->presetById(UUID_BRUTE_THOROUGH);
		config.unite(preset->config().toMap());

		QueueBatchCommand(UUID_TECHNIQUEJTRBRUTEGUI, config, error);
	}
	else if (wizard.field("audit_strong").toBool())
	{
		QMap<QString, QVariant> config;
		config["name"] = QString("%1:%2").arg("Brute").arg("Exhaustive");
		config["jtr_mode"] = "incremental";
		config["display_string"] = config["name"].toString();
		ILC7Preset *preset = g_pLinkage->GetPresetManager()->presetById(UUID_BRUTE_EXHAUSTIVE);
		config.unite(preset->config().toMap());
		QueueBatchCommand(UUID_TECHNIQUEJTRBRUTEGUI, config, error);

		config.clear();
		config["name"] = QString("%1:%2").arg("Dictionary").arg("Exhaustive");
		config["jtr_mode"] = "wordlist";
		config["display_string"] = config["name"].toString();
		preset = g_pLinkage->GetPresetManager()->presetById(UUID_DICTIONARY_EXHAUSTIVE);
		config.unite(preset->config().toMap());

		QueueBatchCommand(UUID_TECHNIQUEJTRDICTIONARYGUI, config, error);
	}

	// Reporting Options
	if (wizard.field("report_export").toBool())
	{
		QMap<QString, QVariant> config;

		if (wizard.field("report_csv").toBool())
		{
			config["format"] = "CSV";
		}
/*		if (wizard.field("report_pdf").toBool())
		{
			config["format"] = "PDF";
		}
*/		if (wizard.field("report_html").toBool())
		{
			config["format"] = "HTML";
		}
		if (wizard.field("report_xml").toBool())
		{
			config["format"] = "XML";
		}
		config["filename"] = QDir::fromNativeSeparators(wizard.field("report_file").toString());

		config["include_style"] = true;

		config["column_audited_status"] = true;
		config["column_domain"] = true;
		config["column_hashes"] = wizard.field("report_display_hashes").toBool();
		config["column_last_changed_time"] = true;
		config["column_locked_out"] = true;
		config["column_machine"] = true;
		config["column_passwords"] = wizard.field("report_display_passwords").toBool();
		config["column_user_id"] = true;
		config["column_user_info"] = true;
		config["column_username"] = true;

		QString disp;
		disp += QString("%1 Format, File: %2").arg(config["format"].toString()).arg(QDir::toNativeSeparators(config["filename"].toString()));
		if (config["include_style"].toBool())
		{
			disp += " (include style)";
		}
		if (config["column_audited_status"].toBool())
		{
			disp += " +Audited Status";
		}
		if (config["column_domain"].toBool())
		{
			disp += " +Domain";
		}
		if (config["column_hashes"].toBool())
		{
			disp += " +Hashes";
		}
		if (config["column_last_changed_time"].toBool())
		{
			disp += " +Last Changed Time";
		}
		if (config["column_locked_out"].toBool())
		{
			disp += " +State Flags";
		}
		if (config["column_machine"].toBool())
		{
			disp += " +Machine";
		}
		if (config["column_passwords"].toBool())
		{
			disp += " +Passwords";
		}
		if (config["column_user_id"].toBool())
		{
			disp += " +User Id";
		}
		if (config["column_user_info"].toBool())
		{
			disp += " +User Info";
		}
		if (config["column_username"].toBool())
		{
			disp += " +Username";
		}

		config["display_string"] = disp;

		if (!QueueBatchCommand(UUID_REPORTEXPORTACCOUNTSGUI, config, error))
		{
			return false;
		}
	}

	// Scheduling Options
	int item;
	if (!m_batch_workqueue->Validate(error, item))
	{
		return false;
	}

	if (wizard.field("schedule_now").toBool())
	{
		m_batch_workqueue->StartRequest();
	}
	else if (wizard.field("schedule_later").toBool())
	{
		QDateTime dt = wizard.field("schedule_datetime").toDateTime();

		ILC7TaskScheduler *sched = g_pLinkage->GetTaskScheduler();
		ILC7EditableTask *task = sched->NewTask();
		if (task == NULL)
		{
			return false;
		}

		task->SetStartTime(dt);
		task->SetName("Wizard Task");
		task->SetRecurrence(ILC7Task::ONE_TIME);
		task->SetSaveTaskOutput(true);
		if (!sched->ScheduleTask(task, error, cancelled))
		{
			return false;
		}
		if (cancelled)
		{
			return true;
		}

		// If task is scheduled we can close the session
		if (!guilink->RequestCloseSession(true, true))
		{
			cancelled = true;
			return true;
		}

	}

	return true;
}

void CLC7WizardComponent::NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler)
{
	TR;
	switch (activity)
	{
	case ILC7Linkage::SESSION_OPEN_POST:
	case ILC7Linkage::SESSION_NEW_POST:
		if (handler && handler->GetId() == BATCH_WORKQUEUE_HANDLER_ID)
		{
			m_batch_workqueue = (ILC7WorkQueue *)handler;
		}
		break;
	case ILC7Linkage::SESSION_CLOSE_PRE:
		if (handler && handler->GetId() == BATCH_WORKQUEUE_HANDLER_ID)
		{
			m_batch_workqueue = NULL;
		}
		break;
	default:
		break;
	}
}


ILC7Component::RETURNCODE CLC7WizardComponent::ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl)
{	TR;
	if(command=="wizard")
	{
		bool cancelled;
		if(!RunWizard(config,cancelled, error))
		{
			return FAIL;
		}
		config["cancelled"] = cancelled;
		return SUCCESS;
	}
	return FAIL;
}

bool CLC7WizardComponent::ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error)
{TR;
	return true;
}


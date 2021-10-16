#include"stdafx.h"

CLC7CoreSettings::CLC7CoreSettings(ILC7Linkage *pLinkage)
{TR;
	m_pLinkage = pLinkage;
}


CLC7CoreSettings::~CLC7CoreSettings()
{TR;
}

ILC7Interface *CLC7CoreSettings::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Component")
	{
		return this;
	}
	return NULL;
}


QUuid CLC7CoreSettings::GetID()
{TR;
	return UUID_LC7CORESETTINGS;
}

void CLC7CoreSettings::AddOption(QList<QVariant> & keys,
	QString settingskey,
	QString name,
	QString desc,
	QVariant default_value,
	bool require_restart,
	QString option1key, QVariant option1value,
	QString option2key, QVariant option2value,
	QString option3key, QVariant option3value,
	QString option4key, QVariant option4value,
	QString option5key, QVariant option5value)
{TR;
	QMap<QString, QVariant> options;
	options["name"] = name;
	options["desc"] = desc;
	options["default"] = default_value;
	options["type"] = QString(default_value.typeName());
	options["settingskey"] = settingskey;
	options["require_restart"] = require_restart;
	if (!option1key.isEmpty())
	{
		options[option1key] = option1value;
	}
	if (!option2key.isEmpty())
	{
		options[option2key] = option2value;
	}
	if (!option3key.isEmpty())
	{
		options[option3key] = option3value;
	}
	if (!option4key.isEmpty())
	{
		options[option4key] = option4value;
	}
	if (!option5key.isEmpty())
	{
		options[option5key] = option5value;
	}

	keys.append(options);
}


bool CLC7CoreSettings::GetOptions(QMap<QString, QVariant> & config, QString & error)
{TR;
	QList<QVariant> keys;

	QDir docsdir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));

	QString default_sessions_directory = docsdir.absoluteFilePath("LC7 Sessions");
	QString default_task_output_directory = docsdir.absoluteFilePath("LC7 Scheduled Task Output");
	QString default_reports_directory = docsdir.absoluteFilePath("LC7 Reports");
	
	AddOption(keys,
		"_core_:sessions_directory",
		"Session Documents Directory",
		"The directory where session files are normally saved",
		QDir::toNativeSeparators(default_sessions_directory),
		false);

	AddOption(keys,
		"_core_:task_output_directory",
		"Scheduled Task Output Directory",
		"The directory where scheduled tasks save their output sessions after running",
		QDir::toNativeSeparators(default_task_output_directory),
		false);

#if	PLATFORM==PLATFORM_WIN32 || PLATFORM==PLATFORM_WIN64
	AddOption(keys,
		"_core_:task_scheduler_version",
		"Use Task Scheduler Version 2",
		"Use version 2 of the Task Scheduler API, because v1 is deprecated in newer Windows versions",
		true,
		true);
#endif

	AddOption(keys,
		"_core_:reports_directory",
		"Report Documents Directory",
		"The directory where generated reports are normally saved",
		QDir::toNativeSeparators(default_reports_directory),
		false);

	AddOption(keys,
		"_core_:allow_insecure_sessions",
		"Allow Storing Credentials In Session Files",
		"Permit the storage of credentials in session files. This may be a security risk in your environment. You may turn it on for convenience if you wish, but it is disallowed by default.",
		false,
		false);

	AddOption(keys,
		"_core_:autosave",
		"Auto-Save Sessions",
		"Save sessions in the background to recovery files so if L0phtCrack 7 crashes, you can recover.",
		true,
		false);

	AddOption(keys,
		"_core_:autosave_time",
		"Auto-Save Frequency",
		"Number of minutes between auto-saves",
		5,
		false,
		"minimum",1);
	

	AddOption(keys,
		"_core_:enablethermal",
		"Enable GPU Thermal Protection",
		"Protects the system from overheating while cracking. This will pause cracking when the GPU gets too hot, and resume when it has cooled down.",
		true,
		false);

	AddOption(keys,
		"_core_:gputemplimit",
		"HOT GPU Temperature (C)",
		"Temperature in degrees Celsius at which the GPU has overheated. Operation will pause until it has reached the WARM temperature.",
		95,
		false);

	AddOption(keys,
		"_core_:gpucooltemp",
		"WARM GPU Temperature (C)",
		"When operations are paused due to overheating, wait until the specified temperature in degrees Celsius is reached before starting again.",
		75,
		false);


	config["keys"] = keys;

	return true;
}

ILC7Component::RETURNCODE CLC7CoreSettings::ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl)
{TR;
	if (command == "get_options")
	{
		if (!GetOptions(config, error))
		{
			return FAIL;
		}
		return SUCCESS;
	}
	return FAIL;
}

bool CLC7CoreSettings::ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error)
{TR;
	return true;
}


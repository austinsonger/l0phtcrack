#include"stdafx.h"

CWindowsImportSettings::CWindowsImportSettings(ILC7Linkage *pLinkage)
{
	TR;
	m_pLinkage = pLinkage;
}


CWindowsImportSettings::~CWindowsImportSettings()
{
	TR;
}

ILC7Interface *CWindowsImportSettings::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Component")
	{
		return this;
	}
	return NULL;
}


QUuid CWindowsImportSettings::GetID()
{
	TR;
	return UUID_WINDOWSIMPORTSETTINGS;
}

void CWindowsImportSettings::AddOption(QList<QVariant> & keys,
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
{
	TR;
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

bool CWindowsImportSettings::GetOptions(QMap<QString, QVariant> & config, QString & error)
{
	TR;
	QList<QVariant> keys;

	// Enable Challenge/Response Hashes
	AddOption(keys,
		UUID_IMPORTWINPLUGIN.toString() + ":enable_chalresp",
		"Enable Importing Challenge/Response Negotiations",
		"Allow PWDump importer to import NTLMv1/v2 hashes from a sniffer session, such as output from Responder. This will result in longer calibrations.",
		false,
		true);

	config["keys"] = keys;

	return true;
}


ILC7Component::RETURNCODE CWindowsImportSettings::ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl)
{
	TR;
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

bool CWindowsImportSettings::ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error)
{
	TR;
	return true;
}


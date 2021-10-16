#include"stdafx.h"

CPasswordUIOptions::CPasswordUIOptions()
{TR;
}

CPasswordUIOptions::~CPasswordUIOptions()
{TR;
}


ILC7Interface *CPasswordUIOptions::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Component")
	{
		return this;
	}
	return NULL;
}


QUuid CPasswordUIOptions::GetID()
{TR;
	return UUID_PASSWORDUIOPTIONS;
}

void CPasswordUIOptions::AddOption(QList<QVariant> & keys,
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
	if(!option1key.isEmpty())
	{
		options[option1key]=option1value;
	}
	if(!option2key.isEmpty())
	{
		options[option2key]=option2value;
	}
	if(!option3key.isEmpty())
	{
		options[option3key]=option3value;
	}
	if(!option4key.isEmpty())
	{
		options[option4key]=option4value;
	}
	if(!option5key.isEmpty())
	{
		options[option5key]=option5value;
	}

	keys.append(options);
}


bool CPasswordUIOptions::GetOptions(QMap<QString, QVariant> & config, QString & error)
{TR;
	QList<QVariant> keys;

	AddOption(keys,
		UUID_PASSWORDUIOPTIONS.toString()+":switch_to_accounts",
		"Switch to accounts view after import/audit", 
		"Show accounts view after performing an import or an audit",  
		true,
		false);

	AddOption(keys,
		UUID_PASSWORDUIOPTIONS.toString() + ":obscure_passwords",
		"Don't display cracked passwords",
		"Obscure cracked passwords with *'s rather than displaying them",
		false,
		false);

	config["keys"]=keys;
	
	return true;
}

ILC7Component::RETURNCODE CPasswordUIOptions::ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl)
{TR;
	if(command=="get_options")
	{
		if(!GetOptions(config,error))
		{
			return FAIL;
		}
		return SUCCESS;
	}
	return FAIL;
}

bool CPasswordUIOptions::ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error)
{TR;
	return true;
}


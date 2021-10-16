#include"stdafx.h"

CSystemOptions::CSystemOptions(ILC7Linkage *pLinkage)
{TR;
	m_pLinkage=pLinkage;
}


CSystemOptions::~CSystemOptions()
{TR;
}


ILC7Interface *CSystemOptions::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Component")
	{
		return this;
	}
	return NULL;
}

QUuid CSystemOptions::GetID()
{TR;
	return UUID_SYSTEMCOLOR;
}


void CSystemOptions::AddOption(QList<QVariant> & keys,
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


bool CSystemOptions::GetOptions(QMap<QString, QVariant> & config, QString & error)
{TR;
	QList<QVariant> keys;

	ILC7ColorManager *colman=m_pLinkage->GetGUILinkage()->GetColorManager();
	QMap<QString, int> default_shades=colman->GetDefaultShades();
	
	AddOption(keys,
		"_ui_:showstartupdialog",
		"Show session dialog at startup", 
		"Show session create/open/recent convenience dialog at startup",  
		true,
		false);

	AddOption(keys,
		"_ui_:switch_to_queue",
		"Switch to queue view after adding to queue", 
		"Show queue view after adding an action to the queue",  
		true,
		false);

	AddOption(keys,
		"_ui_:switch_to_schedule",
		"Switch to schedule view after scheduling queue",
		"Show queue view after adding an task to the scheduler",
		true,
		false);

	AddOption(keys,
		"_update_:check_for_updates",
		"Check for LC7 updates",
		"Check the L0phtCrack website for system updates",
		true,
		false);

	AddOption(keys,
		"_debug_:debug_logging",
		"Turn on debug logging",
		"Enable logging of debug information to %TEMP%\\lc7_log.txt",
		false,
		true);

	AddOption(keys,
		"_theme_:basecolor",
		"Base Color", 
		"Primary color to use for the user interface.",  
		colman->GetDefaultBaseColor(),
		false);

	AddOption(keys,
		"_theme_:highlightcolor",
		"Highlight Color", 
		"Color to use for highlights in the user interface.",  
		colman->GetDefaultHighlightColor(),
		false);
	
#ifdef _DEBUG
	foreach(QString shade, default_shades.keys())
	{
		AddOption(keys,"_theme_:shade_"+shade,"shade_"+shade,"shade_"+shade,default_shades[shade], false);
	}
#endif

	config["keys"]=keys;
	
	return true;
}

ILC7Component::RETURNCODE CSystemOptions::ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl)
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

bool CSystemOptions::ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error)
{TR;
	return true;
}


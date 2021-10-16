#include<stdafx.h>
#include"reportsconfig.h"

CReportsGUI::CReportsGUI()
{TR;
}

CReportsGUI::~CReportsGUI()
{TR;
}

ILC7Interface *CReportsGUI::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Component")
	{
		return this;
	}
	return NULL;
}

QUuid CReportsGUI::GetID()
{TR;
	return UUID_REPORTSGUI;
}

ILC7Component::RETURNCODE CReportsGUI::ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl)
{TR;
	return FAIL;
}


bool CReportsGUI::ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error)
{TR;
	return true;
}

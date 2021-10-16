#include<stdafx.h>

CReports::CReports()
{TR;
}

CReports::~CReports()
{TR;

}

QUuid CReports::GetID()
{TR;
	return UUID_REPORTS;
}

ILC7Interface *CReports::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Component")
	{
		return this;
	}
	return NULL;
}

ILC7Component::RETURNCODE CReports::ExecuteCommand(QString command, QStringList args, QMap<QString,QVariant> & config, QString & error, ILC7CommandControl *ctrl)
{TR;
	if(command=="get_defaults")
	{
		return SUCCESS;
	}
	else if(command=="generate")
	{
	
		return SUCCESS;
	}

	return FAIL;
}

bool CReports::ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error)
{TR;
	if(command=="generate")
	{

	}

	return true;
}

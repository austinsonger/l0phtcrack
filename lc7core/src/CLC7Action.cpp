#include<stdafx.h>


QList<CLC7Action *> CLC7Action::s_dead_actions;

void CLC7Action::GarbageCollect(void)
{TR;
	foreach(CLC7Action *act, s_dead_actions)
	{
		delete act;
	}
	s_dead_actions.clear();
}


CLC7Action::CLC7Action(CLC7ActionCategory *category, QUuid componentid, QString command, QStringList args, QString name, QString desc, QIcon icon)
{ 
	m_category = category;
	m_componentid=componentid;
	m_command=command;
	m_args = args;
	m_name=name;
	m_desc=desc;
	m_refcount=0;
	m_icon=icon;
}

CLC7Action::~CLC7Action()
{TR;
	if(m_refcount!=0)
	{
		// Still has references
		QByteArray msg("CLC7Action still has references: ");
		msg+=m_componentid.toString().toLatin1();
		msg+=m_command.toLatin1();
		TRDBG(msg);
	}
}

ILC7Interface *CLC7Action::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Action")
	{
		return this;
	}
	return NULL;
}


int CLC7Action::Ref()
{TR;
	if(m_category)
	{
		m_category->Ref();
	}
	m_refcount++;
	return m_refcount;
}

int CLC7Action::Unref()
{TR;
	m_refcount--;
	if(m_refcount==0)
	{
		if(m_category)
		{
			m_category->RemoveChildAction(this);
		}
		s_dead_actions.append(this);
	}

	if(m_category)
	{
		m_category->Unref();
	}
	
	return m_refcount;
}

ILC7ActionCategory *CLC7Action::ParentCategory()
{TR;
	return m_category;
}

QUuid CLC7Action::ComponentId()
{TR;
	return m_componentid;
}

QString CLC7Action::Command()
{TR;
	return m_command;
}

QStringList CLC7Action::Args()
{TR;
	return m_args;
}

QString CLC7Action::Name()
{TR;
	return m_name;
}

QString CLC7Action::Desc()
{TR;
	return m_desc;
}

QIcon CLC7Action::Icon()
{TR;
	return m_icon;
}
#include<stdafx.h>

QList<CLC7ActionCategory *> CLC7ActionCategory::s_dead_categories;

void CLC7ActionCategory::GarbageCollect(void)
{TR;
	foreach(CLC7ActionCategory *cat, s_dead_categories)
	{
		delete cat;
	}
	s_dead_categories.clear();
}


// top level category constructor
CLC7ActionCategory::CLC7ActionCategory()
{TR;
	m_category=NULL;
	m_internal_name="";
	m_name="";
	m_desc="";
	m_icon=QIcon();
	m_refcount=1;
	m_gen = 0;
}

// regular constructor
CLC7ActionCategory::CLC7ActionCategory(CLC7ActionCategory *category, QString internal_name, QString name, QString desc, QIcon icon)
{TR;
	m_category=category;
	m_internal_name=internal_name;
	m_name=name;
	m_desc=desc;
	m_icon=icon;
	m_refcount=0;
	m_gen = 0;
}

CLC7ActionCategory::~CLC7ActionCategory()
{TR;
	// Allow top-level category to be deleted with one reference
	if((m_category && (m_refcount!=0 || m_action_categories.size()!=0 || m_actions.size()!=0)) || 
	   (!m_category && (m_refcount!=1 || m_action_categories.size()!=0 || m_actions.size()!=0)))
	{
		// Still has references
		QByteArray msg("CLC7ActionCategories still has references: ");
		msg+=m_internal_name;
		TRDBG(msg);
	}
}

ILC7Interface *CLC7ActionCategory::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7ActionCategory")
	{
		return this;
	}
	return NULL;
}

int CLC7ActionCategory::Ref()
{TR;
	if(m_category)
	{
		m_category->Ref();
	}
	m_refcount++;
	m_gen++;
	return m_refcount;
}

int CLC7ActionCategory::Unref()
{TR;
	m_refcount--;
	m_gen++;
	if (m_refcount == 0)
	{
		if(m_category)
		{
			m_category->RemoveChildCategory(this);
		}
		s_dead_categories.append(this);
	}

	if(m_category)
	{
		m_category->Unref();
	}
	
	return m_refcount;
}

quint32 CLC7ActionCategory::GetGenerationNumber()
{
	return m_gen;
}

void CLC7ActionCategory::RemoveChildCategory(CLC7ActionCategory *child)
{TR;
	m_action_categories.remove(child->InternalName());
}

void CLC7ActionCategory::RemoveChildAction(CLC7Action *child)
{TR;
	//QPair<QUuid,QString> key=QPair<QUuid,QString>(child->ComponentId(),child->Command());
	m_actions.removeAll(child);
}


ILC7ActionCategory *CLC7ActionCategory::CreateActionCategory(QString internal_name, QString name, QString desc, QIcon icon)
{TR;
	if(m_action_categories.contains(internal_name))
	{
		m_action_categories[internal_name]->Ref();
		return m_action_categories[internal_name];
	}
	CLC7ActionCategory *cat=new CLC7ActionCategory(this,internal_name,name,desc,icon);
	m_action_categories[internal_name]=cat;
	cat->Ref();
	return cat;
}

void CLC7ActionCategory::RemoveActionCategory(ILC7ActionCategory *cat)
{TR;
	/*
	if(!m_action_categories.contains(internal_name))
	{
		QByteArray msg("CLC7ActionCategory doesn't exist: ");
		msg+=internal_name;
		TRDBG(msg);
		return;
	}
	*/

	// This action can only cause action categories to get deleted
	//m_action_categories[internal_name]->Unref();
	((CLC7ActionCategory *)cat)->Unref();
	
	CLC7ActionCategory::GarbageCollect();
}

ILC7Action *CLC7ActionCategory::CreateAction(QUuid componentid, QString command, QStringList args, QString name, QString desc, QIcon icon)
{TR;
	/*
	if(m_actions.contains(QPair<QUuid,QString>(componentid,command)))
	{
		CLC7Action *action=m_actions[QPair<QUuid,QString>(componentid,command)];
		action->Ref();
		return action;
	}
	*/

	CLC7Action *action=new CLC7Action(this, componentid, command, args, name, desc, icon);
	m_actions.append(action);
	action->Ref();
	return action;
}

void CLC7ActionCategory::RemoveAction(ILC7Action *act)
{TR;
	/*
	QPair<QUuid,QString> key=QPair<QUuid,QString>(componentid,command);
	if(!m_actions.contains(key))
	{
		QByteArray msg("CLC7ActionCategory doesn't exist: ");
		msg+=componentid.toString().toLatin1();
		msg+=" ";
		msg+=command.toLatin1();
		TRDBG(msg);
		return;
	}
	*/

	// This action can cause both actions and action categories to be deleted
	//m_actions[key]->Unref();
	((CLC7Action *)act)->Unref();

	CLC7Action::GarbageCollect();
	CLC7ActionCategory::GarbageCollect();
}

QList<ILC7ActionCategory *> CLC7ActionCategory::GetActionCategories()
{TR;
	QList<ILC7ActionCategory *> l;
	
	foreach(CLC7ActionCategory *cat, m_action_categories.values())
	{
		l.append((ILC7ActionCategory *)cat);
	}
	
	return l;
}


QList<ILC7Action *> CLC7ActionCategory::GetActions()
{TR;
	QList<ILC7Action *> l;
	
	foreach(CLC7Action *act, m_actions)
	{
		l.append((ILC7Action *)act);
	}
	
	return l;
}

ILC7ActionCategory *CLC7ActionCategory::ParentCategory()
{TR;
	return m_category;
}

QString CLC7ActionCategory::InternalName()
{TR;
	return m_internal_name;
}

QString CLC7ActionCategory::Name()
{TR;
	return m_name;
}

QString CLC7ActionCategory::Desc()
{TR;
	return m_desc;
}

QIcon CLC7ActionCategory::Icon()
{TR;
	return m_icon;
}

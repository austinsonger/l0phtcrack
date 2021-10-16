#include"stdafx.h"


CLC7NavBarGroup::CLC7NavBarGroup(CLC7NavBar *bar):QWidget(NULL)
{
	m_bar=bar;

	m_header=new CLC7NavBarHeader(this);
	
	setLayout(new QVBoxLayout());
	layout()->setContentsMargins(0,0,0,0);
	layout()->setSpacing(0);
	layout()->addWidget(m_header);
		
	setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
}



void CLC7NavBarGroup::setTitle(QString title)
{TR;
	m_header->setText(title);
}

QString CLC7NavBarGroup::title()
{TR;
	return m_header->text();
}

CLC7NavBarItem *CLC7NavBarGroup::addItem(QWidget *buddy)
{TR;
	CLC7NavBarItem *item=new CLC7NavBarItem(this, buddy);
	
	if(m_items.count()>0)
	{
		m_items.last()->setObjectName("");
	}

	m_items_by_buddy.insert(buddy,item);
	m_items.append(item);
	((QVBoxLayout *)layout())->insertWidget(-1,item);
	bar()->m_buttongroup.addButton(item);
	
	if(m_items.count()>0)
	{
		m_items.last()->setObjectName("last");
	}

	//	hack to refresh stylesheet
	foreach(CLC7NavBarItem *x,m_items)
	{
		x->setStyleSheet("/* */");
		x->setStyleSheet("");
	}

	return item;
}

CLC7NavBarItem *CLC7NavBarGroup::insertItem(int pos, QWidget *buddy)
{TR;
	CLC7NavBarItem *item=new CLC7NavBarItem(this, buddy);
	
	if(m_items.count()>0)
	{
		m_items.last()->setObjectName("");
	}
	
	m_items_by_buddy.insert(buddy,item);
	m_items.insert(pos,item);
	if(pos<0)
	{
		((QVBoxLayout *)layout())->insertWidget(pos,item);
	}
	else
	{
		// Skip header
		((QVBoxLayout *)layout())->insertWidget(pos+1,item);
	}
	bar()->m_buttongroup.addButton(item);
	
	if(m_items.count()>0)
	{
		m_items.last()->setObjectName("last");
	}

	//	hack to refresh stylesheet
	foreach(CLC7NavBarItem *x,m_items)
	{
		x->setStyleSheet("/* */");
		x->setStyleSheet("");
	}

	return item;
}

void CLC7NavBarGroup::removeItem(int pos)
{TR;
	CLC7NavBarItem *item=m_items[pos];
	QWidget *buddy=m_items_by_buddy.key(item);
	m_items_by_buddy.remove(buddy);
	
	if(m_items.count()>0)
	{
		m_items.last()->setObjectName("");
	}
	m_items.removeAt(pos);	
	bar()->m_buttongroup.removeButton(item);
	if(m_items.count()>0)
	{
		m_items.last()->setObjectName("last");
	}

	delete item;

	//	hack to refresh stylesheet
	foreach(CLC7NavBarItem *x,m_items)
	{
		x->setStyleSheet("/* */");
		x->setStyleSheet("");
	}
}
	

void CLC7NavBarGroup::removeItem(CLC7NavBarItem *item)
{TR;
	QWidget *buddy=item->buddy();
	
	if(m_items.count()>0)
	{
		m_items.last()->setObjectName("");
	}
	
	m_items_by_buddy.remove(buddy);
	int pos=m_items.indexOf(item);
	m_items.removeAt(pos);
	bar()->m_buttongroup.removeButton(item);

	if(m_items.count()>0)
	{
		m_items.last()->setObjectName("last");
	}

	delete item;

	//	hack to refresh stylesheet
	foreach(CLC7NavBarItem *x,m_items)
	{
		x->setStyleSheet("/* */");
		x->setStyleSheet("");
	}
}

void CLC7NavBarGroup::removeItem(QWidget *buddy)
{TR;
	CLC7NavBarItem *item=m_items_by_buddy.take(buddy);
	
	if(m_items.count()>0)
	{
		m_items.last()->setObjectName("");
	}

	int pos=m_items.indexOf(item);
	m_items.removeAt(pos);
	bar()->m_buttongroup.removeButton(item);

	if(m_items.count()>0)
	{
		m_items.last()->setObjectName("last");
	}

	delete item;

	//	hack to refresh stylesheet
	foreach(CLC7NavBarItem *x,m_items)
	{
		x->setStyleSheet("/* */");
		x->setStyleSheet("");
	}
}


int CLC7NavBarGroup::indexOf(CLC7NavBarItem *item)
{TR;
	return m_items.indexOf(item);
}

int CLC7NavBarGroup::indexOf(QWidget *buddy)
{TR;
	int pos=m_items.indexOf(m_items_by_buddy[buddy]);
	return pos;
}

CLC7NavBarItem *CLC7NavBarGroup::item(int pos)
{TR;
	return m_items[pos];
}

CLC7NavBarItem *CLC7NavBarGroup::item(QWidget *buddy)
{TR;
	return m_items_by_buddy[buddy];
}

int CLC7NavBarGroup::count()
{;
	return m_items.count();
}

void CLC7NavBarGroup::paintEvent(QPaintEvent *e)
{
    QStyleOption opt;
    opt.init(this);
    QStylePainter p(this);
    p.drawPrimitive(QStyle::PE_Widget, opt);
}

CLC7NavBar *CLC7NavBarGroup::bar()
{
	return m_bar;
}

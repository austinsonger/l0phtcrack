#include"stdafx.h"

CLC7NavBar::CLC7NavBar(QStackedWidget *stackedwidget):QScrollArea(NULL), m_buttongroup(this)
{
	TR;
	m_stackedwidget=stackedwidget;

    installEventFilter(this);

	ILC7ColorManager *colman = CLC7App::getInstance()->GetMainWindow()->GetColorManager();

	m_buttongroup.setExclusive(true);

	m_groupwidget=new CLC7NavBarWidget(this);
	setWidget(m_groupwidget);
	
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setWidgetResizable(true);
	
	m_scrollupbutton=new QPushButton(this);
	m_scrollupbutton->setFixedHeight(16*colman->GetSizeRatio());
	m_scrollupbutton->setObjectName("scroll-up-button"); 
	m_scrollupbutton->setIcon(colman->GetMonoColorIcon(":/qss_icons/rc/up_arrow_disabled.png"));

	m_scrolldownbutton=new QPushButton(this);
	m_scrolldownbutton->setFixedHeight(16 * colman->GetSizeRatio());
	m_scrolldownbutton->setObjectName("scroll-down-button");
	m_scrolldownbutton->setIcon(colman->GetMonoColorIcon(":/qss_icons/rc/down_arrow_disabled.png"));
	
	m_scrollupbutton->setVisible(false);
	m_scrolldownbutton->setVisible(false);

	connect(m_scrollupbutton, &QAbstractButton::pressed, this, &CLC7NavBar::slot_scrollUpButtonPressed);
	connect(m_scrolldownbutton, &QAbstractButton::pressed, this, &CLC7NavBar::slot_scrollDownButtonPressed);
	connect(m_scrollupbutton, &QAbstractButton::released, this, &CLC7NavBar::slot_scrollUpButtonReleased);
	connect(m_scrolldownbutton, &QAbstractButton::released, this, &CLC7NavBar::slot_scrollDownButtonReleased);

	m_timer = new QTimer(this);
	connect(m_timer, SIGNAL(timeout()), this, SLOT(slot_doScrolling()));
	m_scrolling_up=false;
	m_scrolling_down=false;
}

void CLC7NavBar::slot_doScrolling(void)
{TR;
	if (m_scrolling_up)
	{
		this->verticalScrollBar()->setValue(this->verticalScrollBar()->value() - 5);
	}
	if (m_scrolling_down)
	{
		this->verticalScrollBar()->setValue(this->verticalScrollBar()->value() + 5);
	}
}

void CLC7NavBar::slot_scrollUpButtonPressed(void)
{TR;
	m_scrolling_up = true;
	m_timer->start(20);
}

void CLC7NavBar::slot_scrollDownButtonPressed(void)
{TR;
	m_scrolling_down = true;
	m_timer->start(20);
}

void CLC7NavBar::slot_scrollUpButtonReleased(void)
{TR;
	m_timer->stop();
	m_scrolling_up = false;
}

void CLC7NavBar::slot_scrollDownButtonReleased(void)
{TR;
	m_timer->stop();
	m_scrolling_down = false;
}

void CLC7NavBar::resizeEvent(QResizeEvent *evt)
{TR;
	QSize sz=evt->size();
	m_scrollupbutton->resize(200,16);
	m_scrollupbutton->move(0,0);
	m_scrolldownbutton->resize(200, 16);
	m_scrolldownbutton->move(0,sz.height()-3);
}

bool CLC7NavBar::eventFilter(QObject *object, QEvent *event)
{
    if(object==this && event->type()==QEvent::Enter)
	{
		QSize sz=size();
		if(sz.height()<m_groupwidget->height())
		{
			m_scrollupbutton->setVisible(true);
			m_scrolldownbutton->setVisible(true);
		}

		return true;
	}
	else if(object==this && event->type()==QEvent::Leave)
	{
		m_scrollupbutton->setVisible(false);
		m_scrolldownbutton->setVisible(false);
		return true;
	}

	return QScrollArea::eventFilter(object,event);
}

CLC7NavBarGroup *CLC7NavBar::addGroup(QString groupname)
{TR;
	CLC7NavBarGroup *group=new CLC7NavBarGroup(this);
	if(!groupname.isEmpty())
	{
		m_groups_by_name[groupname]=group;
	}
	m_groups.append(group);

	QVBoxLayout *layout=(QVBoxLayout *)(m_groupwidget->layout());
	layout->insertWidget(-1,group);

	hide();
	show();

	return group;
}

CLC7NavBarGroup *CLC7NavBar::insertGroup(int pos, QString groupname)
{TR;
	CLC7NavBarGroup *group=new CLC7NavBarGroup(this);
	if(!groupname.isEmpty())
	{
		m_groups_by_name[groupname]=group;
	}
	m_groups.insert(pos,group);

	QVBoxLayout *layout=(QVBoxLayout *)(m_groupwidget->layout());
	layout->insertWidget(pos,group);

	hide();
	show();

	return group;
}

void CLC7NavBar::removeGroup(int pos)
{TR;
	CLC7NavBarGroup *group=m_groups[pos];
	QString key=m_groups_by_name.key(group);
	if(!key.isEmpty())
	{
		m_groups_by_name.remove(key);
	}
	m_groups.removeAt(pos);
	
	QVBoxLayout *layout=(QVBoxLayout *)(m_groupwidget->layout());
	layout->removeWidget(group);
	delete group;

	hide();
	show();
}

void CLC7NavBar::removeGroup(CLC7NavBarGroup *group)
{TR;
	int pos=m_groups.indexOf(group);
	QString key=m_groups_by_name.key(group);
	if(!key.isEmpty())
	{
		m_groups_by_name.remove(key);
	}
	m_groups.removeAt(pos);
	
	QVBoxLayout *layout=(QVBoxLayout *)(m_groupwidget->layout());
	layout->removeWidget(group);

	delete group;

	hide();
	show();
}

void CLC7NavBar::removeGroup(QString groupname)
{TR;
	if(m_groups_by_name.contains(groupname))
	{
		CLC7NavBarGroup *group=m_groups_by_name[groupname];
		int pos=m_groups.indexOf(group);
		QString key=m_groups_by_name.key(group);
		if(!key.isEmpty())
		{
			m_groups_by_name.remove(key);
		}
		m_groups.removeAt(pos);
		
		QVBoxLayout *layout=(QVBoxLayout *)(m_groupwidget->layout());
		layout->removeWidget(group);

		delete group;

		hide();
		show();
	}
}

CLC7NavBarGroup *CLC7NavBar::group(QString groupname)
{TR;
	if(m_groups_by_name.contains(groupname))
	{
		CLC7NavBarGroup *group=m_groups_by_name[groupname];
		return group;
	}
	return NULL;
}

CLC7NavBarGroup *CLC7NavBar::group(int pos)
{TR;
	return m_groups[pos];
}

int CLC7NavBar::indexOf(CLC7NavBarGroup *group)
{TR;
	return m_groups.indexOf(group);
}

int CLC7NavBar::indexOf(QString groupname)
{TR;
	if(m_groups_by_name.contains(groupname))
	{
		CLC7NavBarGroup *group=m_groups_by_name[groupname];
		return m_groups.indexOf(group);
	}
	return -1;
}

CLC7NavBarItem *CLC7NavBar::item(QWidget *buddy)
{TR;
	foreach(CLC7NavBarGroup *group, m_groups)
	{
		CLC7NavBarItem *item=group->item(buddy);
		if(item)
		{
			return item;
		}
	}
	return NULL;
}
	
int CLC7NavBar::count()
{TR;
	return m_groups.count();
}

QStackedWidget *CLC7NavBar::stackedwidget()
{
	return m_stackedwidget;
}
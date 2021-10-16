#include"stdafx.h"

CLC7NavBarItem::CLC7NavBarItem(CLC7NavBarGroup *group, QWidget *buddy):QPushButton(NULL)
{TR;
	m_buddy=buddy;
	m_group=group;
	
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	setCheckable(true);
	setAutoExclusive(true);

	connect(this, &QAbstractButton::clicked, this, &CLC7NavBarItem::onClicked);
}

CLC7NavBarItem::~CLC7NavBarItem()
{TR;
	
}

QWidget *CLC7NavBarItem::buddy()
{
	return m_buddy;
}

CLC7NavBarGroup *CLC7NavBarItem::group()
{
	return m_group;
}

void CLC7NavBarItem::onClicked(bool checked)
{TR;
	m_group->bar()->stackedwidget()->setCurrentWidget(m_buddy);
}


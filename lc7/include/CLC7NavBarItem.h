#ifndef __INC_CLC7NAVBARITEM_H
#define __INC_CLC7NAVBARITEM_H

#include"qlabel.h"
#include"qpushbutton.h"
#include"qwidget.h"

class CLC7NavBarGroup;

class CLC7NavBarItem: public QPushButton
{
	Q_OBJECT;
private:

	QWidget *m_buddy;
	CLC7NavBarGroup *m_group;

	friend class CLC7NavBarGroup;

public:

	CLC7NavBarItem(CLC7NavBarGroup *group, QWidget *buddy);
	virtual ~CLC7NavBarItem();

	CLC7NavBarGroup *group();
	QWidget *buddy();

private slots:
	void onClicked(bool checked);

};


#endif
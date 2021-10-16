#ifndef __INC_CLC7NAVBARGROUP_H
#define __INC_CLC7NAVBARGROUP_H

#include"CLC7NavBarItem.h"
#include"CLC7NavBarHeader.h"

#include"qwidget.h"


class CLC7NavBar;

class CLC7NavBarGroup: public QWidget
{
	Q_OBJECT;
private:

	CLC7NavBar *m_bar;
	CLC7NavBarHeader *m_header;
	QVector<CLC7NavBarItem *> m_items;
	QMap<QWidget *, CLC7NavBarItem *> m_items_by_buddy;

	friend class CLC7NavBar;

	void paintEvent(QPaintEvent *e);

public:

	CLC7NavBarGroup(CLC7NavBar *bar);
	
	void setTitle(QString title);
	QString title();

	CLC7NavBar *bar();

	CLC7NavBarItem *addItem(QWidget *buddy);
	CLC7NavBarItem *insertItem(int pos, QWidget *buddy);
	void removeItem(int pos);
	void removeItem(CLC7NavBarItem *item);
	void removeItem(QWidget *buddy);
	int indexOf(CLC7NavBarItem *item);
	int indexOf(QWidget *buddy);
	CLC7NavBarItem *item(int pos);
	CLC7NavBarItem *item(QWidget *buddy);
	int count();
	
};


#endif


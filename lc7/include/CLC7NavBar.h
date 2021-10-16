#ifndef __INC_CLC7NAVBAR_H
#define __INC_CLC7NAVBAR_H

#include"CLC7NavBarGroup.h"

#include"qvector.h"
#include"qscrollarea.h"
#include"qwidget.h"
#include"qbuttongroup.h"


//#define DEBUG_BORDERS


class CLC7NavBar: public QScrollArea
{
	Q_OBJECT;
private:
	friend class CLC7NavBarGroup;
	friend class CLC7NavBarItem;

	QButtonGroup m_buttongroup;
	QMap<QString, CLC7NavBarGroup *> m_groups_by_name;
	QVector<CLC7NavBarGroup *> m_groups;
	QWidget *m_groupwidget;
	QStackedWidget *m_stackedwidget;
	QAbstractButton *m_scrollupbutton;
	QAbstractButton *m_scrolldownbutton;
	bool m_scrolling_up;
	bool m_scrolling_down;
	QTimer *m_timer;

	virtual void resizeEvent(QResizeEvent *evt);
	bool eventFilter(QObject *object, QEvent *event);

public slots:
	void slot_scrollUpButtonPressed(void);
	void slot_scrollDownButtonPressed(void);
	void slot_scrollUpButtonReleased(void);
	void slot_scrollDownButtonReleased(void);
	void slot_doScrolling(void);

public:

	CLC7NavBar(QStackedWidget *stackedwidget);

	QStackedWidget *stackedwidget();

	CLC7NavBarGroup *addGroup(QString groupname=QString());
	CLC7NavBarGroup *insertGroup(int pos, QString groupname=QString());
	CLC7NavBarGroup *group(QString groupname);
	CLC7NavBarGroup *group(int pos);
	
	CLC7NavBarItem *item(QWidget *buddy);
	
	void removeGroup(int pos);
	void removeGroup(CLC7NavBarGroup *group);
	void removeGroup(QString groupname);
	int indexOf(CLC7NavBarGroup *group);
	int indexOf(QString groupname);
	
	int count();


};

#endif

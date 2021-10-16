#ifndef __INC_CLC7NAVBARWIDGET_H
#define __INC_CLC7NAVBARWIDGET_H

#include"qwidget.h"

class CLC7NavBarWidget: public QWidget
{
	Q_OBJECT;
private:

	void paintEvent(QPaintEvent *e);

public:

	CLC7NavBarWidget(QWidget *parent);
	
};

#endif


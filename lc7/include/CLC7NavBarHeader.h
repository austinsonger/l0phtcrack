#ifndef __INC_CLC7NAVBARHEADER_H
#define __INC_CLC7NAVBARHEADER_H

#include"qlabel.h"
#include"qpushbutton.h"
#include"qwidget.h"

class CLC7NavBarGroup;

class CLC7NavBarHeader: public QLabel
{
	Q_OBJECT;
private:
	CLC7NavBarGroup *m_group;

public:

	CLC7NavBarHeader(CLC7NavBarGroup *group);
	virtual ~CLC7NavBarHeader();
};


#endif
#ifndef __INC_CLC7MENUWIDGET_H
#define __INC_CLC7MENUWIDGET_H

#include "ui_LC7MenuWidget.h"

class CLC7MenuWidget:public QWidget
{
	Q_OBJECT
private:
	Ui::LC7MenuWidget ui;

private slots:
	void slot_newSessionButton();
	void slot_openSessionButton();
	void slot_saveSessionButton();
	void slot_saveSessionAsButton();
	void slot_quitButton();

public:
	CLC7MenuWidget(QWidget *parent);
	virtual ~CLC7MenuWidget();
};

#endif
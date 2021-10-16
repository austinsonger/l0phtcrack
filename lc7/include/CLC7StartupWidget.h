#ifndef __INC_CLC7STARTUPWIDGET_H
#define __INC_CLC7STARTUPWIDGET_H

#include "qwidget.h"
#include "ui_LC7Startup.h"

class ILC7Controller;

class CLC7StartupWidget: public QFrame
{
	Q_OBJECT
private:
	Ui::LC7StartupWidget ui;
	ILC7Controller *m_ctrl;

public:
	CLC7StartupWidget(QList<ILC7Action *> custom_actions, QWidget *parent, ILC7Controller *ctrl);
	virtual ~CLC7StartupWidget();

	virtual void RecolorCallback(void);

protected:
	virtual void keyPressEvent(QKeyEvent *event);
	
	void addWizardAction(ILC7Action *act);

private slots:
	void onShowDialogCheckbox(int);
	void onNewSessionButton(bool checked);
	void onOpenSessionButton(bool checked);
	void onRecentSessionsDoubleClicked(QListWidgetItem *item);
	void onCloseButton(bool checked);
	void slot_wizardButton_clicked(bool checked);

signals:
	void sig_newSession();
	void sig_openSession(QString filename);
	void sig_closeWindow();

};

#endif
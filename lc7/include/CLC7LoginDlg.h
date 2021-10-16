#ifndef __INC_CLC7LOGINDLG_H
#define __INC_CLC7LOGINDLG_H

#include"ui_LC7LoginDlg.h"

class CLC7LoginDlg :public QDialog
{
	Q_OBJECT
private:

	ILC7Controller *m_ctrl;
	Ui::LC7LoginDlg ui;

	QString m_username;
	QString m_password;

	void UpdateUI();

public:

	CLC7LoginDlg(QWidget *parent, ILC7Controller *ctrl, QString title, QString message, QString username, QString password);
	virtual ~CLC7LoginDlg();

	QString Username();
	QString Password();
	
};

#endif
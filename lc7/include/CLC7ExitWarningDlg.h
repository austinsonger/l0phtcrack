#ifndef __INC_CLC7ExitWarningDlg_H
#define __INC_CLC7ExitWarningDlg_H

#include "ui_LC7ExitWarningDlg.h"

class CLC7ExitWarningDlg:public QDialog
{
	Q_OBJECT
private:
	Ui::LC7ExitWarningDlg ui;
	int m_seconds;
	QTimer m_timer;

private slots:
	void slot_tick(void);

protected:

	void UpdateUI();

public:
	CLC7ExitWarningDlg(QObject *parent, int seconds);
	virtual ~CLC7ExitWarningDlg();

};

#endif
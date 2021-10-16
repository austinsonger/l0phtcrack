#ifndef __INC_CLC7ProgressBox_H
#define __INC_CLC7ProgressBox_H

#include "ui_LC7ProgressDialog.h"

class CLC7ProgressBox:public QDialog, public ILC7ProgressBox
{
	Q_OBJECT
private:
	Ui::LC7ProgressDialog ui;
	bool m_is_cancelled;
	bool m_can_cancel;

signals:
	void sig_updateStatusText(QString text);
	void sig_updateProgressBar(quint32 cur);

private slots:
	void slot_cancelButton();
	void slot_updateStatusText(QString text);
	void slot_updateProgressBar(quint32 cur);

public:
	CLC7ProgressBox(QString title, QString status, quint32 progresscur, quint32 progressmax, bool can_cancel=true);
	virtual ~CLC7ProgressBox();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	virtual void UpdateStatusText(QString text);
	virtual void UpdateProgressBar(quint32 cur);
	virtual bool IsCancelled();
	virtual void Release();
};

#endif
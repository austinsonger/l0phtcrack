#ifndef __INC_CLC7AUTOSAVEDLG_H
#define __INC_CLC7AUTOSAVEDLG_H

#include"ui_LC7AutosaveDlg.h"

class CLC7AutosaveDlg :public QDialog
{
	Q_OBJECT
private:

	Ui::LC7AutosaveDlg ui;
	ILC7Controller *m_ctrl;
	QString m_autosaveSessionPath;

protected:

	void UpdateUI();
	void RefreshContent();

public:
	
	CLC7AutosaveDlg(ILC7Controller *ctrl, QWidget *parent = NULL);
	virtual ~CLC7AutosaveDlg();

	QString autosaveSessionPath();

public slots:

	void slot_autosavedSessionTable_currentItemChanged(QTableWidgetItem * current, QTableWidgetItem * previous);
	void slot_autosavedSessionTable_cellDoubleClicked(int row, int column);
	void slot_restoreButton_clicked(bool checked);
	void slot_closeButton_clicked(bool checked);
	void slot_deleteButton_clicked(bool checked);
	void slot_deleteAllButton_clicked(bool checked);
};

#endif
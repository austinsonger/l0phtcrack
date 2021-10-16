#ifndef __INC_GENERATEREMOTEAGENTDLG_H
#define __INC_GENERATEREMOTEAGENTDLG_H

#include"ui_generateremoteagent.h"

class GenerateRemoteAgentDlg: public QDialog
{
	Q_OBJECT;

private:
	Ui::GenerateRemoteAgentDialog ui;

	void UpdateUI(void);

	void DoGenerateRemoteAgent(QString location);

public:

	GenerateRemoteAgentDlg(QWidget *parent=0);
	~GenerateRemoteAgentDlg();
	
public slots:
	void slot_browseButton_clicked(int checked);
	void slot_generateButton_clicked(int checked);
	void slot_remoteAgentEdit_textChanged(const QString &text);

};

#endif
#ifndef __INC_CPAGEWINDOWSIMPORT_H
#define __INC_CPAGEWINDOWSIMPORT_H

class CPageWindowsImport : public CLC7WizardPage
{
	Q_OBJECT

public:
	CPageWindowsImport(QWidget *parent = 0);

	int nextId() const;
	bool isComplete() const;
	void UpdateUI(void);

public slots:
	void slot_linkActivated_createRemoteAgent(const QString &link);

private:
	QButtonGroup m_buttongroup;
	CLabelRadioButton *m_importLocalRadioButton;
	CLabelRadioButton *m_importRemoteSMBRadioButton;
	CLabelRadioButton *m_importPWDumpRadioButton;
};

#endif
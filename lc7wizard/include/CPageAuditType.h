#ifndef __INC_CPAGEAUDITTYPE_H
#define __INC_CPAGEAUDITTYPE_H

class CPageAuditType : public CLC7WizardPage
{
	Q_OBJECT

public:
	CPageAuditType(QWidget *parent = 0);

	int nextId() const;
	bool isComplete() const;
	void UpdateUI(void);

private:
	QButtonGroup m_buttongroup;
	CLabelRadioButton *m_quickRadioButton;
	CLabelRadioButton *m_commonRadioButton;
	CLabelRadioButton *m_thoroughRadioButton;
	CLabelRadioButton *m_strongRadioButton;
	QTextEdit *m_configurationEdit;

private slots:
	void slot_buttonClicked_buttongroup(QAbstractButton *);

		
};

#endif
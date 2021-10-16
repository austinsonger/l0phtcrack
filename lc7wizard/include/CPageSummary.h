#ifndef __INC_CPAGESUMMARY_H
#define __INC_CPAGESUMMARY_H

class CPageSummary : public CLC7WizardPage
{
	Q_OBJECT

public:
	CPageSummary(QWidget *parent = 0);

	void initializePage();
	int nextId() const;
	bool isComplete() const;
	void UpdateUI(void);

private:

	QTextEdit *m_actionsEdit;
};

#endif
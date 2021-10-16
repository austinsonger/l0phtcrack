#ifndef __INC_CPAGEWINDOWSORUNIX_H
#define __INC_CPAGEWINDOWSORUNIX_H

class CPageWindowsOrUnix : public CLC7WizardPage
{
	Q_OBJECT

public:
	CPageWindowsOrUnix(QWidget *parent = 0);

	int nextId() const;
	bool isComplete() const;
	void UpdateUI(void);

private:
	QButtonGroup m_buttongroup;
	CLabelRadioButton *m_windowsRadioButton;
	CLabelRadioButton *m_unixRadioButton;
};

#endif
#ifndef __INC_CPAGEUNIXIMPORT_H
#define __INC_CPAGEUNIXIMPORT_H

class CPageUnixImport : public CLC7WizardPage
{
	Q_OBJECT

public:
	CPageUnixImport(QWidget *parent = 0);

	int nextId() const;
	bool isComplete() const;
	void UpdateUI(void);

private:
	QButtonGroup m_buttongroup;
	CLabelRadioButton *m_importSSHRadioButton;
	CLabelRadioButton *m_importShadowRadioButton;

};

#endif
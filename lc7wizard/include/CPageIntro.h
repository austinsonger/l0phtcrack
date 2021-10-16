#ifndef __INC_CPAGEINTRO_H
#define __INC_CPAGEINTRO_H

class CPageIntro : public CLC7WizardPage
{
	Q_OBJECT

public:
	CPageIntro(QWidget *parent = 0);

	int nextId() const;
	bool isComplete() const;
	void UpdateUI(void);

private:

};

#endif
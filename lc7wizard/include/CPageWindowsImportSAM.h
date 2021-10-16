#ifndef __INC_CPAGEWINDOWSIMPORTSAM_H
#define __INC_CPAGEWINDOWSIMPORTSAM_H

class CPageWindowsImportSAM : public CLC7WizardPage
{
	Q_OBJECT

public:
	CPageWindowsImportSAM(QWidget *parent = 0);

	int nextId() const;
	bool isComplete() const;
	void UpdateUI(void);

private:

};

#endif
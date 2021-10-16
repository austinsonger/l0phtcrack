#ifndef __INC_CPAGEWINDOWSIMPORTLOCAL_H
#define __INC_CPAGEWINDOWSIMPORTLOCAL_H

class CPageWindowsImportLocal : public CLC7WizardPage
{
	Q_OBJECT

public:
	CPageWindowsImportLocal(QWidget *parent = 0);

	int nextId() const;
	void UpdateUI(void);

private:

};

#endif
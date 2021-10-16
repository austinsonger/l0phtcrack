#ifndef __INC_CPAGEWINDOWSIMPORTPWDUMP_H
#define __INC_CPAGEWINDOWSIMPORTPWDUMP_H

class CPageWindowsImportPWDump : public CLC7WizardPage
{
	Q_OBJECT

public:
	CPageWindowsImportPWDump(QWidget *parent = 0);

	int nextId() const;
	void UpdateUI(void);

private:

};

#endif
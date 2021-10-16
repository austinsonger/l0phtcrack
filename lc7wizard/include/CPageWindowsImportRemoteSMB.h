#ifndef __INC_CPAGEWINDOWSIMPORTREMOTESMB_H
#define __INC_CPAGEWINDOWSIMPORTREMOTESMB_H

class CPageWindowsImportRemoteSMB : public CLC7WizardPage
{
	Q_OBJECT

public:
	CPageWindowsImportRemoteSMB(QWidget *parent = 0);

	int nextId() const;
	void UpdateUI(void);

private:

};

#endif
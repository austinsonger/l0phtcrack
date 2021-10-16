#ifndef __INC_CPAGEWINDOWSIMPORTREMOTERDP_H
#define __INC_CPAGEWINDOWSIMPORTREMOTERDP_H

class CPageWindowsImportRemoteRDP : public CLC7WizardPage
{
	Q_OBJECT

public:
	CPageWindowsImportRemoteRDP(QWidget *parent = 0);

	int nextId() const;
	void UpdateUI(void);

private:

};

#endif
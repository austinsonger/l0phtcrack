#ifndef __INC_CPAGEUNIXIMPORTSSH_H
#define __INC_CPAGEUNIXIMPORTSSH_H

class CPageUnixImportSSH : public CLC7WizardPage
{
	Q_OBJECT

public:
	CPageUnixImportSSH(QWidget *parent = 0);
	~CPageUnixImportSSH();

	int nextId() const;
	void UpdateUI(void);

private:

};

#endif
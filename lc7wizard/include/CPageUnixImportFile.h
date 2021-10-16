#ifndef __INC_CPAGEUNIXIMPORTFILE_H
#define __INC_CPAGEUNIXIMPORTFILE_H

class CPageUnixImportFile : public CLC7WizardPage
{
	Q_OBJECT

public:
	CPageUnixImportFile(QWidget *parent = 0);
	~CPageUnixImportFile();

	int nextId() const;
	void UpdateUI(void);

private:

};

#endif
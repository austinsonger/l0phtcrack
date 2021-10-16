#ifndef __INC_CLC7WIZARD_H
#define __INC_CLC7WIZARD_H

class CLC7Wizard: public QWizard
{
	Q_OBJECT

public:
	enum {
		Page_Intro, 
		Page_Windows_Or_Unix, 
		Page_Windows_Import, 
		Page_Windows_Import_Local, 
		Page_Windows_Import_Remote_SMB, 
		Page_Windows_Import_PWDump,
		Page_Windows_Import_SAM,
		Page_Unix_Import,
		Page_Unix_Import_File,
		Page_Unix_Import_SSH,
		Page_Audit_Type,
		Page_Reporting,
		Page_Scheduling,
		Page_Summary
	};

	CLC7Wizard(QWidget *parent = 0);

protected:
	void showEvent(QShowEvent *ev);

signals:
	void window_loaded(void);

private slots:
	void showHelp();
	void slot_window_loaded(void);
};

#endif
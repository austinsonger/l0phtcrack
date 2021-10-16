#ifndef __INC_CLC7JTRCONSOLE_H
#define __INC_CLC7JTRCONSOLE_H

#include <QWidget>
#include"ui_jtrconsole.h"
#include"CLC7ExecuteJTR.h"

class ExitCodeThread :public QThread
{
	Q_OBJECT;
protected:
	int m_exitcode;

public:

	ExitCodeThread()
	{
		m_exitcode = -1;
	}

	int exitCode() 
	{
		return m_exitcode;
	}
};

class CLC7JTRConsole : public QDialog
{
	Q_OBJECT;

public:

	CLC7JTRConsole();
	~CLC7JTRConsole();

	int execGenerateCharset(QString charset_file, QByteArray inchars, QStringList dictionary_files, QStringList pot_files);

private:
	Ui::JTRConsole ui;
	ExitCodeThread *m_executing_thread;

	void UpdateUI();

public slots:
	
	void slot_consoleOut(QString str);
	void slot_threadFinished();

};

class GenerateCharsetThread :public ExitCodeThread, public ILC7ExecuteJTRHandler
{
	Q_OBJECT;

public:
	QString m_charset_file;
	QByteArray m_inchars;
	QStringList m_dictionary_files;
	QStringList m_pot_files;
	CLC7JTRConsole *m_console;

signals:
	void sig_consoleOut(QString str);
	void sig_update();

public:
	GenerateCharsetThread(CLC7JTRConsole *console);
	virtual void run(void);
	virtual void ProcessStdOut(QByteArray str);
	virtual void ProcessStdErr(QByteArray str);
};


#endif

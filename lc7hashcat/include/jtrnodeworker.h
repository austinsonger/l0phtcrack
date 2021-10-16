#ifndef __INC_JTRNODEWORKER_H
#define __INC_JTRNODEWORKER_H

#include"CLC7ExecuteJTR.h"


class CJTRNodeWorker : public QThread, public ILC7ExecuteJTRHandler
{
    Q_OBJECT

public:
	CJTRNodeWorker(JTRWORKERCTX *ctx, JTRPASS *pass, int passnode, bool restore, QString potfilename, QString sessionfilename, QString hashesfilename);
    virtual ~CJTRNodeWorker();

	virtual void run();
	bool preflight(CLC7ExecuteJTR::PREFLIGHT & preflight);

	void stop(bool timeout);
	bool lasterror(QString &err);
	bool fallbackretry();
	int passnode();
	JTRDLL_STATUS get_status();

protected:
	virtual void ProcessStdOut(QByteArray str);
	virtual void ProcessStdErr(QByteArray str);

private:

	JTRWORKERCTX *m_ctx;
	JTRPASS *m_pass;
	int m_passnode;
	bool m_restore;
	QString m_potfilename;
	QString m_sessionfilename;
	QString m_hashesfilename;
	bool m_fallbackretry;
	
	QStringList m_args;

	CLC7ExecuteJTR *m_exejtr;

	bool m_is_error;
	QString m_error;
	
	QList<unsigned int> m_etas;

protected:

	bool ExecuteJTRCommandLine();
	bool VerifyRestoreSession();
	bool GenerateCommandLine(bool preflight);

	void set_error(QString err);

};


#endif
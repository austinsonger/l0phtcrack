#ifndef __INC_CLC7EXECUTEJTR_H
#define __INC_CLC7EXECUTEJTR_H

class CLC7JTREXEDLL; 

class ILC7ExecuteJTRHandler
{
public:
	virtual ~ILC7ExecuteJTRHandler() {}
	virtual void ProcessStdOut(QByteArray str)=0;
	virtual void ProcessStdErr(QByteArray str)=0;
};



class CLC7ExecuteJTR
{
public:
	
	struct PREFLIGHT
	{
		bool valid;
		quint32 salt_count;
		quint32 wordlist_rule_count;
		//quint64 wordlist_candidates;
		quint64 mask_candidates;
		quint64 incremental_candidates;
	};

private:

	QString m_command;
	QStringList m_args;
	QString m_extra_opencl_kernel_args;

	CLC7JTREXEDLL *m_jtrdll;
	bool m_illegal_instruction;

	void CheckCache(QDir cachedir);

public:
	CLC7ExecuteJTR(QString version);
	~CLC7ExecuteJTR();

	bool IsValid();
	bool HadIllegalInstruction();

	void SetCommandLine(QStringList args, QString extra_opencl_kernel_args = QString());
	
	int ExecuteWait(QString & out, QString &err);
	int ExecutePipe(ILC7ExecuteJTRHandler *handler);
	int ExecutePipeGuarded(ILC7ExecuteJTRHandler *handler);

	void Preflight(PREFLIGHT &preflight);

	void Abort(bool timeout);
	JTRDLL_STATUS GetStatus(void);

	void Terminate(void);


};

#endif
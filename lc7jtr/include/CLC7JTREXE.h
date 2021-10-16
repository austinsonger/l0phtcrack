#ifndef __INC_CLC7JTREXE_H
#define __INC_CLC7JTREXE_H

#include"jtrdll.h"

class CLC7JTREXE:public QObject
{
	Q_OBJECT
private:

	QMutex m_reqcmd_mutex;
	QMutex m_responses_mutex;
	quint32 m_cmdid;
	QString m_jtrdllversion;
	QString m_jtrexepath;
	QProcess m_jtrprocess;
	QMap<quint32, QByteArray> m_responses;

#ifdef _WIN32
	typedef HANDLE PIPETYPE;
#endif

	QByteArray RequestCommand(QString cmdstr);
	PIPETYPE OpenControlPipe(QString pipename);
	void CloseControlPipe(PIPETYPE pipe);
	bool readPipe(PIPETYPE pipe, size_t length, void *data);
	bool waitForCommand(PIPETYPE pipe, QString &cmd, QByteArray &data);

public:
	CLC7JTREXE(QString jtrdllversion);
	~CLC7JTREXE();

	bool IsValid();

	int main(int argc, char **argv, struct JTRDLL_HOOKS *hooks);
	void abort(bool timeout);
	void terminate();
	void get_status(struct JTRDLL_STATUS *jtrdllstatus);
	int get_charset_info(const char *path, unsigned char * charmin, unsigned char *charmax, unsigned char *len, unsigned char *count, unsigned char allchars[256]);
	void preflight(int argc, char **argv, struct JTRDLL_HOOKS *hooks, struct JTRDLL_PREFLIGHT *jtrdllpreflight);
	void set_extra_opencl_kernel_args(const char *extra_opencl_kernel_args);

signals:
	void sig_RequestCommand(QString cmdstr, QByteArray & ret);

public slots:

	void slot_readyRead(void);
	void slot_RequestCommand(QString cmdstr, QByteArray & ret);
};


#endif
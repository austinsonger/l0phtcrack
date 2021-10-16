#ifndef __INC_UNIXSSHIMPORTER_H
#define __INC_UNIXSSHIMPORTER_H

#include "libssh2.h"

class UnixSSHImporter
{
public:
	enum AUTHTYPE {
		PASSWORD = 0,
		PUBLICKEY = 1
	};
	enum ELEVTYPE {
		NOELEVATION = 0,
		SUDO = 1,
		SU = 2
	};

protected:

	ILC7AccountList *m_accountlist;
	ILC7CommandControl *m_ctrl;

	bool m_include_non_login;
	QString m_host;
	int m_port;
	QString m_username;
	AUTHTYPE m_authtype;
	QString m_password;
	QString m_privkeyfile;
	QString m_privkeypassword;

	ELEVTYPE m_elevtype;
	QString m_sudopassword;
	QString m_supassword;

	LC7Remediations m_remediations;
	QList<int> m_accounts_to_force_change;
	QList<int> m_accounts_to_lockout;
	QList<int> m_accounts_to_disable;

	quint32 m_account_limit;

	SOCKET m_socket;
	LIBSSH2_SESSION *m_ssh;
	LIBSSH2_CHANNEL *m_channel;

	enum SYSTEM_TYPE
	{
		ST_UNKNOWN=0,
		ST_LINUX=1,
		ST_BSD=2,
		ST_SOLARIS=3,
		ST_AIX=4
	};
	SYSTEM_TYPE m_systemtype;

	enum SYSTEM_VARIANT_TYPE
	{
		ST_DEFAULT=0,
		ST_OPENBSD=1,
		ST_FREEBSD=2,
	};
	SYSTEM_VARIANT_TYPE m_systemvarianttype;

	void UpdateStatus(QString statustext, quint32 cur, bool statuslog = true);

	void gracefulTerminate();
	void connectAndAuthenticate(void);
	void openChannel();
	QString getLastError(void);
	QString receive_until_match(QByteArrayList matches, const char *error);
	void write_string(QByteArray str, const char *error);
	QString receiveCommandOutput(QString command);
	QString receiveSUCommandOutput(QString command);
	QString receiveSUDOCommandOutput(QString command);
	QString receiveElevatedCommandOutput(QString command);
	void getSystemType();

	QString getRemediationLine(int remediationtype, QString username);
	void generateRemediationScript(QString & remediationscript);
	void writeRemoteFile(QString remotefilename, QString contents);
	
	
public:
	UnixSSHImporter(ILC7AccountList *accountlist, ILC7CommandControl *ctrl);
	~UnixSSHImporter();

	void setRemediations(const LC7Remediations &remediations);
	void setAccountsToRemediate(QList<int> accounts_to_disable, QList<int> accounts_to_force_change, QList<int> accounts_to_lockout);

	void setHost(QString host);
	void setIncludeNonLogin(bool include_non_login);
	void setAuthType(AUTHTYPE authtype);
	void setUsername(QString username);
	void setPassword(QString password);
	void setPrivateKeyFile(QString privatekeyfile);
	void setPrivateKeyPassword(QString privatekeypassword);
	void setElevType(ELEVTYPE elevtype);
	void setSUDOPassword(QString sudopassword);
	void setSUPassword(QString supassword);
	void SetAccountLimit(quint32 alim);

	bool TestCredentials(QList<FOURCC> &hashtypes, QString &error, bool &cancelled);
	bool DoImport(FOURCC hashtype, QString & error, bool & cancelled);
	bool DoRemediate(QString & error, bool & cancelled);
};

#endif
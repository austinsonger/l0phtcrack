#ifndef __INC_CLC7UnixImporter
#define __INC_CLC7UnixImporter

class CLC7UnixImporter:public ILC7UnixImporter
{
protected:

	ILC7AccountList *m_accountlist;
	ILC7CommandControl *m_ctrl;

	struct ACCT {
		DWORD valid_fields;
#define FIELD_USERNAME (1 << 0)
		QString username;
#define FIELD_PASSWORD (1 << 1)
		QByteArray password;
#define FIELD_UID (1 << 2)
		quint32 uid;
#define FIELD_GID (1 << 3)
		quint32 gid;
#define FIELD_USERINFO (1 << 4)
		QString userinfo;
#define FIELD_HOMEDIR (1 << 5)
		QString homedir;
#define FIELD_SHELL (1 << 6)
		QString shell;
#define FIELD_LAST_CHANGED_DATE (1 << 7)
		quint64 last_changed_date;				// in secs since epoch
#define FIELD_DAYS_LEFT_UNTIL_CHANGE_ALLOWED (1 << 8)
		qint32 days_left_until_change_allowed;
#define FIELD_DAYS_LEFT_UNTIL_PASSWORD_EXPIRES (1 << 9)
		qint32 days_left_until_password_expires;
#define FIELD_WARNING_DAYS_FOR_PASSWORD_EXPIRE (1 << 10)
		qint32 warning_days_for_password_expire;
#define FIELD_DISABLE_DAYS_AFTER_PASSWORD_EXPIRE (1 << 11)
		qint32 disable_days_after_password_expire;
#define FIELD_PASSWORD_EXPIRED_DATE (1 << 12)
		quint64 password_expired_date;			// in secs since epoch
#define FIELD_ACCOUNT_EXPIRED_DATE (1 << 13)
		quint64 account_expired_date;			// in secs since epoch
#define FIELD_LOCKED_OUT (1 << 14)
		bool locked_out;
#define FIELD_PASSWORD_MAX_AGE (1 << 15)
		bool password_max_age;					// in weeks
#define FIELD_ADMCHG (1 << 17)
		bool admchg;		// must change password, forced by admin
	};

	void UpdateStatus(QString statustext, quint32 cur, bool statuslog);
	bool CountLinesColonFile(QString filename, int fields, int & linecount, QString &error);
	bool ParseColonFile(QString filename, int fields, QList<QList<QString>> & lines, QString &error);
	void MergeAccounts(ACCT *acct1, ACCT *acct2);

	CLC7UnixImporter(ILC7AccountList *accountlist, ILC7CommandControl *ctrl);
	~CLC7UnixImporter();

public:

	bool ImportPasswdFile(QString filename, QList<ACCT> & accts, QString &error, bool &cancelled);
	bool ImportShadowFile(QString filename, QList<ACCT> & accts, QString &error, bool &cancelled);

};

#endif
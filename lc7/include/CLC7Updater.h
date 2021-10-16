#ifndef __INC_CLC7UPDATER_H
#define __INC_CLC7UPDATER_H


class CLC7Updater:public QThread
{
	Q_OBJECT
private:

	ILC7Controller *m_ctrl;

	QString m_url;
	QString m_sha256digest;
	QString m_new_version;
	QString m_current_version;
	QString m_md5digest;
	QString m_releasenotes;

	bool m_manual_check;

public:

	CLC7Updater(ILC7Controller *ctrl);
	~CLC7Updater();

	void run(void);
	void setManualCheck();
	void doUpdate();

signals:
	void sig_doUpdate();

};


#endif
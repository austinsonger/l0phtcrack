#ifndef LC7SIGNERTASK_H
#define LC7SIGNERTASK_H

#include <QtCore>
#include"qcommandline.h"
#include"CkRsa.h"
#include"CkByteData.h"

class LC7SignerTask : public QObject
{
	Q_OBJECT

public:
	LC7SignerTask(QObject *parent);
	~LC7SignerTask();

public slots:
    void run();
    void parseError(const QString & name);
    void switchFound(const QString & name);
    void optionFound(const QString & name, const QVariant & value);
    void paramFound(const QString & name, const QVariant & value);

signals:
    void finished();

private:
	QCoreApplication *m_app;
	QCommandLine *m_cmdline;
	CkRsa m_rsa;
	bool m_verify;
	QString m_key_file;
	QString m_in_file;
	QString m_out_file;

	void do_verify();
	void do_sign();
	
	bool WriteSignedFile(QString out_file, QByteArray indata, QByteArray sig);
	bool ReadSignedFile(QString in_file, QByteArray &indata, QByteArray &sig);


};

#endif // LC7SIGNERTASK_H

#ifndef __INC_FILEDOWNLOADER_H
#define __INC_FILEDOWNLOADER_H

#include <QObject>
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

class CLC7FileDownloader : public QObject
{
	Q_OBJECT
public:
	CLC7FileDownloader(QObject *parent = 0);
	virtual ~CLC7FileDownloader();

	bool download(QUrl url, int msec_timeout, QString & error);
	QByteArray downloadedData() const;

signals:
	void downloaded();

private slots:
	void fileDownloaded(QNetworkReply* pReply);
	void timeout(void);

private:
	QNetworkAccessManager m_WebCtrl;
	QNetworkReply *m_pReply;
	QByteArray m_DownloadedData;
	QEventLoop m_loop;
	bool m_timedout;
	bool m_error;
	QString m_error_string;


};


#endif
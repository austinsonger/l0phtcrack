#include"stdafx.h"

CLC7FileDownloader::CLC7FileDownloader(QObject *parent) : QObject(parent)
{TR;
	m_pReply = NULL;
	m_timedout = false;
	m_error = false;

	connect(&m_WebCtrl, SIGNAL(finished(QNetworkReply*)),this, SLOT(fileDownloaded(QNetworkReply*)));
}

CLC7FileDownloader::~CLC7FileDownloader()
{ 
}

void CLC7FileDownloader::fileDownloaded(QNetworkReply* pReply) 
{TR;
	QNetworkReply::NetworkError err = pReply->error();
	if (err == QNetworkReply::NoError)
	{
		m_DownloadedData = pReply->readAll();
	}
	else
	{
		m_error = true;
		m_error_string = QString(QMetaEnum::fromType<QNetworkReply::NetworkError>().valueToKey(err));
	}

	pReply->deleteLater();
	m_pReply = NULL;

	m_loop.quit();
}

void CLC7FileDownloader::timeout(void)
{TR;
	if (!m_pReply)
	{
		return;
	}

	m_pReply->abort();

	m_pReply->deleteLater();
	m_pReply = NULL;

	m_timedout = true;

	m_loop.quit();
}


QByteArray CLC7FileDownloader::downloadedData() const
{TR;
	return m_DownloadedData;
}

bool CLC7FileDownloader::download(QUrl url, int msec_timeout, QString & error)
{TR;
	QNetworkRequest request(url);
	m_pReply = m_WebCtrl.get(request);
	if (msec_timeout > 0)
	{
		QTimer::singleShot(msec_timeout, this, SLOT(timeout()));
	}

	m_loop.exec();

	if (m_timedout)
	{
		error = "Timed out.";
		return false;
	}
	if (m_error)
	{
		error = QString("%1").arg(m_error_string);
		return false;
	}

	return true;
}

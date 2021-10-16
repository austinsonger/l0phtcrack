#include"stdafx.h"
#include<qnetworkproxy.h>
#include<qnetworkrequest.h>

HelpNAM::HelpNAM(QNetworkAccessManager *manager, QObject *parent)
    : QNetworkAccessManager(parent)
{
    setCache(manager->cache());
    setCookieJar(manager->cookieJar());
    setProxy(manager->proxy());
    setProxyFactory(manager->proxyFactory());
}

QNetworkReply *HelpNAM::createRequest(
    QNetworkAccessManager::Operation operation, const QNetworkRequest &request,
    QIODevice *device)
{TR;
	if (request.url().scheme() != "help")
		return NULL;

	QNetworkRequest newrequest(request);
	QUrl url = newrequest.url();

	// Ensure absolute path
	QUrl rooturl("help:///");
	QString path = g_pLinkage->GetStartupDirectory() + "/help" + rooturl.resolved(url).path();

	// Add index.html if necessary
	if(QFileInfo(path).isDir())
	{
		while(path.endsWith("/"))
			path.chop(1);
		path += "/index.html";
	}

	if (!QFileInfo(path).exists())
	{
		path = g_pLinkage->GetStartupDirectory() + "/help/index.html";
	}
	

	url.setScheme("file");
	url.setPath(path);

	newrequest.setUrl(url);

	return QNetworkAccessManager::createRequest(operation, newrequest, device);
}

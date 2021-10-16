#ifndef NETWORKACCESSMANAGER_H
#define NETWORKACCESSMANAGER_H

#include <QNetworkAccessManager>

class HelpNAM : public QNetworkAccessManager
{
    Q_OBJECT

public:
	HelpNAM(QNetworkAccessManager *oldManager, QObject *parent = 0);

protected:    
    QNetworkReply *createRequest(Operation operation, const QNetworkRequest &request, QIODevice *device);
};

#endif

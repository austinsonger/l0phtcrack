#ifndef HELPBROWSER_H
#define HELPBROWSER_H

#include <QTextBrowser>
#include <QtHelp/qhelpengine.h>
#include <QDebug>

class HelpBrowser : public QTextBrowser
{
public:
	HelpBrowser(QHelpEngine* helpEngine, QWidget* parent = 0);
	QVariant loadResource(int type, const QUrl& name);

public slots:
	virtual void setSource(const QUrl &name);

private:
	QHelpEngine* helpEngine;
	QByteArray filterHTML(QByteArray html, QString urlstr);
	QByteArray filterCSS(QByteArray html, QString urlstr);

	
};

#endif
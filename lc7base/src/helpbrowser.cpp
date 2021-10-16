#include "helpbrowser.h"

HelpBrowser::HelpBrowser(QHelpEngine* helpEngine,
	QWidget* parent) :QTextBrowser(parent),
	helpEngine(helpEngine)
{
}

QByteArray HelpBrowser::filterHTML(QByteArray html, QString urlstr)
{
	QByteArray out;
	QString htmlstr = QString::fromUtf8(html);

	
	QRegExp re_br("<p [^>]*><span [^>]*><br/></span></p>");
	htmlstr = htmlstr.replace(re_br, "<br/>");

	QRegExp re_ulstyle("<ul style=\"[^\"]*\">");
	htmlstr = htmlstr.replace(re_ulstyle, "<ul>");
	
	out = htmlstr.toUtf8();


	return out;
}

QByteArray HelpBrowser::filterCSS(QByteArray css, QString urlstr)
{
	QByteArray out;
	if (urlstr.endsWith("hnd.css"))
	{
		QString cssstr = QString::fromUtf8(css);
		
		// Replace fonts with Verdana and Courier
		cssstr.replace("Arial", "Verdana");
		cssstr.replace("Tahoma", "Verdana");
		cssstr.replace("Helvetica", "Geneva");

		// Remove colors
		QRegExp re_color("color: [^;]*;");
		cssstr.replace(re_color,"");
		
		// Normalize lists
		QRegExp re_list1("text-indent: 0px; margin-left: [^;]*;");
		cssstr = cssstr.replace(re_list1, "text-indent: 0px; margin-left: 24px;");

		QRegExp re_list2("list-style-type: [^;]*;");
		cssstr = cssstr.replace(re_list2, "list-stype-type: disc;");

		out = cssstr.toUtf8();
	}
	else
	{
		out = css;
	}

	return out;
}

QVariant HelpBrowser::loadResource(int type, const QUrl &name)
{
	if (name.scheme() == "qthelp")
	{
		QString urlstr = name.toString();
		QByteArray filedata = helpEngine->fileData(name);

		if (urlstr.endsWith(".html"))
		{
			filedata = filterHTML(filedata, urlstr);
		}
		else if (urlstr.endsWith(".css"))
		{
			filedata = filterCSS(filedata, urlstr);
		}

		return QVariant(filedata);
	}
	
	Q_ASSERT(0);
	return QTextBrowser::loadResource(type, name);
}

void HelpBrowser::setSource(const QUrl &name)
{
	if (name.scheme() == "qthelp")
	{
		QTextBrowser::setSource(name);
	}
	else 
	{
		QDesktopServices::openUrl(name);
	}
}

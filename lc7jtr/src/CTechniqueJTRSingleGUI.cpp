#include<stdafx.h>

CTechniqueJTRSingleGUI::CTechniqueJTRSingleGUI()
{TR;
}

CTechniqueJTRSingleGUI::~CTechniqueJTRSingleGUI()
{TR;
}


ILC7Interface *CTechniqueJTRSingleGUI::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Component")
	{
		return this;
	}
	return NULL;
}

QUuid CTechniqueJTRSingleGUI::GetID()
{TR;
	return UUID_TECHNIQUEJTRSINGLEGUI;
}

ILC7Component::RETURNCODE CTechniqueJTRSingleGUI::ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl)
{TR;
	if (command == "gui" && args[0] == "create")
	{
		//QWidget *parent=(QWidget *)(config["parentwidget"].toULongLong());
		QWidget *page = (QWidget *)(config["pagewidget"].toULongLong());

		connect(this, SIGNAL(sig_isValid(bool)), page, SLOT(slot_isValid(bool)));

		emit sig_isValid(true);

		QWidget *widget = new QWidget();

		config["widget"] = QVariant((qulonglong)widget);

		return SUCCESS;
	}
	else if (command == "gui" && args[0] == "store")
	{
		config.clear();
		config["name"] = QString("User Info");
		config["jtr_mode"] = "single";

		QString disp;
		disp += config["name"].toString();
		
		config["display_string"] = disp;

		return SUCCESS;
	}
	else if (command == "gui" && args[0] == "queue")
	{
		ILC7WorkQueue *pwq = (ILC7WorkQueue *)(config["workqueue"].toULongLong());
		config.remove("workqueue");
		LC7WorkQueueItem item(UUID_TECHNIQUEJTR, "crack", QStringList(), config, \
			QString("Perform User-Info/Single Crack (%1)").arg(config["display_string"].toString()), true, true);
		pwq->AppendWorkQueueItem(item);
		return SUCCESS;
	}

	return FAIL;
}

bool CTechniqueJTRSingleGUI::ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error)
{TR;
	return true;
}

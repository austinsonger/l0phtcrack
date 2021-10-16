#include<stdafx.h>
#include"techniquebruteconfig.h"

CTechniqueJTRBruteGUI::CTechniqueJTRBruteGUI()
{TR;
}

CTechniqueJTRBruteGUI::~CTechniqueJTRBruteGUI()
{TR;
}

ILC7Interface *CTechniqueJTRBruteGUI::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Component")
	{
		return this;
	}
	return NULL;
}

QUuid CTechniqueJTRBruteGUI::GetID()
{TR;
	return UUID_TECHNIQUEJTRBRUTEGUI;
}

ILC7Component::RETURNCODE CTechniqueJTRBruteGUI::ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl)
{TR;
	if(command=="gui" && args[0]=="create")
	{
		QWidget *page=(QWidget *)(config["pagewidget"].toULongLong());

		QWidget *configwidget = new TechniqueBruteConfig();
		
		QWidget *presetwidget = g_pLinkage->GetGUILinkage()->CreatePresetWidget(page, configwidget, UUID_LC7JTRPLUGIN.toString() + ":brute_presets");
		
		config["widget"] = QVariant((qulonglong)presetwidget);

		return SUCCESS;
	}
	else if(command=="gui" && args[0]=="store")
	{
		// Get preset widget
		QWidget *presetwidget = (QWidget *)(config["widget"].toULongLong());

		config.clear();
		config["name"] = QString("%1:%2").arg("Brute").arg(presetwidget->property("preset_name").toString());
		config["jtr_mode"] = "incremental";

		// Get selected preset config
		QMap<QString, QVariant> preset_config = presetwidget->property("config").toMap();

		config.unite(preset_config);

		QString disp;
		disp += config["name"].toString();

		config["display_string"] = disp;

		return SUCCESS;
	}
	else if(command=="gui" && args[0]=="queue")
	{
		ILC7WorkQueue *pwq=(ILC7WorkQueue *)(config["workqueue"].toULongLong());
		config.remove("workqueue");
		LC7WorkQueueItem item(UUID_TECHNIQUEJTR, "crack", QStringList(), config, 
			QString("Perform Brute-Force/Incremental Crack (%1)").arg(config["display_string"].toString()), true, true);
		pwq->AppendWorkQueueItem(item);
		return SUCCESS;
	}
	
	return FAIL;
}

bool CTechniqueJTRBruteGUI::ValidateCommand(QMap<QString,QVariant> & state, QString command, QStringList args, QMap<QString,QVariant> & config, QString & error)
{TR;
	return true;
}

#include<stdafx.h>
#include"techniquedictionaryconfig.h"

CTechniqueJTRDictionaryGUI::CTechniqueJTRDictionaryGUI()
{TR;
}

CTechniqueJTRDictionaryGUI::~CTechniqueJTRDictionaryGUI()
{TR;
}

ILC7Interface *CTechniqueJTRDictionaryGUI::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Component")
	{
		return this;
	}
	return NULL;
}


QUuid CTechniqueJTRDictionaryGUI::GetID()
{TR;
	return UUID_TECHNIQUEJTRDICTIONARYGUI;
}

ILC7Component::RETURNCODE CTechniqueJTRDictionaryGUI::ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl)
{TR;
	if(command=="gui" && args[0]=="create")
	{
		QWidget *page = (QWidget *)(config["pagewidget"].toULongLong());

		QWidget *configwidget = new TechniqueDictionaryConfig();

		QWidget *presetwidget = g_pLinkage->GetGUILinkage()->CreatePresetWidget(page, configwidget, UUID_LC7JTRPLUGIN.toString() + ":dictionary_presets");

		config["widget"] = QVariant((qulonglong)presetwidget);

		return SUCCESS;

	}
	else if(command=="gui" && args[0]=="store")
	{
		// Get preset widget
		QWidget *presetwidget = (QWidget *)(config["widget"].toULongLong());

		config.clear();
		config["name"] = QString("%1:%2").arg("Dictionary").arg(presetwidget->property("preset_name").toString());
		config["jtr_mode"] = "wordlist";

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
			QString("Perform Dictionary / Wordlist Crack (%1)").arg(config["display_string"].toString()), true, true);
		pwq->AppendWorkQueueItem(item);
		return SUCCESS;
	}
	
	return FAIL;
}

bool CTechniqueJTRDictionaryGUI::ValidateCommand(QMap<QString,QVariant> & state, QString command, QStringList args, QMap<QString,QVariant> & config, QString & error)
{TR;
	return true;
}

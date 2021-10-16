#include<stdafx.h>

CSystemPage::CSystemPage(QWidget *parent, ILC7Linkage *pLinkage, ILC7Controller *pCtrl)
	: QWidget(parent)
{TR;
	m_pLinkage=pLinkage;
	m_ctrl = pCtrl;

	ui.setupUi(this);
	m_enable_ui=true;

	ui.SessionWarningLabel->setObjectName("warning");

	m_bRefreshingContent=false;

	m_pLinkage->RegisterNotifySessionActivity(SINGLE_WORKQUEUE_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CSystemPage::NotifySessionActivity);
	m_pLinkage->RegisterNotifySessionActivity(BATCH_WORKQUEUE_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CSystemPage::NotifySessionActivity);
	m_batch_workqueue=NULL;
	m_single_workqueue=NULL;

	QWidget *w=ui.settingsPane;
	
	m_varManager = new QtVariantPropertyManager(w);
    m_varFactory = new QtVariantEditorFactory(w);
	m_propBrowser = new QtTreePropertyBrowser();

	m_propBrowser->setFactoryForManager(m_varManager, m_varFactory);
	m_propBrowser->setPropertiesWithoutValueMarked(true);
	m_propBrowser->setRootIsDecorated(false);
	m_propBrowser->setAlternatingRowColors(false);

	m_helpbutton = m_pLinkage->GetGUILinkage()->GetHelpButton();
	connect(m_helpbutton, &QAbstractButton::clicked, this, &CSystemPage::slot_helpButtonClicked);

	connect(m_varManager,SIGNAL(valueChanged(QtProperty *, const QVariant &)),this,SLOT(PropertyValueChanged(QtProperty *, const QVariant &)));
	connect(ui.resetSavedButton, &QAbstractButton::clicked, this, &CSystemPage::slot_resetSavedButton_clicked);
	connect(ui.resetButton, &QAbstractButton::clicked, this, &CSystemPage::slot_resetButton_clicked);
	connect(ui.clearButton, &QAbstractButton::clicked, this, &CSystemPage::slot_clearButton_clicked);

	ui.settingsPane->layout()->addWidget(m_propBrowser);

	connect(ui.InstallPluginButton,&QPushButton::clicked,this,&CSystemPage::slot_onInstallPlugin);
	connect(ui.UninstallPluginsButton,&QPushButton::clicked,this,&CSystemPage::slot_onUninstallPlugins);
	connect(ui.EnablePluginsButton,&QPushButton::clicked,this,&CSystemPage::slot_onEnablePlugins);
	connect(ui.DisablePluginsButton,&QPushButton::clicked,this,&CSystemPage::slot_onDisablePlugins);

	// Set up plugin table	
	ui.PluginTable->setColumnCount(5);
	ui.PluginTable->setRowCount(1);

	/* Set header as first row */
	QWidget* wdg = new QWidget(ui.PluginTable->horizontalHeader());
	wdg->setAutoFillBackground(true);
	QCheckBox *box = new QCheckBox();
	box->setObjectName("box");
	box->setTristate(true);
	QHBoxLayout* layout = new QHBoxLayout(wdg);
	layout->setSpacing(0);
	layout->setMargin(0);
	wdg->setContentsMargins(0,0,0,0);
	layout->addWidget(box);
	layout->setAlignment( Qt::AlignCenter );
	wdg->setLayout(layout);
	ui.PluginTable->setCellWidget(0,0,wdg);

	ui.PluginTable->setItem(0,0,new QTableWidgetItem(""));
	ui.PluginTable->setItem(0,1,new QTableWidgetItem("Name"));
	ui.PluginTable->setItem(0,2,new QTableWidgetItem("Version"));
	ui.PluginTable->setItem(0,3,new QTableWidgetItem("Author"));
	ui.PluginTable->setItem(0,4,new QTableWidgetItem("State"));

	//QString headerbkgdstr=m_pLinkage->GetGUILinkage()->GetColorManager()->GetBaseShade("HEADER_BKGD");
	//wdg->setStyleSheet("background: "+headerbkgdstr);
	//for(int i=0;i<ui.PluginTable->columnCount();i++)
	//{
		//ui.PluginTable->item(0,i)->setBackgroundColor(QColor(headerbkgdstr));
	//}

	box->setProperty("pluginlibrary",QVariant());
	connect(box, &QCheckBox::stateChanged, this, &CSystemPage::slot_onSystemPageCheckbox);

	// Recolor callback
	ILC7ColorManager *colman=m_pLinkage->GetGUILinkage()->GetColorManager();
	colman->RegisterRecolorCallback(this, (void(QObject::*)(void))&CSystemPage::RecolorCallback);
	RecolorCallback();
}

CSystemPage::~CSystemPage()
{TR;
	m_pLinkage->UnregisterNotifySessionActivity(SINGLE_WORKQUEUE_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CSystemPage::NotifySessionActivity);
	m_pLinkage->UnregisterNotifySessionActivity(BATCH_WORKQUEUE_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CSystemPage::NotifySessionActivity);	
}


void CSystemPage::NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler)
{TR;
	switch(activity)
	{
	case ILC7Linkage::SESSION_OPEN_POST:
	case ILC7Linkage::SESSION_NEW_POST:
		if (handler && handler->GetId() == BATCH_WORKQUEUE_HANDLER_ID)
		{
			m_batch_workqueue = (ILC7WorkQueue *)handler;
		}
		if (handler && handler->GetId() == SINGLE_WORKQUEUE_HANDLER_ID)
		{
			m_single_workqueue = (ILC7WorkQueue *)handler;
		}
		UpdateUI();
		break;
	case ILC7Linkage::SESSION_CLOSE_PRE:
		if (handler && handler->GetId() == BATCH_WORKQUEUE_HANDLER_ID)
		{
			m_batch_workqueue = NULL;		
		}
		if (handler && handler->GetId() == SINGLE_WORKQUEUE_HANDLER_ID)
		{
			m_single_workqueue = NULL;		
		}
		UpdateUI();
		break;
	default:
		break;
	}
}

void CSystemPage::slot_helpButtonClicked()
{
	TR;
	UpdateUI();
}

void CSystemPage::slot_uiEnable(bool enable)
{TR;
	m_enable_ui=enable;
	UpdateUI();
}

void CSystemPage::RecolorCallback(void)
{TR;
	ILC7ColorManager *colman=m_pLinkage->GetGUILinkage()->GetColorManager();
	ui.lc_icon->setPixmap(colman->GetHueColorPixmapResource(":/lc7/lc7logo.png"));
	ui.lc_icon->setFixedSize(QSize(256, 256)*colman->GetSizeRatio());

//	ui.AboutText->document()->setDefaultStyleSheet(QString("a { text-decoration: underline; color:%1 }\na:visited { text-decoration: underline; color:%2 } ").arg(colman->GetHighlightShade("HIGHLIGHT_BKGD_0")).arg(colman->GetHighlightShade("HIGHLIGHT_BKGD_1")));

//	RefreshContent();
}

void CSystemPage::showEvent(QShowEvent *evt)
{TR;
	QWidget::showEvent(evt);
	
	RefreshContent();
}

void CSystemPage::hideEvent(QHideEvent *evt)
{TR;
	QWidget::hideEvent(evt);
}

void CSystemPage::RefreshAbout()
{TR;
	ui.AboutText->setHtml(ui.AboutText->toHtml().replace("[VERSION_STRING]", VERSION_STRING));
}

void CSystemPage::RefreshSettings(bool reset_defaults)
{TR;
	m_propBrowser->clear();
	m_settingskeys.clear();
	m_require_restart.clear();

	/////////////////////////// Settings

	ILC7Settings *settings=m_pLinkage->GetSettings();
	
	// Find system action category

	ILC7ActionCategory *system_cat=NULL;
	QList<ILC7ActionCategory *> cats=m_pLinkage->GetActionCategories();
	foreach(ILC7ActionCategory * cat,cats)
	{
		if(cat->InternalName()=="system")
		{
			system_cat=cat;
			break;
		}
	}
	if(!system_cat)
	{
		UpdateUI();
		m_bRefreshingContent=false;
		return;
	}

	
	m_varManager->clear();
	m_propBrowser->clear();

	// Go through system actions
	foreach(ILC7Action *act,system_cat->GetActions())
	{
		QString section_name=act->Name();
		QString section_desc=act->Desc();

		// Get options for section
		if(act->Command()=="get_options")
		{
			ILC7Component *comp=m_pLinkage->FindComponentByID(act->ComponentId());
			QMap<QString,QVariant> config;
			QString error;
			if(comp->ExecuteCommand(act->Command(), act->Args(), config, error)!=ILC7Component::SUCCESS)
			{
				m_pLinkage->GetGUILinkage()->ErrorMessage(QString("Error getting system options for section '%1'").arg(section_name),error);
				m_bRefreshingContent=false;
				return;
			}

			bool found=false;
			QtProperty *sectionItem;
			foreach(sectionItem, m_varManager->properties())
			{
				if(sectionItem->propertyName()==section_name)
				{
 					found=true;
					break;
				}
			}
			if(!found)
			{
				sectionItem = m_varManager->addProperty(QtVariantPropertyManager::groupTypeId(), section_name);
			}


			sectionItem->setWhatsThis(section_desc);
			sectionItem->setToolTip(section_desc);
			sectionItem->setStatusTip(section_desc);

			m_propBrowser->addProperty(sectionItem);

			QList<QVariant> keys=config["keys"].toList();
			foreach(QVariant key, keys)
			{
				QMap<QString,QVariant> keyparms = key.toMap();

				QString name = QString("    ")+keyparms["name"].toString(); 
				QString desc = keyparms["desc"].toString(); 
				QString settingskey = keyparms["settingskey"].toString(); 
				QVariant::Type type = QVariant::nameToType(keyparms["type"].toString().toLatin1());
				QVariant defaultvalue = keyparms["default"];
				bool require_restart = keyparms["require_restart"].toBool();
				
				
				bool set_default = false;
				if (!settings->contains(settingskey) || reset_defaults)
				{
					set_default = true;
				}

				QtVariantProperty *item = m_varManager->addProperty(type, name);
				item->setStatusTip(desc);
				item->setToolTip(desc);
				item->setWhatsThis(desc);
				m_settingskeys[item]=settingskey;
				m_require_restart[item] = require_restart;


				if (!set_default)
				{
					item->setValue(settings->value(settingskey));
				}
				else
				{
					settings->setValue(settingskey, defaultvalue);
					item->setValue(defaultvalue);
				}

				foreach(QString keyparm, keyparms.keys())
				{
					if(keyparm=="name" || keyparm=="desc" || keyparm=="settingskey" || keyparm=="type" || keyparm=="default")
						continue;
					QVariant attrval = keyparms[keyparm];
					item->setAttribute(keyparm,attrval);
				}

				sectionItem->addSubProperty(item);
			}

		}
	}

	m_pLinkage->GetSettings()->sync();
}

void CSystemPage::slot_onSystemPageCheckbox(int state)
{TR;
	static bool in_signal=false;
	if(in_signal)
	{
		return;
	}
	in_signal=true;

	QCheckBox *box=(QCheckBox*)sender();
	
	QVariant plvar = box->property("pluginlibrary");
	if(!plvar.isValid() || plvar.isNull())
	{
		box->setCheckState(box->checkState()==Qt::Unchecked?Qt::Unchecked:Qt::Checked);
		bool remove=box->checkState()==Qt::Unchecked;

		// Check/uncheck all
		int i=1;
		for(i=1;i<ui.PluginTable->rowCount();i++)
		{
			QCheckBox *box2=ui.PluginTable->cellWidget(i,0)->findChild<QCheckBox*>("box");
			ILC7PluginLibrary *lib = (ILC7PluginLibrary *) box2->property("pluginlibrary").toULongLong();

			box2->setCheckState(box->checkState());

			// Check/uncheck one
			if(remove)
			{
				m_selected_plugins.removeAll(lib);
			}
			else if(!m_selected_plugins.contains(lib))
			{
				m_selected_plugins.append(lib);
			}
		}
	}
	else
	{
		ILC7PluginLibrary *lib = (ILC7PluginLibrary *)plvar.toULongLong();
		bool remove=box->checkState()==Qt::Unchecked;

		// Check/uncheck one
		if(remove)
		{
			m_selected_plugins.removeAll(lib);
		}
		else if(!m_selected_plugins.contains(lib))
		{
			m_selected_plugins.append(lib);
		}
	}

	UpdateUI();

	in_signal=false;
}

void CSystemPage::RefreshPlugins()
{TR;
	ILC7PluginRegistry *reg=m_pLinkage->GetPluginRegistry();

	QList<ILC7PluginLibrary *> libs=reg->GetPluginLibraries();
	ui.PluginTable->setRowCount(libs.size()+1);
	
	int row=1;
	foreach(ILC7PluginLibrary *lib,libs)
	{
		QWidget* wdg = new QWidget;
		wdg->setAutoFillBackground(false);
		QCheckBox *box = new QCheckBox();
		box->setObjectName("box");
		QHBoxLayout* layout = new QHBoxLayout(wdg);
		layout->setSpacing(0);
		layout->setMargin(0);
		wdg->setContentsMargins(0,0,0,0);
		layout->addWidget(box);
		layout->setAlignment( Qt::AlignCenter );
		wdg->setLayout(layout);

		ui.PluginTable->setCellWidget(row,0,wdg);

		ui.PluginTable->setItem(row,1,new QTableWidgetItem(lib->GetDisplayName()));
		ui.PluginTable->setItem(row,2,new QTableWidgetItem(lib->GetDisplayVersion()));
		ui.PluginTable->setItem(row,3,new QTableWidgetItem(lib->GetAuthorName()));

		QString statestr;
		if(m_pLinkage->GetPluginRegistry()->IsPluginLibraryDisabled(lib->GetInternalName()))
		{
			statestr="Disabled";
		}
		else
		{
			switch(lib->GetState())
			{
				case ILC7PluginLibrary::ACTIVATED:
					statestr="Enabled";
					break;
				case ILC7PluginLibrary::DEACTIVATED:
					statestr="Deactivated";
					break;
				case ILC7PluginLibrary::FAILED:
					statestr=QString("Failed (%1)").arg(lib->GetFailureReason());
					break;
				default:
					statestr="Unknown";
					Q_ASSERT(0);
					break;
			}
		}

		ui.PluginTable->setItem(row,4,new QTableWidgetItem(statestr));

//		ui.PluginTable->item(row,0)->setData(257,QVariant((qulonglong)lib));

		box->setProperty("pluginlibrary",QVariant((qulonglong)lib));

		if(m_selected_plugins.contains(lib))
		{
			box->setChecked(true);
		}

		connect(box, &QCheckBox::stateChanged, this, &CSystemPage::slot_onSystemPageCheckbox);

		row++;
	}

	ui.PluginTable->resizeColumnsToContents();
	ui.PluginTable->horizontalHeader()->setSectionResizeMode(0,QHeaderView::ResizeToContents);
	ui.PluginTable->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);
	ui.PluginTable->horizontalHeader()->setSectionResizeMode(2,QHeaderView::ResizeToContents);
	ui.PluginTable->horizontalHeader()->setSectionResizeMode(3,QHeaderView::ResizeToContents);
	ui.PluginTable->horizontalHeader()->setSectionResizeMode(4,QHeaderView::ResizeToContents);
}


void CSystemPage::RefreshContent()
{TR;
	if(m_bRefreshingContent)
	{
		return;
	}
	m_bRefreshingContent=true;

	RefreshSettings();
	RefreshPlugins();
	RefreshAbout();
	
	UpdateUI();
	m_bRefreshingContent=false;
}

void CSystemPage::UpdateUI()
{TR;
	bool have_session = m_single_workqueue!=NULL;

	ui.help->setVisible(m_helpbutton->isChecked());

	ui.SessionWarningLabel->setVisible(have_session);

	if(!m_enable_ui)
	{
		ui.settings->setEnabled(false);
		ui.plugins->setEnabled(false);
		return;
	}
	
	ui.settings->setEnabled(true);
	ui.plugins->setEnabled(true);

	if(m_selected_plugins.size() == m_pLinkage->GetPluginRegistry()->GetPluginLibraries().size())
	{
		QCheckBox *box=ui.PluginTable->cellWidget(0,0)->findChild<QCheckBox *>("box");
		box->setCheckState(Qt::Checked);
	}
	else if (m_selected_plugins.size() > 0)
	{
		QCheckBox *box=ui.PluginTable->cellWidget(0,0)->findChild<QCheckBox *>("box");
		box->setCheckState(Qt::PartiallyChecked);
	}
	else
	{
		QCheckBox *box=ui.PluginTable->cellWidget(0,0)->findChild<QCheckBox *>("box");
		box->setCheckState(Qt::Unchecked);
	}

	bool have_plugins = m_selected_plugins.size()>0;
	bool have_system_plugins = false;
	bool have_enabled_plugins = false;
	bool have_disabled_plugins = false;
	foreach(ILC7PluginLibrary *lib, m_selected_plugins)
	{
		if(m_pLinkage->GetPluginRegistry()->IsPluginLibraryDisabled(lib->GetInternalName()))
		{
			have_disabled_plugins=true;
		}
		else
		{
			have_enabled_plugins=true;
		}
		if (lib->IsSystemLibrary())
		{
			have_system_plugins = true;
		}
	}

	if(!have_session)
	{
		ui.PluginTable->setEnabled(true);
		ui.UninstallPluginsButton->setEnabled(have_plugins && !have_system_plugins);
		ui.EnablePluginsButton->setEnabled(have_disabled_plugins);
		ui.DisablePluginsButton->setEnabled(have_enabled_plugins);
		ui.InstallPluginButton->setEnabled(true);
	}
	else
	{
		ui.PluginTable->setEnabled(false);
		ui.UninstallPluginsButton->setEnabled(false);
		ui.EnablePluginsButton->setEnabled(false);
		ui.DisablePluginsButton->setEnabled(false);
		ui.InstallPluginButton->setEnabled(false);
	}

	int row;
	for(row=1;row<ui.PluginTable->rowCount();row++)
	{
		QCheckBox *box=ui.PluginTable->cellWidget(row,0)->findChild<QCheckBox *>("box");
		QString bcolor=box->isChecked()?
			m_pLinkage->GetGUILinkage()->GetColorManager()->GetHighlightShade("HIGHLIGHT_BKGD"):
			m_pLinkage->GetGUILinkage()->GetColorManager()->GetBaseShade("WIDGET_BKGD");
		QString fcolor=box->isChecked()?
			m_pLinkage->GetGUILinkage()->GetColorManager()->GetInverseTextColor():
			m_pLinkage->GetGUILinkage()->GetColorManager()->GetTextColor();
		for(int col=1; col<ui.PluginTable->columnCount();col++)
		{
			ui.PluginTable->item(row,col)->setBackgroundColor(QColor(bcolor));
			ui.PluginTable->item(row,col)->setTextColor(QColor(fcolor));
		}
	}
}

void CSystemPage::PropertyValueChanged(QtProperty *property, const QVariant &value)
{TR;
	QtVariantProperty *varprop=(QtVariantProperty *)property;
	// If this isn't a property we care about, it could be a sub-property, so ignore
	if(!m_settingskeys.contains(varprop))
	{
		return;
	}

	ILC7Settings *settings=m_pLinkage->GetSettings();
	QString settingskey=m_settingskeys[varprop];
	if(settings->value(settingskey)!=value)
	{
		settings->setValue(settingskey,value);

		if (m_require_restart.value(varprop,false))
		{
			m_pLinkage->GetGUILinkage()->InfoMessage("Restart required", "Changing this option requires that you restart L0phtCrack before it takes effect.");
		}
	}
}

void CSystemPage::ResetPlugins()
{TR;
	QStringList slSelectedPlugins;
	ILC7PluginLibrary *lib;
	foreach(lib, m_selected_plugins)
	{
		slSelectedPlugins.append(lib->GetInternalName());
	}

	m_pLinkage->GetPluginRegistry()->UnloadPluginLibraries();

	QList<ILC7PluginLibrary *> failed_plugins;
	QString error;
	if(!m_pLinkage->GetPluginRegistry()->LoadPluginLibraries(failed_plugins,error))
	{
		m_pLinkage->GetGUILinkage()->ErrorMessage("Loading Plugins Failed",
			QString("Some plugins failed to load:\n\n%1").arg(error));
	}


	m_selected_plugins.clear();

	QString sel;
	foreach(sel, slSelectedPlugins)
	{
		ILC7PluginLibrary *lib = m_pLinkage->GetPluginRegistry()->GetPluginLibraryByInternalName(sel);
		if(lib)
		{
			m_selected_plugins.append(lib);
		}
	}
	
	RefreshPlugins();
	
	UpdateUI();
}

void CSystemPage::slot_onInstallPlugin()
{TR;
	QString pluginfile;
	if (!m_pLinkage->GetGUILinkage()->OpenFileDialog("Choose Plugin File",
		QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).first(), "LC7 Plugins (*.lc7plugin)", pluginfile))
	{
		return;
	}

	QString error;
	if (!m_pLinkage->GetPluginRegistry()->InstallPluginLibrary(pluginfile, error))
	{
		m_pLinkage->GetGUILinkage()->ErrorMessage("Unable to install plugin", error);
		return;
	}

	m_pLinkage->GetGUILinkage()->InfoMessage("Plugin Installed", "The plugin was successfully installed");
}

void CSystemPage::slot_onUninstallPlugins()
{TR;
}

void CSystemPage::slot_onEnablePlugins()
{TR;
	QList<ILC7PluginLibrary *> depending_plugins;
	depending_plugins=m_pLinkage->GetPluginRegistry()->FindDependingPlugins(m_selected_plugins);

	// Only disabled depending plugins are required
	QList<ILC7PluginLibrary *> required_plugins;
	foreach(ILC7PluginLibrary *lib, depending_plugins)
	{
		if(m_pLinkage->GetPluginRegistry()->IsPluginLibraryDisabled(lib->GetInternalName()))
		{
			required_plugins.append(lib);
		}
	}
	
	if(required_plugins.size()>0)
	{
		QString reqs;
		foreach(ILC7PluginLibrary *req, required_plugins)
		{
			reqs+=QString("%1\n").arg(req->GetDisplayName());
		}

		if(!m_pLinkage->GetGUILinkage()->YesNoBox("Enable Required Plugins",
			QString("Enabling the chosen plugins will also enable the following required plugins:\n\n%1\n\nWould you like to proceed?").
			arg(reqs)))
		{
			return;
		}
	}

	ILC7PluginLibrary *lib;
	QList<ILC7PluginLibrary *> all_plugins(m_selected_plugins);
	foreach(lib, required_plugins)
	{
		if(!all_plugins.contains(lib))
		{
			all_plugins.append(lib);
		}
	}

	foreach(lib, all_plugins)
	{
		m_pLinkage->GetPluginRegistry()->EnablePluginLibrary(lib->GetInternalName());
	}

	ResetPlugins();
}

void CSystemPage::slot_onDisablePlugins()
{TR;
	QList<ILC7PluginLibrary *> dependent_plugins;
	dependent_plugins=m_pLinkage->GetPluginRegistry()->FindDependentPlugins(m_selected_plugins);

	// Only enabled dependent plugins are required
	QList<ILC7PluginLibrary *> required_plugins;
	foreach(ILC7PluginLibrary *lib, dependent_plugins)
	{
		if(!m_pLinkage->GetPluginRegistry()->IsPluginLibraryDisabled(lib->GetInternalName()))
		{
			required_plugins.append(lib);
		}
	}

	if(required_plugins.size()>0)
	{
		QString reqs;
		foreach(ILC7PluginLibrary *req, required_plugins)
		{
			reqs+=QString("%1\n").arg(req->GetDisplayName());
		}

		if(!m_pLinkage->GetGUILinkage()->YesNoBox("Disable Dependent Plugins",
			QString("Disabling the chosen plugins will also disable the following dependent plugins:\n\n%1\n\nWould you like to proceed?").
			arg(reqs)))
		{
			return;
		}
	}
	
	ILC7PluginLibrary *lib;
	QList<ILC7PluginLibrary *> all_plugins(m_selected_plugins);
	foreach(lib, required_plugins)
	{
		if(!all_plugins.contains(lib))
		{
			all_plugins.append(lib);
		}
	}

	foreach(lib, all_plugins)
	{
		m_pLinkage->GetPluginRegistry()->DisablePluginLibrary(lib->GetInternalName());
	}

	ResetPlugins();
}

void CSystemPage::slot_clearButton_clicked(int checked)
{
	if (!m_pLinkage->GetGUILinkage()->YesNoBox("Are you sure?", "This will reset the system settings, changing all of the values on this page to their defaults. You will also clear your saved passwords, paths, and other settings not on this page. Question dialogs will also prompt you again. Is this what you want?"))
	{
		return;
	}
	
	disconnect(m_varManager, SIGNAL(valueChanged(QtProperty *, const QVariant &)), this, SLOT(PropertyValueChanged(QtProperty *, const QVariant &)));

	m_bRefreshingContent = true;
	
	ILC7Settings *settings = m_pLinkage->GetSettings();
	foreach(QString key, settings->allKeys())
	{
		if (key != "_core_:presets")
		{
			settings->remove(key);
		}
	}
	RefreshSettings(true);
	QDir dcache(m_pLinkage->GetCacheDirectory());
	dcache.removeRecursively();

	m_bRefreshingContent = false;
	//	RefreshPlugins();
	connect(m_varManager, SIGNAL(valueChanged(QtProperty *, const QVariant &)), this, SLOT(PropertyValueChanged(QtProperty *, const QVariant &)));

	m_pLinkage->GetGUILinkage()->InfoMessage("All Preferences Cleared", "System settings and saved information have been reset.");
}

void CSystemPage::slot_resetButton_clicked(int checked)
{
	if (!m_pLinkage->GetGUILinkage()->YesNoBox("Are you sure?", "Resetting the system settings will change all of the values on this page to their defaults. You will retain your saved passwords, paths, and other settings not on this page. Is this what you want?"))
	{
		return;
	}
	
	disconnect(m_varManager, SIGNAL(valueChanged(QtProperty *, const QVariant &)), this, SLOT(PropertyValueChanged(QtProperty *, const QVariant &)));

	m_bRefreshingContent = true;
	RefreshSettings(true);
	m_bRefreshingContent = false;
	//	RefreshPlugins();

	connect(m_varManager, SIGNAL(valueChanged(QtProperty *, const QVariant &)), this, SLOT(PropertyValueChanged(QtProperty *, const QVariant &)));

	m_pLinkage->GetGUILinkage()->InfoMessage("System Settings Reset", "System settings have been reset.");
}


void CSystemPage::slot_resetSavedButton_clicked(int checked)
{
	if (!m_pLinkage->GetGUILinkage()->YesNoBox("Are you sure?", "This will cause all of the dialogs you have answered and checked the box \"Don't ask this question again\" to once again prompt you. Are you sure?"))
	{
		return;
	}

	ILC7Settings *settings = m_pLinkage->GetSettings();
	QStringList delkeys;
	foreach(QString key, settings->allKeys())
	{
		if (key.startsWith("_neveragain_"))
		{
			delkeys.append(key);
		}
	}
	foreach(QString key, delkeys)
	{
		settings->remove(key);
	}

	m_pLinkage->GetGUILinkage()->InfoMessage("Question Dialogs Reset", "Question dialog prompts restored.");
}

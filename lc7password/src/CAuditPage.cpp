#include<stdafx.h>

/*
class ResizedItemDelegate : public QStyledItemDelegate
{
public:
	QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
	{
		int sizeratio = g_pLinkage->GetGUILinkage()->GetColorManager()->GetSizeRatio();
		QSize sz = QStyledItemDelegate::sizeHint(option, index);
		sz.setHeight(24 * sizeratio);
		return sz;
	}
};
*/

CAuditPage::CAuditPage(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	m_enable_ui=true;
	m_last_content_gen = 0;

	// Hook up to account list and queue notifications
	g_pLinkage->RegisterNotifySessionActivity(ACCOUNTLIST_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CAuditPage::NotifySessionActivity);
	g_pLinkage->RegisterNotifySessionActivity(SINGLE_WORKQUEUE_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CAuditPage::NotifySessionActivity);
	g_pLinkage->RegisterNotifySessionActivity(BATCH_WORKQUEUE_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CAuditPage::NotifySessionActivity);
	m_batch_workqueue=NULL;
	m_single_workqueue=NULL;
	m_accountlist=NULL;

	m_technique_tree_model=NULL;

	connect(ui.RunButton, &QAbstractButton::clicked,this, &CAuditPage::onRunButton);
	connect(ui.AddQueueButton, &QAbstractButton::clicked,this, &CAuditPage::onAddQueueButton);
			
	m_is_valid = false;
	//ui.RunButton->setEnabled(false);
	//ui.AddQueueButton->setEnabled(false);

	m_colman = g_pLinkage->GetGUILinkage()->GetColorManager();
	
	QList<int> sizes;
	sizes << 128 * m_colman->GetSizeRatio();
	sizes << 256 * m_colman->GetSizeRatio();
	ui.splitter->setSizes(sizes);

	m_colman->RegisterRecolorCallback(this, (void(QObject::*)(void))&CAuditPage::RecolorCallback);
	m_colman->StyleCommandLinkButton(ui.RunButton);
	m_colman->StyleCommandLinkButton(ui.AddQueueButton);
	RecolorCallback();

	m_helpbutton=g_pLinkage->GetGUILinkage()->GetHelpButton();
	connect(m_helpbutton, &QAbstractButton::clicked, this, &CAuditPage::slot_helpButtonClicked);

	UpdateUI();
}

CAuditPage::~CAuditPage()
{TR;
	g_pLinkage->UnregisterNotifySessionActivity(ACCOUNTLIST_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CAuditPage::NotifySessionActivity);
	g_pLinkage->UnregisterNotifySessionActivity(SINGLE_WORKQUEUE_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CAuditPage::NotifySessionActivity);
	g_pLinkage->UnregisterNotifySessionActivity(BATCH_WORKQUEUE_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CAuditPage::NotifySessionActivity);	
}

void CAuditPage::slot_helpButtonClicked()
{TR;
	UpdateUI();
}

void CAuditPage::NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler)
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
		if (handler && handler->GetId() == ACCOUNTLIST_HANDLER_ID)
		{
			m_accountlist = (ILC7AccountList *)handler;
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
		if (handler && handler->GetId() == ACCOUNTLIST_HANDLER_ID)
		{
			m_accountlist = NULL;		
		}
		UpdateUI();
		break;
	default:
		break;
	}
}

void CAuditPage::slot_uiEnable(bool enable)
{TR;
	m_enable_ui=enable;
	UpdateUI();
}

void CAuditPage::RecolorCallback(void)
{TR;
	ui.RunButton->setIcon(QIcon(m_colman->GetHueColorPixmapResource(":/qss_icons/rc/commandlink.png")));
	ui.AddQueueButton->setIcon(QIcon(m_colman->GetHueColorPixmapResource(":/qss_icons/rc/commandlink.png")));

	int ratio = g_pLinkage->GetGUILinkage()->GetColorManager()->GetSizeRatio();
	ui.Tree->setIconSize(QSize(24 * ratio, 24 * ratio));

	m_last_content_gen = 0;
	RefreshContent();
}


CAuditPage *CreateAuditPage()
{
	return new CAuditPage();
}

void CAuditPage::UpdateUI()
{TR;
	ui.help->setVisible(m_helpbutton->isChecked());
	/*
	if(m_accountlist==NULL)
	{
		ui.NoSessionLabel->setVisible(true);
		ui.NoSessionLabel->setObjectName("warning");
	}
	else
	{
		ui.NoSessionLabel->setVisible(false);
	}
	*/
	if(!m_enable_ui || m_accountlist==NULL || m_single_workqueue==NULL)
	{
		setEnabled(false);
		return;
	}

	setEnabled(true);
	
	bool enable_run = m_is_valid; 
	bool enable_addqueue = m_is_valid;

	if (ui.Tree->selectionModel() != NULL)
	{
		QModelIndex selected=ui.Tree->selectionModel()->currentIndex();
		ILC7Action *act=(ILC7Action *)selected.data(257).toULongLong();
		if(!act)
		{
			enable_run=false;
			enable_addqueue=false;
		}
	}
	else
	{
		enable_run=false;
		enable_addqueue=false;
	}
	
	ui.RunButton->setEnabled(enable_run);
	ui.AddQueueButton->setEnabled(enable_addqueue);
}



void CAuditPage::showEvent(QShowEvent *evt)
{TR;
	QWidget::showEvent(evt);
	
	RefreshContent();
}

void CAuditPage::hideEvent(QHideEvent *evt)
{TR;
	QWidget::hideEvent(evt);
}


void CAuditPage::AddCategory(ILC7ActionCategory *cat, QStandardItem *item, bool flatten)
{TR;
	QList<ILC7ActionCategory *> subcats=cat->GetActionCategories();
	foreach(ILC7ActionCategory *subcat,subcats)
	{
		QList<QVariant> data;
		data << subcat->Name();
		
		QStandardItem *subtreeitem;
		if(!flatten)
		{
			subtreeitem=new QStandardItem(subcat->Name());
			subtreeitem->setData(QVariant((qulonglong)subcat),258);
			item->appendRow(subtreeitem);
		}
		else
		{
			subtreeitem=item;
		}

		AddCategory(subcat,subtreeitem, false);
	}

	QIcon default_icon(m_colman->GetMonoColorIcon(
		":/lc7password/audit_icon.png",
		QColor(m_colman->GetHighlightColor()),
		QColor(m_colman->GetTextColor()),
		QColor(m_colman->GetBaseShade("TEXT_DISABLED"))
		));

	QList<ILC7Action *> acts=cat->GetActions();
	foreach(ILC7Action *act,acts)
	{
		if(act->Command()=="gui")
		{
			QStandardItem *subtreeitem=new QStandardItem(act->Name());
			subtreeitem->setData(QVariant((qulonglong)act),257);
			QIcon icon(act->Icon().isNull() ? default_icon : act->Icon());
			subtreeitem->setIcon(icon);
			//subtreeitem->setSizeHint(QSize(24, 24));
			item->appendRow(subtreeitem);
		}
	}
}

void CAuditPage::RefreshContent()
{TR;
	QList<ILC7ActionCategory *> cats=g_pLinkage->GetActionCategories();
	foreach(ILC7ActionCategory *cat,cats)
	{
		if(cat->InternalName()=="technique")
		{
			int catgen = cat->GetGenerationNumber();
			if (m_last_content_gen == catgen)
			{
				// Nothing has changed, don't bother
				return;
			}

			if(m_technique_tree_model)
			{
				ui.Tree->setModel(NULL);
				delete m_technique_tree_model;
				m_technique_tree_model=NULL;
			}
			
			m_technique_tree_model=new QStandardItemModel(ui.Tree);

			QStringList headers;
			headers.append("Audit Techniques");
			//headers.append("Type");

			m_technique_tree_model->setHorizontalHeaderLabels(headers);

			AddCategory(cat, m_technique_tree_model->invisibleRootItem(), false);

			ui.Tree->setModel(m_technique_tree_model);

			int i;
			for(i=0;i<headers.length()-1;i++)
			{
				//ui.Tree->header()->setSectionResizeMode(i,QHeaderView::Stretch);
			}
			ui.Tree->header()->setSectionResizeMode(QHeaderView::Stretch);
			//ui.Tree->setColumnWidth(i,64);
			//ui.Tree->header()->setSectionResizeMode(i,QHeaderView::Fixed);
			
			connect(ui.Tree,&QAbstractItemView::entered,this,&CAuditPage::onTreeEntered);
			connect(ui.Tree,&QAbstractItemView::viewportEntered,this,&CAuditPage::onTreeViewportEntered);
			connect(ui.Tree->selectionModel(),&QItemSelectionModel::currentChanged,this,&CAuditPage::onTreeCurrentChanged);
			ui.Tree->setMouseTracking(true);
			
			ui.Tree->expandAll();

			ui.OptionsView->setWidget(new QWidget());

			UpdateUI();

			m_last_content_gen = catgen;

			return;
		}
	}

}

void CAuditPage::setDescriptionText(QModelIndex selected)
{TR;
	ILC7Action *act=(ILC7Action *)selected.data(257).toULongLong();
	if(act!=NULL)
	{
		ui.Desc->setText(act->Desc());
	}
	else
	{
		ILC7ActionCategory *cat=(ILC7ActionCategory *)selected.data(258).toULongLong();
		if(cat!=NULL)
		{
			ui.Desc->setText(cat->Desc());
		}
		else
		{
			ui.Desc->clear();
		}
	}
}

void CAuditPage::onTreeViewportEntered(void)
{TR;
	QModelIndex selected=ui.Tree->currentIndex();
	setDescriptionText(selected);
}

void CAuditPage::onTreeEntered(const QModelIndex &selected)
{TR;
	setDescriptionText(selected);
}

void CAuditPage::onTreeCurrentChanged(const QModelIndex &selected, const QModelIndex &deselected)
{TR;
	setDescriptionText(selected);

	ILC7Action *act=(ILC7Action *)selected.data(257).toULongLong();
	if(act)
	{
		QMap<QString,QVariant> config;
		QString error;

		ILC7Component *comp = g_pLinkage->FindComponentByID(act->ComponentId());
		
		config["pagewidget"] = QVariant((qulonglong)this);
		if(comp->ExecuteCommand(act->Command(), QStringList("create"), config, error)!=ILC7Component::SUCCESS)
		{
			g_pLinkage->GetGUILinkage()->ErrorMessage("Error during audit",error);
			return;
		}
		config.remove("pagewidget");
	
		QWidget *widget=(QWidget *)config["widget"].toULongLong();
		if(!widget)
		{
			widget=new QWidget();
		}
		ui.OptionsView->setWidget(widget);
	}
	else
	{
		ui.OptionsView->setWidget(new QWidget());
	}

	UpdateUI();
}

void CAuditPage::onAddQueueButton(bool checked)
{TR;
	QModelIndex selected=ui.Tree->selectionModel()->currentIndex();
	if(selected==QModelIndex())
	{
		return;
	}

	ILC7Action *act=(ILC7Action *)selected.data(257).toULongLong();
	if(act==NULL)
	{
		return;
	}

	QMap<QString,QVariant> config;
	QString error;
	ILC7Component *comp = g_pLinkage->FindComponentByID(act->ComponentId());
	
	config["widget"]=(qulonglong)(ui.OptionsView->widget());
	if(comp->ExecuteCommand(act->Command(), QStringList("store"), config, error)!=ILC7Component::SUCCESS)
	{
		g_pLinkage->GetGUILinkage()->ErrorMessage("Error during audit",error);
		return;
	}
	config.remove("widget");

	config["workqueue"]=(qulonglong)m_batch_workqueue;
	if(comp->ExecuteCommand(act->Command(), QStringList("queue"), config, error)!=ILC7Component::SUCCESS)
	{
		g_pLinkage->GetGUILinkage()->ErrorMessage("Error queuing operation",error);
		return;
	}
	config.remove("workqueue");	
	
	g_pLinkage->GetGUILinkage()->AppendToActivityLog("Audit operation queued.\n");

	if(g_pLinkage->GetSettings()->value("_ui_:switch_to_queue",true).toBool())
	{
		g_pLinkage->GetGUILinkage()->SwitchToMainMenuTab("base/queue");
	}

//	g_pLinkage->GetGUILinkage()->InfoMessage("Work Queue","Audit operation queued.");


	UpdateUI();
}

void CAuditPage::onRunButton(bool checked)
{TR;
	QModelIndex selected=ui.Tree->selectionModel()->currentIndex();
	if(selected==QModelIndex())
	{
		return;
	}

	ILC7Action *act=(ILC7Action *)selected.data(257).toULongLong();
	if(act==NULL)
	{
		return;
	}

	QMap<QString,QVariant> config;
	QString error;

	ILC7Component *comp = g_pLinkage->FindComponentByID(act->ComponentId());
	
	config["widget"] = (qulonglong)(ui.OptionsView->widget());
	if(comp->ExecuteCommand(act->Command(), QStringList("store"), config,error)!=ILC7Component::SUCCESS)
	{
		g_pLinkage->GetGUILinkage()->ErrorMessage("Error during audit",error);
		return;
	}
	config.remove("widget");
	
	if(m_batch_workqueue->GetWorkQueueState()!=ILC7WorkQueue::UNVALIDATED && 
		!g_pLinkage->GetGUILinkage()->YesNoBox("Warning","This will reset the currently stopped batch queue jobs and mark all operations as unvalidated. Are you sure?"))
	{
		return;
	}

	m_batch_workqueue->ResetWorkQueueState();
	m_single_workqueue->ResetWorkQueueState();
	m_single_workqueue->ClearWorkQueue();
	
	config["workqueue"]=(qulonglong)m_single_workqueue;
	if(comp->ExecuteCommand(act->Command(), QStringList("queue"), config, error)!=ILC7Component::SUCCESS)
	{
		g_pLinkage->GetGUILinkage()->ErrorMessage("Error queuing operation",error);
		return;
	}
	config.remove("workqueue");	
	
	int failed_item=-1;
	if(!m_single_workqueue->Validate(error,failed_item))
	{
		g_pLinkage->GetGUILinkage()->ErrorMessage("Error validating audit operation",error);
		return;
	}
	
	if(!m_single_workqueue->StartRequest())
	{
		QString error=m_single_workqueue->GetLastError();
		g_pLinkage->GetGUILinkage()->ErrorMessage("Error executing audit operation",error);
		return;
	}

	if(g_pLinkage->GetSettings()->value(UUID_PASSWORDUIOPTIONS.toString()+":switch_to_accounts",true).toBool())
	{
		g_pLinkage->GetGUILinkage()->SwitchToMainMenuTab("passwords/accounts");
	}
}

void CAuditPage::slot_isValid(bool valid)
{TR;
	m_is_valid = valid;
	UpdateUI();
}
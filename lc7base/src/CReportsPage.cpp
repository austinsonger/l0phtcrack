#include<stdafx.h>

CReportsPage::CReportsPage(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	m_enable_ui = true;

	// Hook up to account list and queue notifications
	g_pLinkage->RegisterNotifySessionActivity(SINGLE_WORKQUEUE_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CReportsPage::NotifySessionActivity);
	g_pLinkage->RegisterNotifySessionActivity(BATCH_WORKQUEUE_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CReportsPage::NotifySessionActivity);
	m_batch_workqueue = NULL;
	m_single_workqueue = NULL;
	
	m_report_tree_model = NULL;

	m_helpbutton = g_pLinkage->GetGUILinkage()->GetHelpButton();

	connect(ui.RunButton, &QAbstractButton::clicked, this, &CReportsPage::onRunButton);
	connect(ui.AddQueueButton, &QAbstractButton::clicked, this, &CReportsPage::onAddQueueButton);
	connect(m_helpbutton, &QAbstractButton::clicked, this, &CReportsPage::slot_helpButtonClicked);

	m_is_valid = false;
	//ui.RunButton->setEnabled(false);
	//ui.AddQueueButton->setEnabled(false);

	ILC7ColorManager *colman = g_pLinkage->GetGUILinkage()->GetColorManager();

	QList<int> sizes;
	sizes << 128 * colman->GetSizeRatio();
	sizes << 256 * colman->GetSizeRatio();
	ui.splitter->setSizes(sizes);


	colman->RegisterRecolorCallback(this, (void(QObject::*)(void))&CReportsPage::RecolorCallback);
	colman->StyleCommandLinkButton(ui.RunButton);
	colman->StyleCommandLinkButton(ui.AddQueueButton);

	RecolorCallback();

	UpdateUI();
}

CReportsPage::~CReportsPage()
{
	TR;
	g_pLinkage->UnregisterNotifySessionActivity(SINGLE_WORKQUEUE_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CReportsPage::NotifySessionActivity);
	g_pLinkage->UnregisterNotifySessionActivity(BATCH_WORKQUEUE_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CReportsPage::NotifySessionActivity);
}

void CReportsPage::slot_helpButtonClicked()
{
	TR;
	UpdateUI();
}

void CReportsPage::NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler)
{
	TR;
	switch (activity)
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

void CReportsPage::slot_uiEnable(bool enable)
{
	TR;
	m_enable_ui = enable;
	UpdateUI();
}

void CReportsPage::RecolorCallback(void)
{
	TR;
	ILC7ColorManager *colman = g_pLinkage->GetGUILinkage()->GetColorManager();
	ui.RunButton->setIcon(QIcon(colman->GetHueColorPixmapResource(":/qss_icons/rc/commandlink.png")));
	ui.AddQueueButton->setIcon(QIcon(colman->GetHueColorPixmapResource(":/qss_icons/rc/commandlink.png")));

	int ratio = g_pLinkage->GetGUILinkage()->GetColorManager()->GetSizeRatio();
	ui.Tree->setIconSize(QSize(24 * ratio, 24 * ratio));

	RefreshContent();
}


CReportsPage *CreateReportsPage()
{
	return new CReportsPage();
}

void CReportsPage::UpdateUI()
{
	TR;
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
	if (!m_enable_ui || m_single_workqueue == NULL)
	{
		setEnabled(false);
		return;
	}

	setEnabled(true);

	bool enable_run = m_is_valid;
	bool enable_addqueue = m_is_valid;

	if (ui.Tree->selectionModel() != NULL)
	{
		QModelIndex selected = ui.Tree->selectionModel()->currentIndex();
		ILC7Action *act = (ILC7Action *)selected.data(257).toULongLong();
		if (!act)
		{
			enable_run = false;
			enable_addqueue = false;
		}
	}
	else
	{
		enable_run = false;
		enable_addqueue = false;
	}

	ui.RunButton->setEnabled(enable_run);
	ui.AddQueueButton->setEnabled(enable_addqueue);
}



void CReportsPage::showEvent(QShowEvent *evt)
{
	TR;
	QWidget::showEvent(evt);

	RefreshContent();
}

void CReportsPage::hideEvent(QHideEvent *evt)
{
	TR;
	QWidget::hideEvent(evt);
}


void CReportsPage::AddCategory(ILC7ActionCategory *cat, QStandardItem *item, bool flatten)
{
	TR;
	QList<ILC7ActionCategory *> subcats = cat->GetActionCategories();
	foreach(ILC7ActionCategory *subcat, subcats)
	{
		QList<QVariant> data;
		data << subcat->Name();

		QStandardItem *subtreeitem;
		if (!flatten)
		{
			subtreeitem = new QStandardItem(subcat->Name());
			subtreeitem->setData(QVariant((qulonglong)subcat), 258);
			item->appendRow(subtreeitem);
		}
		else
		{
			subtreeitem = item;
		}

		AddCategory(subcat, subtreeitem, false);
	}

	ILC7ColorManager *colman = g_pLinkage->GetGUILinkage()->GetColorManager();
	QIcon default_icon(colman->GetMonoColorIcon(
		":/base/reports_icon.png",
		QColor(colman->GetHighlightColor()),
		QColor(colman->GetTextColor()),
		QColor(colman->GetBaseShade("TEXT_DISABLED"))
		));

	QList<ILC7Action *> acts = cat->GetActions();
	foreach(ILC7Action *act, acts)
	{
		if (act->Command() == "gui")
		{
			QStandardItem *subtreeitem = new QStandardItem(act->Name());
			subtreeitem->setData(QVariant((qulonglong)act), 257);
			QIcon icon(act->Icon().isNull() ? default_icon : act->Icon());
			subtreeitem->setIcon(icon);
			//subtreeitem->setSizeHint(QSize(24, 24));
			item->appendRow(subtreeitem);
		}
	}
}

void CReportsPage::RefreshContent()
{
	TR;
	QList<ILC7ActionCategory *> cats = g_pLinkage->GetActionCategories();
	foreach(ILC7ActionCategory *cat, cats)
	{
		if (cat->InternalName() == "reports")
		{
			if (m_report_tree_model)
			{
				ui.Tree->setModel(NULL);
				delete m_report_tree_model;
				m_report_tree_model = NULL;
			}

			m_report_tree_model = new QStandardItemModel(ui.Tree);

			QStringList headers;
			headers.append("Report Types");
			//headers.append("Type");

			m_report_tree_model->setHorizontalHeaderLabels(headers);

			AddCategory(cat, m_report_tree_model->invisibleRootItem(), false);

			ui.Tree->setModel(m_report_tree_model);

			int i;
			for (i = 0; i<headers.length() - 1; i++)
			{
				//ui.Tree->header()->setSectionResizeMode(i,QHeaderView::Stretch);
			}
			ui.Tree->header()->setSectionResizeMode(QHeaderView::Stretch);
			//ui.Tree->setColumnWidth(i,64);
			//ui.Tree->header()->setSectionResizeMode(i,QHeaderView::Fixed);

			connect(ui.Tree, &QAbstractItemView::entered, this, &CReportsPage::onTreeEntered);
			connect(ui.Tree, &QAbstractItemView::viewportEntered, this, &CReportsPage::onTreeViewportEntered);
			connect(ui.Tree->selectionModel(), &QItemSelectionModel::currentChanged, this, &CReportsPage::onTreeCurrentChanged);
			ui.Tree->setMouseTracking(true);

			ui.Tree->expandAll();

			ui.OptionsView->setWidget(new QWidget());

			UpdateUI();

			return;
		}
	}

}

void CReportsPage::setDescriptionText(QModelIndex selected)
{
	TR;
	ILC7Action *act = (ILC7Action *)selected.data(257).toULongLong();
	if (act != NULL)
	{
		ui.Desc->setText(act->Desc());
	}
	else
	{
		ILC7ActionCategory *cat = (ILC7ActionCategory *)selected.data(258).toULongLong();
		if (cat != NULL)
		{
			ui.Desc->setText(cat->Desc());
		}
		else
		{
			ui.Desc->clear();
		}
	}
}

void CReportsPage::onTreeViewportEntered(void)
{
	TR;
	QModelIndex selected = ui.Tree->currentIndex();
	setDescriptionText(selected);
}

void CReportsPage::onTreeEntered(const QModelIndex &selected)
{
	TR;
	setDescriptionText(selected);
}

void CReportsPage::onTreeCurrentChanged(const QModelIndex &selected, const QModelIndex &deselected)
{
	TR;
	setDescriptionText(selected);

	ILC7Action *act = (ILC7Action *)selected.data(257).toULongLong();
	if (act)
	{
		QMap<QString, QVariant> config;
		QString error;

		ILC7Component *comp = g_pLinkage->FindComponentByID(act->ComponentId());

		config["pagewidget"] = QVariant((qulonglong)this);
		if (comp->ExecuteCommand(act->Command(), QStringList("create"), config, error) != ILC7Component::SUCCESS)
		{
			g_pLinkage->GetGUILinkage()->ErrorMessage("Error during audit", error);
			return;
		}
		config.remove("pagewidget");

		QWidget *widget = (QWidget *)config["widget"].toULongLong();
		if (!widget)
		{
			widget = new QWidget();
		}
		ui.OptionsView->setWidget(widget);
	}
	else
	{
		ui.OptionsView->setWidget(new QWidget());
	}

	UpdateUI();
}

void CReportsPage::onAddQueueButton(bool checked)
{
	TR;
	QModelIndex selected = ui.Tree->selectionModel()->currentIndex();
	if (selected == QModelIndex())
	{
		return;
	}

	ILC7Action *act = (ILC7Action *)selected.data(257).toULongLong();
	if (act == NULL)
	{
		return;
	}

	QMap<QString, QVariant> config;
	QString error;
	ILC7Component *comp = g_pLinkage->FindComponentByID(act->ComponentId());

	config["widget"] = (qulonglong)(ui.OptionsView->widget());
	if (comp->ExecuteCommand(act->Command(), QStringList("store"), config, error) != ILC7Component::SUCCESS)
	{
		g_pLinkage->GetGUILinkage()->ErrorMessage("Error during audit", error);
		return;
	}
	config.remove("widget");

	config["workqueue"] = (qulonglong)m_batch_workqueue;
	if (comp->ExecuteCommand(act->Command(), QStringList("queue"), config, error) != ILC7Component::SUCCESS)
	{
		g_pLinkage->GetGUILinkage()->ErrorMessage("Error queuing operation", error);
		return;
	}
	config.remove("workqueue");

	g_pLinkage->GetGUILinkage()->AppendToActivityLog("Audit operation queued.\n");

	if (g_pLinkage->GetSettings()->value("_ui_:switch_to_queue", true).toBool())
	{
		g_pLinkage->GetGUILinkage()->SwitchToMainMenuTab("base/queue");
	}

	//	g_pLinkage->GetGUILinkage()->InfoMessage("Work Queue","Audit operation queued.");


	UpdateUI();
}

void CReportsPage::onRunButton(bool checked)
{
	TR;
	QModelIndex selected = ui.Tree->selectionModel()->currentIndex();
	if (selected == QModelIndex())
	{
		return;
	}

	ILC7Action *act = (ILC7Action *)selected.data(257).toULongLong();
	if (act == NULL)
	{
		return;
	}

	QMap<QString, QVariant> config;
	QString error;

	ILC7Component *comp = g_pLinkage->FindComponentByID(act->ComponentId());

	config["widget"] = (qulonglong)(ui.OptionsView->widget());
	if (comp->ExecuteCommand(act->Command(), QStringList("store"), config, error) != ILC7Component::SUCCESS)
	{
		g_pLinkage->GetGUILinkage()->ErrorMessage("Error generating report", error);
		return;
	}
	config.remove("widget");

	if (m_batch_workqueue->GetWorkQueueState() != ILC7WorkQueue::UNVALIDATED &&
		!g_pLinkage->GetGUILinkage()->YesNoBox("Warning", "This will reset the currently stopped batch queue jobs and mark all operations as unvalidated. Are you sure?"))
	{
		return;
	}

	m_batch_workqueue->ResetWorkQueueState();
	m_single_workqueue->ResetWorkQueueState();
	m_single_workqueue->ClearWorkQueue();

	config["workqueue"] = (qulonglong)m_single_workqueue;
	if (comp->ExecuteCommand(act->Command(), QStringList("queue"), config, error) != ILC7Component::SUCCESS)
	{
		g_pLinkage->GetGUILinkage()->ErrorMessage("Error queuing operation", error);
		return;
	}
	config.remove("workqueue");

	int failed_item = -1;
	if (!m_single_workqueue->Validate(error, failed_item))
	{
		g_pLinkage->GetGUILinkage()->ErrorMessage("Error validating audit operation", error);
		return;
	}

	if (!m_single_workqueue->StartRequest())
	{
		QString error = m_single_workqueue->GetLastError();
		g_pLinkage->GetGUILinkage()->ErrorMessage("Error executing audit operation", error);
		return;
	}
}

void CReportsPage::slot_isValid(bool valid)
{
	TR;
	m_is_valid = valid;
	UpdateUI();
}
#include<stdafx.h>


CQueuePage *CreateQueuePage()
{
	return new CQueuePage();
}

CQueuePage::CQueuePage(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	m_enable_ui=true;
	m_sch_enabled = true;

	g_pLinkage->RegisterNotifySessionActivity(SINGLE_WORKQUEUE_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CQueuePage::NotifySessionActivity);
	g_pLinkage->RegisterNotifySessionActivity(BATCH_WORKQUEUE_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CQueuePage::NotifySessionActivity);
	m_batch_workqueue=NULL;
	m_single_workqueue=NULL;
	
	connect(ui.moveUpButton, &QAbstractButton::clicked, this, &CQueuePage::onMoveUpButton);
	connect(ui.moveDownButton, &QAbstractButton::clicked, this, &CQueuePage::onMoveDownButton);
	connect(ui.removeButton, &QAbstractButton::clicked, this, &CQueuePage::onRemoveButton);
	connect(ui.runQueueButton, &QAbstractButton::clicked,this, &CQueuePage::onRunQueueButton);
	connect(ui.validateQueueButton, &QAbstractButton::clicked,this, &CQueuePage::onValidateQueueButton);
	connect(ui.scheduleQueueButton, &QAbstractButton::clicked,this, &CQueuePage::onScheduleQueueButton);
	connect(ui.queueTable,&QTableWidget::currentItemChanged,this, &CQueuePage::onQueueTableCurrentItemChanged);	

	ui.queueTable->setColumnCount(2);
	QStringList headers;
	headers.append("Description");
	headers.append("Status");
	ui.queueTable->setHorizontalHeaderLabels(headers);
	ui.queueTable->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch);
	ui.queueTable->horizontalHeader()->setSectionResizeMode(1,QHeaderView::ResizeToContents);

	ILC7ColorManager *colman=g_pLinkage->GetGUILinkage()->GetColorManager();
	colman->RegisterRecolorCallback(this, (void(QObject::*)(void))&CQueuePage::RecolorCallback);
	colman->StyleCommandLinkButton(ui.runQueueButton);
	colman->StyleCommandLinkButton(ui.validateQueueButton);
	colman->StyleCommandLinkButton(ui.scheduleQueueButton);
	RecolorCallback();

	m_helpbutton = g_pLinkage->GetGUILinkage()->GetHelpButton();
	connect(m_helpbutton, &QAbstractButton::clicked, this, &CQueuePage::slot_helpButtonClicked);

	UpdateUI();
}

CQueuePage::~CQueuePage()
{TR;
	g_pLinkage->UnregisterNotifySessionActivity(SINGLE_WORKQUEUE_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CQueuePage::NotifySessionActivity);
	g_pLinkage->UnregisterNotifySessionActivity(BATCH_WORKQUEUE_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CQueuePage::NotifySessionActivity);	
}

void CQueuePage::slot_helpButtonClicked()
{TR;
	UpdateUI();
}

void CQueuePage::NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler)
{TR;
	switch(activity)
	{
	case ILC7Linkage::SESSION_OPEN_POST:
	case ILC7Linkage::SESSION_NEW_POST:
		if (handler && handler->GetId() == BATCH_WORKQUEUE_HANDLER_ID)
		{
			m_batch_workqueue = (ILC7WorkQueue *)handler;
			m_batch_workqueue->AddQueueChangedListener(this,(void (QObject::*)(void))&CQueuePage::onQueueChanged);
			onQueueChanged();
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
			onQueueChanged();
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


void CQueuePage::slot_uiEnable(bool enable)
{TR;
	m_enable_ui = enable;
	UpdateUI();
}

void CQueuePage::RecolorCallback(void)
{TR;
	ui.runQueueButton->setIcon(QIcon(g_pLinkage->GetGUILinkage()->GetColorManager()->GetHueColorPixmapResource(":/qss_icons/rc/commandlink.png")));
	ui.validateQueueButton->setIcon(QIcon(g_pLinkage->GetGUILinkage()->GetColorManager()->GetHueColorPixmapResource(":/qss_icons/rc/commandlink.png")));
	ui.scheduleQueueButton->setIcon(QIcon(g_pLinkage->GetGUILinkage()->GetColorManager()->GetHueColorPixmapResource(":/qss_icons/rc/commandlink.png")));
}

void CQueuePage::onQueueChanged(void)
{TR;
	RefreshContent();
	UpdateUI();
}


void CQueuePage::UpdateUI()
{TR;
	ui.help->setVisible(m_helpbutton->isChecked());
	/*
	if(m_single_workqueue==NULL)
	{
		ui.NoSessionLabel->setVisible(true);
		ui.NoSessionLabel->setObjectName("warning");
	}
	else
	{
		ui.NoSessionLabel->setVisible(false);
	}
	*/
	
	if(!m_enable_ui || m_batch_workqueue==NULL || m_single_workqueue==NULL)
	{
		setEnabled(false);
		return;
	}
	setEnabled(true);

	int selected_row=ui.queueTable->selectionModel()->currentIndex().row();
	int count=ui.queueTable->rowCount();

	if(count>=1)
	{
		if (selected_row != -1)
		{
			if (count >= 2)
			{
				ui.moveUpButton->setEnabled(selected_row > 0);
				ui.moveDownButton->setEnabled(selected_row < (count - 1));
			}
			else
			{
				ui.moveUpButton->setEnabled(false);
				ui.moveDownButton->setEnabled(false);
			}
			ui.removeButton->setEnabled(true);
		}
		else
		{
			ui.moveUpButton->setEnabled(false);
			ui.moveDownButton->setEnabled(false);
			ui.removeButton->setEnabled(false);
		}

		ui.validateQueueButton->setEnabled(
			m_batch_workqueue->GetWorkQueueState() != ILC7WorkQueue::PAUSED &&
			m_batch_workqueue->GetWorkQueueState() != ILC7WorkQueue::IN_PROGRESS &&
			m_batch_workqueue->GetWorkQueueState() != ILC7WorkQueue::VALIDATED);
		ui.runQueueButton->setEnabled(
			m_batch_workqueue->GetWorkQueueState() == ILC7WorkQueue::VALIDATED);
		ui.scheduleQueueButton->setEnabled(m_sch_enabled && 
			m_batch_workqueue->GetWorkQueueState() == ILC7WorkQueue::VALIDATED);
	}
	else
	{
		ui.moveUpButton->setEnabled(false);
		ui.moveDownButton->setEnabled(false);
		ui.removeButton->setEnabled(false);
		ui.validateQueueButton->setEnabled(false);
		ui.runQueueButton->setEnabled(false);
		ui.scheduleQueueButton->setEnabled(false);
	}	
}

void CQueuePage::showEvent(QShowEvent *evt)
{TR;
	QWidget::showEvent(evt);
	
	RefreshContent();
}

void CQueuePage::hideEvent(QHideEvent *evt)
{TR;
	QWidget::hideEvent(evt);
}

void CQueuePage::RefreshContent()
{TR;
	int row=ui.queueTable->currentRow();

	ui.queueTable->clearContents();

	if(!m_batch_workqueue)
	{
		return;
	}
	
	int cnt=m_batch_workqueue->GetWorkQueueItemCount();

	ui.queueTable->setRowCount(cnt);

	for(int i=0;i<cnt;i++)
	{
		LC7WorkQueueItem item=m_batch_workqueue->GetWorkQueueItemAt(i);
		ILC7WorkQueue::STATE state=m_batch_workqueue->GetWorkQueueItemStateAt(i);

		QString sttext;
		QColor stcolor;

		switch(state)
		{
		case ILC7WorkQueue::UNVALIDATED:
			sttext="Unvalidated";
			stcolor=QColor("silver");
			break;
		case ILC7WorkQueue::VALIDATED:
			sttext="Validated";
			stcolor=QColor("white");
			break;
		case ILC7WorkQueue::INVALID:
			sttext="Invalid";
			stcolor=QColor("coral");
			break;
		case ILC7WorkQueue::IN_PROGRESS:
			sttext=">> In Progress";
stcolor = QColor("skyblue");
break;
		case ILC7WorkQueue::STOPPED:
			sttext = "Stopped";
			stcolor = QColor("gold");
			break;
		case ILC7WorkQueue::PAUSED:
			sttext = "Paused";
			stcolor = QColor("royalblue");
			break;
		case ILC7WorkQueue::COMPLETE:
			sttext = "Complete";
			stcolor = QColor("lawngreen");
			break;
		case ILC7WorkQueue::FAIL:
			sttext = "Failed";
			stcolor = QColor("red");
			break;
		default:
			sttext = "???";
			stcolor = QColor("red");
			break;
		}

		ui.queueTable->setItem(i, 0, new QTableWidgetItem(item.GetDescription()));
		ui.queueTable->setItem(i, 1, new QTableWidgetItem(sttext));

		ui.queueTable->item(i, 0)->setTextColor(stcolor);
		ui.queueTable->item(i, 1)->setTextColor(stcolor);
	}

	if (row != -1 && row < cnt)
	{
		ui.queueTable->setCurrentCell(row, 0);
	}
}

void CQueuePage::onQueueTableCurrentItemChanged(QTableWidgetItem *current, QTableWidgetItem *previous)
{
	TR;
	UpdateUI();
}


void CQueuePage::onMoveUpButton(bool checked)
{
	TR;
	int row = ui.queueTable->currentRow();
	if (row == 0 || row == -1)
	{
		return;
	}

	m_batch_workqueue->SwapWorkQueueItem(row, row - 1);

	ui.queueTable->setCurrentCell(row - 1, 0);
}

void CQueuePage::onMoveDownButton(bool checked)
{
	TR;
	int row = ui.queueTable->currentRow();
	if (row == ui.queueTable->rowCount() - 1 || row == -1)
	{
		return;
	}

	m_batch_workqueue->SwapWorkQueueItem(row, row + 1);

	ui.queueTable->setCurrentCell(row + 1, 0);
}

void CQueuePage::onRemoveButton(bool checked)
{
	TR;
	int rowcnt = ui.queueTable->rowCount();
	if (rowcnt == 0)
	{
		return;
	}
	int row = ui.queueTable->currentRow();
	if (row == -1)
	{
		return;
	}

	m_batch_workqueue->RemoveWorkQueueItem(row);	
	if (rowcnt > 1)
	{
		if (row == rowcnt - 1)
		{
			ui.queueTable->setCurrentCell(row - 1, 0);
		}
		else
		{
			ui.queueTable->setCurrentCell(row, 0);
		}
	}
}


void CQueuePage::onRunQueueButton(bool checked)
{
	TR;
	if (m_batch_workqueue->GetWorkQueueState() != ILC7WorkQueue::VALIDATED)
	{
		return;
	}

	if (!m_batch_workqueue->StartRequest())
	{
		QString error = m_batch_workqueue->GetLastError();
		g_pLinkage->GetGUILinkage()->ErrorMessage("Queue execution failed", error);
	}
}

void CQueuePage::onValidateQueueButton(bool checked)
{
	TR;
	if (m_batch_workqueue->GetWorkQueueItemCount() == 0)
	{
		return;
	}
	if (m_batch_workqueue->GetWorkQueueState() == ILC7WorkQueue::PAUSED ||
		m_batch_workqueue->GetWorkQueueState() == ILC7WorkQueue::IN_PROGRESS ||
		m_batch_workqueue->GetWorkQueueState() == ILC7WorkQueue::VALIDATED)
	{
		Q_ASSERT(0);
		return;
	}

	QString error;
	int failed_item=-1;
	if(!m_batch_workqueue->Validate(error,failed_item))
	{
		g_pLinkage->GetGUILinkage()->ErrorMessage("Validation failed",error);
	}
	else
	{
		g_pLinkage->GetGUILinkage()->InfoMessage("Validation successful","Validation was successful, queue can now run or be scheduled.");
	}

}

void CQueuePage::onScheduleQueueButton(bool checked)
{TR;
	if (!(m_batch_workqueue->GetWorkQueueState() == ILC7WorkQueue::VALIDATED ||
		m_batch_workqueue->GetWorkQueueState() == ILC7WorkQueue::STOPPED ||
		m_batch_workqueue->GetWorkQueueState() == ILC7WorkQueue::FAIL ||
		m_batch_workqueue->GetWorkQueueState() == ILC7WorkQueue::COMPLETE))
	{
		return;
	}

	ILC7TaskScheduler *sched = g_pLinkage->GetTaskScheduler();
	ILC7EditableTask *task=sched->NewTask();
	if (!task)
	{
		return;
	}
	CEditTaskDlg dlg(task, true);
	if (!dlg.exec())
	{
		sched->DeleteTask(task);
		return;
	}

	g_pLinkage->GetGUILinkage()->InfoMessage("Success", "Queue scheduled successfully.");

	if (g_pLinkage->GetSettings()->value("_ui_:switch_to_schedule", true).toBool())
	{
		g_pLinkage->GetGUILinkage()->SwitchToMainMenuTab("base/schedule");
	}

}

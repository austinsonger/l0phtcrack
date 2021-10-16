#include<stdafx.h>

class QSortableTableWidgetItem:public QTableWidgetItem
{
public:
	QSortableTableWidgetItem(int type = Type) :QTableWidgetItem(type) {} 
	QSortableTableWidgetItem(const QString &text, int type = Type) : QTableWidgetItem(text, type) {}
	QSortableTableWidgetItem(const QIcon &icon, const QString &text, int type = Type) : QTableWidgetItem(icon, text, type) {}
	QSortableTableWidgetItem(const QSortableTableWidgetItem &other) {}

	virtual bool operator<(const QTableWidgetItem &other) const
	{
		QVariant x = data(Qt::UserRole + 1);
		QVariant y = other.data(Qt::UserRole + 1);

		if (x.type() == QVariant::DateTime && y.type() == QVariant::DateTime)
		{
			return x.toDateTime() < y.toDateTime();
		}
		if (x.type() == QVariant::Date && y.type() == QVariant::Date)
		{
			return x.toDate() < y.toDate();
		}

		return QTableWidgetItem::operator<(other);
	}
};


CSchedulePage::CSchedulePage(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	m_enable_ui = true;
	
	g_pLinkage->RegisterNotifySessionActivity(SINGLE_WORKQUEUE_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CSchedulePage::NotifySessionActivity);
	g_pLinkage->RegisterNotifySessionActivity(BATCH_WORKQUEUE_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CSchedulePage::NotifySessionActivity);
	m_batch_workqueue = NULL;
	m_single_workqueue = NULL;


	ui.ScheduledTasksList->setColumnCount(5);
	QStringList headers;
	headers.append("Name");
	headers.append("Start Time");
	headers.append("End Time");
	headers.append("Recurrence");
	headers.append("Task Items");
	ui.ScheduledTasksList->setHorizontalHeaderLabels(headers);
	ui.ScheduledTasksList->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	ui.ScheduledTasksList->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	ui.ScheduledTasksList->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
	ui.ScheduledTasksList->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
	ui.ScheduledTasksList->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);

	ui.TaskOutputSessionsTable->setColumnCount(2);
	QStringList headers2;
	headers2.append("Name");
	headers2.append("Time");
	ui.TaskOutputSessionsTable->setHorizontalHeaderLabels(headers2);
	ui.TaskOutputSessionsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
	ui.TaskOutputSessionsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);


	m_helpbutton = g_pLinkage->GetGUILinkage()->GetHelpButton();
	connect(m_helpbutton, &QAbstractButton::clicked, this, &CSchedulePage::slot_helpButtonClicked);

	connect(ui.ScheduledTasksList, &QTableWidget::itemSelectionChanged, this, &CSchedulePage::slot_scheduledTasksList_itemSelectionChanged);
	connect(ui.TaskOutputSessionsTable, &QTableWidget::itemSelectionChanged, this, &CSchedulePage::slot_taskOutputSessionsTable_itemSelectionChanged);

	connect(ui.RemoveTaskButton, &QAbstractButton::clicked, this, &CSchedulePage::slot_removeTaskButton_clicked);
	connect(ui.CopyTaskToNewSessionButton, &QAbstractButton::clicked, this, &CSchedulePage::slot_copyTaskToNewSessionButton_clicked);
	connect(ui.RunTaskNowButton, &QAbstractButton::clicked, this, &CSchedulePage::slot_runTaskNowButton_clicked);
	connect(ui.EditTaskScheduleButton, &QAbstractButton::clicked, this, &CSchedulePage::slot_editTaskScheduleButton_clicked);

	connect(ui.OpenTaskOutputSessionButton, &QAbstractButton::clicked, this, &CSchedulePage::slot_openTaskOutputButton_clicked);
	connect(ui.RemoveTaskOutputSessionsButton, &QAbstractButton::clicked, this, &CSchedulePage::slot_removeTaskOutputSessionsButton_clicked);

	g_pLinkage->GetTaskScheduler()->RegisterTaskFinishedCallback(this, (void(QObject::*)(ILC7Task *)) &CSchedulePage::slot_taskFinished);

	RefreshContent();
}


CSchedulePage::~CSchedulePage()
{TR;
	g_pLinkage->GetTaskScheduler()->UnregisterTaskFinishedCallback(this, (void(QObject::*)(ILC7Task *)) &CSchedulePage::slot_taskFinished);

	g_pLinkage->UnregisterNotifySessionActivity(SINGLE_WORKQUEUE_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CSchedulePage::NotifySessionActivity);
	g_pLinkage->UnregisterNotifySessionActivity(BATCH_WORKQUEUE_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CSchedulePage::NotifySessionActivity);
}

CSchedulePage *CreateSchedulePage()
{TR;
	return new CSchedulePage();
}

void CSchedulePage::NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler)
{TR;
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


void CSchedulePage::slot_scheduledTasksList_itemSelectionChanged(void)
{TR;
	UpdateUI();
}

void CSchedulePage::slot_taskOutputSessionsTable_itemSelectionChanged(void)
{TR;
	UpdateUI();
}

void CSchedulePage::slot_helpButtonClicked()
{TR;
	UpdateUI();
}

void CSchedulePage::slot_uiEnable(bool enable)
{TR;
	m_enable_ui=enable;
	UpdateUI();
	
}

void CSchedulePage::slot_removeTaskButton_clicked(bool)
{TR;
	ILC7TaskScheduler *sched = g_pLinkage->GetTaskScheduler();

	QModelIndexList sel = ui.ScheduledTasksList->selectionModel()->selectedRows();
	if (sel.size() == 0)
	{
		return;
	}

	bool failed = false;
	QString errors;
	foreach(QModelIndex mi, sel)
	{
		ILC7Task *task = (ILC7Task *)mi.data(Qt::UserRole).toULongLong();

		QString error;
		ILC7EditableTask *editabletask = sched->UnscheduleTask(task, error);
		if (editabletask == NULL)
		{
			errors += task->GetName() + ": "+ error + "\n";
			failed = true;
		}

		sched->DeleteTask(editabletask);
	}

	if (failed)
	{
		g_pLinkage->GetGUILinkage()->ErrorMessage("Failed to remove task", errors);
	}
	
	RefreshContent();
}

void CSchedulePage::slot_copyTaskToNewSessionButton_clicked(bool)
{TR;
	ILC7TaskScheduler *sched = g_pLinkage->GetTaskScheduler();

	QModelIndexList sel = ui.ScheduledTasksList->selectionModel()->selectedRows();
	if (sel.size() != 1)
	{
		return;
	}
	ILC7Task *task = (ILC7Task *)sel.first().data(Qt::UserRole).toULongLong();

	if (!sched->NewSessionFromTask(task))
	{
		g_pLinkage->GetGUILinkage()->ErrorMessage("Failed to create session","Could not create session from task.");
	}
}


void CSchedulePage::slot_runTaskNowButton_clicked(void)
{TR;
	ILC7TaskScheduler *sched = g_pLinkage->GetTaskScheduler();

	QModelIndexList sel = ui.ScheduledTasksList->selectionModel()->selectedRows();
	if (sel.size() != 1)
	{
		return;
	}
	ILC7Task *task = (ILC7Task *)sel.first().data(Qt::UserRole).toULongLong();

	if (!sched->RunTask(task, false))
	{
		g_pLinkage->GetGUILinkage()->ErrorMessage("Failed to run task", "Could not run task.");
	}
}

void CSchedulePage::slot_editTaskScheduleButton_clicked(bool)
{TR;
	ILC7TaskScheduler *sched = g_pLinkage->GetTaskScheduler();

	QModelIndexList sel = ui.ScheduledTasksList->selectionModel()->selectedRows();
	if (sel.size() != 1)
	{
		return;
	}
	ILC7Task *task = (ILC7Task *)sel.first().data(Qt::UserRole).toULongLong();

	if (task->GetExternallyModified())
	{
		g_pLinkage->GetGUILinkage()->ErrorMessage("Can't edit this task", "Can't edit task once it has been externally modified in the Windows Task Scheduler. Use the Task Scheduler to edit this task directly, or remove it and add a new task here.");
		return;
	}

	ILC7EditableTask *editabletask = sched->EditScheduledTask(task);

	CEditTaskDlg dlg(editabletask, false);
	if (!dlg.exec())
	{
		sched->DeleteTask(editabletask);
		return;
	}

	RefreshContent();
}

void CSchedulePage::slot_openTaskOutputButton_clicked(void)
{TR;
	QModelIndexList sel = ui.TaskOutputSessionsTable->selectionModel()->selectedRows();
	if (sel.size() != 1)
	{
		return;
	}
	QString sessionpath = sel.first().data(Qt::UserRole).toString();

	g_pLinkage->GetGUILinkage()->RequestOpenSession(sessionpath);
}

void CSchedulePage::slot_removeTaskOutputSessionsButton_clicked(void)
{TR;
	QModelIndexList sel = ui.TaskOutputSessionsTable->selectionModel()->selectedRows();
	if (sel.size() == 0)
	{
		return;
	}
	
	QStringList dels;
	foreach(QModelIndex idx, sel)
	{
		QString sessionpath = idx.data(Qt::UserRole).toString();
		dels.append(sessionpath);
	}

	QString del;
	foreach(del, dels)
	{
		QFile::remove(del);
	}

	
	RefreshContent();
}



void CSchedulePage::showEvent(QShowEvent *evt)
{TR;
	QWidget::showEvent(evt);

	RefreshContent();
}

void CSchedulePage::hideEvent(QHideEvent *evt)
{TR;
	QWidget::hideEvent(evt);
}


void CSchedulePage::RefreshCurrentlyScheduledTasks(void)
{TR;
	ILC7TaskScheduler *sched = g_pLinkage->GetTaskScheduler();

	sched->RefreshTasks();

	int cnt = sched->GetScheduledTaskCount();
	ui.ScheduledTasksList->setRowCount(cnt);
	for (int i = 0; i < cnt; i++)
	{
		ILC7Task *task = sched->GetScheduledTask(i);

		ui.ScheduledTasksList->setItem(i, 0, new QTableWidgetItem(task->GetName()));
		ui.ScheduledTasksList->setItem(i, 1, new QSortableTableWidgetItem(task->GetStartTime().toString(Qt::DateFormat::SystemLocaleShortDate)));
		ui.ScheduledTasksList->item(i, 1)->setData(Qt::UserRole + 1, task->GetStartTime());
		if (task->GetExpirationEnabled())
		{
			ui.ScheduledTasksList->setItem(i, 2, new QSortableTableWidgetItem(task->GetExpirationDate().toString(Qt::DateFormat::SystemLocaleShortDate)));
			ui.ScheduledTasksList->item(i, 2)->setData(Qt::UserRole + 1, task->GetExpirationDate());
		}
		else
		{
			ui.ScheduledTasksList->setItem(i, 2, new QTableWidgetItem("None"));
		}

		QString rec;
		if (task->GetExternallyModified())
		{
			rec = "Task was externally modified";
		}
		else
		{
			switch (task->GetRecurrence())
			{
			case ILC7Task::RECURRENCE::ONE_TIME:
				rec = "One Time";
				break;
			case ILC7Task::RECURRENCE::DAILY:
				if (task->GetDailyRecurrence() == 1)
				{
					rec = "Daily";
				}
				else
				{
					rec = QString("Every %1 days").arg(task->GetDailyRecurrence());
				}
				break;
			case ILC7Task::RECURRENCE::WEEKLY:
			{
				if (task->GetWeeklyRecurrence() == 1)
				{
					rec = "Weekly";
				}
				else
				{
					rec = QString("Every %1 weeks").arg(task->GetWeeklyRecurrence());
				}

				quint32 ed = task->GetEnabledWeekDaysBitMask();
				if (ed != 0x000007F)
				{
					rec += " on ";
					QStringList days;
					const char *dstr[] = {
						"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
					};
					int d = 0;
					while (ed != 0 && (d < 7))
					{
						if ((ed & 1) == 1)
						{
							days.append(dstr[d]);
						}
						ed >>= 1;
						d++;
					}
					rec += days.join(",");
				}
				else
				{
					rec += " on every day";
				}
			}
			break;
			case ILC7Task::RECURRENCE::MONTHLY:
			{
				quint32 em = task->GetEnabledMonthsBitMask();
				if (em != 0x00000FFF)
				{
					rec = "Monthly: ";

					QStringList months;
					const char *mstr[] = {
						"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
					};
					int m = 0;
					while (em != 0 && (m < 12))
					{
						if ((em & 1) == 1)
						{
							months.append(mstr[m]);
						}
						em >>= 1;
						m++;
					}
					rec += months.join(",");
				}
				else
				{
					rec = "Monthly";
				}

				if (task->GetMonthlyMode() == ILC7Task::SPECIFIC_DAY)
				{
					rec += QString(" on day %1").arg(task->GetSpecificDayOfMonth());
				}
				else
				{
					switch (task->GetAbstractTiming())
					{
					default:
						Q_ASSERT(0);
					case ILC7Task::FIRST:
						rec += " on the first ";
						break;
					case ILC7Task::SECOND:
						rec += " on the second ";
						break;
					case ILC7Task::THIRD:
						rec += " on the third ";
						break;
					case ILC7Task::FOURTH:
						rec += " on the fourth ";
						break;
					case ILC7Task::LAST:
						rec += " on the last ";
						break;
					}

					switch (task->GetAbstractDayOfWeek())
					{
					default:
						Q_ASSERT(0);
					case ILC7Task::SUNDAY:
						rec += "Sunday";
						break;
					case ILC7Task::MONDAY:
						rec += "Monday";
						break;
					case ILC7Task::TUESDAY:
						rec += "Tuesday";
						break;
					case ILC7Task::WEDNESDAY:
						rec += "Wednesday";
						break;
					case ILC7Task::THURSDAY:
						rec += "Thursday";
						break;
					case ILC7Task::FRIDAY:
						rec += "Friday";
						break;
					case ILC7Task::SATURDAY:
						rec += "Saturday";
						break;
					}
				}
			}
			break;
			}
		}
		ui.ScheduledTasksList->setItem(i, 3, new QTableWidgetItem(rec));

		ui.ScheduledTasksList->setItem(i, 4, new QTableWidgetItem(task->GetTaskDescriptions().join("\n")));

		ui.ScheduledTasksList->item(i, 0)->setData(Qt::UserRole, QVariant((qulonglong)task));
	}

	UpdateUI();

}

void CSchedulePage::RefreshTaskOutputSessions(void)
{TR;
	// Get task save path
	QDir taskoutdir(g_pLinkage->GetSettings()->value("_core_:task_output_directory").toString());
	
	if (!taskoutdir.exists())
	{
		return;
	}

	QStringList namefilters;
	namefilters << "*.lc7";
	QStringList entries = taskoutdir.entryList(namefilters, QDir::Files, QDir::Time);

	ui.TaskOutputSessionsTable->setRowCount(entries.size());

	int i = 0;
	foreach(QString entry, entries)
	{
		QString entrypath = taskoutdir.absoluteFilePath(entry);

		QFileInfo fi(entrypath);
		QDateTime dt(fi.lastModified());

		ui.TaskOutputSessionsTable->setItem(i, 0, new QTableWidgetItem(fi.fileName()));
		ui.TaskOutputSessionsTable->setItem(i, 1, new QSortableTableWidgetItem(dt.toString(Qt::DateFormat::SystemLocaleShortDate)));
		ui.TaskOutputSessionsTable->item(i, 1)->setData(Qt::UserRole + 1, dt);

		ui.TaskOutputSessionsTable->item(i, 0)->setData(Qt::UserRole, QVariant(entrypath));

		i++;
	}
}

void CSchedulePage::RefreshContent(void)
{TR;
	RefreshCurrentlyScheduledTasks();
	RefreshTaskOutputSessions();
}

void CSchedulePage::UpdateUI()
{TR;
	ui.help->setVisible(m_helpbutton->isChecked());

	if (!m_enable_ui)
	{
		ui.CopyTaskToNewSessionButton->setEnabled(false);
		ui.EditTaskScheduleButton->setEnabled(false);
		ui.RemoveTaskButton->setEnabled(false);
		ui.RunTaskNowButton->setEnabled(false);
		ui.OpenTaskOutputSessionButton->setEnabled(false);
		ui.RemoveTaskOutputSessionsButton->setEnabled(false);
		return;
	}

	{
		QModelIndexList sel = ui.ScheduledTasksList->selectionModel()->selectedRows();
		bool selected = sel.count() > 0;
		bool selected_one = sel.count() == 1;

		ui.CopyTaskToNewSessionButton->setEnabled(selected_one);
		ui.EditTaskScheduleButton->setEnabled(selected_one);
		ui.RemoveTaskButton->setEnabled(selected);
		ui.RunTaskNowButton->setEnabled(selected_one);
	}

	{
		QModelIndexList sel = ui.TaskOutputSessionsTable->selectionModel()->selectedRows();
		bool selected = sel.count() > 0;
		bool selected_one = sel.count() == 1;

		ui.OpenTaskOutputSessionButton->setEnabled(selected_one);
		ui.RemoveTaskOutputSessionsButton->setEnabled(selected);
	}

}

void CSchedulePage::slot_taskFinished(ILC7Task *finished_task)
{TR;
	RefreshContent();
}
#include<stdafx.h>

CEditTaskDlg::CEditTaskDlg(ILC7EditableTask *task, bool isNewTask) : QDialog(NULL)
{
	ui.setupUi(this);

	m_task = task;
	QDataStream save(&m_saved_task,QIODevice::WriteOnly);
	m_task->Save(save);
	
	m_is_new_task = isNewTask;
	
	connect(ui.buttonBoxOkCancel, &QDialogButtonBox::clicked, this, &CEditTaskDlg::slot_buttonBoxClicked);
	connect(this, &CEditTaskDlg::rejected, this, &CEditTaskDlg::slot_rejectedRollback);

	connect(ui.taskName, &QLineEdit::textChanged, this, &CEditTaskDlg::slot_textChanged_taskName);
	connect(ui.saveTaskOutputCheckBox, &QCheckBox::clicked, this, &CEditTaskDlg::slot_clicked_saveTaskOutputCheckBox);
	connect(ui.startDateTime, &QDateTimeEdit::dateTimeChanged, this, &CEditTaskDlg::slot_dateTimeChanged_startDateTime);
	connect(ui.expirationCheckBox, &QCheckBox::clicked, this, &CEditTaskDlg::slot_clicked_expirationCheckBox);
	connect(ui.expirationDate, &QDateEdit::dateChanged, this, &CEditTaskDlg::slot_dateChanged_expirationDate);
	connect(ui.OneTimeRadio, &QRadioButton::clicked, this, &CEditTaskDlg::slot_clicked_OneTimeRadio);
	connect(ui.DailyRadio, &QRadioButton::clicked, this, &CEditTaskDlg::slot_clicked_DailyRadio);
	connect(ui.WeeklyRadio, &QRadioButton::clicked, this, &CEditTaskDlg::slot_clicked_WeeklyRadio);
	connect(ui.MonthlyRadio, &QRadioButton::clicked, this, &CEditTaskDlg::slot_clicked_MonthlyRadio);
	connect(ui.everyNDaysEdit, &QLineEdit::textChanged, this, &CEditTaskDlg::slot_textChanged_everyNDaysEdit);
	connect(ui.everyNWeeksEdit, &QLineEdit::textChanged, this, &CEditTaskDlg::slot_textChanged_everyNWeeksEdit);
	connect(ui.sundayCheckBox, &QCheckBox::clicked, this, &CEditTaskDlg::slot_clicked_sundayCheckBox);
	connect(ui.mondayCheckBox, &QCheckBox::clicked, this, &CEditTaskDlg::slot_clicked_mondayCheckBox);
	connect(ui.tuesdayCheckBox, &QCheckBox::clicked, this, &CEditTaskDlg::slot_clicked_tuesdayCheckBox);
	connect(ui.wednesdayCheckBox, &QCheckBox::clicked, this, &CEditTaskDlg::slot_clicked_wednesdayCheckBox);
	connect(ui.thursdayCheckBox, &QCheckBox::clicked, this, &CEditTaskDlg::slot_clicked_thursdayCheckBox);
	connect(ui.fridayCheckBox, &QCheckBox::clicked, this, &CEditTaskDlg::slot_clicked_fridayCheckBox);
	connect(ui.saturdayCheckBox, &QCheckBox::clicked, this, &CEditTaskDlg::slot_clicked_saturdayCheckBox);
	connect(ui.dayRadioButton, &QRadioButton::clicked, this, &CEditTaskDlg::slot_clicked_dayRadioButton);
	connect(ui.theRadioButton, &QRadioButton::clicked, this, &CEditTaskDlg::slot_clicked_theRadioButton);
	connect(ui.dayNMonthSpinBox, (void (QSpinBox::*)(int))&QSpinBox::valueChanged, this, &CEditTaskDlg::slot_valueChanged_dayNMonthSpinBox);
	connect(ui.whichTimeComboBox, (void (QComboBox::*)(int))&QComboBox::currentIndexChanged, this, &CEditTaskDlg::slot_currentIndexChanged_whichTimeComboBox);
	connect(ui.dayOfTheWeekComboBox, (void (QComboBox::*)(int))&QComboBox::currentIndexChanged, this, &CEditTaskDlg::slot_currentIndexChanged_dayOfTheWeekComboBox);
	connect(ui.januaryCheckBox, &QCheckBox::clicked, this, &CEditTaskDlg::slot_clicked_januaryCheckBox);
	connect(ui.februaryCheckBox, &QCheckBox::clicked, this, &CEditTaskDlg::slot_clicked_februaryCheckBox);
	connect(ui.marchCheckBox, &QCheckBox::clicked, this, &CEditTaskDlg::slot_clicked_marchCheckBox);
	connect(ui.aprilCheckBox, &QCheckBox::clicked, this, &CEditTaskDlg::slot_clicked_aprilCheckBox);
	connect(ui.mayCheckBox, &QCheckBox::clicked, this, &CEditTaskDlg::slot_clicked_mayCheckBox);
	connect(ui.juneCheckBox, &QCheckBox::clicked, this, &CEditTaskDlg::slot_clicked_juneCheckBox);
	connect(ui.julyCheckBox, &QCheckBox::clicked, this, &CEditTaskDlg::slot_clicked_julyCheckBox);
	connect(ui.augustCheckBox, &QCheckBox::clicked, this, &CEditTaskDlg::slot_clicked_augustCheckBox);
	connect(ui.septemberCheckBox, &QCheckBox::clicked, this, &CEditTaskDlg::slot_clicked_septemberCheckBox);
	connect(ui.octoberCheckBox, &QCheckBox::clicked, this, &CEditTaskDlg::slot_clicked_octoberCheckBox);
	connect(ui.novemberCheckBox, &QCheckBox::clicked, this, &CEditTaskDlg::slot_clicked_novemberCheckBox);
	connect(ui.decemberCheckBox, &QCheckBox::clicked, this, &CEditTaskDlg::slot_clicked_decemberCheckBox);

	ui.everyNDaysEdit->setValidator(new QIntValidator(1, 365, this));
	ui.everyNWeeksEdit->setValidator(new QIntValidator(1, 52, this));

	RefreshContent();
}

CEditTaskDlg::~CEditTaskDlg()
{TR;
}

void CEditTaskDlg::RefreshContent(void)
{TR;
	ui.taskName->setText(m_task->GetName());

	// Clear
	ui.scheduledOperationsTable->clear();
	ui.scheduledOperationsTable->setColumnCount(1);
	ui.scheduledOperationsTable->setRowCount(0);

	// Set up
	QStringList descs = m_task->GetTaskDescriptions();
	ui.scheduledOperationsTable->setRowCount(descs.count());

	QStringList verticallabels;
	for (int i = 1; i <= descs.count(); i++)
	{
		verticallabels.append(QString("%1").arg(i));
	}

	ui.scheduledOperationsTable->setVerticalHeaderLabels(verticallabels);

	QStringList headerlabels;
	headerlabels.append("Task Queue Items");
	ui.scheduledOperationsTable->setHorizontalHeaderLabels(headerlabels);

	int i = 0;
	foreach(QString desc, descs)
	{
		ui.scheduledOperationsTable->setItem(i, 0, new QTableWidgetItem(desc));
		i++;
	}

	// Task
	ui.startDateTime->setDateTime(m_task->GetStartTime());
	ui.expirationCheckBox->setChecked(m_task->GetExpirationEnabled());
	ui.expirationDate->setDate(m_task->GetExpirationDate());
	switch (m_task->GetRecurrence())
	{
	case ILC7Task::ONE_TIME:
		ui.OneTimeRadio->setChecked(true);
		break;
	case ILC7Task::DAILY:
		ui.DailyRadio->setChecked(true);
		break;
	case ILC7Task::WEEKLY:
		ui.WeeklyRadio->setChecked(true);
		break;
	case ILC7Task::MONTHLY:
		ui.MonthlyRadio->setChecked(true);
		break;
	}
	
	ui.everyNDaysEdit->setText(QString("%1").arg(m_task->GetDailyRecurrence()));

	ui.everyNWeeksEdit->setText(QString("%1").arg(m_task->GetWeeklyRecurrence()));

	ui.sundayCheckBox->setChecked((m_task->GetEnabledWeekDaysBitMask() & (1 << 0)) != 0);
	ui.mondayCheckBox->setChecked((m_task->GetEnabledWeekDaysBitMask() & (1 << 1)) != 0);
	ui.tuesdayCheckBox->setChecked((m_task->GetEnabledWeekDaysBitMask() & (1 << 2)) != 0);
	ui.wednesdayCheckBox->setChecked((m_task->GetEnabledWeekDaysBitMask() & (1 << 3)) != 0);
	ui.thursdayCheckBox->setChecked((m_task->GetEnabledWeekDaysBitMask() & (1 << 4)) != 0);
	ui.fridayCheckBox->setChecked((m_task->GetEnabledWeekDaysBitMask() & (1 << 5)) != 0);
	ui.saturdayCheckBox->setChecked((m_task->GetEnabledWeekDaysBitMask() & (1 << 6)) != 0);

	switch (m_task->GetMonthlyMode())
	{
	case ILC7Task::SPECIFIC_DAY:
		ui.dayRadioButton->setChecked(true);
		break;
	case ILC7Task::ABSTRACT_DAY:
		ui.theRadioButton->setChecked(true);
		break;
	}

	ui.dayNMonthSpinBox->setValue(m_task->GetSpecificDayOfMonth());
	ui.whichTimeComboBox->setCurrentIndex(m_task->GetAbstractTiming());
	
	ui.dayOfTheWeekComboBox->setCurrentIndex(m_task->GetAbstractDayOfWeek());
	ui.januaryCheckBox->setChecked((m_task->GetEnabledMonthsBitMask() & (1 << 0)) != 0);
	ui.februaryCheckBox->setChecked((m_task->GetEnabledMonthsBitMask() & (1 << 1)) != 0);
	ui.marchCheckBox->setChecked((m_task->GetEnabledMonthsBitMask() & (1 << 2)) != 0);
	ui.aprilCheckBox->setChecked((m_task->GetEnabledMonthsBitMask() & (1 << 3)) != 0);
	ui.mayCheckBox->setChecked((m_task->GetEnabledMonthsBitMask() & (1 << 4)) != 0);
	ui.juneCheckBox->setChecked((m_task->GetEnabledMonthsBitMask() & (1 << 5)) != 0);
	ui.julyCheckBox->setChecked((m_task->GetEnabledMonthsBitMask() & (1 << 6)) != 0);
	ui.augustCheckBox->setChecked((m_task->GetEnabledMonthsBitMask() & (1 << 7)) != 0);
	ui.septemberCheckBox->setChecked((m_task->GetEnabledMonthsBitMask() & (1 << 8)) != 0);
	ui.octoberCheckBox->setChecked((m_task->GetEnabledMonthsBitMask() & (1 << 9)) != 0);
	ui.novemberCheckBox->setChecked((m_task->GetEnabledMonthsBitMask() & (1 << 10)) != 0);
	ui.decemberCheckBox->setChecked((m_task->GetEnabledMonthsBitMask() & (1 << 11)) != 0);

	UpdateUI();
}

void CEditTaskDlg::UpdateUI(void)
{TR;
	if (m_is_new_task)
	{
		this->setWindowTitle("New Job");
	}
	else
	{
		this->setWindowTitle("Edit Job");
	}

	ui.expirationDate->setEnabled(ui.expirationCheckBox->isChecked());

	if (ui.OneTimeRadio->isChecked())
	{
		ui.stackedWidget->setCurrentIndex(0);
	}
	else if (ui.DailyRadio->isChecked())
	{
		ui.stackedWidget->setCurrentIndex(1);
	}
	else if (ui.WeeklyRadio->isChecked())
	{
		ui.stackedWidget->setCurrentIndex(2);
	}
	else if (ui.MonthlyRadio->isChecked())
	{
		ui.stackedWidget->setCurrentIndex(3);
	}

	ui.dayNMonthSpinBox->setEnabled(ui.dayRadioButton->isChecked());
	ui.ofTheMonthLabel->setEnabled(ui.dayRadioButton->isChecked());

	ui.whichTimeComboBox->setEnabled(ui.theRadioButton->isChecked());
	ui.whichTimeComboBox->setEnabled(ui.theRadioButton->isChecked());
	ui.dayOfTheWeekComboBox->setEnabled(ui.theRadioButton->isChecked());
	ui.ofTheMonthLabel2->setEnabled(ui.theRadioButton->isChecked());

	QString error;
	ui.buttonBoxOkCancel->button(QDialogButtonBox::Ok)->setEnabled(m_task->Validate(error));
}

void CEditTaskDlg::slot_buttonBoxClicked(QAbstractButton *button)
{TR;
	if (ui.buttonBoxOkCancel->buttonRole(button) == QDialogButtonBox::AcceptRole)
	{
		QString error;
		if (!m_task->Validate(error))
		{
			g_pLinkage->GetGUILinkage()->ErrorMessage("Validation Error", error);
			return;
		}

		if (VerifyDialog())
		{
			if (m_is_new_task)
			{
				bool cancelled = false;
				if (!CommitNew(error, cancelled))
				{
					g_pLinkage->GetGUILinkage()->ErrorMessage("Create Task Error", error);
					return;
				}
				if (cancelled)
				{
					return;
				}
			}
			else
			{
				bool cancelled = false;
				if (!CommitEdit(error, cancelled))
				{
					g_pLinkage->GetGUILinkage()->ErrorMessage("Edit Task Error", error);
					return;
				}
				if (cancelled)
				{
					return;
				}
			}

			accept();
		}
	}
	else if (ui.buttonBoxOkCancel->buttonRole(button) == QDialogButtonBox::RejectRole)
	{
		reject();
	}
	else
	{
		Q_ASSERT(0);
	}
}

void CEditTaskDlg::slot_rejectedRollback()
{TR;
	QDataStream load(&m_saved_task, QIODevice::ReadOnly);
	m_task->Load(load);
}


bool CEditTaskDlg::VerifyDialog(void)
{TR;
	if (ui.OneTimeRadio->isChecked())
	{
		QDateTime picked = ui.startDateTime->dateTime();
		QDateTime now = QDateTime::currentDateTime();
		if (picked < now)
		{
			if (!g_pLinkage->GetGUILinkage()->YesNoBox("Are you sure?", "The selected start date/time is in the past. This job will never run. Are you sure you sure this is what you want?"))
			{
				return false;
			}
		}
	}

	if (ui.expirationCheckBox->isChecked())
	{
		QDateTime start = ui.startDateTime->dateTime();
		QDateTime end = QDateTime(ui.expirationDate->date(), QTime(0, 0, 0, 0));
		QDateTime now = QDateTime::currentDateTime();

		if (end < now)
		{
			if (!g_pLinkage->GetGUILinkage()->YesNoBox("Are you sure?", "The selected expiration date/time is in the past. This job will never run. Are you sure you sure this is what you want?"))
			{
				return false;
			}
		}

		if (end <= start)
		{
			if (!g_pLinkage->GetGUILinkage()->YesNoBox("Are you sure?", "The selected expiration date/time is before or equal to the start time. This job will never run. Are you sure you sure this is what you want?"))
			{
				return false;
			}
		}
	}
	
	return true;
}


void CEditTaskDlg::slot_textChanged_taskName(const QString &name)
{TR;
	m_task->SetName(name);
	UpdateUI();
}

void CEditTaskDlg::slot_clicked_saveTaskOutputCheckBox(bool checked)
{TR;
	m_task->SetSaveTaskOutput(checked);
	UpdateUI();
}


void CEditTaskDlg::slot_dateTimeChanged_startDateTime(const QDateTime &dateTime)
{TR;
	m_task->SetStartTime(dateTime);
	UpdateUI();
}

void CEditTaskDlg::slot_clicked_expirationCheckBox(bool checked)
{TR;
	m_task->SetExpirationEnabled(checked);
	UpdateUI();
}

void CEditTaskDlg::slot_dateChanged_expirationDate(const QDate &date)
{TR;
	m_task->SetExpirationDate(date);
	UpdateUI();
}

void CEditTaskDlg::slot_clicked_OneTimeRadio(bool checked)
{TR;
	m_task->SetRecurrence(ILC7Task::ONE_TIME);
	UpdateUI();
}

void CEditTaskDlg::slot_clicked_DailyRadio(bool checked)
{TR;
	m_task->SetRecurrence(ILC7Task::DAILY);

	UpdateUI();
}

void CEditTaskDlg::slot_clicked_WeeklyRadio(bool checked)
{TR;
	m_task->SetRecurrence(ILC7Task::WEEKLY);
	UpdateUI();
}

void CEditTaskDlg::slot_clicked_MonthlyRadio(bool checked)
{TR;
	m_task->SetRecurrence(ILC7Task::MONTHLY);
	UpdateUI();
}

void CEditTaskDlg::slot_textChanged_everyNDaysEdit(const QString &days)
{TR;
	m_task->SetDailyRecurrence(days.toInt());
	UpdateUI();
}

void CEditTaskDlg::slot_textChanged_everyNWeeksEdit(const QString &weeks)
{TR;
	m_task->SetWeeklyRecurrence(weeks.toInt());
	UpdateUI();
}

void CEditTaskDlg::slot_clicked_sundayCheckBox(bool checked)
{TR;
	if (checked)
	{
		m_task->SetEnabledWeekDaysBitMask(m_task->GetEnabledWeekDaysBitMask() | (1 << 0));
	}
	else
	{
		m_task->SetEnabledWeekDaysBitMask(m_task->GetEnabledWeekDaysBitMask() & ~(1 << 0));
	}
	UpdateUI();
}

void CEditTaskDlg::slot_clicked_mondayCheckBox(bool checked)
{TR;
	if (checked)
	{
		m_task->SetEnabledWeekDaysBitMask(m_task->GetEnabledWeekDaysBitMask() | (1 << 1));
	}
	else
	{
		m_task->SetEnabledWeekDaysBitMask(m_task->GetEnabledWeekDaysBitMask() & ~(1 << 1));
	}
	UpdateUI();
}

void CEditTaskDlg::slot_clicked_tuesdayCheckBox(bool checked)
{TR;
	if (checked)
	{
		m_task->SetEnabledWeekDaysBitMask(m_task->GetEnabledWeekDaysBitMask() | (1 << 2));
	}
	else
	{
		m_task->SetEnabledWeekDaysBitMask(m_task->GetEnabledWeekDaysBitMask() & ~(1 << 2));
	}
	UpdateUI();
}

void CEditTaskDlg::slot_clicked_wednesdayCheckBox(bool checked)
{TR;
	if (checked)
	{
		m_task->SetEnabledWeekDaysBitMask(m_task->GetEnabledWeekDaysBitMask() | (1 << 3));
	}
	else
	{
		m_task->SetEnabledWeekDaysBitMask(m_task->GetEnabledWeekDaysBitMask() & ~(1 << 3));
	}
	UpdateUI();
}

void CEditTaskDlg::slot_clicked_thursdayCheckBox(bool checked)
{TR;
	if (checked)
	{
		m_task->SetEnabledWeekDaysBitMask(m_task->GetEnabledWeekDaysBitMask() | (1 << 4));
	}
	else
	{
		m_task->SetEnabledWeekDaysBitMask(m_task->GetEnabledWeekDaysBitMask() & ~(1 << 4));
	}
	UpdateUI();
}

void CEditTaskDlg::slot_clicked_fridayCheckBox(bool checked)
{TR;
	if (checked)
	{
		m_task->SetEnabledWeekDaysBitMask(m_task->GetEnabledWeekDaysBitMask() | (1 << 5));
	}
	else
	{
		m_task->SetEnabledWeekDaysBitMask(m_task->GetEnabledWeekDaysBitMask() & ~(1 << 5));
	}
	UpdateUI();
}

void CEditTaskDlg::slot_clicked_saturdayCheckBox(bool checked)
{TR;
	if (checked)
	{
		m_task->SetEnabledWeekDaysBitMask(m_task->GetEnabledWeekDaysBitMask() | (1 << 6));
	}
	else
	{
		m_task->SetEnabledWeekDaysBitMask(m_task->GetEnabledWeekDaysBitMask() & ~(1 << 6));
	}
	UpdateUI();
}

void CEditTaskDlg::slot_clicked_dayRadioButton(bool checked)
{TR;
	m_task->SetMonthlyMode(ILC7Task::SPECIFIC_DAY);
	UpdateUI();
}

void CEditTaskDlg::slot_clicked_theRadioButton(bool checked)
{TR;
	m_task->SetMonthlyMode(ILC7Task::ABSTRACT_DAY);
	UpdateUI();
}

void CEditTaskDlg::slot_valueChanged_dayNMonthSpinBox(int value)
{TR;
	m_task->SetSpecificDayOfMonth(value);
	UpdateUI();
}

void CEditTaskDlg::slot_currentIndexChanged_whichTimeComboBox(int index)
{TR;
	m_task->SetAbstractTiming((ILC7Task::TIMING)index);
	UpdateUI();
}

void CEditTaskDlg::slot_currentIndexChanged_dayOfTheWeekComboBox(int index)
{TR;
	m_task->SetAbstractDayOfWeek((ILC7Task::DAY_OF_WEEK)index);
	UpdateUI();
}

void CEditTaskDlg::slot_clicked_januaryCheckBox(bool checked)
{TR;
	if (checked)
	{
		m_task->SetEnabledMonthsBitMask(m_task->GetEnabledMonthsBitMask() | (1 << 0));
	}
	else
	{
		m_task->SetEnabledMonthsBitMask(m_task->GetEnabledMonthsBitMask() & ~(1 << 0));
	}
}

void CEditTaskDlg::slot_clicked_februaryCheckBox(bool checked)
{TR;
	if (checked)
	{
		m_task->SetEnabledMonthsBitMask(m_task->GetEnabledMonthsBitMask() | (1 << 1));
	}
	else
	{
		m_task->SetEnabledMonthsBitMask(m_task->GetEnabledMonthsBitMask() & ~(1 << 1));
	}
}

void CEditTaskDlg::slot_clicked_marchCheckBox(bool checked)
{TR;
	if (checked)
	{
		m_task->SetEnabledMonthsBitMask(m_task->GetEnabledMonthsBitMask() | (1 << 2));
	}
	else
	{
		m_task->SetEnabledMonthsBitMask(m_task->GetEnabledMonthsBitMask() & ~(1 << 2));
	}
}

void CEditTaskDlg::slot_clicked_aprilCheckBox(bool checked)
{TR;
	if (checked)
	{
		m_task->SetEnabledMonthsBitMask(m_task->GetEnabledMonthsBitMask() | (1 << 3));
	}
	else
	{
		m_task->SetEnabledMonthsBitMask(m_task->GetEnabledMonthsBitMask() & ~(1 << 3));
	}
}

void CEditTaskDlg::slot_clicked_mayCheckBox(bool checked)
{TR;
	if (checked)
	{
		m_task->SetEnabledMonthsBitMask(m_task->GetEnabledMonthsBitMask() | (1 << 4));
	}
	else
	{
		m_task->SetEnabledMonthsBitMask(m_task->GetEnabledMonthsBitMask() & ~(1 << 4));
	}
}

void CEditTaskDlg::slot_clicked_juneCheckBox(bool checked)
{TR;
	if (checked)
	{
		m_task->SetEnabledMonthsBitMask(m_task->GetEnabledMonthsBitMask() | (1 << 5));
	}
	else
	{
		m_task->SetEnabledMonthsBitMask(m_task->GetEnabledMonthsBitMask() & ~(1 << 5));
	}
}

void CEditTaskDlg::slot_clicked_julyCheckBox(bool checked)
{TR;
	if (checked)
	{
		m_task->SetEnabledMonthsBitMask(m_task->GetEnabledMonthsBitMask() | (1 << 6));
	}
	else
	{
		m_task->SetEnabledMonthsBitMask(m_task->GetEnabledMonthsBitMask() & ~(1 << 6));
	}
}

void CEditTaskDlg::slot_clicked_augustCheckBox(bool checked)
{TR;
	if (checked)
	{
		m_task->SetEnabledMonthsBitMask(m_task->GetEnabledMonthsBitMask() | (1 << 7));
	}
	else
	{
		m_task->SetEnabledMonthsBitMask(m_task->GetEnabledMonthsBitMask() & ~(1 << 7));
	}
}

void CEditTaskDlg::slot_clicked_septemberCheckBox(bool checked)
{TR;
	if (checked)
	{
		m_task->SetEnabledMonthsBitMask(m_task->GetEnabledMonthsBitMask() | (1 << 8));
	}
	else
	{
		m_task->SetEnabledMonthsBitMask(m_task->GetEnabledMonthsBitMask() & ~(1 << 8));
	}
}

void CEditTaskDlg::slot_clicked_octoberCheckBox(bool checked)
{TR;
	if (checked)
	{
		m_task->SetEnabledMonthsBitMask(m_task->GetEnabledMonthsBitMask() | (1 << 9));
	}
	else
	{
		m_task->SetEnabledMonthsBitMask(m_task->GetEnabledMonthsBitMask() & ~(1 << 9));
	}
}

void CEditTaskDlg::slot_clicked_novemberCheckBox(bool checked)
{TR;
	if (checked)
	{
		m_task->SetEnabledMonthsBitMask(m_task->GetEnabledMonthsBitMask() | (1 << 10));
	}
	else
	{
		m_task->SetEnabledMonthsBitMask(m_task->GetEnabledMonthsBitMask() & ~(1 << 10));
	}
}

void CEditTaskDlg::slot_clicked_decemberCheckBox(bool checked)
{TR;
	if (checked)
	{
		m_task->SetEnabledMonthsBitMask(m_task->GetEnabledMonthsBitMask() | (1 << 11));
	}
	else
	{
		m_task->SetEnabledMonthsBitMask(m_task->GetEnabledMonthsBitMask() & ~(1 << 11));
	}
}


bool CEditTaskDlg::CommitNew(QString &error, bool &cancelled)
{TR;
	ILC7TaskScheduler *sched = g_pLinkage->GetTaskScheduler();
	
	if (sched->ScheduleTask(m_task, error, cancelled) == NULL && !cancelled)
	{
		return false;
	}
	return true;
}

bool CEditTaskDlg::CommitEdit(QString &error, bool &cancelled)
{TR;
	ILC7TaskScheduler *sched = g_pLinkage->GetTaskScheduler();

	if (!sched->CommitScheduledTask(m_task, error, cancelled) && !cancelled)
	{
		return false;
	}
	return true;
}
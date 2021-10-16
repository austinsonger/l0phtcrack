#include"stdafx.h"

CPageScheduling::CPageScheduling(QWidget *parent)
	: CLC7WizardPage(parent), m_buttongroup(this)
{
	TR;
	setTitle(tr("Job Scheduling"));
	//setPixmap(QWizard::WatermarkPixmap, QPixmap(":/images/watermark.png"));

	QLabel *topLabel = new QLabel(tr("When would you like to perform this job? Jobs can be run immediately, or they can be scheduled for a later date and time. If you want more control over job execution, such as recurrence, this is available through the queue directly, but not through this wizard."));
	topLabel->setWordWrap(true);

	m_immediateRadioButton = new CLabelRadioButton(tr("<b>&Run this job immediately</b><br>The job will start as soon as the wizard is completed."));
	WIZARD_CONNECT_BUTTON(m_immediateRadioButton->radioButton());
	m_scheduleRadioButton = new CLabelRadioButton(tr("<b>&Schedule this job to run later</b><br>The job will start at a specified date and time.<ul><li>If L0phtCrack 7 is open, it will ask to interrupt your session and perform the job.</li><li>If LC7 is closed, it will open LC7 and run in the system tray.</li><li>If you are logged out, it will run in the background and you can attach to the job by logging in and starting LC7.</li><ul>"));
	WIZARD_CONNECT_BUTTON(m_scheduleRadioButton->radioButton());

	m_scheduleDateTimeEdit = new QDateTimeEdit(QDateTime::currentDateTime());
	m_scheduleDateTimeEdit->setCalendarPopup(true);
	connect(m_scheduleDateTimeEdit, &QDateTimeEdit::dateTimeChanged, this, &CPageScheduling::slot_scheduleDateTimeEdit_dateTimeChanged);

	m_buttongroup.addButton(m_immediateRadioButton->radioButton());
	m_buttongroup.addButton(m_scheduleRadioButton->radioButton());

	m_immediateRadioButton->radioButton()->setChecked(true);

	registerField("schedule_now", m_immediateRadioButton->radioButton());
	registerField("schedule_later", m_scheduleRadioButton->radioButton());
	registerField("schedule_datetime", m_scheduleDateTimeEdit);

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(topLabel);
	layout->addWidget(m_immediateRadioButton);
	layout->addWidget(m_scheduleRadioButton);
	layout->addWidget(m_scheduleDateTimeEdit);
	setLayout(layout);

	UpdateUI();
}

int CPageScheduling::nextId() const
{TR;
	return CLC7Wizard::Page_Summary;
}

bool CPageScheduling::isComplete() const
{TR;
	return m_immediateRadioButton->radioButton()->isChecked() || 
		(m_scheduleRadioButton->radioButton()->isChecked() && m_scheduleDateTimeEdit->dateTime() > QDateTime::currentDateTime());
}


void CPageScheduling::UpdateUI()
{
	TR;

	m_scheduleDateTimeEdit->setEnabled(m_scheduleRadioButton->radioButton()->isChecked());
}

void CPageScheduling::slot_scheduleDateTimeEdit_dateTimeChanged(const QDateTime &dateTime)
{
	TR;
	emit completeChanged();
}


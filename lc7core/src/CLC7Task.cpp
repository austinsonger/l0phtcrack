#include<stdafx.h>


CLC7Task::CLC7Task()
{TR;
	m_id="";
	m_name = "";
	m_save_task_output = true;
	m_start_time=QDateTime::currentDateTime();
	m_expiration_enabled=false;
	m_expiration_date=QDate::currentDate();

	m_externally_modified = false;

	m_recurrence=ILC7Task::ONE_TIME;
	m_daily_recurrence=1;
	m_weekly_recurrence=1;
	m_enabled_days_of_week = 0;
	m_monthly_mode=ILC7Task::SPECIFIC_DAY;
	m_specific_day_of_month=1;
	m_abstract_timing=ILC7Task::FIRST;
	m_abstract_day_of_week=ILC7Task::FRIDAY;
	m_enabled_months=0x00000FFF;
}

CLC7Task::CLC7Task(const CLC7Task &copy)
{TR;
	m_sessiondata = copy.m_sessiondata;
	m_task_descs = copy.m_task_descs;
	m_id = copy.m_id;
	m_name = copy.m_name;
	m_save_task_output = copy.m_save_task_output;
	m_start_time = copy.m_start_time;
	m_expiration_enabled = copy.m_expiration_enabled;
	m_expiration_date = copy.m_expiration_date;

	m_externally_modified = copy.m_externally_modified;

	m_recurrence = copy.m_recurrence;
	m_daily_recurrence = copy.m_daily_recurrence;
	m_weekly_recurrence = copy.m_weekly_recurrence;
	m_enabled_days_of_week = copy.m_enabled_days_of_week;
	m_monthly_mode = copy.m_monthly_mode;
	m_specific_day_of_month = copy.m_specific_day_of_month;
	m_abstract_timing = copy.m_abstract_timing;
	m_abstract_day_of_week = copy.m_abstract_day_of_week;
	m_enabled_months = copy.m_enabled_months;
}


CLC7Task::~CLC7Task()
{TR;
}


ILC7Interface *CLC7Task::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Task")
	{
		return this;
	}
	return NULL;
}


void CLC7Task::SetId(QString id)
{TR;
	m_id = id;
}

QString CLC7Task::GetId() const
{TR;
	return m_id;
}

void CLC7Task::SetExternallyModified(bool externally_modified)
{
	TR;
	m_externally_modified = externally_modified;
}

bool CLC7Task::GetExternallyModified(void) const
{
	TR;
	return m_externally_modified;
}

bool CLC7Task::SetSessionFile(QString sessionfile, QString &error)
{TR;
	QFile sf(sessionfile);
	if (!sf.open(QIODevice::ReadOnly))
	{
		error = "Can't open temporary session file";
		return false;
	}
	m_sessiondata = sf.readAll();
	
	return true;
}

void CLC7Task::AppendTaskDescription(QString desc)
{TR;
	m_task_descs.append(desc);
}

QStringList CLC7Task::GetTaskDescriptions() const
{TR;
	return m_task_descs;
}

QByteArray CLC7Task::GetSessionData() const
{TR;
	return m_sessiondata;
}

void CLC7Task::SetName(QString name)
{TR;
	m_name = name;
}

QString CLC7Task::GetName() const
{TR;
	return m_name;
}

void CLC7Task::SetSaveTaskOutput(bool save)
{TR;
	m_save_task_output = save;
}

bool CLC7Task::GetSaveTaskOutput(void) const
{TR;
	return m_save_task_output;
}

void CLC7Task::SetRecurrence(ILC7Task::RECURRENCE rec)
{TR;
	m_recurrence = rec;
}

ILC7Task::RECURRENCE CLC7Task::GetRecurrence() const
{TR;
	return m_recurrence;
}

void CLC7Task::SetStartTime(QDateTime starttime)
{TR;
	m_start_time = starttime;
}

QDateTime CLC7Task::GetStartTime() const
{TR;
	return m_start_time;
}

void CLC7Task::SetExpirationEnabled(bool expiration_enabled)
{TR;
	m_expiration_enabled = expiration_enabled;
}

bool CLC7Task::GetExpirationEnabled() const
{TR;
	return m_expiration_enabled;
}

void CLC7Task::SetExpirationDate(QDate expirationdate)
{TR;
	m_expiration_date = expirationdate;
}

QDate CLC7Task::GetExpirationDate() const
{TR;
	return m_expiration_date;
}

void CLC7Task::SetDailyRecurrence(int num_days)
{TR;
	m_daily_recurrence = num_days;
}

int CLC7Task::GetDailyRecurrence() const
{TR;
	return m_daily_recurrence;
}

void CLC7Task::SetWeeklyRecurrence(int num_weeks)
{TR;
	m_weekly_recurrence = num_weeks;
}

int CLC7Task::GetWeeklyRecurrence() const
{TR;
	return m_weekly_recurrence;
}

void CLC7Task::SetEnabledWeekDaysBitMask(quint32 weekdays)
{TR;
	m_enabled_days_of_week = weekdays;
}

quint32 CLC7Task::GetEnabledWeekDaysBitMask() const
{TR;
	return m_enabled_days_of_week;
}

void CLC7Task::SetMonthlyMode(ILC7Task::MONTHLY_MODE mm)
{TR;
	m_monthly_mode = mm;
}

ILC7Task::MONTHLY_MODE CLC7Task::GetMonthlyMode() const
{TR;
	return m_monthly_mode;
}

void CLC7Task::SetSpecificDayOfMonth(int day)
{TR;
	m_specific_day_of_month = day;
}

int CLC7Task::GetSpecificDayOfMonth() const
{TR;
	return m_specific_day_of_month;
}

void CLC7Task::SetAbstractTiming(ILC7Task::TIMING at)
{TR;
	m_abstract_timing = at;
}

ILC7Task::TIMING CLC7Task::GetAbstractTiming() const
{TR;
	return m_abstract_timing;
}

void CLC7Task::SetAbstractDayOfWeek(ILC7Task::DAY_OF_WEEK ad)
{TR;
	m_abstract_day_of_week = ad;
}

ILC7Task::DAY_OF_WEEK CLC7Task::GetAbstractDayOfWeek() const
{TR;
	return m_abstract_day_of_week;
}

void CLC7Task::SetEnabledMonthsBitMask(quint32 months)
{TR;
	m_enabled_months = months;
}

quint32 CLC7Task::GetEnabledMonthsBitMask() const
{TR;
	return m_enabled_months;
}


bool CLC7Task::Save(QDataStream &out) const
{TR;
	quint32 version = 1;

	out << version;

	out << m_id;
	out << m_task_descs;
	out << m_sessiondata;

	out << m_name;
	out << m_save_task_output;
	out << m_start_time;
	out << m_expiration_enabled;
	out << m_expiration_date;
	out << m_recurrence;
	out << m_daily_recurrence;
	out << m_weekly_recurrence;
	out << m_enabled_days_of_week;
	out << m_monthly_mode;
	out << m_specific_day_of_month;
	out << m_abstract_timing;
	out << m_abstract_day_of_week;
	out << m_enabled_months;
	
	return out.status()==QDataStream::Ok;
}

bool CLC7Task::Load(QDataStream &in)
{TR;
	quint32 version;
	in >> version;
	if (version == 1)
	{
		in >> m_id;
		in >> m_task_descs;
		in >> m_sessiondata;

		in >> m_name;
		in >> m_save_task_output;
		in >> m_start_time;
		in >> m_expiration_enabled;
		in >> m_expiration_date;
		in >> (int &)m_recurrence;
		in >> m_daily_recurrence;
		in >> m_weekly_recurrence;
		in >> m_enabled_days_of_week;
		in >> (int &)m_monthly_mode;
		in >> m_specific_day_of_month;
		in >> (int &)m_abstract_timing;
		in >> (int &)m_abstract_day_of_week;
		in >> m_enabled_months;
	}
	else
	{
		return false;
	}

	return true;
}

bool CLC7Task::Validate(QString &error) const
{TR;
	if (m_id.isEmpty())
	{
		error = "id is not valid";
		return false;
	}
	if (m_name.isEmpty())
	{
		error = "name is not valid";
		return false;
	}
	if (m_start_time.isNull() || !m_start_time.isValid())
	{
		error = "name is not valid";
		return false;
	}
	if (m_expiration_enabled)
	{
		if (m_expiration_date.isNull() || !m_expiration_date.isValid())
		{
			error = "expiration date is not valid";
			return false;
		}
	}
	if (m_recurrence == ILC7Task::ONE_TIME)
	{
	}
	else if (m_recurrence == ILC7Task::DAILY)
	{
		if (m_daily_recurrence < 1 || m_daily_recurrence>365)
		{
			error = "daily recurrence value out of range 1-365";
			return false;
		}
	}
	else if (m_recurrence == ILC7Task::WEEKLY)
	{
		if (m_weekly_recurrence < 1 || m_weekly_recurrence>52)
		{
			error = "weekly recurrence value out of range 1-52";
			return false;
		}

		if (m_enabled_days_of_week > 0x7F || m_enabled_days_of_week == 0)
		{
			error = "invalid selection of enabled week days";
			return false;
		}
	}
	else if (m_recurrence == ILC7Task::MONTHLY)
	{
		if (m_monthly_mode == ILC7Task::SPECIFIC_DAY)
		{
			if (m_specific_day_of_month < 1 || m_specific_day_of_month>31)
			{
				error = "day of month value out of range 1-31";
				return false;
			}

		}
		else if (m_monthly_mode == ILC7Task::ABSTRACT_DAY)
		{
			if (m_abstract_timing<ILC7Task::FIRST || m_abstract_timing>ILC7Task::LAST)
			{
				error = "invalid timing mode";
				return false;
			}
			if (m_abstract_day_of_week<ILC7Task::SUNDAY || m_abstract_day_of_week>ILC7Task::SATURDAY)
			{
				error = "invalid day of week";
				return false;
			}
		}
		else
		{
			error = "invalid monthly mode";
			return false;
		}
		
		if (m_enabled_months > 0xFFF || m_enabled_months==0)
		{
			error = "invalid selection of enabled months";
			return false;
		}
	}
	else
	{
		error = "invalid recurrence mode";
		return false;
	}

	return true;
}

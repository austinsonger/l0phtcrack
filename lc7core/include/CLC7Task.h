#ifndef __INC_CLC7TASK_H
#define __INC_CLC7TASK_H

class CLC7Task :public ILC7EditableTask
{
public:
	CLC7Task();
	CLC7Task(const CLC7Task &copy);
	virtual ~CLC7Task();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	virtual bool Validate(QString &error) const;

	virtual void SetId(QString id);
	virtual QString GetId() const;
	
	virtual void SetExternallyModified(bool externally_modified);
	virtual bool GetExternallyModified(void) const;

	virtual QStringList GetTaskDescriptions() const;
	
	virtual void SetName(QString name);
	virtual QString GetName() const;

	virtual void SetSaveTaskOutput(bool save);
	virtual bool GetSaveTaskOutput(void) const;

	virtual void SetStartTime(QDateTime starttime);
	virtual QDateTime GetStartTime() const;

	virtual void SetExpirationEnabled(bool expiration_enabled);
	virtual bool GetExpirationEnabled() const;
	virtual void SetExpirationDate(QDate expirationdate);
	virtual QDate GetExpirationDate() const;

	virtual void SetRecurrence(RECURRENCE rec);
	virtual RECURRENCE GetRecurrence() const;

	// Daily
	virtual void SetDailyRecurrence(int num_days);
	virtual int GetDailyRecurrence() const;

	// Weekly
	virtual void SetWeeklyRecurrence(int num_weeks);
	virtual int GetWeeklyRecurrence() const;
	virtual void SetEnabledWeekDaysBitMask(quint32 weekdays);
	virtual quint32 GetEnabledWeekDaysBitMask() const;

	// Monthly
	virtual void SetMonthlyMode(MONTHLY_MODE mm);
	virtual MONTHLY_MODE GetMonthlyMode() const;

	virtual void SetSpecificDayOfMonth(int day);
	virtual int GetSpecificDayOfMonth() const;

	virtual void SetAbstractTiming(TIMING at);
	virtual TIMING GetAbstractTiming() const;
	virtual void SetAbstractDayOfWeek(DAY_OF_WEEK ad);
	virtual DAY_OF_WEEK GetAbstractDayOfWeek() const;

	virtual void SetEnabledMonthsBitMask(quint32 months);
	virtual quint32 GetEnabledMonthsBitMask() const;

	// Save/Load
	virtual bool Save(QDataStream & out) const;
	virtual bool Load(QDataStream & in);

public:
	virtual bool SetSessionFile(QString sessionfile, QString &error);
	virtual void AppendTaskDescription(QString desc);
	virtual QByteArray GetSessionData() const;


private:

	QByteArray m_sessiondata;

	QStringList m_task_descs;
	QString m_id;
	QString m_name;

	bool m_externally_modified;

	QDateTime m_start_time;
	bool m_expiration_enabled;
	QDate m_expiration_date;
	bool m_save_task_output;
	ILC7Task::RECURRENCE m_recurrence;
	int m_daily_recurrence;
	int m_weekly_recurrence;
	quint32 m_enabled_days_of_week;
	ILC7Task::MONTHLY_MODE m_monthly_mode;
	int m_specific_day_of_month;
	ILC7Task::TIMING m_abstract_timing;
	ILC7Task::DAY_OF_WEEK m_abstract_day_of_week;
	quint32 m_enabled_months;
};

#endif
#ifndef __INC_ILC7TASK_H
#define __INC_ILC7TASK_H

#include"core/ILC7Interface.h"

class ILC7Task: public ILC7Interface
{
public:
	enum RECURRENCE {
		ONE_TIME=0,
		DAILY,
		WEEKLY,
		MONTHLY
	};

	enum MONTHLY_MODE {
		SPECIFIC_DAY=0,
		ABSTRACT_DAY
	};

	enum TIMING {
		FIRST=0,
		SECOND,
		THIRD,
		FOURTH,
		LAST
	};
	enum DAY_OF_WEEK {
		SUNDAY=0,
		MONDAY,
		TUESDAY,
		WEDNESDAY,
		THURSDAY,
		FRIDAY,
		SATURDAY
	};

protected:
	virtual ~ILC7Task() {}

public:

	virtual QString GetId() const = 0;
	virtual QStringList GetTaskDescriptions() const = 0;

	virtual QString GetName() const = 0;
	virtual bool GetExternallyModified(void) const = 0;

	virtual bool GetSaveTaskOutput(void) const = 0;
	
	virtual QDateTime GetStartTime() const = 0;
	virtual bool GetExpirationEnabled() const = 0;
	virtual QDate GetExpirationDate() const = 0;

	virtual RECURRENCE GetRecurrence() const = 0;
	virtual int GetDailyRecurrence() const = 0;
	virtual int GetWeeklyRecurrence() const = 0;
	virtual quint32 GetEnabledWeekDaysBitMask() const = 0;
	virtual MONTHLY_MODE GetMonthlyMode() const = 0;

	virtual int GetSpecificDayOfMonth() const = 0;

	virtual TIMING GetAbstractTiming() const = 0;
	virtual DAY_OF_WEEK GetAbstractDayOfWeek() const = 0;

	virtual quint32 GetEnabledMonthsBitMask() const = 0;
};


class ILC7EditableTask :public ILC7Task
{
public:

	virtual bool Validate(QString &error) const = 0;

	virtual void SetName(QString name) = 0;

	virtual void SetSaveTaskOutput(bool save)=0;

	virtual void SetStartTime(QDateTime starttime) = 0;

	virtual void SetExpirationEnabled(bool expiration_enabled) = 0;
	virtual void SetExpirationDate(QDate expirationdate) = 0;

	virtual void SetRecurrence(RECURRENCE rec) = 0;
	virtual void SetDailyRecurrence(int num_days) = 0;
	virtual void SetWeeklyRecurrence(int num_weeks) = 0;
	virtual void SetEnabledWeekDaysBitMask(quint32 weekdays) = 0;

	virtual void SetMonthlyMode(MONTHLY_MODE mm) = 0;
	virtual void SetSpecificDayOfMonth(int day) = 0;
	virtual void SetAbstractTiming(TIMING at) = 0;
	virtual void SetAbstractDayOfWeek(DAY_OF_WEEK ad) = 0;
	virtual void SetEnabledMonthsBitMask(quint32 months) = 0;

	// Save/Load
	virtual bool Save(QDataStream & out) const = 0;
	virtual bool Load(QDataStream & in) = 0;
};


#endif

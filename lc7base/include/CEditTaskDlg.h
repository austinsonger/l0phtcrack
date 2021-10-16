#ifndef __INC_CEDITTASKDLG_H
#define __INC_CEDITTASKDLG_H

#include "ui_edittask.h"

class CEditTaskDlg :public QDialog
{
	Q_OBJECT
private:
	Ui::EditTaskDialog ui;
	
	ILC7EditableTask *m_task;
	bool m_is_new_task;
	QByteArray m_saved_task;

private slots:
	
	void slot_buttonBoxClicked(QAbstractButton *button);
	void slot_rejectedRollback(void);

	void slot_textChanged_taskName(const QString &);
	void slot_clicked_saveTaskOutputCheckBox(bool checked);
	void slot_dateTimeChanged_startDateTime(const QDateTime &dateTime);
	void slot_clicked_expirationCheckBox(bool checked);
	void slot_dateChanged_expirationDate(const QDate &date);
	void slot_clicked_OneTimeRadio(bool checked);
	void slot_clicked_DailyRadio(bool checked);
	void slot_clicked_WeeklyRadio(bool checked);
	void slot_clicked_MonthlyRadio(bool checked);
	void slot_textChanged_everyNDaysEdit(const QString &);
	void slot_textChanged_everyNWeeksEdit(const QString &);

	void slot_clicked_sundayCheckBox(bool checked);
	void slot_clicked_mondayCheckBox(bool checked);
	void slot_clicked_tuesdayCheckBox(bool checked);
	void slot_clicked_wednesdayCheckBox(bool checked);
	void slot_clicked_thursdayCheckBox(bool checked);
	void slot_clicked_fridayCheckBox(bool checked);
	void slot_clicked_saturdayCheckBox(bool checked);

	void slot_clicked_dayRadioButton(bool checked);
	void slot_clicked_theRadioButton(bool checked);

	void slot_valueChanged_dayNMonthSpinBox(int);
	void slot_currentIndexChanged_whichTimeComboBox(int index);
	void slot_currentIndexChanged_dayOfTheWeekComboBox(int index);

	void slot_clicked_januaryCheckBox(bool checked);
	void slot_clicked_februaryCheckBox(bool checked);
	void slot_clicked_marchCheckBox(bool checked);
	void slot_clicked_aprilCheckBox(bool checked);
	void slot_clicked_mayCheckBox(bool checked);
	void slot_clicked_juneCheckBox(bool checked);
	void slot_clicked_julyCheckBox(bool checked);
	void slot_clicked_augustCheckBox(bool checked);
	void slot_clicked_septemberCheckBox(bool checked);
	void slot_clicked_octoberCheckBox(bool checked);
	void slot_clicked_novemberCheckBox(bool checked);
	void slot_clicked_decemberCheckBox(bool checked);

protected:
	virtual void RefreshContent(void);
	virtual void UpdateUI(void);
	virtual bool VerifyDialog(void);
	virtual bool CommitNew(QString &error, bool &cancelled);
	virtual bool CommitEdit(QString &error, bool &cancelled);

public:
	CEditTaskDlg(ILC7EditableTask *task, bool isNewTask);
	virtual ~CEditTaskDlg();


};



#endif
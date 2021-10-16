#ifndef __INC_CPAGESCHEDULING_H
#define __INC_CPAGESCHEDULING_H

class CPageScheduling : public CLC7WizardPage
{
	Q_OBJECT

public:
	CPageScheduling(QWidget *parent = 0);

	int nextId() const;
	bool isComplete() const;
	void UpdateUI(void);

private slots:

	void slot_scheduleDateTimeEdit_dateTimeChanged(const QDateTime &dateTime);

private:
	
	CLabelRadioButton *m_immediateRadioButton;
	CLabelRadioButton *m_scheduleRadioButton;
	QDateTimeEdit *m_scheduleDateTimeEdit;
	QButtonGroup m_buttongroup;

};

#endif
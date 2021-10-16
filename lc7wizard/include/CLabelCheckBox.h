#ifndef __INC_CLABELCHECKBOX_H
#define __INC_CLABELCHECKBOX_H

class CLabelCheckBox :public QWidget
{
	Q_OBJECT
public:

	CLabelCheckBox(QString text=QString(), QWidget *parent=0);
	virtual ~CLabelCheckBox();

	QCheckBox *checkBox();
	QLabel *label();

private slots:
	void slot_labelClicked(void);

private:
	QCheckBox *m_checkbox;
	CLabelClickable *m_label;
};

#endif


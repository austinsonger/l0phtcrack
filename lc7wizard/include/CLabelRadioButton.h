#ifndef __INC_CLABELRADIOBUTTON_H
#define __INC_CLABELRADIOBUTTON_H

class CLabelRadioButton :public QWidget
{
	Q_OBJECT
public:

	CLabelRadioButton(QString text = QString(), QWidget *parent = 0);
	virtual ~CLabelRadioButton();

	QRadioButton *radioButton();
	QLabel *label();

private slots:
	void slot_labelClicked(void);

private:

	QRadioButton *m_radiobutton;
	CLabelClickable *m_label;

};

#endif


#ifndef __INC_CLABELCLICKABLE_H
#define __INC_CLABELCLICKABLE_H

class CLabelClickable : public QLabel
{
	Q_OBJECT
public:
	CLabelClickable(QWidget * parent);
	CLabelClickable(const QString & text, QWidget * parent);

signals:
	void clicked();

protected:
	void mousePressEvent(QMouseEvent * event);
};

#endif
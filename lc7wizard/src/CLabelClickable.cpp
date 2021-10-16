#include<stdafx.h>

CLabelClickable::CLabelClickable(QWidget * parent) : QLabel(parent)
{
}

CLabelClickable::CLabelClickable(const QString & text, QWidget * parent) : QLabel(text, parent)
{
}

void CLabelClickable::mousePressEvent(QMouseEvent * event)
{
	emit clicked();
}
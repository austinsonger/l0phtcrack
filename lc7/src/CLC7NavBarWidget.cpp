#include"stdafx.h"


CLC7NavBarWidget::CLC7NavBarWidget(QWidget *parent):QWidget(parent)
{
	TR;
	setLayout(new QVBoxLayout());
	layout()->setContentsMargins(0,0,0,0);
	layout()->setSpacing(16);
	setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
//	layout()->addItem(new QSpacerItem(1,1,QSizePolicy::Fixed,QSizePolicy::Expanding));
}

void CLC7NavBarWidget::paintEvent(QPaintEvent *e)
{
    QStyleOption opt;
    opt.init(this);
    QStylePainter p(this);
    p.drawPrimitive(QStyle::PE_Widget, opt);
}


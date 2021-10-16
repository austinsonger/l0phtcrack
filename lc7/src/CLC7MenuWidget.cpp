#include<stdafx.h>

CLC7MenuWidget::CLC7MenuWidget(QWidget *parent):QWidget(parent)
{
	ui.setupUi(this);

	connect(ui.newSessionButton, SIGNAL(clicked()), this, SLOT(slot_newSessionButton()));
	connect(ui.openSessionButton, SIGNAL(clicked()), this, SLOT(slot_openSessionButton()));
	connect(ui.saveSessionButton, SIGNAL(clicked()), this, SLOT(slot_saveSessionButton()));
	connect(ui.saveSessionAsButton, SIGNAL(clicked()), this, SLOT(slot_saveSessionAsButton()));
	connect(ui.quitButton, SIGNAL(clicked()), this, SLOT(slot_quitButton()));

	setWindowFlags(Qt::Popup);
}

CLC7MenuWidget::~CLC7MenuWidget()
{TR;
}

void CLC7MenuWidget::slot_newSessionButton()
{TR;
}

void CLC7MenuWidget::slot_openSessionButton()
{TR;
}

void CLC7MenuWidget::slot_saveSessionButton()
{TR;
}

void CLC7MenuWidget::slot_saveSessionAsButton()
{TR;
}

void CLC7MenuWidget::slot_quitButton()
{TR;
}

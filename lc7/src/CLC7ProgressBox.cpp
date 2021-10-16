#include<stdafx.h>

CLC7ProgressBox::CLC7ProgressBox(QString title, QString status, quint32 progresscur, quint32 progressmax, bool can_cancel)
{TR;
	m_is_cancelled=false;
	m_can_cancel = can_cancel;

	ui.setupUi(this);
	
	CLC7App *theapp=CLC7App::getInstance();
	LC7Main *mainwin=theapp->GetMainWindow();

	setWindowTitle(title);
	setWindowFlags(windowFlags() & ~(Qt::WindowCloseButtonHint | Qt::WindowContextHelpButtonHint));
	ui.label->setText(status);
	ui.progressBar->setRange(0,progressmax);
	ui.progressBar->setValue(progresscur);
	
	if (m_can_cancel)
	{
		connect(ui.cancelButton, SIGNAL(clicked()), this, SLOT(slot_cancelButton()));
	}
	else
	{
		ui.cancelButton->setVisible(false);
	}

	connect(this,SIGNAL(sig_updateStatusText(QString)),this,SLOT(slot_updateStatusText(QString)));
	connect(this,SIGNAL(sig_updateProgressBar(quint32)),this,SLOT(slot_updateProgressBar(quint32)));
	
	show();
}

CLC7ProgressBox::~CLC7ProgressBox()
{TR;
}

	
ILC7Interface *CLC7ProgressBox::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7ProgressBox")
	{
		return this;
	}
	return NULL;
}

void CLC7ProgressBox::UpdateStatusText(QString text)
{TR;
	emit sig_updateStatusText(text);
}

void CLC7ProgressBox::UpdateProgressBar(quint32 cur)
{TR;
	emit sig_updateProgressBar(cur);	
}

void CLC7ProgressBox::slot_cancelButton()
{TR;
	m_is_cancelled=true;
	ui.cancelButton->setText("Cancelling...");
	ui.cancelButton->setEnabled(false);
}


void CLC7ProgressBox::slot_updateStatusText(QString text)
{TR;
	ui.label->setText(text);
}

void CLC7ProgressBox::slot_updateProgressBar(quint32 cur)
{TR;
	ui.progressBar->setValue(cur);
}

bool CLC7ProgressBox::IsCancelled()
{TR;
	return m_is_cancelled;
}

void CLC7ProgressBox::Release()
{TR;
	delete this;
}
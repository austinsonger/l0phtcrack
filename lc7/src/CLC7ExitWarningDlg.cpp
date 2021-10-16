#include<stdafx.h>

CLC7ExitWarningDlg::CLC7ExitWarningDlg(QObject *parent, int seconds)
{TR;
	ui.setupUi(this);

	m_seconds = seconds;

	m_timer.setInterval(1000);
	m_timer.start();

	connect(&m_timer, &QTimer::timeout, this, &CLC7ExitWarningDlg::slot_tick);
	connect(ui.cancelButton, &QAbstractButton::clicked, this, &CLC7ExitWarningDlg::reject);
	connect(ui.closeNowButton, &QAbstractButton::clicked, this, &CLC7ExitWarningDlg::accept);

	UpdateUI();
}

CLC7ExitWarningDlg::~CLC7ExitWarningDlg()
{TR;
}

void CLC7ExitWarningDlg::slot_tick(void) 
{TR;
	m_seconds--;
	UpdateUI();

	if (m_seconds <= 0)
	{
		accept();
	}
}

void CLC7ExitWarningDlg::UpdateUI()
{TR;
	ui.exitWarningLabel->setText(QString("LC7 is exiting in %1 seconds...").arg(m_seconds));
}
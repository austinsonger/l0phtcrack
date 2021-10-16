#include"stdafx.h"

CPageWindowsOrUnix::CPageWindowsOrUnix(QWidget *parent)
	: CLC7WizardPage(parent), m_buttongroup(this)
{
	TR;
	setTitle(tr("Choose Target System Type"));
	
	QLabel *topLabel = new QLabel(tr(
		"Please choose the type of system from which you would like to retrieve the password hashes:\n\n"
		));
	topLabel->setWordWrap(true);

	m_windowsRadioButton = new CLabelRadioButton(tr("<b>Windows:</b><br>Desktops: Windows XP through 10<br>Servers: Windows Server 2003 or greater"));
	WIZARD_CONNECT_BUTTON(m_windowsRadioButton->radioButton());
	m_unixRadioButton = new CLabelRadioButton(tr("<b>Unix-like:</b><br>Linux, FreeBSD, OpenBSD, Solaris, or AIX"));
	WIZARD_CONNECT_BUTTON(m_unixRadioButton->radioButton());
	m_windowsRadioButton->radioButton()->setChecked(true);
	
	m_buttongroup.addButton(m_windowsRadioButton->radioButton());
	m_buttongroup.addButton(m_unixRadioButton->radioButton());

	registerField("import_windows", m_windowsRadioButton->radioButton());
	registerField("import_unix", m_unixRadioButton->radioButton());

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(topLabel);
	layout->addWidget(m_windowsRadioButton);
	layout->addWidget(m_unixRadioButton);
	setLayout(layout);

	UpdateUI();
}

int CPageWindowsOrUnix::nextId() const
{TR;
	if (m_windowsRadioButton->radioButton()->isChecked())
	{ 
		return CLC7Wizard::Page_Windows_Import;
	}
	return CLC7Wizard::Page_Unix_Import;
}

bool CPageWindowsOrUnix::isComplete() const
{TR;
	return m_windowsRadioButton->radioButton()->isChecked() || m_unixRadioButton->radioButton()->isChecked();
}


void CPageWindowsOrUnix::UpdateUI()
{
	TR;
}
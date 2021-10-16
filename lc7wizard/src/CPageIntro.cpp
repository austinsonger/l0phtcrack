#include"stdafx.h"

CPageIntro::CPageIntro(QWidget *parent)
	: CLC7WizardPage(parent)
{
	TR;

	setTitle(tr("Introduction"));
	//setSubTitle("<br>Intro</br>");
	//setPixmap(QWizard::WatermarkPixmap, QPixmap(":/images/watermark.png"));

	QLabel *label1= new QLabel(
		tr("Welcome to the L0phtCrack 7 Wizard. This wizard will prompt you with step-by-step instructions to get you auditing in minutes.\n\n"
		   "First, the wizard will help you determine from where to retrieve your encrypted passwords.\n\n"
		   "Second, you will be prompted with a few options regarding which methods to use to audit the passwords.\n\n"
		   "Third, you will be prompted with how you wish to report the results.\n\n"
		   "Then L0phtCrack 7 will proceed with auditing the password and report status to you along the way, notifying you when auditing is complete.\n\n"
		   "Press 'next' to continue with the wizard."));
	label1->setWordWrap(true);

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(label1);
	setLayout(layout);

	UpdateUI();
}

int CPageIntro::nextId() const
{TR;
	return CLC7Wizard::Page_Windows_Or_Unix;
}

bool CPageIntro::isComplete() const
{TR;
	return true;
}

void CPageIntro::UpdateUI()
{
	TR;
}

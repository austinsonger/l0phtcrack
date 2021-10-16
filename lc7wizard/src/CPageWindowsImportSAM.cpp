#include"stdafx.h"

CPageWindowsImportSAM::CPageWindowsImportSAM(QWidget *parent)
	: CLC7WizardPage(parent)
{
	TR;
	setTitle(tr("Windows Import From SAM/SYSTEM Backup"));
	//setPixmap(QWizard::WatermarkPixmap, QPixmap(":/images/watermark.png"));

	QLabel *topLabel = new QLabel(tr("Enter the pathname to the backed up SAM and SYSTEM hives.<br><br>To copy registry hives from a running Windows system, run this from an administrative command prompt:<br>"
		"reg save HKLM\\sam sam<br>"
		"reg save HKLM\\system system<br><br>"
		"See the L0phtCrack 7 documentation for more details."));
	topLabel->setWordWrap(true);

	QRadioButton *registerRadioButton = new QRadioButton(tr("&Register your copy"));
	QRadioButton *evaluateRadioButton = new QRadioButton(tr("&Evaluate the product for 30 "
		"days"));
	registerRadioButton->setChecked(true);

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(topLabel);
	layout->addWidget(registerRadioButton);
	layout->addWidget(evaluateRadioButton);
	setLayout(layout);

	UpdateUI();
}

int CPageWindowsImportSAM::nextId() const
{TR;
	return CLC7Wizard::Page_Windows_Or_Unix;
}

bool CPageWindowsImportSAM::isComplete() const
{TR;
	return true;
}


void CPageWindowsImportSAM::UpdateUI()
{
	TR;
}
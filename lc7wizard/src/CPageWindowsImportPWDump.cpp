#include"stdafx.h"
#include"../lc7importwin/include/uuids.h"

CPageWindowsImportPWDump::CPageWindowsImportPWDump(QWidget *parent)
	: CLC7WizardPage(parent)
{
	TR;

	setTitle(tr("Windows Import From PWDump File"));
	//setPixmap(QWizard::WatermarkPixmap, QPixmap(":/images/watermark.png"));

	QLabel *topLabel = new QLabel(tr("Enter the pathname of the PWDump-format file that you would like to import. Compatible tools include the ones listed here:<br><a href=\"https://en.wikipedia.org/wiki/Pwdump\">https://en.wikipedia.org/wiki/Pwdump</a> "));
	topLabel->setOpenExternalLinks(true);
	topLabel->setWordWrap(true);

	QMap<QString, QVariant> config;
	config["simple"] = true;

	CreateConfigWidget(UUID_IMPORTPWDUMPGUI, config);

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(topLabel);
	layout->addWidget(m_configWidget);
	setLayout(layout);

	UpdateUI();
}

int CPageWindowsImportPWDump::nextId() const
{TR;
	return CLC7Wizard::Page_Audit_Type;
}

void CPageWindowsImportPWDump::UpdateUI()
{
	TR;
}

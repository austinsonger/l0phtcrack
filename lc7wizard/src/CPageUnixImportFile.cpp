#include"stdafx.h"
#include"../lc7importunix/include/uuids.h"

CPageUnixImportFile::CPageUnixImportFile(QWidget *parent)
	: CLC7WizardPage(parent)
{
	TR;
	setTitle(tr("Unix Shadow File Import"));
	//setPixmap(QWizard::WatermarkPixmap, QPixmap(":/images/watermark.png"));

	QLabel *topLabel = new QLabel(tr("Enter the pathname of the shadowed password files that you would like to import.<br>Compatible operating systems include Linux, OpenBSD, FreeBSD, Solaris, and AIX"));
	topLabel->setWordWrap(true);

	QMap<QString, QVariant> config;
	config["simple"] = true;

	CreateConfigWidget(UUID_IMPORTSHADOWGUI,config);

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(topLabel);
	layout->addWidget(m_configWidget);
	setLayout(layout);

	UpdateUI();
}

CPageUnixImportFile::~CPageUnixImportFile()
{
}


int CPageUnixImportFile::nextId() const
{TR;
	return CLC7Wizard::Page_Audit_Type;
}

void CPageUnixImportFile::UpdateUI()
{
	TR;
}

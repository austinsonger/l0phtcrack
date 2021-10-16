#include"stdafx.h"
#include"../lc7importunix/include/uuids.h"

CPageUnixImportSSH::CPageUnixImportSSH(QWidget *parent)
	: CLC7WizardPage(parent)
{
	TR;
	setTitle(tr("Unix SSH Import"));
	//setPixmap(QWizard::WatermarkPixmap, QPixmap(":/images/watermark.png"));

	QLabel *topLabel = new QLabel(tr("Enter the machine's hostname or address, and authentication credentials:"));
	topLabel->setWordWrap(true);
	
	QMap<QString, QVariant> config;
	config["simple"] = true;

	CreateConfigWidget(UUID_IMPORTUNIXSSHGUI,config);

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(topLabel);
	layout->addWidget(m_configWidget);
	setLayout(layout);

	UpdateUI();
}

CPageUnixImportSSH::~CPageUnixImportSSH()
{
}

int CPageUnixImportSSH::nextId() const
{TR;
	return CLC7Wizard::Page_Audit_Type;
}

void CPageUnixImportSSH::UpdateUI()
{
	TR;
}

#include"stdafx.h"
#include"../lc7importwin/include/uuids.h"

CPageWindowsImportRemoteSMB::CPageWindowsImportRemoteSMB(QWidget *parent)
	: CLC7WizardPage(parent)
{
	TR;

	setTitle(tr("Windows Import From Remote Machine (SMB)"));
	//setPixmap(QWizard::WatermarkPixmap, QPixmap(":/images/watermark.png"));

	QLabel *topLabel = new QLabel(tr(
		"Choose the machine to which you want to connect. You can use a Windows network path, hostname or IP address. Also, provide the credentials to use while performing the hash extraction. This the target host is on a domain, you will need domain administrative credentials for this to work."
		));
	topLabel->setWordWrap(true);

	QMap<QString, QVariant> config;
	config["force_mode"] = "smb";
	config["simple"] = true;

	CreateConfigWidget(UUID_IMPORTWINDOWSREMOTEGUI, config);

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(topLabel);
	layout->addWidget(m_configWidget);
	setLayout(layout);

	UpdateUI();
}

int CPageWindowsImportRemoteSMB::nextId() const
{
	TR;
	return CLC7Wizard::Page_Audit_Type;
}

void CPageWindowsImportRemoteSMB::UpdateUI(void)
{
	TR;
}

#include"stdafx.h"
#include"../lc7importwin/include/uuids.h"

CPageWindowsImportRemoteRDP::CPageWindowsImportRemoteRDP(QWidget *parent)
	: CLC7WizardPage(parent)
{
	TR;

	setTitle(tr("Windows Import From Remote Machine (RDP)"));
	//setPixmap(QWizard::WatermarkPixmap, QPixmap(":/images/watermark.png"));

	QLabel *topLabel = new QLabel(tr(
		"Choose which credentials to use while performing the hash extraction:"
		));
	topLabel->setWordWrap(true);

	QMap<QString, QVariant> config;
	config["force_mode"] = "rdp";
	config["simple"] = true;
	CreateConfigWidget(UUID_IMPORTWINDOWSREMOTEGUI,config);

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(topLabel);
	layout->addWidget(m_configWidget);
	setLayout(layout);

	UpdateUI();
}

int CPageWindowsImportRemoteRDP::nextId() const
{
	TR;
	return CLC7Wizard::Page_Audit_Type;
}

void CPageWindowsImportRemoteRDP::UpdateUI(void)
{
	TR;

}

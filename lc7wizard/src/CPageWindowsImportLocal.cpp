#include"stdafx.h"
#include"../lc7importwin/include/uuids.h"

CPageWindowsImportLocal::CPageWindowsImportLocal(QWidget *parent)
	: CLC7WizardPage(parent)
{
	TR;

	setTitle(tr("Windows Import From Local Machine"));
	//setPixmap(QWizard::WatermarkPixmap, QPixmap(":/images/watermark.png"));

	QLabel *topLabel = new QLabel(tr(
		"Choose which credentials to use while performing the hash extraction:"
		));
	topLabel->setWordWrap(true);

	QMap<QString, QVariant> config;
	config["simple"] = true;

	CreateConfigWidget(UUID_IMPORTWINDOWSLOCALGUI, config);

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(topLabel);
	layout->addWidget(m_configWidget);
	setLayout(layout);

	UpdateUI();
}

int CPageWindowsImportLocal::nextId() const
{
	TR;
	return CLC7Wizard::Page_Audit_Type;
}

void CPageWindowsImportLocal::UpdateUI(void)
{
	TR;
}

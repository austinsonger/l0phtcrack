#include"stdafx.h"

CPageUnixImport::CPageUnixImport(QWidget *parent)
	: CLC7WizardPage(parent), m_buttongroup(this)
{
	TR;
	setTitle(tr("Unix Import"));
	//setPixmap(QWizard::WatermarkPixmap, QPixmap(":/images/watermark.png"));

	QLabel *topLabel = new QLabel(tr(
		"Choose a source from which to retrieve the hashes. Supported operating systems include Linux, OpenBSD, FreeBSD, Solaris, and AIX."
		));
	topLabel->setWordWrap(true);

	m_importSSHRadioButton = new CLabelRadioButton(tr("<b>A &remote machine via SSH</b><br>Requires admin privileges on the remote machine. Remote machine must either be able to log in as root (not recommended), or use <i>su</i> or <i>sudo</i> to gain root access."));
	WIZARD_CONNECT_BUTTON(m_importSSHRadioButton->radioButton())
	m_importShadowRadioButton = new CLabelRadioButton(tr("<b>An '/etc/shadow', '/etc/master.passwd', or '/etc/security/passwd' file copied from a remote system.</b>"));
	WIZARD_CONNECT_BUTTON(m_importShadowRadioButton->radioButton())

	m_buttongroup.addButton(m_importSSHRadioButton->radioButton());
	m_buttongroup.addButton(m_importShadowRadioButton->radioButton());
	//	m_buttongroup.addButton(m_importSAMSYSTEMRadioButton->radioButton());

	m_importSSHRadioButton->radioButton()->setChecked(true);

	registerField("import_unix_ssh", m_importSSHRadioButton->radioButton());
	registerField("import_unix_shadow", m_importShadowRadioButton->radioButton());

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(topLabel);
	layout->addWidget(m_importSSHRadioButton);
	layout->addWidget(m_importShadowRadioButton);
	setLayout(layout);

	UpdateUI();
}

int CPageUnixImport::nextId() const
{
	TR;
	if (m_importSSHRadioButton->radioButton()->isChecked())
	{
		return CLC7Wizard::Page_Unix_Import_SSH;
	}
	return CLC7Wizard::Page_Unix_Import_File;
}

bool CPageUnixImport::isComplete() const
{
	TR;
	if (m_importSSHRadioButton->radioButton()->isChecked() ||
		m_importShadowRadioButton->radioButton()->isChecked())// ||
		//		m_importSAMSYSTEMRadioButton->radioButton()->isChecked())
	{
		return true;
	}

	return false;
}


void CPageUnixImport::UpdateUI()
{
	TR;
}
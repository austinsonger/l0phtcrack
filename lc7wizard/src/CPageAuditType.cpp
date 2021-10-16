#include"stdafx.h"
#include"../lc7jtr/include/uuids.h"

CPageAuditType::CPageAuditType(QWidget *parent)
	: CLC7WizardPage(parent), m_buttongroup(this)
{
	TR;
	setTitle(tr("Choose Audit Type"));
	//setPixmap(QWizard::WatermarkPixmap, QPixmap(":/images/watermark.png"));

	QLabel *topLabel = new QLabel(tr(
		"Choose the type of audit you would like to perform:"
		));
	topLabel->setWordWrap(true);

	m_quickRadioButton = new CLabelRadioButton(tr("<b>Quick Password Audit</b><br>This method checks for passwords that you could find in a dictionary, with common permutations.<br>"));
	WIZARD_CONNECT_BUTTON(m_quickRadioButton->radioButton());
	m_commonRadioButton = new CLabelRadioButton(tr("<b>Common Password Audit</b><br>This method checks for passwords that you could find in a dictionary, with many permutations.<br>"
		"This is followed by a 1 hour long brute-force attack using an alphanumeric+space character set.<br>"));
	WIZARD_CONNECT_BUTTON(m_commonRadioButton->radioButton());
	m_thoroughRadioButton = new CLabelRadioButton(tr("<b>Thorough Password Audit</b><br>This method checks for passwords that you could find in a dictionary, with extensive permutations.<br>"
		"This is followed by an 6 hour long brute-force attack using a large ASCII character set.<br>"));
	WIZARD_CONNECT_BUTTON(m_thoroughRadioButton->radioButton());
	m_strongRadioButton = new CLabelRadioButton(tr("<b>Strong Password Audit</b><br>This method starts with a 24 hour long brute-force attack using the entire ISO-8859-1 character set.<br>"
		"Then it checks for passwords that you could find in a dictionary, with all available permutations.<br>"
		"<i>Use of a GPU-enabled machine is required. Audit may take several days to complete!</i>"));
	WIZARD_CONNECT_BUTTON(m_strongRadioButton->radioButton());
	
	m_buttongroup.addButton(m_quickRadioButton->radioButton());
	m_buttongroup.addButton(m_commonRadioButton->radioButton());
	m_buttongroup.addButton(m_thoroughRadioButton->radioButton());
	m_buttongroup.addButton(m_strongRadioButton->radioButton());

	m_quickRadioButton->radioButton()->setChecked(true);
	connect(&m_buttongroup, (void (QButtonGroup::*)(QAbstractButton *))&QButtonGroup::buttonClicked, this, &CPageAuditType::slot_buttonClicked_buttongroup);

	registerField("audit_quick", m_quickRadioButton->radioButton());
	registerField("audit_common", m_commonRadioButton->radioButton());
	registerField("audit_thorough", m_thoroughRadioButton->radioButton());
	registerField("audit_strong", m_strongRadioButton->radioButton());

	m_configurationEdit = new QTextEdit(this);
	m_configurationEdit->setReadOnly(true);

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(topLabel);
	layout->addWidget(m_quickRadioButton);
	layout->addWidget(m_commonRadioButton);
	layout->addWidget(m_thoroughRadioButton);
	layout->addWidget(m_strongRadioButton);
	layout->addWidget(m_configurationEdit);
	setLayout(layout);

	// Disable exhaustive without GPU
	ILC7PasswordEngine *jtrengine = GET_ILC7PASSWORDLINKAGE(g_pLinkage)->GetPasswordEngineByID(UUID_JTRPASSWORDENGINE);
	Q_ASSERT(jtrengine);
	ILC7GPUManager *jtrgpumanager = jtrengine->GetGPUManager();
	jtrgpumanager->Detect();
	QVector<LC7GPUInfo> gpuinfo = jtrgpumanager->GetGPUInfo();

	m_strongRadioButton->radioButton()->setEnabled(gpuinfo.size() > 0);

	UpdateUI();
}

int CPageAuditType::nextId() const
{
	TR;
	return CLC7Wizard::Page_Reporting;
}

bool CPageAuditType::isComplete() const
{
	TR;
	return m_quickRadioButton->radioButton()->isChecked() || m_commonRadioButton->radioButton()->isChecked() ||
		m_thoroughRadioButton->radioButton()->isChecked() || m_strongRadioButton->radioButton()->isChecked();
}

void CPageAuditType::UpdateUI()
{
	TR;
	QAbstractButton *button = m_buttongroup.checkedButton();

	if (button == m_quickRadioButton->radioButton())
	{
		m_configurationEdit->setHtml(
			"<b>Dictionary:</b> wordlist-medium.txt, 253525 words. No length limit, 1 hour maximum. 'Jumbo Plus' permutations set."
			);
	}
	else if (button == m_commonRadioButton->radioButton())
	{
		m_configurationEdit->setHtml(
			"<b>Dictionary:</b> wordlist-medium.txt, 253525 words. No length limit. 'Jumbo Plus' permutations set.<br><br>"
			"<b>Brute Force:</b> Letters, numbers, 7 character limit, 1 hour maximum."
			);
	}
	else if (button == m_thoroughRadioButton->radioButton())
	{
		m_configurationEdit->setHtml(
			"<b>Dictionary:</b> wordlist-small.txt, 78571 words. No length limit. 24 hour maximum. 'Jumbo' permutations set. Letter substitutions enabled.<br><br>"
			"<b>Brute Force:</b> Letters, numbers, symbols, 10 character limit, 6 hour maximum."
			);
	}
	else if (button == m_strongRadioButton->radioButton())
	{
		m_configurationEdit->setHtml(
			"<b>Brute Force:</b> Letters, numbers, symbols, and foreign characters, 14 character limit, 24 hour maximum.<br><br>"
			"<b>Dictionary:</b> wordlist-medium.txt, 253525 words. No length limit. No time limit. 'All' permutations set."
			);

	}
}

void CPageAuditType::slot_buttonClicked_buttongroup(QAbstractButton *button)
{
	UpdateUI();
}

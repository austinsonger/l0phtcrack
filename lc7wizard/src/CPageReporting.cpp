#include"stdafx.h"

CPageReporting::CPageReporting(QWidget *parent)
	: CLC7WizardPage(parent)
{
	TR;

	setTitle(tr("Reporting Options"));
	//setPixmap(QWizard::WatermarkPixmap, QPixmap(":/images/watermark.png"));

	QLabel *topLabel = new QLabel(tr(
		"Choose which display and reporting options to use for this audit:"
		));
	topLabel->setWordWrap(true);

	m_exportReportFileGroupBox = new QGroupBox(tr("Generate Report at End of Auditing"));
	m_exportReportFileGroupBox->setCheckable(true);
	m_exportReportFileGroupBox->setChecked(false);
	WIZARD_CONNECT_GROUPBOX_BUTTON(m_exportReportFileGroupBox);
	m_csvRadioButton = new CLabelRadioButton(tr("<b>CSV</b><br>Comma Separated Values<br>For import to a spreadsheet"));
	connect(m_csvRadioButton->radioButton(), &QAbstractButton::clicked, this, &CPageReporting::slot_formatbutton_clicked);
//	m_pdfRadioButton = new CLabelRadioButton(tr("<b>PDF</b><br>Portable Document Format<br>Best for printing"));
//	connect(m_pdfRadioButton->radioButton(), &QAbstractButton::clicked, this, &CPageReporting::slot_formatbutton_clicked);
	m_htmlRadioButton = new CLabelRadioButton(tr("<b>HTML</b><br>Hypertext Markup Language<br>Best for the web or email"));
	connect(m_htmlRadioButton->radioButton(), &QAbstractButton::clicked, this, &CPageReporting::slot_formatbutton_clicked);
	m_xmlRadioButton = new CLabelRadioButton(tr("<b>XML</b><br>Extensible Markup Language<br>Database-ready export format"));
	connect(m_xmlRadioButton->radioButton(), &QAbstractButton::clicked, this, &CPageReporting::slot_formatbutton_clicked);

	m_buttongroup.addButton(m_csvRadioButton->radioButton());
//	m_buttongroup.addButton(m_pdfRadioButton->radioButton());
	m_buttongroup.addButton(m_htmlRadioButton->radioButton());
	m_buttongroup.addButton(m_xmlRadioButton->radioButton());

	m_format_name = "CSV";
	m_format_ext = "csv";

	m_csvRadioButton->radioButton()->setChecked(true);

	m_reportLabel = new QLabel(tr("Report File Location:"));
	m_reportFileLineEdit = new QLineEdit();
	m_reportFileLineEdit->setText(
		QDir::toNativeSeparators(QDir(g_pLinkage->GetReportsDirectory()).absoluteFilePath(
			QString("Report (%1).csv").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh-mm-ss"))
			)));
	WIZARD_CONNECT_LINEEDIT(m_reportFileLineEdit);
	m_browseReportFileButton = new QPushButton(tr("Browse..."));
	connect(m_browseReportFileButton, &QAbstractButton::clicked, this, &CPageReporting::slot_browseCSVFileButton_clicked);

	m_displayPasswordsCheckbox = new CLabelCheckBox(tr("<b>Display &passwords when audited</b><br>Most of the time, you'll want to know what the audited passwords are, but in some situations, you may wish to verify the safety of a password without disclosing what it is. Check this box to view the cracked passwords in the output."));
	WIZARD_CONNECT_BUTTON(m_displayPasswordsCheckbox->checkBox());
	m_displayHashesCheckbox = new CLabelCheckBox(tr("<b>Display encrypted password &hashes</b><br>Check this box to display the encrypted passwords as they are seen by the operating system. These values may be of interest to some users and to others they may seem like excess clutter. To display the encrypted passwords, check this box."));
	WIZARD_CONNECT_BUTTON(m_displayHashesCheckbox->checkBox());
//	m_displayAuditTimeCheckBox = new CLabelCheckBox(tr("<b>Display how long it &took to audit each password</b><br>Checking this box will add a column to the output view that shows how long it took to audit each password."));
//	WIZARD_CONNECT_BUTTON(m_displayAuditTimeCheckBox->checkBox());
//	m_displayAuditMethodCheckBox = new CLabelCheckBox(tr("<b>Display auditing &method</b><br>Check this box to display the method used to find each password. This can be useful for identifying users who have particularly weak passwords."));
//	WIZARD_CONNECT_BUTTON(m_displayAuditMethodCheckBox->checkBox());
	
	m_displayPasswordsCheckbox->checkBox()->setChecked(true);
	m_displayHashesCheckbox->checkBox()->setChecked(true);
//	m_displayAuditTimeCheckBox->checkBox()->setChecked(true);
//	m_displayAuditMethodCheckBox->checkBox()->setChecked(true);

	registerField("report_export", m_exportReportFileGroupBox);
	registerField("report_csv", m_csvRadioButton->radioButton());
//	registerField("report_pdf", m_pdfRadioButton->radioButton());
	registerField("report_html", m_htmlRadioButton->radioButton());
	registerField("report_xml", m_xmlRadioButton->radioButton());
	registerField("report_file", m_reportFileLineEdit);
	registerField("report_display_passwords", m_displayPasswordsCheckbox->checkBox());
	registerField("report_display_hashes", m_displayHashesCheckbox->checkBox());
//	registerField("report_display_audit_time", m_displayAuditTimeCheckBox->checkBox());
//	registerField("report_display_audit_method", m_displayAuditMethodCheckBox->checkBox());

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(m_exportReportFileGroupBox);

	QGridLayout *groupboxlayout = new QGridLayout();
	groupboxlayout->addWidget(m_csvRadioButton,0,0);
//	groupboxlayout->addWidget(m_pdfRadioButton,0,1);
	groupboxlayout->addWidget(m_htmlRadioButton,1,0);
	groupboxlayout->addWidget(m_xmlRadioButton,1,1);
	
	m_exportReportFileGroupBox->setLayout(groupboxlayout);
	
	QHBoxLayout *layoutreport = new QHBoxLayout;
	layoutreport->addWidget(m_reportLabel);
	layoutreport->addWidget(m_reportFileLineEdit);
	layoutreport->addWidget(m_browseReportFileButton);

	groupboxlayout->addLayout(layoutreport,2,0,1,2);

	layout->addWidget(m_displayPasswordsCheckbox);
	layout->addWidget(m_displayHashesCheckbox);
//	layout->addWidget(m_displayAuditTimeCheckBox);
//	layout->addWidget(m_displayAuditMethodCheckBox);
	setLayout(layout);

	UpdateUI();
	emit completeChanged();
}

int CPageReporting::nextId() const
{
	TR;
	return CLC7Wizard::Page_Scheduling;
}

bool CPageReporting::isComplete() const
{
	TR;

	if (m_exportReportFileGroupBox->isChecked())
	{
		QString filename = m_reportFileLineEdit->text();
		if (filename.isEmpty())
		{
			return false;
		}
	}

	return true;
}

void CPageReporting::UpdateUI(void)
{
	TR;
}

void CPageReporting::slot_browseCSVFileButton_clicked(bool checked)
{
	TR;
	QString startdir = QDir::fromNativeSeparators(m_reportFileLineEdit->text());
	if (startdir.length() == 0)
	{
		startdir = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).first();
	}

	QString filepath;
	if (g_pLinkage->GetGUILinkage()->SaveFileDialog("Choose report file name", startdir,
		QString("%1 Files (*.%2)").arg(m_format_name).arg(m_format_ext), filepath))
	{
		m_reportFileLineEdit->setText(QDir::toNativeSeparators(filepath));
	}

	emit completeChanged();
}

inline QString withoutExtension(const QString & fileName)
{
	int lastdot = fileName.lastIndexOf(".");
	if (lastdot == -1)
	{
		return fileName;
	}
	return fileName.left(lastdot);
}

void CPageReporting::slot_formatbutton_clicked(bool checked)
{
	TR;

	if (m_csvRadioButton->radioButton()->isChecked())
	{
		m_format_name = "CSV";
		m_format_ext = "csv";
	}
/*	else if (m_pdfRadioButton->radioButton()->isChecked())
	{
		m_format_name = "PDF";
		m_format_ext = "pdf";
	}
*/	else if (m_htmlRadioButton->radioButton()->isChecked())
	{
		m_format_name = "HTML";
		m_format_ext = "html";
	}
	else if (m_xmlRadioButton->radioButton()->isChecked())
	{
		m_format_name = "XML";
		m_format_ext = "xml";
	}

	QString filename = QDir::fromNativeSeparators(m_reportFileLineEdit->text());
	if (!withoutExtension(filename).isEmpty())
	{
		filename = withoutExtension(filename) + "." + m_format_ext;
		m_reportFileLineEdit->setText(QDir::toNativeSeparators(filename));
	}

	UpdateUI();
	emit completeChanged();
}
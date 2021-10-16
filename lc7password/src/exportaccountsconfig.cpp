#include "stdafx.h"
#include "ExportAccountsConfig.h"

ExportAccountsConfig::ExportAccountsConfig(QWidget *parent, QWidget *page, const QMap<QString, QVariant> & def_config, bool simple)
	: QWidget(parent)
{
	ui.setupUi(this);

	connect(ui.csvRadioButton, &QAbstractButton::clicked, this, &ExportAccountsConfig::onFormatClicked);
//	connect(ui.pdfRadioButton, &QAbstractButton::clicked, this, &ExportAccountsConfig::onFormatClicked);
	connect(ui.htmlRadioButton, &QAbstractButton::clicked, this, &ExportAccountsConfig::onFormatClicked);
	connect(ui.xmlRadioButton, &QAbstractButton::clicked, this, &ExportAccountsConfig::onFormatClicked);
	connect(ui.fileNameEdit, &QLineEdit::textChanged, this, &ExportAccountsConfig::onTextChanged);
	connect(ui.browseButton, &QAbstractButton::clicked, this, &ExportAccountsConfig::onBrowseFile);
	connect(ui.includeStyleCheckBox, &QAbstractButton::clicked, this, &ExportAccountsConfig::onClicked);
	connect(ui.columnAuditedStatusCheckBox, &QAbstractButton::clicked, this, &ExportAccountsConfig::onClicked);
	connect(ui.columnDomainCheckBox, &QAbstractButton::clicked, this, &ExportAccountsConfig::onClicked);
	connect(ui.columnHashesCheckBox, &QAbstractButton::clicked, this, &ExportAccountsConfig::onClicked);
	connect(ui.columnLastChangedTimeCheckBox, &QAbstractButton::clicked, this, &ExportAccountsConfig::onClicked);
	connect(ui.columnLockedOutCheckBox, &QAbstractButton::clicked, this, &ExportAccountsConfig::onClicked);
	connect(ui.columnMachineCheckBox, &QAbstractButton::clicked, this, &ExportAccountsConfig::onClicked);
	connect(ui.columnPasswordsCheckBox, &QAbstractButton::clicked, this, &ExportAccountsConfig::onClicked);
	connect(ui.columnUserIdCheckBox, &QAbstractButton::clicked, this, &ExportAccountsConfig::onClicked);
	connect(ui.columnUserInfoCheckBox, &QAbstractButton::clicked, this, &ExportAccountsConfig::onClicked);
	connect(ui.columnUsernameCheckBox, &QAbstractButton::clicked, this, &ExportAccountsConfig::onClicked);

	connect(this, SIGNAL(sig_isValid(bool)), page, SLOT(slot_isValid(bool)));

	QString format = def_config.value("format","CSV").toString();
	QString filename = def_config.value("filename","").toString();
	bool include_style = def_config.value("include_style", true).toBool();
	bool column_audited_status = def_config.value("column_audited_status",true).toBool();
	bool column_domain = def_config.value("column_domain", true).toBool();
	bool column_hashes = def_config.value("column_hashes", true).toBool();
	bool column_last_changed_time = def_config.value("column_last_changed_time", true).toBool();
	bool column_locked_out = def_config.value("column_locked_out", true).toBool();
	bool column_machine = def_config.value("column_machine", true).toBool();
	bool column_passwords = def_config.value("column_passwords", true).toBool();
	bool column_user_id = def_config.value("column_user_id", true).toBool();
	bool column_user_info = def_config.value("column_user_info", true).toBool();
	bool column_username = def_config.value("column_username", true).toBool();

	m_default_include_style = include_style;

	ui.csvRadioButton->setChecked(false);
//	ui.pdfRadioButton->setChecked(false);
	ui.htmlRadioButton->setChecked(false);
	ui.xmlRadioButton->setChecked(false);

	ui.includeStyleCheckBox->setChecked(include_style);

	if (format == "CSV")
	{
		ui.csvRadioButton->setChecked(true);
		m_format_name = "CSV";
		m_format_ext = "csv";
		ui.includeStyleCheckBox->setChecked(false);
	}
/*	else if (format == "PDF")
	{
		ui.pdfRadioButton->setChecked(true);
		m_format_name = "PDF";
		m_format_ext = "pdf";
	}
*/	else if (format == "HTML")
	{
		ui.htmlRadioButton->setChecked(true);
		m_format_name = "HTML";
		m_format_ext = "html";
	}
	else if (format == "XML")
	{
		ui.xmlRadioButton->setChecked(true);
		m_format_name = "XML";
		m_format_ext = "xml";
		ui.includeStyleCheckBox->setChecked(false);
	}

	ui.fileNameEdit->setText(QDir::toNativeSeparators(filename));
	

	ui.columnAuditedStatusCheckBox->setChecked(column_audited_status);
	ui.columnDomainCheckBox->setChecked(column_domain);
	ui.columnHashesCheckBox->setChecked(column_hashes);
	ui.columnLastChangedTimeCheckBox->setChecked(column_last_changed_time);
	ui.columnLockedOutCheckBox->setChecked(column_locked_out);
	ui.columnMachineCheckBox->setChecked(column_machine);
	ui.columnPasswordsCheckBox->setChecked(column_passwords);
	ui.columnUserIdCheckBox->setChecked(column_user_id);
	ui.columnUserInfoCheckBox->setChecked(column_user_info);
	ui.columnUsernameCheckBox->setChecked(column_username);

	UpdateUI();
}

ExportAccountsConfig::~ExportAccountsConfig()
{
	TR;

}

QMap<QString, QVariant> ExportAccountsConfig::GetConfig()
{
	TR;
	QMap<QString, QVariant> config;

	config["format"] = m_format_name;
	config["filename"] = QDir::fromNativeSeparators(ui.fileNameEdit->text());
	
	config["include_style"] = ui.includeStyleCheckBox->isChecked();

	config["column_audited_status"]=ui.columnAuditedStatusCheckBox->isChecked();
	config["column_domain"] = ui.columnDomainCheckBox->isChecked();
	config["column_hashes"] = ui.columnHashesCheckBox->isChecked();
	config["column_last_changed_time"] = ui.columnLastChangedTimeCheckBox->isChecked();
	config["column_locked_out"] = ui.columnLockedOutCheckBox->isChecked();
	config["column_machine"] = ui.columnMachineCheckBox->isChecked();
	config["column_passwords"] = ui.columnPasswordsCheckBox->isChecked();
	config["column_user_id"] = ui.columnUserIdCheckBox->isChecked();
	config["column_user_info"] = ui.columnUserInfoCheckBox->isChecked();
	config["column_username"] = ui.columnUsernameCheckBox->isChecked();

	QString disp;
	disp += QString("%1 Format, File: %2").arg(m_format_name).arg(QDir::toNativeSeparators(ui.fileNameEdit->text()));
	if (config["include_style"].toBool())
	{
		disp += " (include style)";
	}
	if (config["column_audited_status"].toBool())
	{
		disp += " +Audited Status";
	}
	if (config["column_domain"].toBool())
	{
		disp += " +Domain";
	}
	if (config["column_hashes"].toBool())
	{
		disp += " +Hashes";
	}
	if (config["column_last_changed_time"].toBool())
	{
		disp += " +Last Changed Time";
	}
	if (config["column_locked_out"].toBool())
	{
		disp += " +State Flags";
	}
	if (config["column_machine"].toBool())
	{
		disp += " +Machine";
	}
	if (config["column_passwords"].toBool())
	{
		disp += " +Passwords";
	}
	if (config["column_user_id"].toBool())
	{
		disp += " +User Id";
	}
	if (config["column_user_info"].toBool())
	{
		disp += " +User Info";
	}
	if (config["column_username"].toBool())
	{
		disp += " +Username";
	}

	config["display_string"] = disp;


	return config;
}

void ExportAccountsConfig::UpdateUI()
{
	TR;

	bool valid = true;
	if (ui.fileNameEdit->text().isEmpty())
	{
		valid = false;
	}

	if (/*m_format_name == "PDF" || */m_format_name == "HTML")
	{
		ui.includeStyleCheckBox->setEnabled(true);
	}
	else
	{
		ui.includeStyleCheckBox->setEnabled(false);
	}

	emit sig_isValid(valid);
}

void ExportAccountsConfig::onTextChanged(const QString & str)
{
	TR;
	UpdateUI();
}

void ExportAccountsConfig::onClicked(bool checked)
{
	TR;
	UpdateUI();
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

void ExportAccountsConfig::onFormatClicked(bool checked)
{
	TR;

	if (ui.csvRadioButton->isChecked())
	{
		m_format_name = "CSV";
		m_format_ext = "csv";
		m_default_include_style = ui.includeStyleCheckBox->isChecked();
		ui.includeStyleCheckBox->setChecked(false);
	}
/*	else if (ui.pdfRadioButton->isChecked())
	{
		m_format_name = "PDF";
		m_format_ext = "pdf";
		ui.includeStyleCheckBox->setChecked(m_default_include_style);
	}
*/	else if (ui.htmlRadioButton->isChecked())
	{
		m_format_name = "HTML";
		m_format_ext = "html";
		ui.includeStyleCheckBox->setChecked(m_default_include_style);
	}
	else if (ui.xmlRadioButton->isChecked())
	{
		m_format_name = "XML";
		m_format_ext = "xml";
		m_default_include_style = ui.includeStyleCheckBox->isChecked();
		ui.includeStyleCheckBox->setChecked(false);
	}

	QString filename = QDir::fromNativeSeparators(ui.fileNameEdit->text());
	filename = withoutExtension(filename) + "." + m_format_ext;
	ui.fileNameEdit->setText(QDir::toNativeSeparators(filename));
	
	UpdateUI();
}

void ExportAccountsConfig::onBrowseFile(bool checked)
{
	TR;

	QString startdir = QDir::fromNativeSeparators(ui.fileNameEdit->text());
	if (startdir.length() == 0)
	{
		startdir = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).first();
	}

	QString filepath;
	if (g_pLinkage->GetGUILinkage()->SaveFileDialog("Choose export file name", startdir, 
		QString("%1 Files (*.%2)").arg(m_format_name).arg(m_format_ext), filepath))
	{
		ui.fileNameEdit->setText(QDir::toNativeSeparators(filepath));
	}

	UpdateUI();

}
#include "stdafx.h"
#include "importNTDSconfig.h"

ImportNTDSConfig::ImportNTDSConfig(QWidget *parent, QWidget *page, const QMap<QString, QVariant> & def_config)
	: QWidget(parent)
{
	ui.setupUi(this);

	connect(ui.browseButtonNTDS, &QAbstractButton::clicked, this, &ImportNTDSConfig::slot_clicked_browseButtonNTDS);
	connect(ui.browseButtonSYSTEM, &QAbstractButton::clicked, this, &ImportNTDSConfig::slot_clicked_browseButtonSYSTEM);
	connect(ui.KeepCurrentAccounts, &QAbstractButton::clicked, this, &ImportNTDSConfig::slot_clicked_KeepCurrentAccounts);
	connect(ui.IncludeMachineAccounts, &QAbstractButton::clicked, this, &ImportNTDSConfig::slot_clicked_IncludeMachineAccounts);
	connect(ui.NTDSFileNameEdit, &QLineEdit::textChanged, this, &ImportNTDSConfig::slot_textChanged_NTDSFileNameEdit);
	connect(ui.SYSTEMFileNameEdit, &QLineEdit::textChanged, this, &ImportNTDSConfig::slot_textChanged_SYSTEMFileNameEdit);
	connect(ui.LimitAccounts, &QGroupBox::clicked, this, &ImportNTDSConfig::slot_clicked_LimitAccounts);
	connect(ui.AccountLimit, &QLineEdit::textChanged, this, &ImportNTDSConfig::slot_textChanged_AccountLimit);
	connect(this, SIGNAL(sig_isValid(bool)), page, SLOT(slot_isValid(bool)));

	bool keep_current_accounts = def_config["keep_current_accounts"].toBool();
	bool include_machine_accounts = def_config["include_machine_accounts"].toBool();
	QString ntds_filename = def_config["ntds_filename"].toString();
	QString system_filename = def_config["system_filename"].toString();
	bool limit_accounts = def_config["limit_accounts"].toBool();
	quint32 account_limit = def_config["account_limit"].toUInt();

	ui.KeepCurrentAccounts->setChecked(keep_current_accounts);
	ui.IncludeMachineAccounts->setChecked(include_machine_accounts);
	ui.NTDSFileNameEdit->setText(QDir::toNativeSeparators(ntds_filename));
	ui.SYSTEMFileNameEdit->setText(QDir::toNativeSeparators(system_filename));
	ui.AccountLimit->setText(QString("%1").arg(account_limit));
	ui.LimitAccounts->setChecked(limit_accounts);
	ui.AccountLimit->setValidator(new QIntValidator(1, INT_MAX));

	UpdateUI();
}

ImportNTDSConfig::~ImportNTDSConfig()
{TR;

}

bool ImportNTDSConfig::GetKeepCurrentAccounts()
{TR;
	return ui.KeepCurrentAccounts->isChecked();
}

bool ImportNTDSConfig::GetIncludeMachineAccounts()
{
	TR;
	return ui.IncludeMachineAccounts->isChecked();
}

QString ImportNTDSConfig::GetNTDSFilename()
{TR;
	return QDir::fromNativeSeparators(ui.NTDSFileNameEdit->text());
}

QString ImportNTDSConfig::GetSYSTEMFilename()
{
	TR;
	return QDir::fromNativeSeparators(ui.SYSTEMFileNameEdit->text());
}


bool ImportNTDSConfig::GetLimitAccounts()
{
	TR;
	return ui.LimitAccounts->isChecked();
}

quint32 ImportNTDSConfig::GetAccountLimit()
{
	TR;
	return ui.AccountLimit->text().toUInt();
}


void ImportNTDSConfig::UpdateUI()
{TR;
	bool valid = true;

	QString NTDS_filename = QDir::fromNativeSeparators(ui.NTDSFileNameEdit->text());
	if (NTDS_filename.isEmpty() || !QFileInfo(NTDS_filename).isFile())
	{
		valid = false;
	}

	QString system_filename = QDir::fromNativeSeparators(ui.SYSTEMFileNameEdit->text());
	if (system_filename.isEmpty() || !QFileInfo(system_filename).isFile())
	{
		valid = false;
	}

	if (ui.LimitAccounts->isChecked())
	{
		ui.AccountLimit->setEnabled(true);
		if (ui.AccountLimit->text().isEmpty() || ui.AccountLimit->text().toInt() == 0)
		{
			valid = false;
		}
	}
	else
	{
		ui.AccountLimit->setEnabled(false);
	}

	emit sig_isValid(valid);
}

void ImportNTDSConfig::slot_textChanged_NTDSFileNameEdit(const QString & str)
{TR;
	UpdateUI();
}

void ImportNTDSConfig::slot_textChanged_SYSTEMFileNameEdit(const QString & str)
{
	TR;
	UpdateUI();
}

void ImportNTDSConfig::slot_clicked_browseButtonNTDS(bool checked)
{TR;
	QString startdir = QDir::fromNativeSeparators(ui.NTDSFileNameEdit->text());
	if (startdir.length() == 0)
	{
		startdir = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).first();
	}

	QString filepath;
	if (g_pLinkage->GetGUILinkage()->OpenFileDialog("Choose NTDS.DIT File", startdir, QString(), filepath))
	{
		ui.NTDSFileNameEdit->setText(QDir::toNativeSeparators(filepath));
	}

	UpdateUI();
}

void ImportNTDSConfig::slot_clicked_browseButtonSYSTEM(bool checked)
{
	TR;
	QString startdir = QDir::fromNativeSeparators(ui.SYSTEMFileNameEdit->text());
	if (startdir.length() == 0)
	{
		startdir = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).first();
	}

	QString filepath;
	if (g_pLinkage->GetGUILinkage()->OpenFileDialog("Choose SYSTEM File", startdir, QString(), filepath))
	{
		ui.SYSTEMFileNameEdit->setText(QDir::toNativeSeparators(filepath));
	}

	UpdateUI();
}


void ImportNTDSConfig::slot_clicked_KeepCurrentAccounts(bool checked)
{TR;
	UpdateUI();
}

void ImportNTDSConfig::slot_clicked_IncludeMachineAccounts(bool checked)
{
	TR;
	UpdateUI();
}

void ImportNTDSConfig::slot_clicked_LimitAccounts(bool checked)
{
	TR;
	UpdateUI();
}

void ImportNTDSConfig::slot_textChanged_AccountLimit(const QString &str)
{
	TR;
	UpdateUI();
}

#include "stdafx.h"
#include "importSAMconfig.h"

ImportSAMConfig::ImportSAMConfig(QWidget *parent, QWidget *page, const QMap<QString, QVariant> & def_config)
	: QWidget(parent)
{
	ui.setupUi(this);

	connect(ui.browseButtonSAM, &QAbstractButton::clicked, this, &ImportSAMConfig::slot_clicked_browseButtonSAM);
	connect(ui.browseButtonSYSTEM, &QAbstractButton::clicked, this, &ImportSAMConfig::slot_clicked_browseButtonSYSTEM);
	connect(ui.KeepCurrentAccounts, &QAbstractButton::clicked, this, &ImportSAMConfig::slot_clicked_KeepCurrentAccounts);
	connect(ui.SAMFileNameEdit, &QLineEdit::textChanged, this, &ImportSAMConfig::slot_textChanged_SAMFileNameEdit);
	connect(ui.SYSTEMFileNameEdit, &QLineEdit::textChanged, this, &ImportSAMConfig::slot_textChanged_SYSTEMFileNameEdit);
	connect(ui.LimitAccounts, &QGroupBox::clicked, this, &ImportSAMConfig::slot_clicked_LimitAccounts);
	connect(ui.AccountLimit, &QLineEdit::textChanged, this, &ImportSAMConfig::slot_textChanged_AccountLimit);
	connect(this, SIGNAL(sig_isValid(bool)), page, SLOT(slot_isValid(bool)));

	bool keep_current_accounts = def_config["keep_current_accounts"].toBool();
	QString sam_filename = def_config["sam_filename"].toString();
	QString system_filename = def_config["system_filename"].toString();
	bool limit_accounts = def_config["limit_accounts"].toBool();
	quint32 account_limit = def_config["account_limit"].toUInt();

	ui.KeepCurrentAccounts->setChecked(keep_current_accounts);
	ui.SAMFileNameEdit->setText(QDir::toNativeSeparators(sam_filename));
	ui.SYSTEMFileNameEdit->setText(QDir::toNativeSeparators(system_filename));
	ui.AccountLimit->setText(QString("%1").arg(account_limit));
	ui.LimitAccounts->setChecked(limit_accounts);
	ui.AccountLimit->setValidator(new QIntValidator(1, INT_MAX));

	UpdateUI();
}

ImportSAMConfig::~ImportSAMConfig()
{TR;

}

bool ImportSAMConfig::GetKeepCurrentAccounts()
{TR;
	return ui.KeepCurrentAccounts->isChecked();
}

QString ImportSAMConfig::GetSAMFilename()
{TR;
	return QDir::fromNativeSeparators(ui.SAMFileNameEdit->text());
}

QString ImportSAMConfig::GetSYSTEMFilename()
{
	TR;
	return QDir::fromNativeSeparators(ui.SYSTEMFileNameEdit->text());
}


bool ImportSAMConfig::GetLimitAccounts()
{
	TR;
	return ui.LimitAccounts->isChecked();
}

quint32 ImportSAMConfig::GetAccountLimit()
{
	TR;
	return ui.AccountLimit->text().toUInt();
}


void ImportSAMConfig::UpdateUI()
{TR;
	bool valid = true;

	QString sam_filename = QDir::fromNativeSeparators(ui.SAMFileNameEdit->text());
	if (sam_filename.isEmpty() || !QFileInfo(sam_filename).isFile())
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

void ImportSAMConfig::slot_textChanged_SAMFileNameEdit(const QString & str)
{TR;
	UpdateUI();
}

void ImportSAMConfig::slot_textChanged_SYSTEMFileNameEdit(const QString & str)
{
	TR;
	UpdateUI();
}

void ImportSAMConfig::slot_clicked_browseButtonSAM(bool checked)
{TR;
	QString startdir = QDir::fromNativeSeparators(ui.SAMFileNameEdit->text());
	if (startdir.length() == 0)
	{
		startdir = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).first();
	}

	QString filepath;
	if (g_pLinkage->GetGUILinkage()->OpenFileDialog("Choose SAM File", startdir, QString(), filepath))
	{
		ui.SAMFileNameEdit->setText(QDir::toNativeSeparators(filepath));
	}

	UpdateUI();
}

void ImportSAMConfig::slot_clicked_browseButtonSYSTEM(bool checked)
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


void ImportSAMConfig::slot_clicked_KeepCurrentAccounts(bool checked)
{TR;
	UpdateUI();
}


void ImportSAMConfig::slot_clicked_LimitAccounts(bool checked)
{
	TR;
	UpdateUI();
}

void ImportSAMConfig::slot_textChanged_AccountLimit(const QString &str)
{
	TR;
	UpdateUI();
}

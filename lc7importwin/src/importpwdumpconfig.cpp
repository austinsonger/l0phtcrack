#include "stdafx.h"
#include "importpwdumpconfig.h"

ImportPWDumpConfig::ImportPWDumpConfig(QWidget *parent, QWidget *page, const QMap<QString, QVariant> & def_config, bool simple)
	: QWidget(parent)
{
	ui.setupUi(this);

	connect(ui.browseButton, &QAbstractButton::clicked, this, &ImportPWDumpConfig::slot_clicked_browseButton);
	connect(ui.KeepCurrentAccounts, &QAbstractButton::clicked, this, &ImportPWDumpConfig::slot_clicked_KeepCurrentAccounts);
	connect(ui.fileNameEdit, &QLineEdit::textChanged, this, &ImportPWDumpConfig::slot_textChanged_fileNameEdit);
	connect(ui.LimitAccounts, &QGroupBox::clicked, this, &ImportPWDumpConfig::slot_clicked_LimitAccounts);
	connect(ui.AccountLimit, &QLineEdit::textChanged, this, &ImportPWDumpConfig::slot_textChanged_AccountLimit);

	connect(this, SIGNAL(sig_isValid(bool)), page, SLOT(slot_isValid(bool)));

	bool keep_current_accounts = def_config["keep_current_accounts"].toBool();
	QString filename = def_config["filename"].toString();
	bool limit_accounts = def_config["limit_accounts"].toBool();
	quint32 account_limit = def_config["account_limit"].toUInt();

	ui.KeepCurrentAccounts->setChecked(keep_current_accounts);
	ui.fileNameEdit->setText(QDir::toNativeSeparators(filename));
	ui.AccountLimit->setText(QString("%1").arg(account_limit));
	ui.LimitAccounts->setChecked(limit_accounts);
	ui.AccountLimit->setValidator(new QIntValidator(1,INT_MAX));

	if (simple)
	{
		ui.KeepCurrentAccounts->setChecked(false);
		ui.KeepCurrentAccounts->setVisible(false);

		ui.LimitAccounts->setChecked(false);
		ui.LimitAccounts->setVisible(false);
	}

	UpdateUI();
}

ImportPWDumpConfig::~ImportPWDumpConfig()
{TR;

}

bool ImportPWDumpConfig::GetKeepCurrentAccounts()
{TR;
	return ui.KeepCurrentAccounts->isChecked();
}

QString ImportPWDumpConfig::GetFilename()
{TR;
	return QDir::fromNativeSeparators(ui.fileNameEdit->text());
}

bool ImportPWDumpConfig::GetLimitAccounts()
{
	TR;
	return ui.LimitAccounts->isChecked();
}

quint32 ImportPWDumpConfig::GetAccountLimit()
{
	TR;
	return ui.AccountLimit->text().toUInt();
}


void ImportPWDumpConfig::UpdateUI()
{TR;
	bool valid = true;

	QString filename = QDir::fromNativeSeparators(ui.fileNameEdit->text());
	if (filename.isEmpty() || !QFileInfo(filename).isFile())
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

void ImportPWDumpConfig::slot_textChanged_fileNameEdit(const QString & str)
{TR;
	UpdateUI();
}

void ImportPWDumpConfig::slot_clicked_browseButton(bool checked)
{TR;
	QString startdir = QDir::fromNativeSeparators(ui.fileNameEdit->text());
	if (startdir.length() == 0)
	{
		startdir = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).first();
	}

	QString filepath;
	if (g_pLinkage->GetGUILinkage()->OpenFileDialog("Choose PWDump File", startdir, QString(), filepath))
	{
		ui.fileNameEdit->setText(QDir::toNativeSeparators(filepath));
	}

	UpdateUI();
}

void ImportPWDumpConfig::slot_clicked_KeepCurrentAccounts(bool checked)
{TR;
	UpdateUI();
}

void ImportPWDumpConfig::slot_clicked_LimitAccounts(bool checked)
{
	TR;
	UpdateUI();
}

void ImportPWDumpConfig::slot_textChanged_AccountLimit(const QString &str)
{
	TR;
	UpdateUI();
}

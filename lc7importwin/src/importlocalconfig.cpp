#include "stdafx.h"
#include "importlocalconfig.h"

ImportLocalConfig::ImportLocalConfig(QWidget *parent, QWidget *page, const QMap<QString,QVariant> & def_config, bool simple)
	: QWidget(parent)
{
	ui.setupUi(this);
		
	connect(ui.UseCurrentCreds,&QAbstractButton::clicked, this, &ImportLocalConfig::onClicked);
	connect(ui.UseSpecificCreds,&QAbstractButton::clicked, this, &ImportLocalConfig::onClicked);
	connect(ui.UseSavedCreds,&QAbstractButton::clicked, this, &ImportLocalConfig::onClicked);
	connect(ui.Username,&QLineEdit::textChanged,this, &ImportLocalConfig::onTextChanged);
	connect(ui.Password,&QLineEdit::textChanged,this, &ImportLocalConfig::onTextChanged);
	connect(ui.Domain,&QLineEdit::textChanged,this, &ImportLocalConfig::onTextChanged);
	connect(ui.LimitAccounts, &QGroupBox::clicked, this, &ImportLocalConfig::onClicked);
	connect(ui.AccountLimit, &QLineEdit::textChanged, this, &ImportLocalConfig::onTextChanged);
	connect(this, SIGNAL(sig_isValid(bool)), page, SLOT(slot_isValid(bool)));

	bool keep_current_accounts=def_config["keep_current_accounts"].toBool();
	bool include_machine_accounts=def_config["include_machine_accounts"].toBool();
	bool use_current_creds=def_config["use_current_creds"].toBool();
	bool use_saved_creds=def_config["use_saved_creds"].toBool();
	bool use_specific_creds=def_config["use_specific_creds"].toBool();
	QString username=def_config["username"].toString();
	LC7SecureString password = def_config["password"].value<LC7SecureString>();
	QString domain=def_config["domain"].toString();
	bool limit_accounts = def_config["limit_accounts"].toBool();
	quint32 account_limit = def_config["account_limit"].toUInt();
	//bool save_creds=def_config["save_creds"].toBool();

	ui.KeepCurrentAccounts->setChecked(keep_current_accounts);
	ui.IncludeMachineAccounts->setChecked(include_machine_accounts);
	ui.UseCurrentCreds->setChecked(use_current_creds);
	ui.UseSavedCreds->setChecked(use_saved_creds);
	ui.UseSpecificCreds->setChecked(use_specific_creds);
	ui.Username->setText(username);
	ui.Password->setText(password.GetString());
	ui.Domain->setText(domain);
	ui.SaveCreds->setChecked(false);// save_creds);
	ui.AccountLimit->setText(QString("%1").arg(account_limit));
	ui.LimitAccounts->setChecked(limit_accounts);
	ui.AccountLimit->setValidator(new QIntValidator(1, INT_MAX));

	if (simple)
	{
		ui.KeepCurrentAccounts->setChecked(false);
		ui.IncludeMachineAccounts->setChecked(false);
		ui.KeepCurrentAccounts->setVisible(false);
		ui.IncludeMachineAccounts->setVisible(false);

		
		ui.LimitAccounts->setChecked(false);
		ui.LimitAccounts->setVisible(false);
	}

	UpdateUI();
}

ImportLocalConfig::~ImportLocalConfig()
{TR;

}

bool ImportLocalConfig::GetKeepCurrentAccounts()
{TR;
	return ui.KeepCurrentAccounts->isChecked();
}

bool ImportLocalConfig::GetIncludeMachineAccounts()
{TR;
	return ui.IncludeMachineAccounts->isChecked();
}

bool ImportLocalConfig::GetUseCurrentCreds()
{TR;
	return ui.UseCurrentCreds->isChecked();
}

bool ImportLocalConfig::GetUseSavedCreds()
{TR;
	return ui.UseSavedCreds->isChecked();
}

bool ImportLocalConfig::GetUseSpecificCreds()
{TR;
	return ui.UseSpecificCreds->isChecked();
}

QString ImportLocalConfig::GetUsername()
{TR;
	return ui.Username->text();
}

LC7SecureString ImportLocalConfig::GetPassword()
{TR;
	return LC7SecureString(ui.Password->text(), QString("the Windows password for user '%1\\%2' on the local machine").arg(ui.Domain->text()).arg(ui.Username->text()));
}

QString ImportLocalConfig::GetDomain()
{TR;
	return ui.Domain->text();
}

bool ImportLocalConfig::GetSaveCredentials()
{TR;
	return ui.SaveCreds->isChecked();
}

bool ImportLocalConfig::GetLimitAccounts()
{
	TR;
	return ui.LimitAccounts->isChecked();
}

quint32 ImportLocalConfig::GetAccountLimit()
{
	TR;
	return ui.AccountLimit->text().toUInt();
}


void ImportLocalConfig::UpdateUI()
{TR;
	if(g_pLinkage->SecureKeyExists("credentials", QString("importwindowslocal_username")))
	{
		ui.UseSavedCreds->setEnabled(true);
	}
	else
	{
		ui.UseSavedCreds->setEnabled(false);
	}

	ui.CredentialsBox->setEnabled(ui.UseSpecificCreds->isChecked());

	bool valid=true;
	if(ui.UseSpecificCreds->isChecked() && ui.Username->text().isEmpty())
	{
		valid=false;
	}

	if (ui.LimitAccounts->isChecked())
	{
		ui.AccountLimit->setEnabled(true);
		if (ui.AccountLimit->text().isEmpty() || ui.AccountLimit->text().toInt()==0)
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

void ImportLocalConfig::onTextChanged(const QString & str)
{TR;
	UpdateUI();
}

void ImportLocalConfig::onClicked(bool checked)
{TR;
	UpdateUI();
}
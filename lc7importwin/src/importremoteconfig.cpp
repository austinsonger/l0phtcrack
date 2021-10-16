#include "stdafx.h"
#include "importremoteconfig.h"

ImportRemoteConfig::ImportRemoteConfig(QWidget *parent, QWidget *page, const QMap<QString,QVariant> & def_config, bool simple)
	: QWidget(parent)
{
	ui.setupUi(this);
	m_page=page;

	connect(ui.HostComboBox,&QComboBox::editTextChanged, this, &ImportRemoteConfig::onTextChanged);
	connect(ui.HostComboBox,&QComboBox::currentTextChanged, this, &ImportRemoteConfig::onTextChanged);
	connect(ui.UseCurrentCreds,&QAbstractButton::clicked, this, &ImportRemoteConfig::onClicked);
	connect(ui.UseSpecificCreds,&QAbstractButton::clicked, this, &ImportRemoteConfig::onClicked);
	connect(ui.UseSavedCreds,&QAbstractButton::clicked, this, &ImportRemoteConfig::onClicked);
	connect(ui.UseSavedDefaultCreds,&QAbstractButton::clicked, this, &ImportRemoteConfig::onClicked);
	connect(ui.Username,&QLineEdit::textChanged,this, &ImportRemoteConfig::onTextChanged);
	connect(ui.Password,&QLineEdit::textChanged,this, &ImportRemoteConfig::onTextChanged);
	connect(ui.Domain,&QLineEdit::textChanged,this, &ImportRemoteConfig::onTextChanged);
	connect(ui.LimitAccounts, &QGroupBox::clicked, this, &ImportRemoteConfig::onClicked);
	connect(ui.AccountLimit, &QLineEdit::textChanged, this, &ImportRemoteConfig::onTextChanged);
	connect(ui.ImportMode, (void (QComboBox::*)(int)) &QComboBox::currentIndexChanged, this, &ImportRemoteConfig::onCurrentIndexChanged);

	connect(this,SIGNAL(sig_isValid(bool)),page,SLOT(slot_isValid(bool)));

	bool keep_current_accounts=def_config.value("keep_current_accounts",false).toBool();
	bool include_machine_accounts=def_config.value("include_machine_accounts",false).toBool();
	bool use_current_creds=def_config.value("use_current_creds",true).toBool();
	bool use_saved_creds=def_config.value("use_saved_creds",false).toBool();
	bool use_saved_default_creds = def_config.value("use_saved_default_creds", false).toBool();
	bool use_specific_creds = def_config.value("use_specific_creds", false).toBool();
	QString host=def_config["host"].toString();
	QStringList host_history=def_config["host_history"].toStringList();
	QString username=def_config["username"].toString();
	LC7SecureString password = def_config["password"].value<LC7SecureString>();
	QString domain=def_config["domain"].toString();
	bool limit_accounts = def_config.value("limit_accounts",false).toBool();
	quint32 account_limit = def_config["account_limit"].toUInt();
	//bool save_creds=def_config["save_creds"].toBool();
	//bool save_default_creds=def_config["save_default_creds"].toBool();
	quint32 import_mode = def_config.value("import_mode",0).toUInt();

	ui.KeepCurrentAccounts->setChecked(keep_current_accounts);
	ui.IncludeMachineAccounts->setChecked(include_machine_accounts);
	ui.UseCurrentCreds->setChecked(use_current_creds);
	ui.UseSavedCreds->setChecked(use_saved_creds);
	ui.UseSavedDefaultCreds->setChecked(use_saved_default_creds);
	foreach(QString hostitem,host_history)
	{
		ui.HostComboBox->addItem(hostitem);
	}
	ui.HostComboBox->setCurrentText(host);
	ui.Username->setText(username);
	ui.Password->setText(password.GetString());
	ui.Domain->setText(domain);
	ui.SaveCreds->setChecked(false);// save_creds);
	ui.SaveDefaultCreds->setChecked(false);// save_default_creds);
	ui.AccountLimit->setText(QString("%1").arg(account_limit));
	ui.LimitAccounts->setChecked(limit_accounts);
	ui.AccountLimit->setValidator(new QIntValidator(1, INT_MAX));

	ui.ImportMode->addItem("AD replication first, then SMB agent");
	ui.ImportMode->addItem("SMB agent first, then AD replication");
	ui.ImportMode->addItem("AD replication only");
	ui.ImportMode->addItem("SMB agent only");
	ui.ImportMode->setCurrentIndex(import_mode);

	if (simple)
	{
		ui.KeepCurrentAccounts->setChecked(false);
		ui.IncludeMachineAccounts->setChecked(false);
		ui.KeepCurrentAccounts->setVisible(false);
		ui.IncludeMachineAccounts->setVisible(false);
		
		ui.LimitAccounts->setChecked(false);
		ui.LimitAccounts->setVisible(false);
		ui.ImportModeLabel->setVisible(false);
		ui.ImportMode->setVisible(false);
	}

	UpdateUI();
}

ImportRemoteConfig::~ImportRemoteConfig()
{TR;

}

bool ImportRemoteConfig::GetKeepCurrentAccounts()
{TR;
	return ui.KeepCurrentAccounts->isChecked();
}

bool ImportRemoteConfig::GetIncludeMachineAccounts()
{TR;
	return ui.IncludeMachineAccounts->isChecked();
}

bool ImportRemoteConfig::GetUseCurrentCreds()
{TR;
	return ui.UseCurrentCreds->isChecked();
}

bool ImportRemoteConfig::GetUseSavedCreds()
{TR;
	return ui.UseSavedCreds->isChecked();
}

bool ImportRemoteConfig::GetUseSavedDefaultCreds()
{TR;
	return ui.UseSavedDefaultCreds->isChecked();
}

bool ImportRemoteConfig::GetUseSpecificCreds()
{TR;
	return ui.UseSpecificCreds->isChecked();
}

QString ImportRemoteConfig::GetHost()
{TR;
	return ui.HostComboBox->currentText();
}

QStringList ImportRemoteConfig::GetHostHistory()
{TR;
	QStringList hh;
	for(int i=0;i<ui.HostComboBox->count();i++)
	{
		hh.append(ui.HostComboBox->itemText(i));
	}

	return hh;
}

QString ImportRemoteConfig::GetUsername()
{TR;
	return ui.Username->text();
}

LC7SecureString ImportRemoteConfig::GetPassword()
{TR;
	return LC7SecureString(ui.Password->text(), QString("the Windows password for user '%1\\%2' on host '%3'").arg(ui.Domain->text()).arg(ui.Username->text()).arg(ui.HostComboBox->currentText()));
}

QString ImportRemoteConfig::GetDomain()
{TR;
	return ui.Domain->text();
}

bool ImportRemoteConfig::GetSaveCredentials()
{TR;
	return ui.SaveCreds->isChecked();
}

bool ImportRemoteConfig::GetSaveDefaultCredentials()
{TR;
	return ui.SaveDefaultCreds->isChecked();
}

bool ImportRemoteConfig::GetLimitAccounts()
{
	TR;
	return ui.LimitAccounts->isChecked();
}

quint32 ImportRemoteConfig::GetAccountLimit()
{
	TR;
	return ui.AccountLimit->text().toUInt();
}

quint32 ImportRemoteConfig::GetImportMode()
{
	TR;
	return ui.ImportMode->currentIndex();
}

void ImportRemoteConfig::UpdateUI()
{TR;
	QString host=GetHost();

	ui.UseCurrentCreds->setEnabled(true);

	if (g_pLinkage->SecureKeyExists("credentials", QString("importwindowsremote_username_%1").arg(host)))
	{
		ui.UseSavedCreds->setEnabled(true);
	}
	else
	{
		ui.UseSavedCreds->setEnabled(false);
		if (ui.UseSavedCreds->isChecked())
		{
			ui.UseSpecificCreds->setChecked(true);
		}
	}
	
	if(g_pLinkage->SecureKeyExists("credentials", QString("importwindowsremote_username_%1").arg("%%DEFAULT%%")))
	{
		ui.UseSavedDefaultCreds->setEnabled(true);
	}
	else
	{
		ui.UseSavedDefaultCreds->setEnabled(false);
		if (ui.UseSavedDefaultCreds->isChecked())
		{
			ui.UseSpecificCreds->setChecked(true);
		}
	}

	ui.CredentialsBox->setEnabled(ui.UseSpecificCreds->isChecked());

	bool valid=true;
	if(host.isEmpty())
	{
		valid=false;
	}
	else if(ui.UseSpecificCreds->isChecked() && ui.Username->text().isEmpty())
	{
		valid=false;
	}

	if(ui.UseCurrentCreds->isChecked() && !ui.UseCurrentCreds->isEnabled())
	{
		valid=false;
	}
	if(ui.UseSavedCreds->isChecked() && !ui.UseSavedCreds->isEnabled())
	{
		valid=false;
	}
	if(ui.UseSavedDefaultCreds->isChecked() && !ui.UseSavedDefaultCreds->isEnabled())
	{
		valid=false;
	}
	if(ui.UseSpecificCreds->isChecked() && !ui.UseSpecificCreds->isEnabled())
	{
		valid=false;
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

void ImportRemoteConfig::onTextChanged(const QString & str)
{TR;
	UpdateUI();
}

void ImportRemoteConfig::onClicked(bool checked)
{TR;
	UpdateUI();
}

void ImportRemoteConfig::onCurrentIndexChanged(int index)
{
	TR;
	UpdateUI();
}

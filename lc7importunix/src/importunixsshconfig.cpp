#include "stdafx.h"

ImportUnixSSHConfig::ImportUnixSSHConfig(QWidget *parent, QWidget *page, const QMap<QString, QVariant> & def_config, bool simple)
	: QWidget(parent)
{
	ui.setupUi(this);
	ui.credentialsVerifiedLabel->setObjectName("success");
	m_page = page;

	m_connected_and_validated = false;

	m_radiogroup.addButton(ui.UseSavedCreds);
	m_radiogroup.addButton(ui.UseSavedDefaultCreds);
	m_radiogroup.addButton(ui.UseSpecificCreds);

	connect(ui.HostComboBox, &QComboBox::editTextChanged, this, &ImportUnixSSHConfig::onTextChanged);
	connect(ui.HostComboBox, &QComboBox::currentTextChanged, this, &ImportUnixSSHConfig::onTextChanged);
	connect(ui.KeepCurrentAccounts, &QAbstractButton::clicked, this, &ImportUnixSSHConfig::onSafeClicked);
	connect(ui.IncludeNonLogin, &QAbstractButton::clicked, this, &ImportUnixSSHConfig::onSafeClicked);
	connect(ui.UseSpecificCreds, &QAbstractButton::clicked, this, &ImportUnixSSHConfig::onClicked);
	connect(ui.UseSavedCreds, &QAbstractButton::clicked, this, &ImportUnixSSHConfig::onClicked);
	connect(ui.UseSavedDefaultCreds, &QAbstractButton::clicked, this, &ImportUnixSSHConfig::onClicked);
	connect(ui.Username, &QLineEdit::textChanged, this, &ImportUnixSSHConfig::onUsernameTextChanged);
	connect(ui.Password, &QLineEdit::textChanged, this, &ImportUnixSSHConfig::onTextChanged);
	connect(ui.passwordRadio, &QAbstractButton::clicked, this, &ImportUnixSSHConfig::onClicked);
	connect(ui.publicKeyRadio, &QAbstractButton::clicked, this, &ImportUnixSSHConfig::onClicked);
	connect(ui.privateKeyFileEdit, &QLineEdit::textChanged, this, &ImportUnixSSHConfig::onTextChanged);
	connect(ui.browsePrivateKeyButton, &QAbstractButton::clicked, this, &ImportUnixSSHConfig::onBrowsePrivateKeyButton);
	connect(ui.noElevationRadio, &QAbstractButton::clicked, this, &ImportUnixSSHConfig::onClicked);
	connect(ui.sudoRadio, &QAbstractButton::clicked, this, &ImportUnixSSHConfig::onClicked);
	connect(ui.suRadio, &QAbstractButton::clicked, this, &ImportUnixSSHConfig::onClicked);
	connect(ui.sudoPasswordEdit, &QLineEdit::textChanged, this, &ImportUnixSSHConfig::onTextChanged);
	connect(ui.suPasswordEdit, &QLineEdit::textChanged, this, &ImportUnixSSHConfig::onTextChanged);
	connect(ui.hashTypeList, &QListWidget::currentRowChanged, this, &ImportUnixSSHConfig::onCurrentRowChanged);
	connect(ui.connectAndVerifyButton, &QAbstractButton::clicked, this, &ImportUnixSSHConfig::onConnectAndVerify);
	connect(ui.LimitAccounts, &QGroupBox::clicked, this, &ImportUnixSSHConfig::onSafeClicked);
	connect(ui.AccountLimit, &QLineEdit::textChanged, this, &ImportUnixSSHConfig::onSafeTextChanged);
	connect(this, SIGNAL(sig_isValid(bool)), page, SLOT(slot_isValid(bool)));

	bool keep_current_accounts = def_config["keep_current_accounts"].toBool();
	bool include_non_login = def_config["include_non_login"].toBool();
	bool use_saved_creds = def_config["use_saved_creds"].toBool();
	bool use_saved_default_creds = def_config["use_saved_default_creds"].toBool();
	bool use_specific_creds = def_config["use_specific_creds"].toBool();
	QString host = def_config["host"].toString();
	QStringList host_history = def_config["host_history"].toStringList();
	QString username = def_config["username"].toString();
	bool use_password_auth = def_config["use_password_auth"].toBool();
	bool use_public_key_auth = def_config["use_public_key_auth"].toBool();
	QString password = def_config["password"].value<LC7SecureString>().GetString();
	QString private_key_file = def_config["private_key_file"].toString();
	bool no_elevation = def_config["no_elevation"].toBool();
	bool sudo_elevation = def_config["sudo_elevation"].toBool();
	QString sudo_password = def_config["sudo_password"].value<LC7SecureString>().GetString();
	bool su_elevation = def_config["su_elevation"].toBool();
	QString su_password = def_config["su_password"].value<LC7SecureString>().GetString();
	//bool save_creds = def_config["save_creds"].toBool();
	//bool save_default_creds = def_config["save_default_creds"].toBool();
	m_hashtype = (FOURCC)def_config["hashtype"].toUInt();
	bool limit_accounts = def_config["limit_accounts"].toBool();
	quint32 account_limit = def_config["account_limit"].toUInt();

	ui.KeepCurrentAccounts->setChecked(keep_current_accounts);
	ui.IncludeNonLogin->setChecked(include_non_login);
	ui.UseSavedCreds->setChecked(use_saved_creds);
	ui.UseSavedDefaultCreds->setChecked(use_saved_default_creds);
	foreach(QString hostitem, host_history)
	{
		ui.HostComboBox->addItem(hostitem);
	}
	ui.HostComboBox->setCurrentText(host);
	ui.Username->setText(username);
	ui.Password->setText(password);
	ui.passwordRadio->setChecked(use_password_auth);
	ui.publicKeyRadio->setChecked(use_public_key_auth);
	ui.privateKeyFileEdit->setText(QDir::toNativeSeparators(private_key_file));
	ui.noElevationRadio->setChecked(no_elevation);
	ui.sudoRadio->setChecked(sudo_elevation);
	ui.suRadio->setChecked(su_elevation);
	ui.sudoPasswordEdit->setText(sudo_password);
	ui.suPasswordEdit->setText(su_password);
	ui.SaveCreds->setChecked(false);
	ui.SaveDefaultCreds->setChecked(false);
	ui.AccountLimit->setText(QString("%1").arg(account_limit));
	ui.LimitAccounts->setChecked(limit_accounts);
	ui.AccountLimit->setValidator(new QIntValidator(1, INT_MAX));

	if (g_pLinkage->SecureKeyExists("credentials", QString("importunixssh_username_%1").arg(host)))
	{
		ui.UseSavedCreds->setChecked(true);
	}
	else
	{
		if (g_pLinkage->SecureKeyExists("credentials", QString("importunixssh_username_%1").arg("%%DEFAULT%%")))
		{
			ui.UseSavedDefaultCreds->setChecked(true);
		}			
	}

	ILC7ColorManager *colman = g_pLinkage->GetGUILinkage()->GetColorManager();
	colman->StyleCommandLinkButton(ui.connectAndVerifyButton);

	if (simple)
	{
		ui.KeepCurrentAccounts->setChecked(false);
		ui.IncludeNonLogin->setChecked(false);
		ui.KeepCurrentAccounts->setVisible(false);
		ui.IncludeNonLogin->setVisible(false);
		ui.LimitAccounts->setChecked(false);
		ui.LimitAccounts->setVisible(false);

	}


	UpdateUI();
}

ImportUnixSSHConfig::~ImportUnixSSHConfig()
{
	TR;

}

bool ImportUnixSSHConfig::GetKeepCurrentAccounts()
{
	return ui.KeepCurrentAccounts->isChecked();
}

bool ImportUnixSSHConfig::GetIncludeNonLogin()
{
	return ui.IncludeNonLogin->isChecked();
}

bool ImportUnixSSHConfig::GetUseSavedCreds()
{
	return ui.UseSavedCreds->isChecked();
}

bool ImportUnixSSHConfig::GetUseSavedDefaultCreds()
{
	return ui.UseSavedDefaultCreds->isChecked();
}

bool ImportUnixSSHConfig::GetUseSpecificCreds()
{
	return ui.UseSpecificCreds->isChecked();
}

QString ImportUnixSSHConfig::GetHost()
{
	return ui.HostComboBox->currentText();
}

QStringList ImportUnixSSHConfig::GetHostHistory()
{
	QStringList hh;
	for (int i = 0; i<ui.HostComboBox->count(); i++)
	{
		hh.append(ui.HostComboBox->itemText(i));
	}

	return hh;
}

QString ImportUnixSSHConfig::GetUsername()
{
	return ui.Username->text();
}

LC7SecureString ImportUnixSSHConfig::GetPassword()
{
	return LC7SecureString(ui.Password->text(), QString("the SSH password for user '%1' on host '%2'").arg(ui.Username->text()).arg(ui.HostComboBox->currentText()));
}

QString ImportUnixSSHConfig::GetPrivateKeyFile()
{
	return ui.privateKeyFileEdit->text();
}

LC7SecureString ImportUnixSSHConfig::GetPrivateKeyPassword()
{
	return LC7SecureString(ui.privateKeyPassword->text(), QString("the SSH private key password for user '%1' on host '%2'").arg(ui.Username->text()).arg(ui.HostComboBox->currentText()));
}

bool ImportUnixSSHConfig::GetUsePasswordAuth()
{
	return ui.passwordRadio->isChecked();
}

bool ImportUnixSSHConfig::GetUsePublicKeyAuth()
{
	return ui.publicKeyRadio->isChecked();
}

bool ImportUnixSSHConfig::GetNoElevation()
{
	return ui.noElevationRadio->isChecked();
}

bool ImportUnixSSHConfig::GetSUDOElevation()
{
	return ui.sudoRadio->isChecked();
}

LC7SecureString ImportUnixSSHConfig::GetSUDOPassword()
{
	return LC7SecureString(ui.sudoPasswordEdit->text(), QString("the 'sudo' password for user '%1' on host '%2'").arg(ui.Username->text()).arg(ui.HostComboBox->currentText()));
}

bool ImportUnixSSHConfig::GetSUElevation()
{
	return ui.suRadio->isChecked();
}

LC7SecureString ImportUnixSSHConfig::GetSUPassword()
{
	return LC7SecureString(ui.suPasswordEdit->text(), QString("the 'su' password for root on host '%1'").arg(ui.HostComboBox->currentText()));
}

bool ImportUnixSSHConfig::GetSaveCredentials()
{
	return ui.SaveCreds->isChecked();
}

bool ImportUnixSSHConfig::GetSaveDefaultCredentials()
{
	return ui.SaveDefaultCreds->isChecked();
}

FOURCC ImportUnixSSHConfig::GetHashType()
{
	if (!ui.hashTypeList->currentItem())
	{
		//Q_ASSERT(0);
		return 0;
	}
	return ui.hashTypeList->currentItem()->data(256).toUInt();
}

bool ImportUnixSSHConfig::GetLimitAccounts()
{
	TR;
	return ui.LimitAccounts->isChecked();
}

quint32 ImportUnixSSHConfig::GetAccountLimit()
{
	TR;
	return ui.AccountLimit->text().toUInt();
}

void ImportUnixSSHConfig::UpdateUI()
{
	TR;
	QString host = GetHost();
	if (g_pLinkage->SecureKeyExists("credentials", QString("importunixssh_username_%1").arg(host)))
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

	if (g_pLinkage->SecureKeyExists("credentials", QString("importunixssh_username_%1").arg("%%DEFAULT%%")))
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
	ui.ElevationCredentialsBox->setEnabled(ui.UseSpecificCreds->isChecked());

	bool valid = true;
	if (host.isEmpty())
	{
		valid = false;
	}
	
	if (ui.UseSpecificCreds->isChecked())
	{
		if (ui.Username->text().isEmpty())
		{
			valid = false;
		}

		ui.browsePrivateKeyButton->setEnabled(ui.publicKeyRadio->isChecked());
		ui.privateKeyFileEdit->setEnabled(ui.publicKeyRadio->isChecked());
		ui.privateKeyPassword->setEnabled(ui.publicKeyRadio->isChecked());
		ui.Password->setEnabled(ui.passwordRadio->isChecked());
		
		if (ui.publicKeyRadio->isChecked())
		{
			QString privkey = ui.privateKeyFileEdit->text();
			if (!QFileInfo(privkey).isFile())
			{
				valid = false;
			}
			ui.browsePrivateKeyButton->setEnabled(true);
			ui.Password->setEnabled(false);
		}

		ui.sudoPasswordEdit->setEnabled(ui.sudoRadio->isChecked() && !ui.passwordRadio->isChecked());
		ui.suPasswordEdit->setEnabled(ui.suRadio->isChecked());
		
		if (ui.sudoRadio->isChecked() && ui.passwordRadio->isChecked()) {
			ui.sudoPasswordEdit->setText(ui.Password->text());
		}
	}
	
	if (ui.UseSavedCreds->isChecked() && !ui.UseSavedCreds->isEnabled())
	{
		valid = false;
	}
	if (ui.UseSavedDefaultCreds->isChecked() && !ui.UseSavedDefaultCreds->isEnabled())
	{
		valid = false;
	}
	if (ui.UseSpecificCreds->isChecked() && !ui.UseSpecificCreds->isEnabled())
	{
		valid = false;
	}

	if (!m_connected_and_validated)
	{
		valid = false;
		ILC7ColorManager *colman = g_pLinkage->GetGUILinkage()->GetColorManager();
		colman->StyleCommandLinkButton(ui.connectAndVerifyButton);
		ui.connectAndVerifyButton->setEnabled(true);
		ui.connectAndVerifyButton->setVisible(true);
		ui.credentialsVerifiedLabel->setVisible(false);
	}
	else
	{
		ILC7ColorManager *colman = g_pLinkage->GetGUILinkage()->GetColorManager();
		ui.connectAndVerifyButton->setEnabled(false);
		ui.connectAndVerifyButton->setVisible(false);
		ui.credentialsVerifiedLabel->setVisible(true);

		if (ui.hashTypeList->currentRow() == -1)
		{
			valid = false;
		}
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

void ImportUnixSSHConfig::onUsernameTextChanged(const QString & str)
{
	onTextChanged(str);
	
	if (str == "root")
	{
		ui.noElevationRadio->setChecked(true);
	}

	UpdateUI();
}


void ImportUnixSSHConfig::onTextChanged(const QString & str)
{
	m_connected_and_validated = false;
	ui.hashTypeList->clear();
	UpdateUI();
}

void ImportUnixSSHConfig::onClicked(bool checked)
{
	m_connected_and_validated = false;
	ui.hashTypeList->clear();
	UpdateUI();
}

void ImportUnixSSHConfig::onSafeTextChanged(const QString & str)
{
	UpdateUI();
}

void ImportUnixSSHConfig::onSafeClicked(bool checked)
{
	UpdateUI();
}


void ImportUnixSSHConfig::onCurrentRowChanged(int row)
{
	//m_connected_and_validated = false;
	UpdateUI();
}

void ImportUnixSSHConfig::onBrowsePrivateKeyButton(bool checked)
{
	TR;
	QString privkey = QDir::fromNativeSeparators(ui.privateKeyFileEdit->text());
	if (!g_pLinkage->GetGUILinkage()->OpenFileDialog("Choose private key file", privkey, QString(), privkey))
	{
		return;
	}

	ui.privateKeyFileEdit->setText(QDir::toNativeSeparators(privkey));

	m_connected_and_validated = false;
	UpdateUI();
}

void ImportUnixSSHConfig::onConnectAndVerify(bool checked)
{
	UnixSSHImporter sshimp(NULL, NULL);

	QString host = GetHost();
	QString username;
	UnixSSHImporter::AUTHTYPE authtype;
	LC7SecureString password;
	QString privkeyfile;
	LC7SecureString privkeypassword;
	UnixSSHImporter::ELEVTYPE elevtype;
	LC7SecureString sudopassword;
	LC7SecureString supassword;

	host = GetHost();

	if (GetUseSavedCreds())
	{
		QString error;
		if (!CImportUnixSSHGUI::LoadCreds(host, authtype, username, password, privkeyfile, privkeypassword,
			elevtype, sudopassword, supassword, error))
		{
			g_pLinkage->GetGUILinkage()->ErrorMessage("SSH Test Failed",
				QString("Saved credentials could not be found: %1").arg(error));
			return;
		}
	}
	else if (GetUseSavedDefaultCreds())
	{
		QString error;
		if (!CImportUnixSSHGUI::LoadCreds("%%DEFAULT%%", authtype, username, password, privkeyfile, privkeypassword,
			elevtype, sudopassword, supassword, error))
		{
			g_pLinkage->GetGUILinkage()->ErrorMessage("SSH Test Failed",
				QString("Default saved credentials could not be found: %1").arg(error));
			return;
		}
	}
	else
	{
		username=GetUsername();
		if (GetUsePasswordAuth())
		{
			authtype=UnixSSHImporter::PASSWORD;
			password=GetPassword();
		}
		else if (GetUsePublicKeyAuth())
		{
			authtype = UnixSSHImporter::PUBLICKEY;
			privkeyfile = GetPrivateKeyFile();
			privkeypassword = GetPrivateKeyPassword();
		}
		if (GetNoElevation())
		{
			elevtype=UnixSSHImporter::NOELEVATION;
		}
		else if (GetSUDOElevation())
		{
			elevtype=UnixSSHImporter::SUDO;
			sudopassword = GetSUDOPassword();
		}
		else if (GetSUElevation())
		{
			elevtype=UnixSSHImporter::SU;
			supassword = GetSUPassword();
		}
	}

	sshimp.setIncludeNonLogin(true);
	sshimp.setHost(host);
	sshimp.setUsername(username);
	sshimp.setAuthType(authtype);
	sshimp.setPassword(password.GetString());
	sshimp.setPrivateKeyFile(privkeyfile);
	sshimp.setPrivateKeyPassword(privkeypassword.GetString());
	sshimp.setElevType(elevtype);
	sshimp.setSUDOPassword(sudopassword.GetString());
	sshimp.setSUPassword(supassword.GetString());
	
	QApplication::setOverrideCursor(Qt::WaitCursor);

	bool cancelled;
	QString error;
	QList<FOURCC> hashtypes;
	if (!sshimp.TestCredentials(hashtypes, error, cancelled))
	{

		QApplication::restoreOverrideCursor();

		g_pLinkage->GetGUILinkage()->ErrorMessage("SSH Test Failed",
			QString("Credentials could not be validated: %1").arg(error));
		return;
	}

	QApplication::restoreOverrideCursor();

	if (cancelled)
	{
		return;
	}

	m_connected_and_validated = true;
	ILC7PasswordLinkage *passlink = GET_ILC7PASSWORDLINKAGE(g_pLinkage);

	int selectedhashnum = -1;
	int curhashnum = 0;
	ui.hashTypeList->clear();
	foreach(FOURCC hashtype, QList<FOURCC>(hashtypes))
	{
		LC7HashType htype;
		if (passlink->LookupHashType(hashtype, htype, error))
		{
			QListWidgetItem * item = new QListWidgetItem(QString("%1 (%2)").arg(htype.name).arg(htype.description));
			item->setData(256, (quint32)hashtype);
			ui.hashTypeList->addItem(item);
			if (hashtype == m_hashtype)
			{
				selectedhashnum = curhashnum;
			}
			curhashnum++;
		}
		else
		{
			hashtypes.removeOne(hashtype);
		}
	}

	if (selectedhashnum != -1)
	{
		ui.hashTypeList->setCurrentRow(selectedhashnum);
	}
	else if (ui.hashTypeList->count()>0)
	{
		ui.hashTypeList->setCurrentRow(0);
	}
	else
	{
		m_hashtype = 0;
	}

	UpdateUI();
}
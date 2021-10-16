#include "stdafx.h"

ImportShadowConfig::ImportShadowConfig(QWidget *parent, QWidget *page, const QMap<QString, QVariant> & def_config, bool simple)
	: QWidget(parent), m_shadowimporter(NULL,NULL)
{
	ui.setupUi(this);

	QList<ILC7UnixImporter *> pimps;
	m_shadowimporter.GetPasswdImporters(pimps);
	foreach(ILC7UnixImporter *pimp, pimps)
	{
		m_importers[pimp->name()] = pimp;
		ui.fileFormatComboBox->addItem(pimp->desc(), pimp->name());
	}

	connect(ui.fileFormatComboBox, (void (QComboBox::*)(int))&QComboBox::currentIndexChanged, this, &ImportShadowConfig::slot_currentIndexChanged_fileFormatComboBox);
	connect(ui.browseFile1, &QAbstractButton::clicked, this, &ImportShadowConfig::slot_clicked_browseFile1);
	connect(ui.browseFile2, &QAbstractButton::clicked, this, &ImportShadowConfig::slot_clicked_browseFile2);
	connect(ui.browseFile3, &QAbstractButton::clicked, this, &ImportShadowConfig::slot_clicked_browseFile3);
	connect(ui.KeepCurrentAccounts, &QAbstractButton::clicked, this, &ImportShadowConfig::slot_clicked_KeepCurrentAccounts);
	connect(ui.includeNonLoginCheckBox, &QAbstractButton::clicked, this, &ImportShadowConfig::slot_clicked_includeNonLoginCheckBox);
	connect(ui.file1Edit, &QLineEdit::textChanged, this, &ImportShadowConfig::slot_textChanged_file1Edit);
	connect(ui.file2Edit, &QLineEdit::textChanged, this, &ImportShadowConfig::slot_textChanged_file2Edit);
	connect(ui.file3Edit, &QLineEdit::textChanged, this, &ImportShadowConfig::slot_textChanged_file3Edit);
	connect(ui.hashTypeList, &QListWidget::currentRowChanged, this, &ImportShadowConfig::slot_currentRowChanged_hashTypeList);
	connect(ui.LimitAccounts, &QGroupBox::clicked, this, &ImportShadowConfig::slot_clicked_LimitAccounts);
	connect(ui.AccountLimit, &QLineEdit::textChanged, this, &ImportShadowConfig::slot_textChanged_AccountLimit);
	connect(this, SIGNAL(sig_isValid(bool)), page, SLOT(slot_isValid(bool)));

	m_keep_current_accounts = def_config["keep_current_accounts"].toBool();
	m_include_non_login = def_config["include_non_login"].toBool();
	m_limit_accounts = def_config["limit_accounts"].toBool();
	m_account_limit = def_config["account_limit"].toUInt();

	m_fileformat = def_config["fileformat"].toString();
	m_fileformatindex = ui.fileFormatComboBox->findData(m_fileformat);
	if (m_fileformatindex == -1)
	{
		m_fileformatindex = 0;
		m_file1name = "";
		m_file2name = "";
		m_file3name = "";
		m_hashtype = 0;

		ui.fileFormatComboBox->setCurrentIndex(0);
		slot_currentIndexChanged_fileFormatComboBox(0);
	}
	else
	{
		m_file1name = def_config["file1name"].toString();
		m_file2name = def_config["file2name"].toString();
		m_file3name = def_config["file3name"].toString();
		m_hashtype = def_config["hashtype"].toUInt();
	}

	m_file1valid = false;
	m_file2valid = false;
	m_file3valid = false;
	
	//ui.invalidFileFormat1Label->setObjectName("error");
	//ui.invalidFileFormat2Label->setObjectName("error");
	//ui.invalidFileFormat3Label->setObjectName("error");
	
	ILC7ColorManager *colman = g_pLinkage->GetGUILinkage()->GetColorManager();

	ui.invalidFileFormat1Label->setStyleSheet(QString("color: %1;").arg(colman->GetHighlightShade("BASE")));
	ui.invalidFileFormat2Label->setStyleSheet(QString("color: %1;").arg(colman->GetHighlightShade("BASE")));
	ui.invalidFileFormat3Label->setStyleSheet(QString("color: %1;").arg(colman->GetHighlightShade("BASE")));

	if (simple)
	{
		m_keep_current_accounts=false;
		m_include_non_login = false;
		
		ui.KeepCurrentAccounts->setVisible(false);
		ui.includeNonLoginCheckBox->setVisible(false);
		
		ui.LimitAccounts->setChecked(false);
		ui.LimitAccounts->setVisible(false);
	}

	RefreshContent();
}

ImportShadowConfig::~ImportShadowConfig()
{TR;

}


void ImportShadowConfig::CheckFileFormat()
{TR;
	m_file1type = QString();
	m_file2type = QString();
	m_file3type = QString();

	if (!m_importers.contains(m_fileformat))
	{
		return;
	}
	ILC7UnixImporter *pimp = m_importers[m_fileformat];

	if (pimp->filetypes().size() >= 1)
		m_file1type = pimp->filetypes()[0];
	if (pimp->filetypes().size() >= 2)
		m_file2type = pimp->filetypes()[1];
	if (pimp->filetypes().size() >= 3)
		m_file3type = pimp->filetypes()[2];
}

void ImportShadowConfig::CheckHashTypes()
{TR;

	FOURCC current_hashtype = m_hashtype;
	m_hashtype = 0;
	m_file1valid = false;
	m_file2valid = false;
	m_file3valid = false;

	ui.hashTypeList->clear();

	if (!m_importers.contains(m_fileformat))
	{
		return;
	}
	ILC7UnixImporter *pimp = m_importers[m_fileformat];

	QList<FOURCC> hashtypes;
	QStringList filenames;
	QList<bool> filevalid;
	if (!m_file1name.isEmpty())
	{
		filenames.append(m_file1name);
	}
	if (!m_file2name.isEmpty())
	{
		filenames.append(m_file2name);
	}
	if (!m_file3name.isEmpty())
	{
		filenames.append(m_file3name);
	}
	
	pimp->CheckHashTypes(filenames, filevalid, hashtypes);

	if (filevalid.size() >= 1)
	{
		m_file1valid = filevalid[0];
	}
	if (filevalid.size() >= 2)
	{
		m_file2valid = filevalid[1];
	}
	if (filevalid.size() >= 3)
	{
		m_file3valid = filevalid[2];
	}

	// Register account types we can import
	ILC7PasswordLinkage *passlink = GET_ILC7PASSWORDLINKAGE(g_pLinkage);

	QString error;
	int selectedhashnum = -1;
	int curhashnum = 0;
	foreach(FOURCC hashtype, QList<FOURCC>(hashtypes))
	{
		LC7HashType htype;
		if (passlink->LookupHashType(hashtype, htype, error))
		{
			QListWidgetItem * item = new QListWidgetItem(QString("%1 (%2)").arg(htype.name).arg(htype.description));
			item->setData(256, (quint32)hashtype);
			ui.hashTypeList->addItem(item);
			if (hashtype == current_hashtype)
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
}

void ImportShadowConfig::RefreshContent()
{TR;
	if (m_fileformatindex >= 0 && m_fileformatindex < ui.fileFormatComboBox->count())
	{ 
		ui.fileFormatComboBox->setCurrentIndex(m_fileformatindex);
	}
	else
	{
		ui.fileFormatComboBox->setCurrentIndex(0);
	}

	CheckFileFormat();

	ui.KeepCurrentAccounts->setChecked(m_keep_current_accounts);
	ui.includeNonLoginCheckBox->setChecked(m_include_non_login);
	ui.LimitAccounts->setChecked(m_limit_accounts);
	if (m_limit_accounts)
	{
		ui.AccountLimit->setText(QString("%1").arg(m_account_limit));
	}
	else
	{
		ui.AccountLimit->setText("");
	}

	ui.file1Edit->setText(QDir::toNativeSeparators(m_file1name));
	ui.file2Edit->setText(QDir::toNativeSeparators(m_file2name));
	ui.file3Edit->setText(QDir::toNativeSeparators(m_file3name));

	CheckHashTypes();

	UpdateUI();
}

bool ImportShadowConfig::GetKeepCurrentAccounts()
{TR;
	return m_keep_current_accounts;
}

bool ImportShadowConfig::GetIncludeNonLogin()
{TR;
	return m_include_non_login;
}

QString ImportShadowConfig::GetFileFormat()
{TR;
	return m_fileformat;
}

FOURCC ImportShadowConfig::GetHashType()
{TR;
	return m_hashtype;
}

bool ImportShadowConfig::GetLimitAccounts()
{
	return m_limit_accounts;
}

quint32 ImportShadowConfig::GetAccountLimit()
{
	return m_account_limit;
}

QString ImportShadowConfig::GetFile1Name()
{TR;
	return m_file1name;
}

QString ImportShadowConfig::GetFile2Name()
{TR;
	return m_file2name;
}

QString ImportShadowConfig::GetFile3Name()
{
	TR;
	return m_file3name;
}

void ImportShadowConfig::UpdateUI()
{TR;
	bool valid = true;
	bool needfile1 = !m_file1type.isEmpty();
	bool needfile2 = !m_file2type.isEmpty();
	bool needfile3 = !m_file3type.isEmpty();

	if (!m_file1type.isEmpty() && (m_file1name.isEmpty() || !QFileInfo(m_file1name).isFile()))
	{
		valid = false;
	}
	if (!m_file2type.isEmpty() && (m_file2name.isEmpty() || !QFileInfo(m_file2name).isFile()))
	{
		valid = false;
	}
	if (!m_file3type.isEmpty() && (m_file3name.isEmpty() || !QFileInfo(m_file3name).isFile()))
	{
		valid = false;
	}

	ui.invalidFileFormat1Label->setVisible(needfile1 && !m_file1name.isEmpty() && !m_file1valid);
	ui.invalidFileFormat2Label->setVisible(needfile2 && !m_file2name.isEmpty() && !m_file2valid);
	ui.invalidFileFormat3Label->setVisible(needfile3 && !m_file3name.isEmpty() && !m_file3valid);

	ui.file1Label->setVisible(needfile1);
	ui.browseFile1->setVisible(needfile1);
	ui.file1Edit->setVisible(needfile1);
	ui.file1Label->setText(QString("%1 File:").arg(m_file1type));

	ui.file2Label->setVisible(needfile2);
	ui.browseFile2->setVisible(needfile2);
	ui.file2Edit->setVisible(needfile2);
	ui.file2Label->setText(QString("%1 File:").arg(m_file2type));
	
	ui.file3Label->setVisible(needfile3);
	ui.browseFile3->setVisible(needfile3);
	ui.file3Edit->setVisible(needfile3);
	ui.file3Label->setText(QString("%1 File:").arg(m_file3type));

	if (m_hashtype == 0)
	{
		valid = false;
	}

	if (m_limit_accounts)
	{
		ui.AccountLimit->setEnabled(true);
		if (m_account_limit == 0)
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

void ImportShadowConfig::slot_textChanged_file1Edit(const QString & str)
{TR;
	m_file1name = QDir::fromNativeSeparators(str);
	CheckHashTypes();
	UpdateUI();
}

void ImportShadowConfig::slot_textChanged_file2Edit(const QString & str)
{TR;
	m_file2name = QDir::fromNativeSeparators(str);
	CheckHashTypes();
	UpdateUI();
}

void ImportShadowConfig::slot_textChanged_file3Edit(const QString & str)
{
	TR;
	m_file3name = QDir::fromNativeSeparators(str);
	CheckHashTypes();
	UpdateUI();
}


void ImportShadowConfig::slot_currentIndexChanged_fileFormatComboBox(int index)
{TR;
	m_fileformatindex = index;
	m_fileformat = ui.fileFormatComboBox->itemData(index).toString();

	CheckFileFormat();
	CheckHashTypes();
	UpdateUI(); 
}

void ImportShadowConfig::slot_clicked_browseFile1(bool checked)
{TR;
	QString startdir = QDir::fromNativeSeparators(m_file1name);
	if (startdir.length() == 0)
	{
		startdir = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).first();
	}

	QString filepath;
	if (g_pLinkage->GetGUILinkage()->OpenFileDialog(QString("Choose %1 File").arg(m_file1type), startdir, QString(), filepath))
	{
		ui.file1Edit->setText(QDir::toNativeSeparators(filepath));
		m_file1name = filepath;
	}

	UpdateUI();
}

void ImportShadowConfig::slot_clicked_browseFile2(bool checked)
{TR;
	QString startdir = QDir::fromNativeSeparators(m_file1name);
	if (startdir.length() == 0)
	{
		startdir = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).first();
	}

	QString filepath;
	if (g_pLinkage->GetGUILinkage()->OpenFileDialog(QString("Choose %1 File").arg(m_file2type), startdir, QString(), filepath))
	{
		ui.file2Edit->setText(QDir::toNativeSeparators(filepath));
		m_file2name = filepath;
	}

	UpdateUI();
}

void ImportShadowConfig::slot_clicked_browseFile3(bool checked)
{
	TR;
	QString startdir = QDir::fromNativeSeparators(m_file3name);
	if (startdir.length() == 0)
	{
		startdir = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).first();
	}

	QString filepath;
	if (g_pLinkage->GetGUILinkage()->OpenFileDialog(QString("Choose %1 File").arg(m_file3type), startdir, QString(), filepath))
	{
		ui.file3Edit->setText(QDir::toNativeSeparators(filepath));
		m_file3name = filepath;
	}

	UpdateUI();
}

void ImportShadowConfig::slot_clicked_KeepCurrentAccounts(bool checked)
{TR;
	m_keep_current_accounts = checked;
	UpdateUI();
}

void ImportShadowConfig::slot_clicked_includeNonLoginCheckBox(bool checked)
{TR;
	m_include_non_login = checked;
	UpdateUI();
}

void ImportShadowConfig::slot_clicked_LimitAccounts(bool checked)
{
	TR;
	m_limit_accounts = checked;
	UpdateUI();
}

void ImportShadowConfig::slot_textChanged_AccountLimit(const QString &str)
{
	TR;
	m_account_limit = str.toUInt();
	UpdateUI();
}


void ImportShadowConfig::slot_currentRowChanged_hashTypeList(int row)
{TR;
	QListWidgetItem * item = ui.hashTypeList->item(row);
	if (item == NULL)
	{
		m_hashtype = 0;
	}
	else
	{
		m_hashtype = item->data(256).toUInt();
	}
	UpdateUI();
}

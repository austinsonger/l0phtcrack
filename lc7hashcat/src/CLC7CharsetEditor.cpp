#include "stdafx.h"
#include "CLC7CharsetEditor.h"


CLC7CharsetEditor::CLC7CharsetEditor() : m_jtrdll("sse2")
{TR;
	ui.setupUi(this);

	m_use_existing = true;
	m_charset_file = "";
	m_charset_valid = false;
	m_mask_valid = false;
	m_use_input_characters = false;
	m_use_input_dictionary_files = false;
	m_use_input_pot_files = false;
	m_input_encoding = QUuid();

	connect(ui.useExistingRadio, &QRadioButton::clicked, this, &CLC7CharsetEditor::slot_useExistingRadio_clicked);
	
	connect(ui.chrFileEdit, &QLineEdit::textChanged, this, &CLC7CharsetEditor::slot_chrFileEdit_textChanged);
	connect(ui.chrFileBrowse, &QPushButton::clicked, this, &CLC7CharsetEditor::slot_chrFileBrowse_clicked);
	
	connect(ui.createNewRadio, &QRadioButton::clicked, this, &CLC7CharsetEditor::slot_createNewRadio_clicked);
	
	connect(ui.useCharsCheckbox, &QCheckBox::clicked, this, &CLC7CharsetEditor::slot_useCharsCheckbox_clicked);
	connect(ui.inputCharactersEdit, &QLineEdit::textChanged, this, &CLC7CharsetEditor::slot_inputCharactersEdit_textChanged);
	connect(ui.inputCharactersEncodingCombo, (void (QComboBox::*)(int))&QComboBox::currentIndexChanged, this, &CLC7CharsetEditor::slot_inputCharactersEncodingCombo_currentIndexChanged);
	
	connect(ui.useDictCheckbox, &QCheckBox::clicked, this, &CLC7CharsetEditor::slot_useDictCheckbox_clicked);
	connect(ui.dictionaryList, &QListWidget::itemSelectionChanged, this, &CLC7CharsetEditor::slot_dictionaryList_itemSelectionChanged);
	connect(ui.plusButtonDict, &QAbstractButton::clicked, this, &CLC7CharsetEditor::slot_plusButtonDict_clicked);
	connect(ui.minusButtonDict, &QAbstractButton::clicked, this, &CLC7CharsetEditor::slot_minusButtonDict_clicked);

	connect(ui.usePotCheckbox, &QCheckBox::clicked, this, &CLC7CharsetEditor::slot_usePotCheckbox_clicked);
	connect(ui.potList, &QListWidget::itemSelectionChanged, this, &CLC7CharsetEditor::slot_potList_itemSelectionChanged);
	connect(ui.plusButtonPot, &QAbstractButton::clicked, this, &CLC7CharsetEditor::slot_plusButtonPot_clicked);
	connect(ui.minusButtonPot, &QAbstractButton::clicked, this, &CLC7CharsetEditor::slot_minusButtonPot_clicked);
	
	connect(ui.maskEdit, &QLineEdit::textChanged, this, &CLC7CharsetEditor::slot_maskEdit_textChanged);
	connect(ui.maskModeCheckBox, &QCheckBox::clicked, this, &CLC7CharsetEditor::slot_maskModeCheckBox_clicked);
	connect(ui.maskEncodingCombo, (void (QComboBox::*)(int))&QComboBox::currentIndexChanged, this, &CLC7CharsetEditor::slot_maskEncodingCombo_currentIndexChanged);
	connect(ui.createButton, &QAbstractButton::clicked, this, &CLC7CharsetEditor::slot_createButton_clicked);

	RefreshContent();
}

CLC7CharsetEditor::~CLC7CharsetEditor()
{TR;
}


void CLC7CharsetEditor::setConfig(QVariant config)
{TR;
	QMap<QString, QVariant> cfgmap = config.toMap();

	m_mask_mode = false;

	if (cfgmap.isEmpty())	
	{
		m_charset_file = "";
	}
	else
	{
		m_charset_file = cfgmap["file"].toString();
		if (cfgmap.contains("mask") && cfgmap.contains("mask_encoding"))
		{
			m_mask_mode = true;
			m_mask = cfgmap["mask"].toString();
			m_mask_encoding = cfgmap["mask_encoding"].toUuid();
		}
	}
	m_use_existing = true;
	m_use_input_characters = false;
	m_use_input_dictionary_files = false;
	m_use_input_pot_files = false;
	m_input_characters = "";
	m_input_encoding = UUID_ENCODING_ISO_8859_1;
	m_dictionary_files.clear();
	m_pot_files.clear();

	RefreshContent();
}

QVariant CLC7CharsetEditor::getConfig(void)
{TR;
	QMap<QString, QVariant> config;

	unsigned char charmin, charmax, count, len;
	unsigned char allchars[256];
	if (m_jtrdll.get_charset_info(m_charset_file.toUtf8(), &charmin, &charmax, &len, &count, allchars) != -1)
	{
		config["file"] = m_charset_file;
		if (m_mask_mode)
		{
			config["mask"] = m_mask;
			config["mask_encoding"] = m_mask_encoding;
		}
	}

	return config;
}


void CLC7CharsetEditor::UpdateCharset()
{TR;

	unsigned char charmin, charmax, count, len;
	unsigned char allchars[256];
	if (m_use_existing && m_jtrdll.get_charset_info(m_charset_file.toUtf8(), &charmin, &charmax, &len, &count, allchars) != -1)
	{
		ui.maxPassLenEdit->setText(QString("%1").arg((unsigned int)len));
		ui.numCharsEdit->setText(QString("%1").arg((unsigned int)count));
		ui.maskModeCheckBox->setChecked(m_mask_mode);
		ui.maskEncodingCombo->clear();
		if (m_mask_mode)
		{
			ui.maskEdit->setText(m_mask);

			ILC7PresetGroup *encodings = g_pLinkage->GetPresetManager()->presetGroup(QString("%1:encodings").arg(UUID_LC7JTRPLUGIN.toString()));
			int ppos;
			for (ppos = 0; ppos < encodings->presetCount(); ppos++)
			{
				ILC7Preset *preset = encodings->presetAt(ppos);
				QString name = preset->name();
				//		QMap<QString, QVariant> config = preset->config().toMap();

				ui.maskEncodingCombo->addItem(name, preset->id());
				if (m_mask_mode && preset->id() == m_mask_encoding)
				{
					ui.maskEncodingCombo->setCurrentIndex(ppos);
				}
			}
		}
		else
		{
			ui.maskEdit->setText("");
		}


		m_charset_valid = true;
	}
	else
	{
		ui.maxPassLenEdit->setText("");
		ui.numCharsEdit->setText("");
		ui.maskModeCheckBox->setChecked(false);
		ui.maskEncodingCombo->clear();
		ui.maskEdit->clear();

		m_charset_valid = false;
	}
}


void CLC7CharsetEditor::RefreshContent()
{TR;
	ILC7ColorManager *colman = g_pLinkage->GetGUILinkage()->GetColorManager();

	QSize size(16, 16);
	size *= colman->GetSizeRatio();

	ui.plusButtonDict->setIconSize(size);
	ui.plusButtonDict->setIcon(colman->GetMonoColorIcon(":lc7/plus.png"));

	ui.minusButtonDict->setIconSize(size);
	ui.minusButtonDict->setIcon(colman->GetMonoColorIcon(":lc7/minus.png"));

	ui.plusButtonPot->setIconSize(size);
	ui.plusButtonPot->setIcon(colman->GetMonoColorIcon(":lc7/plus.png"));

	ui.minusButtonPot->setIconSize(size);
	ui.minusButtonPot->setIcon(colman->GetMonoColorIcon(":lc7/minus.png"));

	ui.useExistingRadio->setChecked(m_use_existing);
	ui.createNewRadio->setChecked(!m_use_existing);

	ui.chrFileEdit->setText(m_charset_file);
	
	UpdateCharset();

	ui.useCharsCheckbox->setChecked(m_use_input_characters);
	ui.inputCharactersEdit->setText(m_input_characters);

	
	ui.inputCharactersEncodingCombo->clear();
	ILC7PresetGroup *encodings = g_pLinkage->GetPresetManager()->presetGroup(QString("%1:encodings").arg(UUID_LC7JTRPLUGIN.toString()));
	int ppos;
	for (ppos = 0; ppos < encodings->presetCount(); ppos++)
	{
		ILC7Preset *preset = encodings->presetAt(ppos);
		QString name = preset->name();
//		QMap<QString, QVariant> config = preset->config().toMap();

		ui.inputCharactersEncodingCombo->addItem(name, preset->id());
		if (preset->id() == m_input_encoding)
		{
			ui.inputCharactersEncodingCombo->setCurrentIndex(ppos);
		}
	}

	ui.useDictCheckbox->setChecked(m_use_input_dictionary_files);
	
	foreach(QString dict, m_dictionary_files)
	{
		ui.dictionaryList->addItem(new QListWidgetItem(dict));
	}

	ui.usePotCheckbox->setChecked(m_use_input_pot_files);

	foreach(QString pot, m_pot_files)
	{
		ui.potList->addItem(new QListWidgetItem(pot));
	}

	UpdateUI();
}

void CLC7CharsetEditor::UpdateUI()
{
	bool valid = true;
	QString invalid_why = "";

	if (m_use_existing)
	{
		ui.createNewGroup->setEnabled(false);
		ui.useExistingGroup->setEnabled(true);

		ui.invalidCharsetLabel->setVisible(!m_charset_valid);

		if (!m_charset_valid)
		{
			valid = false;
			invalid_why = "Invalid character set file.";
		}
		else if (m_mask_mode && !m_mask_valid)
		{
			valid = false;
			invalid_why = "Invalid single character mask.";
		}

		ui.maskEdit->setEnabled(m_mask_mode);
		ui.maskEncodingCombo->setEnabled(m_mask_mode);
	}
	else
	{
		ui.createNewGroup->setEnabled(true);
		ui.useExistingGroup->setEnabled(false);

		ui.invalidCharsetLabel->setVisible(false);

		ui.inputCharactersEdit->setEnabled(m_use_input_characters);
		ui.inputCharactersEncodingLabel->setEnabled(m_use_input_characters);
		ui.inputCharactersEncodingCombo->setEnabled(m_use_input_characters);

		ui.dictionaryList->setEnabled(m_use_input_dictionary_files);
		ui.plusButtonDict->setEnabled(m_use_input_dictionary_files);
		ui.minusButtonDict->setEnabled(m_use_input_dictionary_files && ui.dictionaryList->selectedItems().size()>0);

		ui.potList->setEnabled(m_use_input_pot_files);
		ui.plusButtonPot->setEnabled(m_use_input_pot_files);
		ui.minusButtonPot->setEnabled(m_use_input_pot_files && ui.potList->selectedItems().size()>0);

		bool can_create = (m_use_input_characters && m_input_characters.size() > 0) ||
			(m_use_input_dictionary_files && m_dictionary_files.size() > 0) ||
			(m_use_input_pot_files && m_pot_files.size() > 0);

		ui.createButton->setEnabled(can_create);

		valid = false;
		invalid_why = "You must create a CHR file, or use an existing CHR file.";
	}
	
	emit sig_isValid(valid, invalid_why);
}

QByteArray ConvertInputCharacters(QString input, QUuid encoding_id)
{TR;
	ILC7PresetGroup *encodings = g_pLinkage->GetPresetManager()->presetGroup(QString("%1:encodings").arg(UUID_LC7JTRPLUGIN.toString()));
	if (!encodings)
	{
		Q_ASSERT(0);
		return QByteArray();
	}

	ILC7Preset *encoding = encodings->presetById(encoding_id);
	if (!encoding)
	{
		Q_ASSERT(0);
		return QByteArray();
	}

	QMap<QString, QVariant> config = encoding->config().toMap();
	QList<QByteArray> x = QTextCodec::availableCodecs();
	QList<int> y = QTextCodec::availableMibs();
	QTextCodec *codec = QTextCodec::codecForName(config["icuname"].toString().toLatin1());
	QTextCodec *codec2 = QTextCodec::codecForMib(3);
	QByteArray out = codec->fromUnicode(input);
	
	return out;
}

QString CLC7CharsetEditor::ExtractMask(QString chrfile)
{
	unsigned char charmin, charmax, count, len;
	unsigned char allchars[256];
	if (m_jtrdll.get_charset_info(m_charset_file.toUtf8(), &charmin, &charmax, &len, &count, allchars) != -1)
	{
		QString mask;
		for (int i = 0; i < count; i++)
		{
			unsigned char c = allchars[i];
			if (c == 0)
			{
				continue;
			}

			if (isprint(c) && c!='[' && c!=']' && c!='?' && c < 0x80)
			{
				mask += QChar::fromLatin1(c);
			}
			else
			{
				mask += QString("\\x%1").arg(c, 2, 16, QLatin1Char('0'));
			}
		}

		return QString("[%1]").arg(mask);
	}
	return QString();
}


bool CLC7CharsetEditor::DoCreateCharset(QString & error)
{TR;
	CLC7JTRConsole dlg;

	dlg.setWindowTitle("Creating .CHR File");

	QByteArray inchars;
	if (m_use_input_characters)
	{
		inchars = ConvertInputCharacters(m_input_characters, m_input_encoding);
	}
	QStringList dictfiles;
	if (m_use_input_dictionary_files)
	{
		dictfiles = m_dictionary_files;
	}
	QStringList potfiles;
	if (m_use_input_pot_files)
	{
		potfiles = m_pot_files;
	}

	QString appdata = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	QDir charsetsdir(appdata);
	charsetsdir.mkpath("Charsets");
	if (!charsetsdir.cd("Charsets") || !charsetsdir.exists())
	{
		error = "Couldn't locate charsets folder";
		return false;
	}

	QString preset_name = property("preset_name").toString();
	QString chrfilepath;
	if (!g_pLinkage->GetGUILinkage()->SaveFileDialog("Save .CHR File", charsetsdir.absoluteFilePath(QString("%1.chr").arg(preset_name)), "Character Set Files (*.chr)", chrfilepath))
	{
		error = "The operation was cancelled.";
		return false;
	}

	if (dlg.execGenerateCharset(chrfilepath, inchars, dictfiles, potfiles) == QDialog::Rejected)
	{
		error = "The operation was cancelled.";
		return false;
	}

	m_use_existing = true;
	m_charset_file = chrfilepath;
	
	m_mask_mode = true;
	m_mask = ExtractMask(chrfilepath);
	m_mask_encoding = m_input_encoding;

	ValidateMask();

	RefreshContent();

	return true;
}


void CLC7CharsetEditor::slot_useExistingRadio_clicked(bool checked)
{TR;
	m_use_existing = true;
	
	UpdateUI();
}

void CLC7CharsetEditor::slot_chrFileEdit_textChanged(const QString &text)
{TR;
	m_charset_file = QDir::fromNativeSeparators(text);
	
	UpdateCharset();

	UpdateUI();
}

void CLC7CharsetEditor::slot_chrFileBrowse_clicked(bool checked)
{TR;
	QString startpath;
	if (m_charset_file.isEmpty())
	{
		QString appdata = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
		QDir charsetsdir(appdata);
		charsetsdir.mkpath("Charsets");
		if (!charsetsdir.cd("Charsets") || !charsetsdir.exists())
		{
			QDir chrfiles(g_pLinkage->GetPluginsDirectory());
			chrfiles.cd("lc7jtr{9846c8cf-1db1-467e-83c2-c5655aa81936}");
			startpath = chrfiles.absolutePath();
		}
		else
		{
			startpath = charsetsdir.absolutePath();
		}
	}
	else
	{
		QFileInfo fi(m_charset_file);
		startpath = fi.dir().absolutePath();
	}



	QString filepath;
	if (g_pLinkage->GetGUILinkage()->OpenFileDialog("Choose Character Set File", startpath, "Character Sets (*.chr)", filepath))
	{
		m_charset_file = filepath;

		UpdateCharset();

		ui.chrFileEdit->setText(QDir::toNativeSeparators(filepath));
	}

	UpdateUI();
}

void CLC7CharsetEditor::slot_createNewRadio_clicked(bool checked)
{TR;
	m_use_existing = false;
	UpdateUI();
}

void CLC7CharsetEditor::slot_useCharsCheckbox_clicked(bool checked)
{TR;
	m_use_input_characters = checked;
	UpdateUI();
}

void CLC7CharsetEditor::slot_inputCharactersEdit_textChanged(const QString &text)
{TR;
	m_input_characters = text;
	UpdateUI();
}

void CLC7CharsetEditor::slot_inputCharactersEncodingCombo_currentIndexChanged(int idx)
{TR;
	QUuid encoding_id = ui.inputCharactersEncodingCombo->itemData(idx).toUuid();
	
	m_input_encoding = encoding_id;

	UpdateUI();
}

void CLC7CharsetEditor::slot_useDictCheckbox_clicked(bool checked)
{TR;
	m_use_input_dictionary_files = checked;
	UpdateUI();
}

void CLC7CharsetEditor::slot_dictionaryList_itemSelectionChanged(void)
{TR;
	UpdateUI();
}

void CLC7CharsetEditor::slot_plusButtonDict_clicked(bool checked)
{TR;
	QDir wordlists(g_pLinkage->GetStartupDirectory());
	wordlists.cd("wordlists");

	QString filepath;
	if (g_pLinkage->GetGUILinkage()->OpenFileDialog("Choose Dictionary File", wordlists.absolutePath(), "Wordlists (*.txt *.dic)", filepath))
	{
		m_dictionary_files.append(filepath);
		ui.dictionaryList->addItem(filepath);
	}

	UpdateUI();
}

void CLC7CharsetEditor::slot_minusButtonDict_clicked(bool checked)
{TR;
	QList<QListWidgetItem *> items = ui.dictionaryList->selectedItems();
	foreach(QListWidgetItem *item, items)
	{
		m_dictionary_files.removeOne(item->text());
		delete item;
	}
	UpdateUI();
}

void CLC7CharsetEditor::slot_usePotCheckbox_clicked(bool checked)
{TR;
	m_use_input_pot_files = checked;
	UpdateUI();
}

void CLC7CharsetEditor::slot_potList_itemSelectionChanged(void)
{TR;
	UpdateUI();
}

void CLC7CharsetEditor::slot_plusButtonPot_clicked(bool checked)
{TR;
	QDir wordlists(g_pLinkage->GetStartupDirectory());
	wordlists.cd("wordlists");

	QString filepath;
	if (g_pLinkage->GetGUILinkage()->OpenFileDialog("Choose .pot File", wordlists.absolutePath(), "JtR Pot Files (*.pot)", filepath))
	{
		m_dictionary_files.append(filepath);
		ui.dictionaryList->addItem(filepath);
	}

	UpdateUI();
}

void CLC7CharsetEditor::slot_minusButtonPot_clicked(bool checked)
{TR;
	QList<QListWidgetItem *> items = ui.potList->selectedItems();
	foreach(QListWidgetItem *item, items)
	{
		m_pot_files.removeOne(item->text());
		delete item;
	}
	UpdateUI();
}

void CLC7CharsetEditor::slot_createButton_clicked(bool checked)
{TR;
	QString error;
	if (!DoCreateCharset(error))
	{
		//g_pLinkage->GetGUILinkage()->ErrorMessage("Failed", QString("Charset creation failed: %1").arg(error));
		return;
	}

	RefreshContent();
}

void CLC7CharsetEditor::slot_maskModeCheckBox_clicked(bool checked)
{
	m_mask_mode = ui.maskModeCheckBox->isChecked();
	UpdateUI();
}

void CLC7CharsetEditor::slot_maskEdit_textChanged(const QString & text)
{
	m_mask = text;

	ValidateMask();

	UpdateUI();
}

void CLC7CharsetEditor::slot_maskEncodingCombo_currentIndexChanged(int idx)
{
	QUuid encoding_id = ui.maskEncodingCombo->itemData(idx).toUuid();

	m_mask_encoding = encoding_id;

	ValidateMask();

	UpdateUI();
}

void CLC7CharsetEditor::ValidateMask(void)
{
	if (!m_mask_mode)
	{
		m_mask_valid = false;
		return;
	}

	// xxx mask parser goes here some day
	m_mask_valid = false;
	if (m_mask.size() == 0)
	{
		return;
	}

	m_mask_valid = true;
}



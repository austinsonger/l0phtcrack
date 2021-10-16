#include "stdafx.h"
#include "techniqueDictionaryconfig.h"


TechniqueDictionaryConfig::TechniqueDictionaryConfig()
{TR;
	ui.setupUi(this);

	connect(ui.durationHoursText, &QLineEdit::textChanged, this, &TechniqueDictionaryConfig::slot_durationHoursText_textChanged);
	connect(ui.durationMinutesText, &QLineEdit::textChanged, this, &TechniqueDictionaryConfig::slot_durationMinutesText_textChanged);
	connect(ui.unlimitedCheckBox, &QAbstractButton::clicked, this, &TechniqueDictionaryConfig::slot_unlimitedCheckBox_clicked);
	connect(ui.browseWordlistButton, &QAbstractButton::clicked, this, &TechniqueDictionaryConfig::slot_browseWordlistButton_clicked);
	connect(ui.encodingCombo, (void (QComboBox::*)(int)) &QComboBox::currentIndexChanged, this, &TechniqueDictionaryConfig::slot_encodingCombo_currentIndexChanged);
	connect(ui.permutationRulesCombo, (void (QComboBox::*)(int)) &QComboBox::currentIndexChanged, this, &TechniqueDictionaryConfig::slot_permutationRulesCombo_currentIndexChanged);
	connect(ui.enableCommonLetterSubstitutions, &QAbstractButton::clicked, this, &TechniqueDictionaryConfig::slot_enableCommonLetterSubstitutions_clicked);
	connect(ui.wordListEdit, &QLineEdit::textChanged, this, &TechniqueDictionaryConfig::slot_wordListEdit_textChanged);
	
	ui.durationHoursText->setValidator(new QIntValidator(0, 99999, this));
	ui.durationMinutesText->setValidator(new QIntValidator(0, 59, this));
	
	m_refreshing = false;

	RefreshContent();
}

TechniqueDictionaryConfig::~TechniqueDictionaryConfig()
{TR;
}


void TechniqueDictionaryConfig::setConfig(QVariant config)
{TR;
	m_config = config.toMap();

	if (m_config.isEmpty())
	{
		QDir wordlists(g_pLinkage->GetStartupDirectory());
		wordlists.cd("wordlists");
		QString big = wordlists.absoluteFilePath("wordlist-big.txt");

		m_config["encoding"] = UUID_ENCODING_ISO_8859_1;
		m_config["rule"] = UUID_RULE_WORDLIST;
		m_config["wordlist"] = big;
		m_config["duration_unlimited"] = true;
		m_config["duration_hours"] = 1;
		m_config["duration_minutes"] = 0;
	}

	RefreshContent();
}

QVariant TechniqueDictionaryConfig::getConfig(void)
{TR;
	return m_config;
}


void TechniqueDictionaryConfig::slot_wordListEdit_textChanged(const QString & text)
{TR;
	m_config["wordlist"] = QDir::fromNativeSeparators(text);
	UpdateUI();
}

void TechniqueDictionaryConfig::slot_browseWordlistButton_clicked(bool checked)
{TR;
	QString startpath;
	if (m_config["wordlist"].toString().isEmpty())
	{
		QDir wordlists(g_pLinkage->GetPluginsDirectory());
		wordlists.cd("wordlists");
		startpath = wordlists.absolutePath();
	}
	else
	{
		QFileInfo fi(m_config["wordlist"].toString());
		startpath = fi.dir().absolutePath();
	}

	QString filepath;
	if (g_pLinkage->GetGUILinkage()->OpenFileDialog("Choose Wordlist File", startpath, "Wordlists (*.txt *.dic *.lst); All Files (*)", filepath))
	{
		m_config["wordlist"] = filepath;

		ui.wordListEdit->setText(QDir::toNativeSeparators(filepath));
	}

	UpdateUI();
}



void TechniqueDictionaryConfig::slot_encodingCombo_currentIndexChanged(int index)
{TR;
	if (m_refreshing)
	{
		return;
	}

	QUuid encoding = ui.encodingCombo->itemData(index).toUuid();
	m_config["encoding"] = encoding;
	UpdateUI();
}

void TechniqueDictionaryConfig::slot_permutationRulesCombo_currentIndexChanged(int index)
{TR;
	if (m_refreshing)
	{
		return;
	}

	QUuid rule = ui.permutationRulesCombo->itemData(index).toUuid();
	m_config["rule"] = rule;
	UpdateUI();
}

void TechniqueDictionaryConfig::slot_durationHoursText_textChanged(const QString & text)
{TR;
	m_config["duration_hours"] = text.toInt();
	UpdateUI();
}

void TechniqueDictionaryConfig::slot_durationMinutesText_textChanged(const QString & text)
{TR;
	m_config["duration_minutes"] = text.toInt();
	UpdateUI();
}

void TechniqueDictionaryConfig::slot_unlimitedCheckBox_clicked(bool checked)
{TR;
	m_config["duration_unlimited"] = checked;
	UpdateUI();
}

void TechniqueDictionaryConfig::slot_enableCommonLetterSubstitutions_clicked(bool checked)
{
	TR;
	m_config["leet"] = checked;
	UpdateUI();
}

void TechniqueDictionaryConfig::RefreshContent()
{TR;

	m_refreshing = true;
	
	ILC7PasswordLinkage *passlink = GET_ILC7PASSWORDLINKAGE(g_pLinkage);
	ILC7ColorManager *colman = g_pLinkage->GetGUILinkage()->GetColorManager();

	ui.wordListEdit->setText(QDir::toNativeSeparators(QString("%1").arg(m_config["wordlist"].toString())));

	ui.unlimitedCheckBox->setChecked(m_config["duration_unlimited"].toBool());
	ui.durationHoursText->setText(QString("%1").arg(m_config["duration_hours"].toInt()));
	ui.durationMinutesText->setText(QString("%1").arg(m_config["duration_minutes"].toInt()));

	ILC7PresetManager *manager = g_pLinkage->GetPresetManager();
	ILC7PresetGroup *encodings = manager->presetGroup(QString("%1:encodings").arg(UUID_LC7JTRPLUGIN.toString()));
	if (encodings)
	{
		ui.encodingCombo->clear();

		QUuid defset = m_config["encoding"].toUuid();

		for (int i = 0; i < encodings->presetCount(); i++)
		{
			ILC7Preset *encoding = encodings->presetAt(i);
			ui.encodingCombo->addItem(encoding->name(), QVariant(encoding->id()));

			if (encoding->id() == defset)
			{
				ui.encodingCombo->setCurrentIndex(i);
			}
		}
	}

	ILC7PresetGroup *rules = manager->presetGroup(QString("%1:rules").arg(UUID_LC7JTRPLUGIN.toString()));
	if (rules)
	{
		ui.permutationRulesCombo->clear();

		QUuid defset = m_config["rule"].toUuid();

		for (int i = 0; i < rules->presetCount(); i++)
		{
			ILC7Preset *rule = rules->presetAt(i);
			ui.permutationRulesCombo->addItem(rule->name(), QVariant(rule->id()));

			if (rule->id() == defset)
			{
				ui.permutationRulesCombo->setCurrentIndex(i);
			}
		}
	}

	ui.enableCommonLetterSubstitutions->setChecked(m_config["leet"].toBool());

	UpdateUI();

	m_refreshing = false;
}

void TechniqueDictionaryConfig::UpdateUI()
{TR;
	ui.durationHoursText->setEnabled(!m_config["duration_unlimited"].toBool());
	ui.durationMinutesText->setEnabled(!m_config["duration_unlimited"].toBool());

	bool is_valid = true;
	QString why;
	if (m_config["encoding"].toUuid().isNull())
	{
		is_valid = false;
		why += "Encoding is not selected.\n";
	}
	if (m_config["rule"].toUuid().isNull())
	{
		is_valid = false;
		why += "Rule is not selected.\n";
	}

	if (m_config["wordlist"].toString().isEmpty() || !QFileInfo(m_config["wordlist"].toString()).exists())
	{
		is_valid = false;
		why += "Wordlist is not selected.\n";
	}
	if (!m_config["duration_unlimited"].toBool() &&
		((m_config["duration_hours"].toInt() == 0 && m_config["duration_minutes"].toInt() == 0) || m_config["duration_hours"].toInt() < 0 || m_config["duration_minutes"].toInt() < 0))
	{
		is_valid = false;
		why += "Invalid duration.\n";
	}


	emit sig_isValid(is_valid, why);
}

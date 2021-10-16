#include "stdafx.h"
#include "techniquebruteconfig.h"



TechniqueBruteConfig::TechniqueBruteConfig()
{TR;
	ui.setupUi(this);
	
	m_refreshing = false;
	m_use_advanced = false;

	connect(ui.minCharsEdit, &QLineEdit::textChanged, this, &TechniqueBruteConfig::slot_minCharsEdit_textChanged);
	connect(ui.maxCharsEdit, &QLineEdit::textChanged, this, &TechniqueBruteConfig::slot_maxCharsEdit_textChanged);
	connect(ui.minCheckBox, &QAbstractButton::clicked, this, &TechniqueBruteConfig::slot_minCheckBox_clicked);
	connect(ui.maxCheckBox, &QAbstractButton::clicked, this, &TechniqueBruteConfig::slot_maxCheckBox_clicked);
	connect(ui.defaultCharSetCombo, (void (QComboBox::*)(int))&QComboBox::currentIndexChanged, this, &TechniqueBruteConfig::slot_defaultCharSetCombo_currentIndexChanged);
	connect(ui.durationHoursText, &QLineEdit::textChanged, this, &TechniqueBruteConfig::slot_durationHoursText_textChanged);
	connect(ui.durationMinutesText, &QLineEdit::textChanged, this, &TechniqueBruteConfig::slot_durationMinutesText_textChanged);
	connect(ui.unlimitedCheckBox, &QAbstractButton::clicked, this, &TechniqueBruteConfig::slot_unlimitedCheckBox_clicked);
	connect(ui.editCharSetsButton, &QAbstractButton::clicked, this, &TechniqueBruteConfig::slot_editCharSetsButton_clicked);
	connect(ui.specificCharSetsTable, &QTableWidget::cellChanged, this, &TechniqueBruteConfig::slot_specificCharSetsTable_cellChanged);
	connect(ui.enableAdvancedCharSetsCheckBox, &QAbstractButton::clicked, this, &TechniqueBruteConfig::slot_enableAdvancedCharSetsCheckBox_clicked);

	ui.minCharsEdit->setValidator(new QIntValidator(0, 24, this));
	ui.maxCharsEdit->setValidator(new QIntValidator(0, 24, this));
	ui.durationHoursText->setValidator(new QIntValidator(0, 99999, this));
	ui.durationMinutesText->setValidator(new QIntValidator(0, 59, this));
	ui.specificCharSetsTable->setItemDelegate(new SpecificCharsetTableDelegate(ui.specificCharSetsTable));
	
	ui.specificCharSetsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

	m_LM_Map[UUID_CHARSET_UTF8] = UUID_CHARSET_LM_ASCII;
	m_LM_Map[UUID_CHARSET_LATIN1] = UUID_CHARSET_LANMAN;
	m_LM_Map[UUID_CHARSET_ASCII] = UUID_CHARSET_LM_ASCII;
	m_LM_Map[UUID_CHARSET_ALNUMSPACE] = UUID_CHARSET_UPPERNUM;
	m_LM_Map[UUID_CHARSET_ALNUM] = UUID_CHARSET_UPPERNUM;
	m_LM_Map[UUID_CHARSET_ALPHA] = UUID_CHARSET_UPPER;
	m_LM_Map[UUID_CHARSET_LOWERNUM] = UUID_CHARSET_UPPERNUM;
	m_LM_Map[UUID_CHARSET_LOWERSPACE] = UUID_CHARSET_UPPER;
	m_LM_Map[UUID_CHARSET_LOWER] = UUID_CHARSET_UPPER;

	RefreshContent();
}

TechniqueBruteConfig::~TechniqueBruteConfig()
{TR;
}


void TechniqueBruteConfig::RefreshContent()
{TR;
	m_refreshing = true;

	ILC7PasswordLinkage *passlink = GET_ILC7PASSWORDLINKAGE(g_pLinkage);
	ILC7ColorManager *colman = g_pLinkage->GetGUILinkage()->GetColorManager();

	ui.enableAdvancedCharSetsCheckBox->setChecked(m_use_advanced);
	
	ui.minCheckBox->setChecked(m_config["enable_min"].toBool());
	ui.maxCheckBox->setChecked(m_config["enable_max"].toBool());
	ui.minCharsEdit->setText(QString("%1").arg(m_config["num_chars_min"].toInt()));
	ui.maxCharsEdit->setText(QString("%1").arg(m_config["num_chars_max"].toInt()));
	ui.unlimitedCheckBox->setChecked(m_config["duration_unlimited"].toBool());
	ui.durationHoursText->setText(QString("%1").arg(m_config["duration_hours"].toInt()));
	ui.durationMinutesText->setText(QString("%1").arg(m_config["duration_minutes"].toInt()));
	
	ILC7PresetManager *manager = g_pLinkage->GetPresetManager();
	ILC7PresetGroup *charsets = manager->presetGroup(QString("%1:charsets").arg(UUID_LC7JTRPLUGIN.toString()));
	if (charsets)
	{
		ui.defaultCharSetCombo->clear();

		QUuid defset = m_config["default_charset"].toUuid();
		if (charsets->presetById(defset) == NULL)
		{
			defset = QUuid();
		}

		for (int i = 0; i < charsets->presetCount(); i++)
		{
			ILC7Preset *charset = charsets->presetAt(i);
			ui.defaultCharSetCombo->addItem(charset->name(), QVariant(charset->id()));
			
			if (charset->id() == defset)
			{
				ui.defaultCharSetCombo->setCurrentIndex(i);
			}
		}
	}

	QList<QVariant> csetmap_keys = m_config["csetmap_keys"].toList();
	QList<QVariant> csetmap_values = m_config["csetmap_values"].toList();

	int count = csetmap_keys.size();
	ui.specificCharSetsTable->clearContents();
	ui.specificCharSetsTable->setRowCount(0);
	int row = 0;
	for (int i = 0; i < count; i++)
	{
		fourcc fcc = csetmap_keys[i].toUInt();
		QUuid charset_id = csetmap_values[i].toUuid();

		LC7HashType accttype;
		QString error;
		if (!passlink->LookupHashType(fcc, accttype, error))
		{
			csetmap_keys.removeAt(i);
			csetmap_values.removeAt(i);
			count--;
			continue;
		}

		ILC7Preset *charset = charsets->presetById(charset_id);
		if (!charset)
		{
			csetmap_keys.removeAt(i);
			csetmap_values.removeAt(i);
			count--;
			continue;
		}

		ui.specificCharSetsTable->setRowCount(ui.specificCharSetsTable->rowCount() + 1);
		
		QToolButton *delbutton = new QToolButton(ui.specificCharSetsTable);
		delbutton->setIcon(colman->GetMonoColorIcon(":/lc7/minus.png"));
		delbutton->setIconSize(QSize(16,16) * colman->GetSizeRatio());
		delbutton->setProperty("row", row);
		ui.specificCharSetsTable->setCellWidget(i,0,delbutton);
		connect(delbutton, &QToolButton::clicked, this, &TechniqueBruteConfig::slot_removeRowButton_clicked);

		ui.specificCharSetsTable->setItem(i, 1, new QTableWidgetItem(accttype.name));
		ui.specificCharSetsTable->item(i, 1)->setData(Qt::UserRole, fcc);		
		ui.specificCharSetsTable->setItem(i, 2, new QTableWidgetItem(charset->name()));
		ui.specificCharSetsTable->item(i, 2)->setData(Qt::UserRole, charset_id);

		row++;
	}

	// Put back modified csetmap because we could have removed items with invalid settings
	m_config["csetmap_keys"] = csetmap_keys;
	m_config["csetmap_values"] = csetmap_values;

	// Add editor row
	int rowcount = ui.specificCharSetsTable->rowCount();
	if (rowcount < passlink->ListHashTypes().size())
	{
		ui.specificCharSetsTable->setRowCount(rowcount + 1);

		QToolButton *addbutton = new QToolButton(ui.specificCharSetsTable);
		addbutton->setIcon(colman->GetMonoColorIcon(":/lc7/plus.png"));
		addbutton->setIconSize(QSize(16, 16) * colman->GetSizeRatio());
		ui.specificCharSetsTable->setCellWidget(rowcount, 0, addbutton);
		connect(addbutton, &QToolButton::clicked, this, &TechniqueBruteConfig::slot_addRowButton_clicked);
	}

	m_refreshing = false;

	UpdateUI();
}

void TechniqueBruteConfig::UpdateUI()
{TR;
	ui.minCharsEdit->setEnabled(m_config["enable_min"].toBool());
	ui.maxCharsEdit->setEnabled(m_config["enable_max"].toBool());
	ui.durationHoursText->setEnabled(!m_config["duration_unlimited"].toBool());
	ui.durationMinutesText->setEnabled(!m_config["duration_unlimited"].toBool());

	bool is_valid = true;
	QString why;
	if (m_config["default_charset"].toUuid().isNull())
	{
		is_valid = false;
		why += "Default character set is not selected.\n";
	}
	if (!m_config["duration_unlimited"].toBool() && 
		((m_config["duration_hours"].toInt() == 0 && m_config["duration_minutes"].toInt() == 0) || m_config["duration_hours"].toInt()<0 || m_config["duration_minutes"].toInt()<0))
	{
		is_valid = false;
		why += "Invalid duration.\n";
	}
	if (m_config["enable_min"].toBool() && m_config["enable_max"].toBool() && m_config["num_chars_min"].toInt() > m_config["num_chars_max"].toInt())
	{
		is_valid = false;
		why += "Invalid character length range.";
	}

	QList<QVariant> csetmap_keys = m_config["csetmap_keys"].toList();
	QList<QVariant> csetmap_values = m_config["csetmap_values"].toList();

	QSet<fourcc> keys;
	foreach(QVariant key, csetmap_keys)
	{
		fourcc fcc = key.toUInt();
		if (keys.contains(fcc))
		{
			is_valid = false;
			why += QString("Can't specify more than one character set for the same hash type.");
		}
		keys.insert(fcc);
	}

	ui.hashtypeSpecificLabel->setVisible(m_use_advanced);
	ui.specificCharSetsTable->setVisible(m_use_advanced);
	ui.editCharSetsButton->setVisible(m_use_advanced);
	
	emit sig_isValid(is_valid, why);
}

void TechniqueBruteConfig::SetUseAdvancedFlag()
{TR;
	bool adv = false;
	QUuid defcharset = m_config["default_charset"].toUuid();
	QList<QVariant> csetmap_keys = m_config["csetmap_keys"].toList();
	QList<QVariant> csetmap_values = m_config["csetmap_values"].toList();

	if (m_LM_Map.contains(defcharset))
	{
		QUuid mappedcharset = m_LM_Map[defcharset];

		if (csetmap_keys.size() != 1 || csetmap_keys[0].toUInt() != FOURCC(HASHTYPE_LM) || csetmap_values[0].toUuid() != mappedcharset)
		{
			adv = true;
		}
	}
	else
	{
		if (csetmap_keys.size() != 0)
		{
			adv = true;
		}
	}

	m_use_advanced = adv;
}

void TechniqueBruteConfig::setConfig(QVariant _config)
{TR;
	m_config = _config.toMap();
	
	if (m_config.isEmpty())
	{
		QMap<QString, QVariant> charsetmap;
		QList<QVariant> csetmap_keys;
		QList<QVariant> csetmap_values;

		csetmap_keys << FOURCC(HASHTYPE_LM);
		csetmap_values << UUID_CHARSET_LM_ASCII;

		m_config["enable_min"] = false;
		m_config["num_chars_min"] = 0;
		m_config["enable_max"] = false;
		m_config["num_chars_max"] = 8;
		m_config["default_charset"] = UUID_CHARSET_ASCII;
		m_config["csetmap_keys"] = csetmap_keys;
		m_config["csetmap_values"] = csetmap_values;
		m_config["duration_unlimited"] = true;
		m_config["duration_hours"] = 1;
		m_config["duration_minutes"] = 0;
	}

	SetUseAdvancedFlag();

	RefreshContent();
}

QVariant TechniqueBruteConfig::getConfig(void)
{TR;
	if (!m_use_advanced)
	{
		QUuid defcharset = m_config["default_charset"].toUuid();
		if (m_LM_Map.contains(defcharset))
		{
			QUuid mappedcharset = m_LM_Map[defcharset];
		
			QList<QVariant> csetmap_keys;
			QList<QVariant> csetmap_values;

			csetmap_keys << FOURCC(HASHTYPE_LM);
			csetmap_values << mappedcharset;

			m_config["csetmap_keys"] = csetmap_keys;
			m_config["csetmap_values"] = csetmap_values;
		}
		else
		{
			QList<QVariant> csetmap_keys;
			QList<QVariant> csetmap_values;

			m_config["csetmap_keys"] = csetmap_keys;
			m_config["csetmap_values"] = csetmap_values;
		}		
	}

	return m_config;
}


void TechniqueBruteConfig::slot_minCharsEdit_textChanged(const QString & text)
{TR;
	m_config["num_chars_min"] = text.toInt();
	UpdateUI();
}

void TechniqueBruteConfig::slot_maxCharsEdit_textChanged(const QString & text)
{TR;
	m_config["num_chars_max"] = text.toInt();
	UpdateUI();
}

void TechniqueBruteConfig::slot_minCheckBox_clicked(bool checked)
{TR;
	m_config["enable_min"] = checked;
	UpdateUI();
}

void TechniqueBruteConfig::slot_maxCheckBox_clicked(bool checked)
{TR;
	m_config["enable_max"] = checked;
	UpdateUI();
}

void TechniqueBruteConfig::slot_defaultCharSetCombo_currentIndexChanged(int index)
{TR;
	if (m_refreshing)
	{
		return;
	}
	QUuid defcharset = ui.defaultCharSetCombo->itemData(index).toUuid();
	m_config["default_charset"] = defcharset;
	UpdateUI();
}

void TechniqueBruteConfig::slot_durationHoursText_textChanged(const QString & text)
{TR;
	m_config["duration_hours"] = text.toInt();
	UpdateUI();
}

void TechniqueBruteConfig::slot_durationMinutesText_textChanged(const QString & text)
{TR;
	m_config["duration_minutes"] = text.toInt();
	UpdateUI();
}

void TechniqueBruteConfig::slot_unlimitedCheckBox_clicked(bool checked)
{TR;
	m_config["duration_unlimited"] = checked;
	UpdateUI();
}

void TechniqueBruteConfig::slot_enableAdvancedCharSetsCheckBox_clicked(bool checked)
{TR;
	m_use_advanced = ui.enableAdvancedCharSetsCheckBox->isChecked();
	UpdateUI();
}


void TechniqueBruteConfig::slot_specificCharSetsTable_cellChanged(int row, int column)
{TR;
	if (column == 1)
	{
		fourcc fcc = ui.specificCharSetsTable->item(row, column)->data(Qt::UserRole).toUInt();
		QList<QVariant> csetmap_keys = m_config["csetmap_keys"].toList();
		csetmap_keys[row] = fcc;
		m_config["csetmap_keys"] = csetmap_keys;
	}
	else if (column == 2)
	{
		QUuid id = ui.specificCharSetsTable->item(row, column)->data(Qt::UserRole).toUuid();
		QList<QVariant> csetmap_values = m_config["csetmap_values"].toList();
		csetmap_values[row] = id;
		m_config["csetmap_values"] = csetmap_values;
	}
	UpdateUI();
}

void TechniqueBruteConfig::slot_editCharSetsButton_clicked(bool checked)
{TR;
	CLC7CharsetEditorDlg dlg;
	int res = dlg.exec();
	
	//if (res == QDialog::Rejected)
	//{
//		return;
	//}

	RefreshContent();
}

void TechniqueBruteConfig::slot_addRowButton_clicked(bool checked)
{TR;
	ILC7PasswordLinkage *passlink = GET_ILC7PASSWORDLINKAGE(g_pLinkage);
	ILC7PresetManager *manager = g_pLinkage->GetPresetManager();
	ILC7PresetGroup *charsets = manager->presetGroup(QString("%1:charsets").arg(UUID_LC7JTRPLUGIN.toString()));

	QToolButton *button = (QToolButton *)sender();
	int row = button->property("row").toInt();

	QList<QVariant> csetmap_keys = m_config["csetmap_keys"].toList();
	QList<QVariant> csetmap_values = m_config["csetmap_values"].toList();


	// Collect hash types for all other rows
	QSet<fourcc> otherhashes;
	foreach(QVariant key, csetmap_keys)
	{
		otherhashes.insert(key.toUInt());
	}

	bool found = false;
	fourcc fcc;
	foreach(fcc, passlink->ListHashTypes())
	{
		if (!otherhashes.contains(fcc))
		{
			found = true;
			break;
		}
	}

	if (!found)
	{
		Q_ASSERT(0);
		return;
	}
	
	csetmap_keys.append(fcc);
	csetmap_values.append(charsets->presetAt(0)->id());

	m_config["csetmap_keys"] = csetmap_keys;
	m_config["csetmap_values"] = csetmap_values;

	RefreshContent();
}

void TechniqueBruteConfig::slot_removeRowButton_clicked(bool checked)
{TR;
	QToolButton *button = (QToolButton *)sender();
	int row = button->property("row").toInt();

	QList<QVariant> csetmap_keys = m_config["csetmap_keys"].toList();
	QList<QVariant> csetmap_values = m_config["csetmap_values"].toList();

	csetmap_keys.removeAt(row);
	csetmap_values.removeAt(row);

	m_config["csetmap_keys"] = csetmap_keys;
	m_config["csetmap_values"] = csetmap_values;

//	ui.specificCharSetsTable->removeRow(row);

	RefreshContent();
}


//////////////////////////////////



SpecificCharsetTableDelegate::SpecificCharsetTableDelegate(QTableWidget *parent) : QStyledItemDelegate(parent)
{
	m_table = parent;

}

QWidget* SpecificCharsetTableDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{TR;
	QComboBox *editor = NULL;

	if (index.data(Qt::UserRole).isNull())
	{
		return NULL;
	}

	if (index.column() == 1)
	{
		editor = new QComboBox(parent);
	}
	else if (index.column() == 2)
	{		
		editor = new QComboBox(parent);
	}

	return editor;
}
void SpecificCharsetTableDelegate::setEditorData(QWidget *widget, const QModelIndex &index) const
{TR;
	if (index.column() == 1)
	{
		// Collect hash types for all other rows
		QSet<fourcc> otherhashes;
		int row = index.row();
		for (int r = 0; r < m_table->rowCount(); r++)
		{
			if (r == row)
				continue;
			QTableWidgetItem *item = m_table->item(r, index.column());
			if (!item)
			{
				continue;
			}
			fourcc fcc = item->data(Qt::UserRole).toUInt();
			otherhashes.insert(fcc);
		}
				
		QComboBox *editor = (QComboBox *)widget;

		fourcc current_fcc = index.data(Qt::UserRole).toUInt();

		ILC7PasswordLinkage *passlink = GET_ILC7PASSWORDLINKAGE(g_pLinkage);

		QList<fourcc> hashtypes = passlink->ListHashTypes();
		int idx = 0;
		foreach(fourcc fcc, hashtypes)
		{
			if (otherhashes.contains(fcc))
			{
				continue;
			}

			LC7HashType accttype;
			QString error;
			if (!passlink->LookupHashType(fcc, accttype, error))
			{
				Q_ASSERT(0);
				continue;
			}

			editor->addItem(accttype.name, fcc);
			if (fcc == current_fcc)
			{
				editor->setCurrentIndex(idx);
			}
			idx++;
		}

		editor->showPopup();
	}
	else if (index.column() == 2)
	{
		QComboBox *editor = (QComboBox *)widget;
		QUuid current_cset = index.data(Qt::UserRole).toUuid();

		ILC7PresetManager *manager = g_pLinkage->GetPresetManager();
		ILC7PresetGroup *charsets = manager->presetGroup(QString("%1:charsets").arg(UUID_LC7JTRPLUGIN.toString()));
		if (charsets)
		{
			for (int i = 0; i < charsets->presetCount(); i++)
			{
				ILC7Preset *charset = charsets->presetAt(i);
				editor->addItem(charset->name(), QVariant(charset->id()));

				if (charset->id() == current_cset)
				{
					editor->setCurrentIndex(i);
				}
			}
		}

		editor->showPopup();
	}
}
void SpecificCharsetTableDelegate::setModelData(QWidget* widget, QAbstractItemModel* model, const QModelIndex &index) const
{TR;
	if (index.column() == 1)
	{
		QComboBox *editor = (QComboBox *)widget;
		
		fourcc fcc = editor->currentData().toUInt();
		
		model->setData(index, editor->currentText(), Qt::EditRole);
		model->setData(index, fcc, Qt::UserRole);		
	}
	else if (index.column() == 2)
	{
		QComboBox *editor = (QComboBox *)widget;
		QUuid value = editor->currentData().toUuid();

		model->setData(index, editor->currentText(), Qt::EditRole);
		model->setData(index, value, Qt::UserRole);
	}
}
void SpecificCharsetTableDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{TR;
	editor->setGeometry(option.rect);
}
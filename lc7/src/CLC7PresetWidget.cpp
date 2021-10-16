#include "stdafx.h"


///////////////////////////////////////////////////////////////////////////////////////////////////////


CLC7PresetButtons::CLC7PresetButtons(QWidget *parent) : QWidget(parent)
{
	TR;
	ui.setupUi(this);

	m_colman = ((CLC7App *)(CLC7App::instance()))->GetMainWindow()->GetColorManager();
	m_colman->RegisterRecolorCallback(this, (void (QObject::*)())&CLC7PresetButtons::slot_recolorCallback);
	slot_recolorCallback();
}

CLC7PresetButtons::~CLC7PresetButtons()
{
	TR;
}

void CLC7PresetButtons::slot_recolorCallback(void)
{
	TR;

	QSize size(16, 16);
	size *= m_colman->GetSizeRatio();

	ui.deletePresetButton->setIcon(m_colman->GetMonoColorIcon(":lc7/minus.png", m_colman->GetTextColor(), m_colman->GetInverseTextColor()));
	ui.deletePresetButton->setIconSize(size);

	ui.editPresetButton->setIcon(m_colman->GetMonoColorIcon(":lc7/edit.png", m_colman->GetTextColor(), m_colman->GetInverseTextColor()));
	ui.editPresetButton->setIconSize(size);

	ui.copyPresetButton->setIcon(m_colman->GetMonoColorIcon(":lc7/copy.png", m_colman->GetTextColor(), m_colman->GetInverseTextColor()));
	ui.copyPresetButton->setIconSize(size);

	UpdateUI();
}

void CLC7PresetButtons::UpdateUI()
{

}

///////////////////////////////////////////////////////////////////////////////////////////////////////


CLC7PresetWidget::CLC7PresetWidget(ILC7PresetManager *manager, QWidget *page, QWidget *configwidget, QString preset_group)
{TR;
	ui.setupUi(this);
	
	m_manager = manager;
	m_configwidget = configwidget;
	m_presetgroup_name = preset_group;
	m_no_update_ui = false;

	m_editing = false;
	m_current_preset = nullptr;
	m_config_valid = false;
	m_last_entered_item = nullptr;

	connect(this, SIGNAL(sig_isValid(bool)), page, SLOT(slot_isValid(bool)));
	connect(configwidget, SIGNAL(sig_isValid(bool, QString)), this, SLOT(slot_isValid(bool, QString)));
	
	ui.presetGroupBox->layout()->removeWidget(ui.widget);
	ui.widget = configwidget;
	ui.presetGroupBox->layout()->addWidget(ui.widget);

	m_group = m_manager->presetGroup(m_presetgroup_name);
	if (!m_group)
	{
		m_group = m_manager->newPresetGroup(m_presetgroup_name);
	}
	
	m_colman = ((CLC7App *)(CLC7App::instance()))->GetMainWindow()->GetColorManager();
	m_colman->RegisterRecolorCallback(this, (void (QObject::*)())&CLC7PresetWidget::slot_recolorCallback);
	slot_recolorCallback();

	connect(ui.presetList, &QListWidget::customContextMenuRequested, this, &CLC7PresetWidget::slot_showContextMenu);
	connect(ui.presetList, &CLC7ModifiedListWidget::sig_dropped, this, &CLC7PresetWidget::slot_dropped_presetList);
	connect(ui.presetList, &QListWidget::currentItemChanged, this, &CLC7PresetWidget::slot_currentItemChanged_presetList);
	connect(ui.presetList, &QListWidget::itemEntered, this, &CLC7PresetWidget::slot_itemEntered_presetList);
	connect(ui.presetList, &QListWidget::viewportEntered, this, &CLC7PresetWidget::slot_viewportEntered_presetList);
	connect(ui.presetList, &CLC7ModifiedListWidget::sig_leave, this, &CLC7PresetWidget::slot_leave_presetList);
	connect(ui.presetList, &QListWidget::doubleClicked, this, &CLC7PresetWidget::slot_doubleClicked_presetList);
	connect(ui.presetList, &CLC7ModifiedListWidget::sig_copy, this, &CLC7PresetWidget::slot_clicked_copyPresetButton);
	connect(ui.presetList, &CLC7ModifiedListWidget::sig_edit, this, &CLC7PresetWidget::slot_clicked_editPresetButton);
	connect(ui.presetList, &CLC7ModifiedListWidget::sig_delete, this, &CLC7PresetWidget::slot_clicked_deletePresetButton);
	connect(ui.presetList, &CLC7ModifiedListWidget::sig_new, this, &CLC7PresetWidget::slot_clicked_newPresetButton);
	connect(ui.presetList, &CLC7ModifiedListWidget::sig_moveup, this, &CLC7PresetWidget::slot_clicked_upArrowButton);
	connect(ui.presetList, &CLC7ModifiedListWidget::sig_movedown, this, &CLC7PresetWidget::slot_clicked_downArrowButton);
	connect(ui.newPresetButton, &QAbstractButton::clicked, this, &CLC7PresetWidget::slot_clicked_newPresetButton);
	connect(ui.nameLineEdit, &QLineEdit::textChanged, this, &CLC7PresetWidget::slot_textChanged_nameLineEdit);
	connect(ui.saveButton, &QAbstractButton::clicked, this, &CLC7PresetWidget::slot_clicked_applyButton);
	connect(ui.revertButton, &QAbstractButton::clicked, this, &CLC7PresetWidget::slot_clicked_cancelButton);


	ui.presetList->setMouseTracking(true);
	ui.presetList->setDragDropMode(QAbstractItemView::InternalMove);
	RefreshContent();

	showSaveRevertButtons(false);
}

CLC7PresetWidget::~CLC7PresetWidget()
{TR;
}

void CLC7PresetWidget::RefreshContent()
{TR;
	QUuid def_preset = m_group->defaultPreset();

	ui.presetList->clear();

	int cnt = m_group->presetCount();
	for (int i = 0; i < cnt; i++)
	{
		ILC7Preset *preset = m_group->presetAt(i);

		QListWidgetItem *item = new QListWidgetItem(preset->name());
		item->setData(Qt::UserRole, preset->id());

		ui.presetList->addItem(item);
		
		if (def_preset == preset->id())
		{
			item->setSelected(true);
			ui.presetList->setCurrentItem(item);
		}
	}

	UpdateUI();
}

void CLC7PresetWidget::UpdateUI()
{TR;
	if (m_no_update_ui)
	{
		return;
	}

	bool valid = true;
	
	bool selected = m_current_preset != nullptr;
	bool selected_readonly = m_current_preset ? m_current_preset->readonly() : false;
	
	bool have_preset_buttons = false;

	for (int i = 0; i < ui.presetList->count(); i++)
	{
		QListWidgetItem *item = ui.presetList->item(i);
		ILC7Preset *preset = m_group->presetById(item->data(Qt::UserRole).toUuid());
		
		QFont font = item->font();

		if (preset->id() == m_group->defaultPreset())
		{
			font.setBold(true);
		}
		else
		{
			font.setBold(false);
		}
	
		if (preset->readonly())
		{
			//			item->setTextColor(QColor(m_colman->GetHighlightColor()));
			font.setItalic(true);
		}
		else
		{
			//			item->setTextColor(QColor(m_colman->GetTextColor()));
			font.setItalic(false);
		}

		item->setFont(font);

		((CLC7PresetButtons *)ui.presetList->itemWidget(item))->UpdateUI();
	}
	if (!have_preset_buttons)
	{

	}

	if (m_editing)
	{
		ui.nameLabel->setEnabled(true);
		ui.descriptionLabel->setEnabled(true);
		ui.nameLineEdit->setEnabled(true);
		ui.descriptionTextEdit->setEnabled(true);
		ui.presetGroupBox->setEnabled(true);
		ui.newPresetButton->setEnabled(false);
		ui.presetList->setEnabled(false);
	}
	else
	{
		ui.nameLabel->setEnabled(false);
		ui.descriptionLabel->setEnabled(false);
		ui.nameLineEdit->setEnabled(false);
		ui.descriptionTextEdit->setEnabled(false);
		ui.presetGroupBox->setEnabled(false);
		ui.newPresetButton->setEnabled(true);
		ui.presetList->setEnabled(true);
	}

	if (!selected)
	{
		valid = false;
	}
	if (m_editing)
	{
		valid = false;
	}
	else
	{
		if (ui.nameLineEdit->text().isEmpty())
		{
			valid = false;
		}

		if (!m_config_valid)
		{
			valid = false;
		}
	}

	emit sig_isValid(valid);
}

void CLC7PresetWidget::slot_currentItemChanged_presetList(QListWidgetItem *current, QListWidgetItem *previous)
{TR;if (m_editing)
	{
		Q_ASSERT(0);
		cancelEditPreset();
	}

	if (current == nullptr)
	{
		m_current_preset = nullptr;
		m_group->setDefaultPreset(QUuid());
		m_configwidget->setProperty("config", QVariant());
		
		setProperty("preset_name", QString());
	}
	else
	{
		m_current_preset = m_group->presetById(current->data(Qt::UserRole).toUuid());
		m_group->setDefaultPreset(m_current_preset->id());
		m_configwidget->setProperty("config", m_current_preset->config());
		
		setProperty("preset_name", m_current_preset->name());
	}

	doShowPreset();
	

	UpdateUI();
}

void CLC7PresetWidget::slot_clicked_newPresetButton(bool)
{TR;
	if (m_editing)
	{
		Q_ASSERT(0);
		cancelEditPreset();
	}

	ILC7Preset *preset = m_group->newPreset();
	
	preset->setName("New Preset");

	QListWidgetItem *item = new QListWidgetItem(preset->name());
	item->setData(Qt::UserRole, preset->id());

	ui.presetList->addItem(item);

	ui.presetList->setCurrentItem(item);

	doEditPreset(true);

	UpdateUI();
}

void CLC7PresetWidget::slot_clicked_deletePresetButton(bool)
{TR;
	if (!m_current_preset || m_current_preset->readonly())
	{
		return;
	}

	if (m_editing)
	{
		Q_ASSERT(0);
		cancelEditPreset();
	}

	doDeletePreset();

	UpdateUI();
}

void CLC7PresetWidget::slot_clicked_editPresetButton(bool)
{TR;	
	if (!m_current_preset || m_current_preset->readonly())
	{
		bool yes = CLC7App::getInstance()->GetController()->GetGUILinkage()->YesNoBox("Copy Preset For Editing?", "This preset is a read-only item than can not be edited. Would you like to edit a copy of this preset?");
		if (!yes)
		{
			return;
		}
		slot_clicked_copyPresetButton(true);
		doEditPreset(true);
		UpdateUI();
		return;
	}

	if (m_editing)
	{
		Q_ASSERT(0);
		cancelEditPreset();
	}

	doEditPreset(false);
	
	UpdateUI();
}

void CLC7PresetWidget::slot_clicked_copyPresetButton(bool)
{TR;
	if (!m_current_preset)
	{
		return;
	}

	if (m_editing)
	{
		Q_ASSERT(0);
		cancelEditPreset();
	}
	
	if (!m_group->copyPreset(m_current_preset->position()))
	{
		Q_ASSERT(0);
		return;
	}

	ILC7Preset *preset = m_group->presetAt(m_group->presetCount() - 1);
		
	preset->setName(preset->name()+" (copy)");

	QListWidgetItem *item = new QListWidgetItem(preset->name());
	item->setData(Qt::UserRole, preset->id());

	ui.presetList->addItem(item);

	ui.presetList->setCurrentItem(item);

	UpdateUI();
}

void CLC7PresetWidget::slot_clicked_upArrowButton(bool)
{TR;
	if (m_editing)
	{
		Q_ASSERT(0);
		cancelEditPreset();
	}

	if (m_current_preset == nullptr)
	{
		Q_ASSERT(0);
		return;
	}

	int cur = m_current_preset->position();
	if (!m_group->movePreset(cur, cur - 1))
	{
		Q_ASSERT(0);
		return;
	}

	QListWidgetItem *item = ui.presetList->takeItem(cur);
	ui.presetList->insertItem(cur - 1, item);

	ui.presetList->setCurrentItem(item);

	UpdateUI();
}

void CLC7PresetWidget::slot_clicked_downArrowButton(bool)
{TR;
	if (m_editing)
	{
		Q_ASSERT(0);
		cancelEditPreset();
	}

	if (m_current_preset == nullptr)
	{
		Q_ASSERT(0);
		return;
	}

	int cur = m_current_preset->position();
	if (!m_group->movePreset(cur, cur + 1))
	{
		Q_ASSERT(0);
		return;
	}

	QListWidgetItem *item = ui.presetList->takeItem(cur);
	ui.presetList->insertItem(cur + 1, item);

	ui.presetList->setCurrentItem(item);

	UpdateUI();
}

void CLC7PresetWidget::slot_clicked_applyButton(bool)
{TR;
	if (!m_editing)
	{
		Q_ASSERT(0);
		return;
	}
	applyEditPreset();
}

void CLC7PresetWidget::slot_clicked_cancelButton(bool)
{TR;
	if (!m_editing)
	{
		Q_ASSERT(0);
		return;
	}
	cancelEditPreset();
}



void CLC7PresetWidget::slot_recolorCallback(void)
{TR;
	QSize size(16, 16);
	size *= m_colman->GetSizeRatio();

	UpdateUI();
}

void CLC7PresetWidget::doShowPreset()
{TR;
	if (m_current_preset)
	{
		ui.nameLineEdit->setText(m_current_preset->name());
		ui.descriptionTextEdit->setPlainText(m_current_preset->description());
	}
	else
	{
		ui.nameLineEdit->setText("");
		ui.descriptionTextEdit->setPlainText("");
	}
}


void CLC7PresetWidget::doDeletePreset()
{TR;
	if (m_current_preset == nullptr)
	{
		Q_ASSERT(0);
		return;
	}

	int pos = m_current_preset->position();
	
	if (pos < (m_group->presetCount()-1))
	{
		ui.presetList->setCurrentItem(ui.presetList->item(pos + 1));
	}
	else if (m_group->presetCount() > 0)
	{
		ui.presetList->setCurrentItem(ui.presetList->item(pos - 1));
	}
	else
	{
		ui.presetList->setCurrentItem(nullptr);
	}

	// remove from presets
	m_group->deletePresetAt(pos);

	// remove from list in destructor
	m_no_update_ui = true;
	QListWidgetItem *item = ui.presetList->takeItem(pos);
	if (m_last_entered_item == item)
	{
		m_last_entered_item = nullptr;
	}
	delete item;
	m_no_update_ui = false;

	m_manager->flush();

	
	UpdateUI();
}


void CLC7PresetWidget::doEditPreset(bool isnew)
{TR;
	if (m_editing)
	{
		Q_ASSERT(0);
		return;
	}

	ui.nameLineEdit->setText(m_current_preset->name());
	ui.descriptionTextEdit->setPlainText(m_current_preset->description());
	
	m_editing = true;
	m_isnew = isnew;

	showSaveRevertButtons(true);

	UpdateUI();
}

void CLC7PresetWidget::applyEditPreset(void)
{TR;
	if (!m_config_valid)
	{
		CLC7App::getInstance()->GetController()->GetGUILinkage()->ErrorMessage("Configuration is invalid", m_config_invalid_why);
		return;
	}

	QString name = ui.nameLineEdit->text();

	if (name.isEmpty())
	{
		CLC7App::getInstance()->GetController()->GetGUILinkage()->ErrorMessage("Preset is invalid", "A name for the preset must be specified.");
		return;
	}

	QString desc = ui.descriptionTextEdit->toPlainText();
	QVariant config = m_configwidget->property("config");

	m_current_preset->setName(name);
	m_current_preset->setDescription(desc);
	m_current_preset->setConfig(config);

	m_editing = false;

	m_manager->flush();

	showSaveRevertButtons(false);

	RefreshContent();
}

void CLC7PresetWidget::cancelEditPreset(void)
{TR;
	m_editing = false;
	
	if (m_isnew)
	{
		doDeletePreset();
	}
	else
	{
		doShowPreset();
	}

	showSaveRevertButtons(false);
	
	RefreshContent();
}

void CLC7PresetWidget::slot_textChanged_nameLineEdit(const QString &text)
{TR;
	m_configwidget->setProperty("preset_name", text);
}


void CLC7PresetWidget::slot_isValid(bool valid, QString why)
{TR;
	m_config_valid = valid;
	m_config_invalid_why = why;
	
	UpdateUI();
}

void CLC7PresetWidget::setConfig(QVariant config)
{TR;
	m_configwidget->setProperty("config", config);
}

QVariant CLC7PresetWidget::getConfig(void)
{TR;
	return m_configwidget->property("config");
}

void CLC7PresetWidget::showButtons(QListWidgetItem *item, CLC7PresetButtons *buttonswidget)
{
	ILC7Preset *preset = m_group->presetById(item->data(Qt::UserRole).toUuid());

	if (!preset->readonly())
	{
		buttonswidget->ui.deletePresetButton->show();
	}
	buttonswidget->ui.editPresetButton->show();
	buttonswidget->ui.copyPresetButton->show();
}

void CLC7PresetWidget::hideButtons(CLC7PresetButtons *buttonswidget)
{
	buttonswidget->ui.deletePresetButton->hide();
	buttonswidget->ui.editPresetButton->hide();
	buttonswidget->ui.copyPresetButton->hide();
}

void CLC7PresetWidget::slot_doubleClicked_presetList(const QModelIndex & idx)
{
	slot_clicked_editPresetButton(false);
}

void CLC7PresetWidget::slot_itemEntered_presetList(QListWidgetItem *item)
{
	if (item == m_last_entered_item)
		return;
	
	CLC7PresetButtons *buttonswidget = (CLC7PresetButtons *)ui.presetList->itemWidget(item);
	if (!buttonswidget)
	{
		buttonswidget = new CLC7PresetButtons(ui.presetList);
		ui.presetList->setItemWidget(item, buttonswidget);
		item->setSizeHint(QSize(buttonswidget->width(), buttonswidget->height()));
		hideButtons(buttonswidget);
		buttonswidget->show();

		connect(buttonswidget->ui.editPresetButton, &QAbstractButton::clicked, this, &CLC7PresetWidget::slot_clicked_editPresetButton);
		connect(buttonswidget->ui.copyPresetButton, &QAbstractButton::clicked, this, &CLC7PresetWidget::slot_clicked_copyPresetButton);
		connect(buttonswidget->ui.deletePresetButton, &QAbstractButton::clicked, this, &CLC7PresetWidget::slot_clicked_deletePresetButton);
	}

	showButtons(item, buttonswidget);

	if (m_last_entered_item)
	{
		CLC7PresetButtons *buttonswidget = (CLC7PresetButtons *)ui.presetList->itemWidget(m_last_entered_item);
		hideButtons(buttonswidget);
	}

	m_last_entered_item = item;
}

void CLC7PresetWidget::slot_viewportEntered_presetList()
{
	if (!m_last_entered_item)
	{
		return;
	}

	CLC7PresetButtons *buttonswidget = (CLC7PresetButtons *)ui.presetList->itemWidget(m_last_entered_item);
	if (buttonswidget)
	{
		hideButtons(buttonswidget);
	}
	m_last_entered_item = nullptr;
}

void CLC7PresetWidget::slot_leave_presetList()
{
	if (!m_last_entered_item)
		return;

	CLC7PresetButtons *buttonswidget = (CLC7PresetButtons *)ui.presetList->itemWidget(m_last_entered_item);
	if (buttonswidget)
	{
		hideButtons(buttonswidget);
	}
	m_last_entered_item = nullptr;
}

void CLC7PresetWidget::showSaveRevertButtons(bool show)
{
	if (show)
	{
		ui.saveButton->show();
		ui.revertButton->show();
	}
	else
	{
		ui.saveButton->hide();
		ui.revertButton->hide();
	}
}

void CLC7PresetWidget::slot_dropped_presetList()
{
	for (int i = 0; i < m_group->presetCount(); i++)
	{
		QListWidgetItem *item = ui.presetList->item(i);
		ILC7Preset *preset = m_group->presetById(item->data(Qt::UserRole).toUuid());
		ILC7Preset *expectedpreset = m_group->presetAt(i);

		if (preset != expectedpreset)
		{
			m_group->movePreset(preset->position(), i);
		}
	}
}

void CLC7PresetWidget::slot_showContextMenu(const QPoint &pt)
{
	QPoint globalPos = ui.presetList->mapToGlobal(pt);
		
	QListWidgetItem *item = ui.presetList->itemAt(pt);
	
	QMenu menu;
	if (item)
	{
		ILC7Preset *preset = m_group->presetById(item->data(Qt::UserRole).toUuid());
		menu.addAction("Edit", this, SLOT(slot_menu_edit()));
		menu.addAction("Copy", this, SLOT(slot_menu_copy()));
		if (!preset->readonly())
		{
			menu.addAction("Delete", this, SLOT(slot_menu_delete()));
		}
		menu.addSeparator();
		if (preset->position() > 0)
		{
			menu.addAction("Move Up", this, SLOT(slot_menu_moveup()));
		}
		if (preset->position() < (m_group->presetCount() - 1))
		{
			menu.addAction("Move Down", this, SLOT(slot_menu_movedown()));
		}
	}
	else
	{
		menu.addAction("New", this, SLOT(slot_menu_new()));
	}

	menu.exec(globalPos);
}


void CLC7PresetWidget::slot_menu_edit()
{
	slot_clicked_editPresetButton(false);
}

void CLC7PresetWidget::slot_menu_copy()
{
	slot_clicked_copyPresetButton(false);
}

void CLC7PresetWidget::slot_menu_delete()
{
	slot_clicked_deletePresetButton(false);
}

void CLC7PresetWidget::slot_menu_moveup()
{
	slot_clicked_upArrowButton(false);
}

void CLC7PresetWidget::slot_menu_movedown()
{
	slot_clicked_downArrowButton(false);
}

void CLC7PresetWidget::slot_menu_new()
{
	slot_clicked_newPresetButton(false);
}

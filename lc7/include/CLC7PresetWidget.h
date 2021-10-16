#ifndef __INC_CLC7PresetWidget_H
#define __INC_CLC7PresetWidget_H

#include "ui_presetwidget.h"
#include "ui_presetbuttons.h"
#include "CLC7ModifiedListWidget.h"

class CLC7PresetButtons :public QWidget
{
	friend class CLC7PresetWidget;
	Q_OBJECT

private:

	Ui::PresetButtons ui;
	ILC7ColorManager *m_colman;

protected:

private slots :
	void slot_recolorCallback(void);

public:
	CLC7PresetButtons(QWidget *parent);
	virtual ~CLC7PresetButtons();

	void UpdateUI();
};


class CLC7PresetWidget :public QWidget
{
	Q_OBJECT

	Q_PROPERTY(QVariant config READ getConfig WRITE setConfig);

private:
	ILC7ColorManager *m_colman;
	ILC7PresetManager *m_manager;
	QWidget *m_configwidget;
	QString m_presetgroup_name;
	ILC7PresetGroup *m_group;
	bool m_no_update_ui;

	bool m_editing;
	bool m_isnew;
	bool m_config_valid;
	QString m_config_invalid_why;

	ILC7Preset *m_current_preset;

	QListWidgetItem *m_last_entered_item;

	Ui::PresetWidget ui;

signals:
	void sig_isValid(bool);

protected:

	void doShowPreset(void);
	void doEditPreset(bool isnew);
	void doDeletePreset(void);
	void applyEditPreset(void);
	void cancelEditPreset(void);

	void RefreshContent();
	void UpdateUI();
	void showButtons(QListWidgetItem *item, CLC7PresetButtons *buttonswidget);
	void hideButtons(CLC7PresetButtons *buttonswidget);
	void showSaveRevertButtons(bool show);

private slots:
	
	void slot_showContextMenu(const QPoint &pt);
	void slot_dropped_presetList(void);
	void slot_doubleClicked_presetList(const QModelIndex & idx);
	void slot_itemEntered_presetList(QListWidgetItem *item);
	void slot_currentItemChanged_presetList(QListWidgetItem *current, QListWidgetItem *previous);
	void slot_viewportEntered_presetList();
	void slot_leave_presetList();
	void slot_clicked_newPresetButton(bool);
	void slot_clicked_deletePresetButton(bool);
	void slot_clicked_editPresetButton(bool);
	void slot_clicked_copyPresetButton(bool);
	void slot_clicked_upArrowButton(bool);
	void slot_clicked_downArrowButton(bool);
	void slot_clicked_applyButton(bool);
	void slot_clicked_cancelButton(bool);
	void slot_textChanged_nameLineEdit(const QString &text);
	void slot_recolorCallback(void);
	void slot_isValid(bool, QString why);
	
	void slot_menu_edit();
	void slot_menu_copy();
	void slot_menu_delete();
	void slot_menu_moveup();
	void slot_menu_movedown();
	void slot_menu_new();
	
public:
	CLC7PresetWidget(ILC7PresetManager *manager, QWidget *page, QWidget *configwidget, QString preset_group);
	virtual ~CLC7PresetWidget();

	void setConfig(QVariant config);
	QVariant getConfig(void);

};

#endif
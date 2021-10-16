#ifndef TECHNIQUEBRUTECONFIG_H
#define TECHNIQUEBRUTECONFIG_H

#include <QWidget>
#include"ui_techniquebruteconfig.h"

class TechniqueBruteConfig : public QWidget
{
	Q_OBJECT;
	
	Q_PROPERTY(QVariant config READ getConfig WRITE setConfig);

public:

	TechniqueBruteConfig();
	~TechniqueBruteConfig();
	
	void setConfig(QVariant config);
	QVariant getConfig(void);

private:
	Ui::TechniqueBruteConfig ui;
	QMap<QString, QVariant> m_config;
	bool m_refreshing;
	bool m_use_advanced;
	QMap<QUuid, QUuid> m_LM_Map;
	
	void RefreshContent();
	void SetUseAdvancedFlag();
	void UpdateUI();

private slots:

	void slot_minCharsEdit_textChanged(const QString & text);
	void slot_maxCharsEdit_textChanged(const QString & text);
	void slot_minCheckBox_clicked(bool checked);
	void slot_maxCheckBox_clicked(bool checked);
	void slot_defaultCharSetCombo_currentIndexChanged(int index);
	void slot_durationHoursText_textChanged(const QString & text);
	void slot_durationMinutesText_textChanged(const QString & text);
	void slot_unlimitedCheckBox_clicked(bool checked);
	void slot_specificCharSetsTable_cellChanged(int row, int column);
		
	void slot_editCharSetsButton_clicked(bool checked);
	
	void slot_addRowButton_clicked(bool checked);
	void slot_removeRowButton_clicked(bool checked);
	
	void slot_enableAdvancedCharSetsCheckBox_clicked(bool checked);

signals:
	void sig_isValid(bool valid, QString why);

};


class SpecificCharsetTableDelegate : public QStyledItemDelegate
{
	Q_OBJECT
private:
	QTableWidget *m_table;

public:
	SpecificCharsetTableDelegate(QTableWidget* parent);
	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	void setEditorData(QWidget *editor, const QModelIndex &index) const;
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
	void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};




#endif // TECHNIQUEBRUTECONFIG_H

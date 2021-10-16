#ifndef __INC_CLC7CHARSETEDITOR_H
#define __INC_CLC7CHARSETEDITOR_H

#include <QWidget>
#include"ui_charseteditor.h"

class CLC7CharsetEditor : public QWidget
{
	Q_OBJECT;

	Q_PROPERTY(QVariant config READ getConfig WRITE setConfig);

public:

	CLC7CharsetEditor();
	~CLC7CharsetEditor();

	void setConfig(QVariant config);
	QVariant getConfig(void);

private:
	Ui::CharsetEditor ui;

	CLC7JTREXEDLL m_jtrdll;

	bool m_use_existing;
	QString m_charset_file;
	bool m_charset_valid;
	bool m_mask_mode;
	QString m_mask;
	QUuid m_mask_encoding;
	bool m_mask_valid;

	bool m_use_input_characters;
	QString m_input_characters;
	QUuid m_input_encoding;
	
	bool m_use_input_dictionary_files;
	QStringList m_dictionary_files;
	
	bool m_use_input_pot_files;
	QStringList m_pot_files;

	void RefreshContent();
	void UpdateUI();
	void UpdateCharset();
	bool DoCreateCharset(QString & error);
	QString ExtractMask(QString chrfile);
	void ValidateMask(void);

signals:
	void sig_isValid(bool, QString);

private slots:

	void slot_useExistingRadio_clicked(bool checked);

	void slot_chrFileEdit_textChanged(const QString &text);
	void slot_chrFileBrowse_clicked(bool checked);

	void slot_createNewRadio_clicked(bool checked);

	void slot_useCharsCheckbox_clicked(bool checked);
	void slot_inputCharactersEdit_textChanged(const QString &text);
	void slot_inputCharactersEncodingCombo_currentIndexChanged(int idx);

	void slot_useDictCheckbox_clicked(bool checked);
	void slot_dictionaryList_itemSelectionChanged(void);
	void slot_plusButtonDict_clicked(bool checked);
	void slot_minusButtonDict_clicked(bool checked);

	void slot_usePotCheckbox_clicked(bool checked);
	void slot_potList_itemSelectionChanged(void);
	void slot_plusButtonPot_clicked(bool checked);
	void slot_minusButtonPot_clicked(bool checked);

	void slot_createButton_clicked(bool checked);

	void slot_maskModeCheckBox_clicked(bool checked);
	void slot_maskEdit_textChanged(const QString & text);
	void slot_maskEncodingCombo_currentIndexChanged(int idx);

};

#endif

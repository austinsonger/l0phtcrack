#ifndef TECHNIQUEDICTIONARYCONFIG_H
#define TECHNIQUEDICTIONARYCONFIG_H

#include <QWidget>
#include"ui_techniquedictionaryconfig.h"

class TechniqueDictionaryConfig : public QWidget
{
	Q_OBJECT

	Q_PROPERTY(QVariant config READ getConfig WRITE setConfig);

public:

	TechniqueDictionaryConfig();
	~TechniqueDictionaryConfig();	
	
	void setConfig(QVariant config);
	QVariant getConfig(void);

private:
	Ui::TechniqueDictionaryConfig ui;

	QMap<QString, QVariant> m_config;
	bool m_refreshing;

	void RefreshContent();
	void UpdateUI();

private slots:
	
	void slot_wordListEdit_textChanged(const QString & text);
	void slot_browseWordlistButton_clicked(bool checked);

	void slot_encodingCombo_currentIndexChanged(int index);
	void slot_permutationRulesCombo_currentIndexChanged(int index);
	void slot_enableCommonLetterSubstitutions_clicked(bool checked);

	void slot_durationHoursText_textChanged(const QString & text);
	void slot_durationMinutesText_textChanged(const QString & text);
	void slot_unlimitedCheckBox_clicked(bool checked);
	
signals:
	void sig_isValid(bool valid, QString why);

};

#endif // TECHNIQUEDICTIONARYCONFIG_H

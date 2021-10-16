#ifndef EXPORTACCOUNTSCONFIG_H
#define EXPORTACCOUNTSCONFIG_H

#include <QWidget>
#include"ui_exportaccountsconfig.h"

class ExportAccountsConfig : public QWidget
{
	Q_OBJECT

public:
	ExportAccountsConfig(QWidget *parent, QWidget *page, const QMap<QString, QVariant> & def_config, bool simple = false);
	~ExportAccountsConfig();

	QMap<QString, QVariant> GetConfig();

private:
	Ui::ExportAccountsConfig ui;
	QWidget *m_page;

	QString m_format_name;
	QString m_format_ext;
	bool m_default_include_style;

	void UpdateUI();

private slots:

	void onTextChanged(const QString & str);
	void onClicked(bool checked);
	void onFormatClicked(bool checked);
	void onBrowseFile(bool checked);

signals:
	void sig_isValid(bool valid);

};

#endif // ExportAccountsConfig_H

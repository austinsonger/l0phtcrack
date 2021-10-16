#ifndef IMPORTNTDSCONFIG_H
#define IMPORTNTDSCONFIG_H

#include <QWidget>
#include"ui_importntdsconfig.h"

class ImportNTDSConfig : public QWidget
{
	Q_OBJECT

public:
	ImportNTDSConfig(QWidget *parent, QWidget *page, const QMap<QString, QVariant> & def_config);
	~ImportNTDSConfig();

	QString GetNTDSFilename();
	QString GetSYSTEMFilename();
	bool GetKeepCurrentAccounts();
	bool GetIncludeMachineAccounts();
	bool GetLimitAccounts();
	quint32 GetAccountLimit();

private:
	Ui::ImportNTDSConfig ui;
	QWidget *m_page;

	void UpdateUI();

	private slots:

	void slot_textChanged_NTDSFileNameEdit(const QString & str);
	void slot_textChanged_SYSTEMFileNameEdit(const QString & str);
	void slot_clicked_browseButtonNTDS(bool checked);
	void slot_clicked_browseButtonSYSTEM(bool checked);
	void slot_clicked_KeepCurrentAccounts(bool checked);
	void slot_clicked_IncludeMachineAccounts(bool checked);
	void slot_clicked_LimitAccounts(bool checked);
	void slot_textChanged_AccountLimit(const QString &str);

signals:
	void sig_isValid(bool valid);

};

#endif // IMPORTLOCALCONFIG_H

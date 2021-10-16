#ifndef IMPORTPWDUMPCONFIG_H
#define IMPORTPWDUMPCONFIG_H

#include <QWidget>
#include"ui_importpwdumpconfig.h"

class ImportPWDumpConfig : public QWidget
{
	Q_OBJECT

public:
	ImportPWDumpConfig(QWidget *parent, QWidget *page, const QMap<QString, QVariant> & def_config, bool simple=false);
	~ImportPWDumpConfig();

	QString GetFilename();
	bool GetKeepCurrentAccounts();
	bool GetLimitAccounts();
	quint32 GetAccountLimit();

private:
	Ui::ImportPWDumpConfig ui;
	QWidget *m_page;

	void UpdateUI();

private slots:

	void slot_textChanged_fileNameEdit(const QString & str);
	void slot_clicked_browseButton(bool checked);
	void slot_clicked_KeepCurrentAccounts(bool checked);
	void slot_clicked_LimitAccounts(bool checked);
	void slot_textChanged_AccountLimit(const QString &str);

signals:
	void sig_isValid(bool valid);

};

#endif // IMPORTLOCALCONFIG_H

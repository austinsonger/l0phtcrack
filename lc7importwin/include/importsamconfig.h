#ifndef IMPORTSAMCONFIG_H
#define IMPORTSAMCONFIG_H

#include <QWidget>
#include"ui_importsamconfig.h"

class ImportSAMConfig : public QWidget
{
	Q_OBJECT

public:
	ImportSAMConfig(QWidget *parent, QWidget *page, const QMap<QString, QVariant> & def_config);
	~ImportSAMConfig();

	QString GetSAMFilename();
	QString GetSYSTEMFilename();
	bool GetKeepCurrentAccounts();
	bool GetLimitAccounts();
	quint32 GetAccountLimit();

private:
	Ui::ImportSAMConfig ui;
	QWidget *m_page;

	void UpdateUI();

	private slots:

	void slot_textChanged_SAMFileNameEdit(const QString & str);
	void slot_textChanged_SYSTEMFileNameEdit(const QString & str);
	void slot_clicked_browseButtonSAM(bool checked);
	void slot_clicked_browseButtonSYSTEM(bool checked);
	void slot_clicked_KeepCurrentAccounts(bool checked);
	void slot_clicked_LimitAccounts(bool checked);
	void slot_textChanged_AccountLimit(const QString &str);

signals:
	void sig_isValid(bool valid);

};

#endif // IMPORTLOCALCONFIG_H

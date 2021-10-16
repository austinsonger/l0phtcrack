#ifndef IMPORTDRSRCONFIG_H
#define IMPORTDRSRCONFIG_H

#include <QWidget>
#include"ui_importdrsrconfig.h"

class ImportDRSRConfig : public QWidget
{
	Q_OBJECT

public:
	ImportDRSRConfig(QWidget *parent, QWidget *page, const QMap<QString, QVariant> & def_config);
	~ImportDRSRConfig();

//	QString GetNTDSFilename();
//	QString GetSYSTEMFilename();
	bool GetKeepCurrentAccounts();
	bool GetLimitAccounts();
	quint32 GetAccountLimit();

private:
	Ui::ImportDRSRConfig ui;
	QWidget *m_page;

	void UpdateUI();

	private slots:

	//void slot_textChanged_NTDSFileNameEdit(const QString & str);
	//void slot_textChanged_SYSTEMFileNameEdit(const QString & str);
	//void slot_clicked_browseButtonNTDS(bool checked);
	//void slot_clicked_browseButtonSYSTEM(bool checked);
	void slot_clicked_KeepCurrentAccounts(bool checked);
	void slot_clicked_LimitAccounts(bool checked);
	void slot_textChanged_AccountLimit(const QString &str);

signals:
	void sig_isValid(bool valid);

};

#endif // IMPORTDRSRCONFIG_H

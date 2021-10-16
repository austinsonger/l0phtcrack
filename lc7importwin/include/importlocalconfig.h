#ifndef IMPORTLOCALCONFIG_H
#define IMPORTLOCALCONFIG_H

#include <QWidget>
#include"ui_importlocalconfig.h"

class ImportLocalConfig : public QWidget
{
	Q_OBJECT

public:
	ImportLocalConfig(QWidget *parent, QWidget *page, const QMap<QString,QVariant> & def_config, bool simple=false);
	~ImportLocalConfig();

	bool GetIncludeMachineAccounts();
	bool GetKeepCurrentAccounts();
	bool GetUseCurrentCreds();
	bool GetUseSavedCreds();
	bool GetUseSpecificCreds();
	QString GetUsername();
	LC7SecureString GetPassword();
	QString GetDomain();
	bool GetSaveCredentials();
	bool GetLimitAccounts();
	quint32 GetAccountLimit();

private:
	Ui::ImportLocalConfig ui;
	QWidget *m_page;
	
	void UpdateUI();

private slots:

	void onTextChanged(const QString & str);
	void onClicked(bool checked);

signals:
	void sig_isValid(bool valid);

};

#endif // IMPORTLOCALCONFIG_H

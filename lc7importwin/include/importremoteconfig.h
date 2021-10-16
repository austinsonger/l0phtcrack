#ifndef IMPORTREMOTECONFIG_H
#define IMPORTREMOTECONFIG_H

#include <QWidget>
#include"ui_importremoteconfig.h"

class ImportRemoteConfig : public QWidget
{
	Q_OBJECT

public:
	ImportRemoteConfig(QWidget *parent, QWidget *page, const QMap<QString,QVariant> & def_config, bool simple = false);
	~ImportRemoteConfig();

	bool GetIncludeMachineAccounts();
	bool GetKeepCurrentAccounts();
	bool GetUseCurrentCreds();
	bool GetUseSavedCreds();
	bool GetUseSavedDefaultCreds();
	bool GetUseSpecificCreds();
	QString GetHost();
	QStringList GetHostHistory();
	QString GetUsername();
	LC7SecureString GetPassword();
	QString GetDomain();
	bool GetSaveCredentials();
	bool GetSaveDefaultCredentials();
	bool GetLimitAccounts();
	quint32 GetAccountLimit();
	quint32 GetImportMode();

private:
	Ui::ImportRemoteConfig ui;
	QWidget *m_page;

	void UpdateUI();

private slots:

	void onTextChanged(const QString & str);
	void onClicked(bool checked);
	void onCurrentIndexChanged(int index);

signals:
	void sig_isValid(bool valid);

};

#endif // IMPORTREMOTECONFIG_H

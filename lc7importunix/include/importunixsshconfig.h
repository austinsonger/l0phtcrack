#ifndef IMPORTUNIXSSHCONFIG_H
#define IMPORTUNIXSSHCONFIG_H

#include <QWidget>
#include"ui_importunixsshconfig.h"

class ImportUnixSSHConfig : public QWidget
{
	Q_OBJECT

public:
	ImportUnixSSHConfig(QWidget *parent, QWidget *page, const QMap<QString, QVariant> & def_config, bool simple);
	~ImportUnixSSHConfig();

	bool GetIncludeNonLogin();
	bool GetKeepCurrentAccounts();
	bool GetUseSavedCreds();
	bool GetUseSavedDefaultCreds();
	bool GetUseSpecificCreds();
	QString GetHost();
	QStringList GetHostHistory();
	QString GetUsername();
	bool GetUsePasswordAuth();
	bool GetUsePublicKeyAuth();
	LC7SecureString GetPassword();
	QString GetPrivateKeyFile();
	LC7SecureString GetPrivateKeyPassword();
	bool GetNoElevation();
	bool GetSUDOElevation();
	LC7SecureString GetSUDOPassword();
	bool GetSUElevation();
	LC7SecureString GetSUPassword();
	bool GetSaveCredentials();
	bool GetSaveDefaultCredentials();
	FOURCC GetHashType();
	bool GetLimitAccounts();
	quint32 GetAccountLimit();


private:
	Ui::ImportUnixSSHConfig ui;
	QWidget *m_page;
	QButtonGroup m_radiogroup;

	FOURCC m_hashtype;
	bool m_connected_and_validated;
		
	void UpdateUI();

private slots:

	void onUsernameTextChanged(const QString & str);
	void onTextChanged(const QString & str);
	void onClicked(bool checked);
	void onSafeTextChanged(const QString & str);
	void onSafeClicked(bool checked);
	void onBrowsePrivateKeyButton(bool checked);
	void onCurrentRowChanged(int row);
	void onConnectAndVerify(bool checked);

signals:
	void sig_isValid(bool valid);

};

#endif // ImportUnixSSHConfig_H

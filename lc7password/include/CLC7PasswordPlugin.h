#ifndef __INC_CLC7PASSWORDPLUGIN_H
#define __INC_CLC7PASSWORDPLUGIN_H

class CLC7AccountListFactory;
class CPasswordUIOptions;
class CLC7PasswordLinkage;
class CLC7ReportExportAccounts;
class CLC7ReportExportAccountsGUI;

class CLC7PasswordPlugin:public QObject, public ILC7Plugin
{
	Q_OBJECT;
	
private:
	CLC7AccountListFactory *m_pAccountListFactory;
	
	CLC7ReportExportAccounts *m_pReportExportAccounts;
	CLC7PasswordLinkage *m_pPasswordLinkage;
		
public:

	CLC7PasswordPlugin();
	virtual ~CLC7PasswordPlugin();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	virtual QUuid GetID();
	virtual QList<QUuid> GetInternalDependencies();

	virtual bool Activate();
	virtual bool Deactivate();
};

#endif
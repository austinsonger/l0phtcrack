#ifndef __INC_IMPORTWINDOWS_H
#define __INC_IMPORTWINDOWS_H

#ifdef __APPLE__
#ifndef __INC_WINE
#define __INC_WINE
#undef APIENTRY
#define __builtin_ms_va_list __builtin_va_list
#include<windows.h>
#undef min
#undef max
#define _WINBASE_H 1
#endif
#endif

#include "windows_abstraction.h"
#include <QThread>
#include "lc7api.h"

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/err.h>

class CImportWindows;

// Code common to local and remote windows import
class CImportWindows: public QObject
{
	Q_OBJECT;
private:

	bool m_bRemote;

	QString m_strRemoteMachine;
	QString m_strRemoteMachineWithSlashes;
	QString m_strRemoteAdmin;
	QString m_strRemoteAdminTemp;
	QString m_strRemoteWOW64;
	QString m_strSource;
	QString m_strSourceDLL;
	QString m_strTarget;
	QString m_strTargetDLL;
	QString m_strRemoteDMP;
	QString m_strRemoteDMPStatus;
	QString m_strRemoteREMIn;
	QString m_strRemoteREMOut;
	QString m_strRemoteREMStatus;

	bool m_bSpecificUser;
	QString m_strUser;
	QString m_strPassword;
	QString m_strDomain;

	bool m_bIncludeMachineAccounts;
	int m_account_limit;

	ILC7AccountList *m_accountlist;
	ILC7CommandControl *m_ctrl;

	bool m_bIs64Bit;
	bool m_bRetryAuth;

	int m_estimated_total;
	int m_number_of_users;
	
	std::string m_public_key;
	std::string m_private_key;
	bool m_bSerializedSymmetricKey;
	
	LC7Remediations m_remediations;
	
	QList<int> m_accounts_to_force_change;
	QList<int> m_accounts_to_disable;
	QFile *m_remedation_command_file;

	// Import state
	bool m_bEstablishedConnection;
	bool m_bImpersonated;
	bool m_bAgentStarted;
	WIN_SC_HANDLE m_hRemoteSC;
	WIN_SERVICE_HANDLE m_hLcAgent;

private:

	int m_nNumImported;

	void CheckIs64Bit(void);
	void UpdateStatus(QString statustext, quint32 cur, bool statuslog=true);
	void ApplyCredentials(void);
	bool NeedsAgentUpdate(QByteArray & agent, QByteArray & agentdll, bool &agent_exists, bool & cancelled);
	void GetAllUsefulPathnames(void);
	void GetOrCreatePrivateKey(bool & created);
	void CreateAgentAndDLL(QByteArray & agent,QByteArray & agentdll);
	void WriteAgentFile(QString filepath, QByteArray filedata);
	void CopyAgent(QByteArray agent, QByteArray agentdll);
	void TouchEmptyFile(QString path);
	void FindAgentKeyOffset(QByteArray agent, quint32 *pdwKeyLenOffset, quint32 *pdwKeyOffset);
	bool CheckForEncryptionKey(HANDLE hAgent, quint32 *pdwKeyOffset, bool *pbInitialized);
	void GenerateEncryptionKeyForAgent(HANDLE hAgent, quint32 dwKeyOffset, BOOL bDeleteExistingKey);
	
	void StartAgent(SC_HANDLE hRemoteSC);
	void IssueHashDumpRequest(WIN_SERVICE_HANDLE hLcAgent);
	void IssueRemediationRequest(WIN_SERVICE_HANDLE hLcAgent);
	void WaitForCompletion(QString strStatusFile, HANDLE hProcess, bool & cancelled);
	void DecryptAndImportHashes(QString strDumpFile, bool & cancelled);
	bool WarnMITM(bool changed);
	void LoadAndDecryptBuffer(QFile &f, QByteArray & ba);

	void WriteRemediationCommandFile(QString myremediationpath);
	void OpenRemediationCommandFile(QString myremediationpath, quint32 remsize);
	void WriteRemediationCommand(quint32 command, QList<int> accountnumbers);
	void CloseRemediationCommandFile();
	void PerformRemediations(QString remediationpath, bool & cancelled);

	void DoImportLocal(bool & cancelled);
	void DoImportSMB(bool & cancelled);
	void DoRemediateLocal(bool & cancelled);
	void DoRemediateSMB(bool & cancelled);

public:
	
	CImportWindows(ILC7AccountList *accountlist, ILC7CommandControl *ctrl);
	~CImportWindows();

	void SetRemoteMachine(QString remotemachine);
	void SetSpecificUser(QString user, QString password, QString domain);
	void SetIncludeMachineAccounts(bool include);
	void SetRemediations(const LC7Remediations &remediations);
	
	void SetAccountsToRemediate(QList<int> accounts_to_force_change, QList<int> accounts_to_disable);

	void SetAccountLimit(quint32 alim);

	bool DoImport(QString & error, bool & cancelled);
	bool DoRemediate(QString & error, bool & cancelled);

	QString LastError(void);
	quint32 GetNumberOfUsers(void);

	bool CreateRemoteAgent(QString folder, QString & error);
	bool CreateRemoteAgentInstaller(QString target, QString & error);
};

#endif
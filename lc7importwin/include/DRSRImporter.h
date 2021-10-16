#ifndef __INC_DRSRIMPORTER_H
#define __INC_DRSRIMPORTER_H

#include"drsr.h"
#include"drsr_addons.h"
#include"drsr_alloc.h"

#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif
#include<Sspi.h>

class DRSRImporter
{
private:

	bool m_bImpersonated;

	bool m_bRemote;
	QString m_strRemoteMachine;
	QString m_strRemoteMachineWithSlashes;

	bool m_bSpecificUser;
	QString m_strUser;
	QString m_strPassword;
	QString m_strDomain;

	bool m_bIncludeMachineAccounts;
	int m_account_limit;

	LC7Remediations m_remediations;

	ILC7AccountList *m_accountlist;
	ILC7CommandControl *m_ctrl;

	int m_objects_processed;
	int m_objects_total;
	int m_users_imported;
	
	QString m_last_error;
	
	void UpdateStatus(QString statustext, quint32 cur, bool statuslog = true);

	//
	struct ReplicationCookie
	{
		std::wstring NamingContext;
		GUID InvocationId;
		int64_t HighObjUpdate;
		int64_t Reserved;
		int64_t HighPropUpdate;
		
		ReplicationCookie(std::wstring naming_context) : NamingContext(naming_context), InvocationId(), HighObjUpdate(0), Reserved(0), HighPropUpdate(0) {}
		ReplicationCookie(std::wstring naming_context, GUID invocation_id, int64_t high_obj_update, int64_t high_prop_update, int64_t reserved) : NamingContext(naming_context), InvocationId(invocation_id), HighObjUpdate(high_obj_update), HighPropUpdate(high_prop_update), Reserved(reserved) {}
	};

	bool m_bHasBindingHandle;
	bool m_boundDRSConnection;
	RPC_BINDING_HANDLE m_bindingHandle;
	SecPkgContext_SessionKey m_sessionKey;
	GUID m_clientDsa;
	GUID m_serverSiteObjectGuid;
	DRS_EXT m_serverCapabilities;
	DWORD m_serverReplEpoch;
	DRS_HANDLE m_drsHandle;

	//

	bool ConnectTCP();
	bool ConnectNP();
	bool OpenRPCBindingHandle_TCP();
	bool OpenRPCBindingHandle_NP();
	bool FreeRPCBindingHandle();
	bool OpenDRSConnection(GUID clientDsa);
	static void RPCCOptSecurityCallback(void *rpcContext);
	void RetrieveSessionKey(void* rpcContext);
	bool BindDRSConnection();
	bool ReleaseDRSConnection();
	midl_ptr<DRS_MSG_GETCHGREQ_V10> CreateGenericReplicateRequest(midl_ptr<DSNAME> &&dsName, const std::vector<ATTRTYP> &partialAttributeSet, ULONG maxBytes, ULONG maxObjects);
	midl_ptr<DRS_MSG_GETCHGREQ_V10> CreateReplicateAllRequest(const ReplicationCookie & cookie, const std::vector<ATTRTYP> &partialAttributeSet, ULONG maxBytes, ULONG maxObjects);
	bool CreateNamingContext(std::wstring & naming_context);
	midl_ptr<DRS_MSG_GETCHGREPLY> GetNCChanges(midl_ptr<DRS_MSG_GETCHGREQ_V10> &&request, DWORD *reply_version);
	bool ReadAccounts(const REPLENTINFLIST *objects, int objectCount, /* const REPLVALINF_V3 *linkedValues*/ int valueCount, bool & cancelled);
	bool DecryptSecret(const std::vector<byte> &blob_in, std::vector<byte> &blob_out);
	bool DecryptHash(const std::vector<byte> &enchash, DWORD rid, std::vector<byte> &hash);

public:

	DRSRImporter(ILC7AccountList *accountlist, ILC7CommandControl *ctrl);
	~DRSRImporter();

	void SetAccountLimit(quint32 alim);
	void SetRemoteMachine(QString remote_machine);
	void SetSpecificUser(QString user, QString password, QString domain);
	void SetIncludeMachineAccounts(bool include);
	void SetRemediations(const LC7Remediations &remediations);
	bool DoImport(QString & error, bool & cancelled);
	
	QString LastError(void);
	quint32 GetNumberOfUsers(void);
};

#endif
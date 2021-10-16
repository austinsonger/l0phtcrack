#ifndef __INC_DRSCONNECTION_H
#define __INC_DRSCONNECTION_H

#include "drsr.h"
#include <string>

class DrsConnection
{
public:
	typedef void SecurityCallback(void* rpcContext);

	std::vector<byte> SessionKey;
	GUID ServerSiteGuid;

	struct ReplicationCursor
	{
		long UpToDatenessUsn;
		GUID SourceInvocationId;
		ReplicationCursor(GUID invocationId, long highestUsn) : SourceInvocationId(invocationId), UpToDatenessUsn(highestUsn) {}
	};

private:

	GUID _clientDsa;
	DRS_EXTENSIONS _serverCapabilities;
	DWORD _serverReplEpoch;
	
	static const size_t defaultMaxObjects = 1000;
	// 8MB
	static const size_t defaultMaxBytes = 8 * 1024 * 1024;
	static const DWORD defaultReplEpoch = 0;

public:
	DrsConnection(RPC_BINDING_HANDLE rpcHandle, GUID clientDsa);
	
	std::vector<ReplicationCursor::PTR> GetReplicationCursors(const std::string &namingContext);
	ReplicaObject::PTR ReplicateSingleObject(GUID objectGuid);
	ReplicaObject::PTR ReplicateSingleObject(GUID objectGuid, const std::vector<ATTRTYP> &partialAttributeSet);
	ReplicaObject::PTR ReplicateSingleObject(std::string distinguishedName);
	ReplicaObject::PTR ReplicateSingleObject(std::string distinguishedName, const std::vector<ATTRTYP> &partialAttributeSet);
	ReplicationResult::PTR ReplicationResult^ ReplicateAllObjects(ReplicationCookie^ cookie);
	ReplicationResult^ ReplicateAllObjects(ReplicationCookie^ cookie, ULONG maxBytes, ULONG maxObjects);
	ReplicationResult^ ReplicateAllObjects(ReplicationCookie^ cookie, array<ATTRTYP>^ partialAttributeSet, ULONG maxBytes, ULONG maxObjects);
	String^ ResolveDistinguishedName(NTAccount^ accountName);
	String^ ResolveDistinguishedName(SecurityIdentifier^ objectSid);
	Guid ResolveGuid(NTAccount^ accountName);
	Guid ResolveGuid(SecurityIdentifier^ objectSid);
	Guid ResolveGuid(String^ userPrincipalName);
	bool TestObjectExistence(String^ distinguishedName);
	bool TestObjectExistence(Guid objectGuid);
protected:
	virtual bool ReleaseHandle() override;
private:
	DWORD GetMaxSupportedReplicationRequestVersion();
	DrsConnection();

	void Bind(IntPtr rpcHandle);
	midl_ptr<DRS_MSG_GETCHGREPLY_V9> GetNCChanges(midl_ptr<DRS_MSG_GETCHGREQ_V10> &&request);
	midl_ptr<DRS_MSG_CRACKREPLY_V1> CrackNames(midl_ptr<DRS_MSG_CRACKREQ_V1> &&request);
	String^ TryResolveName(String^ name, DS_NAME_FORMAT formatOffered, DS_NAME_FORMAT formatDesired);
	midl_ptr<DRS_EXTENSIONS_INT> CreateClientInfo();
	midl_ptr<DRS_MSG_GETCHGREQ_V10> CreateReplicateAllRequest(ReplicationCookie^ cookie, array<ATTRTYP>^ partialAttributeSet, ULONG maxBytes, ULONG maxObjects);
	midl_ptr<DRS_MSG_GETCHGREQ_V10> CreateReplicateSingleRequest(String^ distinguishedName, array<ATTRTYP>^ partialAttributeSet);
	midl_ptr<DRS_MSG_GETCHGREQ_V10> CreateReplicateSingleRequest(Guid objectGuid, array<ATTRTYP>^ partialAttributeSet);
	midl_ptr<DRS_MSG_GETCHGREQ_V10> CreateGenericReplicateRequest(midl_ptr<DSNAME> &&dsName, array<ATTRTYP>^ partialAttributeSet, ULONG maxBytes, ULONG maxObjects);
	
	void RetrieveSessionKey(void* rpcContext);

	static midl_ptr<DRS_MSG_GETREPLINFO_REQ_V1> CreateReplicationCursorsRequest(String^ namingContext);
	static midl_ptr<PARTIAL_ATTR_VECTOR_V1_EXT> CreateNativePas(array<ATTRTYP>^ partialAttributeSet);
	static array<byte>^ ReadValue(const ATTRVAL &value);
	static array<array<byte>^>^ ReadValues(const ATTRVALBLOCK &values);
	static ReplicaAttribute^ ReadAttribute(const ATTR &attribute);
	static ReplicaAttribute^ ReadAttribute(const REPLVALINF_V3 &attribute);
	static ReplicaAttributeCollection^ ReadAttributes(const ATTRBLOCK &attributes);
	static ReplicaObject^ ReadObject(const ENTINF &object);
	static ReplicaObjectCollection^ ReadObjects(const REPLENTINFLIST *objects, int objectCount, const REPLVALINF_V3 *linkedValues, int valueCount);
	static Guid ReadGuid(GUID const &guid);
	static String^ ReadName(const DSNAME* dsName);
	static SecurityIdentifier^ ReadSid(const DSNAME* dsName);
};


#endif
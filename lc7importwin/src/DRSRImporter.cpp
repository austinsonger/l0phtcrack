#include<stdafx.h>
#include"ntds.h"
#include"drsr_addons.h"
#include"drsr_alloc.h"
#include"windows_abstraction.h"
#include"Iads.h"
#include"Adshlp.h"
#include"atlbase.h"
//#include"slow_md5.h"
#include"slow_rc4.h"
#include"slow_des.h"


// rid_to_key1: Function to convert the RID to the first decrypt key.
static void rid_to_key1(uint32_t rid, unsigned char deskey[8])
{
	char s[7];

	s[0] = (unsigned char)(rid & 0xFF);
	s[1] = (unsigned char)((rid >> 8) & 0xFF);
	s[2] = (unsigned char)((rid >> 16) & 0xFF);
	s[3] = (unsigned char)((rid >> 24) & 0xFF);
	s[4] = s[0];
	s[5] = s[1];
	s[6] = s[2];

	slowdes_str_to_key(s, deskey);
}

// rid_to_key2: Function to convert the RID to the second decrypt key.
static void rid_to_key2(uint32_t rid, unsigned char deskey[8])
{
	char s[7];

	s[0] = (unsigned char)((rid >> 24) & 0xFF);
	s[1] = (unsigned char)(rid & 0xFF);
	s[2] = (unsigned char)((rid >> 8) & 0xFF);
	s[3] = (unsigned char)((rid >> 16) & 0xFF);
	s[4] = s[0];
	s[5] = s[1];
	s[6] = s[2];

	slowdes_str_to_key(s, deskey);
}


static const size_t s_defaultMaxObjects = 1000;
static const size_t s_defaultMaxBytes = 8 * 1024 * 1024;
static const DWORD s_defaultReplEpoch = 0;
static const GUID s_DcPromoGuid2k = { 0x6abec3d1, 0x3054, 0x41c8,{ 0x62, 0xa3, 0x5a, 0x0c, 0x5b, 0x7d, 0x5d, 0x71 } };
static const GUID s_DcPromoGuid2k3 = { 0x6afab99c, 0x6e26, 0x464a, { 0x5f, 0x97, 0xf5, 0x8f, 0x10, 0x52, 0x18, 0xbc } };

static uint32_t crcTable[] = {
	0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
	0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
	0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
	0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
	0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
	0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
	0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
	0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
	0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
	0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
	0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
	0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
	0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
	0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
	0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
	0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
	0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
	0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
	0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
	0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
	0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
	0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
	0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
	0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
	0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
	0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
	0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
	0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
	0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
	0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
	0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
	0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
	0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
	0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
	0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
	0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
	0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
	0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
	0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
	0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
	0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
	0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
	0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
	0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
	0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
	0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
	0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
	0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
	0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
	0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
	0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
	0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
	0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
	0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
	0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
	0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
	0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
	0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
	0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
	0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
	0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
	0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
	0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
	0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

static uint32_t crc(const std::vector<byte> &buffer)
{
	uint32_t crc = UINT32_MAX;
	for (size_t i = 0; i < buffer.size(); i++)
	{
		crc = crcTable[(crc ^ buffer[i]) & UINT8_MAX] ^ (crc >> 8);
	}
	return crc ^ UINT32_MAX;
}


DRSRImporter::DRSRImporter(ILC7AccountList *accountlist, ILC7CommandControl *ctrl)
{
	TR;
	if (ctrl)
		m_ctrl = ctrl->GetSubControl("Replication Importer: ");
	else
		m_ctrl = nullptr;

	m_accountlist = accountlist;
	m_account_limit = 0;
	m_objects_processed = 0;
	m_objects_total = 0;
	m_users_imported = 0;
	m_bImpersonated = false;
	m_bHasBindingHandle = false;
	m_drsHandle = nullptr;
	m_sessionKey = { 0, nullptr };

	slowdes_init();
}

DRSRImporter::~DRSRImporter()
{
	TR;
	if (m_ctrl)
		m_ctrl->ReleaseSubControl();
}

void DRSRImporter::UpdateStatus(QString statustext, quint32 cur, bool statuslog)
{
	TR;
	if (m_ctrl)
	{
		m_ctrl->SetStatusText(statustext);
		m_ctrl->UpdateCurrentProgressBar(cur);
		if (statuslog)
		{
			m_ctrl->AppendToActivityLog(statustext + "\n");
		}
	}
}


void DRSRImporter::SetAccountLimit(quint32 alim)
{
	TR;
	m_account_limit = alim;
}


void DRSRImporter::SetRemoteMachine(QString remote_machine)
{
	TR;

	// Remove leading and trailing backslashes
	while (remote_machine.at(0) == '\\')
	{
		remote_machine.remove(0, 1);
	}
	while (remote_machine.at(remote_machine.length() - 1) == '\\')
	{
		remote_machine.remove(remote_machine.length() - 1, 1);
	}

	m_bRemote = true;
	m_strRemoteMachine = remote_machine;
	m_strRemoteMachineWithSlashes = QString("\\\\") + m_strRemoteMachine;
}

void DRSRImporter::SetSpecificUser(QString user, QString password, QString domain)
{
	TR;
	m_bSpecificUser = true;
	m_strUser = user;
	m_strPassword = password;
	m_strDomain = domain;
}

void DRSRImporter::SetIncludeMachineAccounts(bool include)
{
	TR;
	m_bIncludeMachineAccounts = include;

}

void DRSRImporter::SetRemediations(const LC7Remediations &remediations)
{
	TR;
	m_remediations = remediations;
}

static const char *rpc_string_error(RPC_STATUS st)
{
	switch (st)
	{
	case RPC_S_OK:
		return "The call succeeded.";
	case RPC_S_INVALID_STRING_BINDING:
		return "The string binding is not valid.";
	case RPC_S_PROTSEQ_NOT_SUPPORTED:
		return "Protocol sequence not supported on this host.";
	case RPC_S_INVALID_RPC_PROTSEQ:
		return "The protocol sequence is not valid.";
	case RPC_S_INVALID_ENDPOINT_FORMAT:
		return "The endpoint format is not valid.";
	case RPC_S_STRING_TOO_LONG:
		return "String too long.";
	case RPC_S_INVALID_NET_ADDR:
		return "The network address is not valid.";
	case RPC_S_INVALID_ARG:
		return "The argument was not valid.";
	case RPC_S_INVALID_NAF_ID:
		return "The network address family identifier is not valid.";
	default:
		break;
	}
	return "Unknown error.";
}


bool DRSRImporter::OpenRPCBindingHandle_TCP()
{
	UpdateStatus("Attempting ncacn_ip_tcp replication binding...", 0);

	std::wstring wstr_remotemachine = m_strRemoteMachine.toStdWString();
	std::wstring wstr_spn=QString("ldaps/%1").arg(m_strRemoteMachine).toStdWString();

	// Connect to remote machine
	RPC_STATUS status;
	unsigned short *StringBinding;
	status = RpcStringBindingCompose(NULL, (RPC_WSTR)L"ncacn_ip_tcp", m_bRemote ? (RPC_WSTR)wstr_remotemachine.c_str() : nullptr, NULL, NULL, &StringBinding);
	if (status != RPC_S_OK)
	{
		m_last_error = rpc_string_error(status);
		return false;
	}
	status = RpcBindingFromStringBinding(StringBinding, &m_bindingHandle);
	if (status != RPC_S_OK)
	{
		RpcStringFree(&StringBinding);
		m_last_error = rpc_string_error(status);
		return false;
	}

	status = RpcBindingSetAuthInfo(m_bindingHandle, (RPC_WSTR)wstr_spn.c_str(), RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_AUTHN_GSS_NEGOTIATE, NULL, RPC_C_AUTHZ_NAME);
	if (status != RPC_S_OK)
	{
		RpcStringFree(&StringBinding);
		RpcBindingFree(&m_bindingHandle);
		m_last_error = rpc_string_error(status);
		return false;
	}

	RpcStringFree(&StringBinding);
	return true;
}

bool DRSRImporter::OpenRPCBindingHandle_NP()
{
	UpdateStatus("Attempting ncacn_np replication binding...", 0);

	std::wstring wstr_remotemachine = m_strRemoteMachine.toStdWString();
	std::wstring wstr_spn = QString("ldaps/%1").arg(m_strRemoteMachine).toStdWString();

	// Connect to remote machine
	RPC_STATUS status;
	unsigned short *StringBinding;
	status = RpcStringBindingCompose(NULL, (RPC_WSTR)L"ncacn_np", m_bRemote ? (RPC_WSTR)wstr_remotemachine.c_str(): nullptr, (RPC_WSTR)L"\\pipe\\lsass", NULL, &StringBinding);
	if (status != RPC_S_OK)
	{
		m_last_error = rpc_string_error(status);
		return false;
	}
	status = RpcBindingFromStringBinding(StringBinding, &m_bindingHandle);
	if (status != RPC_S_OK)
	{
		RpcStringFree(&StringBinding);
		m_last_error = rpc_string_error(status);
		return false;
	}

	status = RpcBindingSetAuthInfo(m_bindingHandle, (RPC_WSTR)wstr_spn.c_str(), RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_AUTHN_GSS_NEGOTIATE, NULL, RPC_C_AUTHZ_NAME);
	if (status != RPC_S_OK)
	{
		RpcStringFree(&StringBinding);
		RpcBindingFree(&m_bindingHandle);
		m_last_error = rpc_string_error(status);
		return false;
	}

	RpcStringFree(&StringBinding);
	return true;
}



bool DRSRImporter::FreeRPCBindingHandle()
{
	if (!m_bHasBindingHandle)
	{
		return true;
	}
	RPC_STATUS status = RpcBindingFree(&m_bindingHandle);
	if (status != RPC_S_OK)
	{
		m_last_error = rpc_string_error(status);
		return false;
	}
	m_bHasBindingHandle = false;
	return true;
}

// xxx: ugly
DRSRImporter *g_last_importer = nullptr;
void DRSRImporter::RPCCOptSecurityCallback(void *rpcContext)
{
	g_last_importer->RetrieveSessionKey(rpcContext);
}

bool DRSRImporter::OpenDRSConnection(GUID clientDsa)
{
	UpdateStatus("Binding domain replication services connection...", 0);
	m_clientDsa = clientDsa;
	m_serverReplEpoch = s_defaultReplEpoch;

	// Register the RetrieveSessionKey as RCP security callback. Mind the delegate lifecycle.
	g_last_importer = this;
	RPC_STATUS status = RpcBindingSetOption(m_bindingHandle, RPC_C_OPT_SECURITY_CALLBACK, (ULONG_PTR)RPCCOptSecurityCallback);
	if (status != RPC_S_OK)
	{
		m_last_error = rpc_string_error(status);
		return false;
	}

	if (!BindDRSConnection())
	{
		return false;
	}
	if (m_serverReplEpoch != s_defaultReplEpoch)
	{
		// The domain must have been renamed, so we need to rebind with the proper dwReplEpoch.
		ReleaseDRSConnection();
		BindDRSConnection();
	}

	UpdateStatus("Binding successful.", 0);
	return true;
}

bool DRSRImporter::BindDRSConnection()
{
	// Init binding parameters
	DRS_EXTENSIONS_INT clientInfo;
	clientInfo.dwFlags = DRS_EXT::ALL_EXT;
	clientInfo.dwFlagsExt = DRS_EXT2::DRS_EXT_LH_BETA2 | DRS_EXT2::DRS_EXT_RECYCLE_BIN | DRS_EXT2::DRS_EXT_PAM;
	clientInfo.dwExtCaps = DRS_EXT2::DRS_EXT_LH_BETA2 | DRS_EXT2::DRS_EXT_RECYCLE_BIN | DRS_EXT2::DRS_EXT_PAM;
	clientInfo.dwReplEpoch = m_serverReplEpoch;
	
	DRS_EXTENSIONS *genericServerInfo = nullptr;
	m_drsHandle = nullptr;

	// Bind
	ULONG result = IDL_DRSBind_NoSEH(m_bindingHandle, &m_clientDsa, (DRS_EXTENSIONS*)&clientInfo, &genericServerInfo, &m_drsHandle);
	if (result != 0)
	{
		m_last_error = "Failed to bind to directory replication service.";
		return false;
	}

	// Parse the server info
	DRS_EXTENSIONS_INT serverInfo = DRS_EXTENSIONS_INT(genericServerInfo);
	m_serverSiteObjectGuid = serverInfo.siteObjGuid;
	m_serverReplEpoch = serverInfo.dwReplEpoch;
	m_serverCapabilities = serverInfo.dwFlags;

	m_boundDRSConnection = true;

	return true;
}

bool DRSRImporter::ReleaseDRSConnection()
{
	if (!m_boundDRSConnection)
	{
		return true;
	}

	ULONG result = IDL_DRSUnbind_NoSEH(&m_drsHandle);
	// Do not validate result, as we do not want any exceptions in ReleaseHandle
	// Return true if the pointer has been nulled by the UnBind operation
	return m_drsHandle == nullptr;
}

void DRSRImporter::RetrieveSessionKey(void* rpcContext)
{
	// Retrieve RPC Security Context
	PSecHandle securityContext = nullptr;
	RPC_STATUS rpcStatus = I_RpcBindingInqSecurityContext(rpcContext, (void**)&securityContext);
	if (rpcStatus != RPC_S_OK)
	{
		// We could not acquire the security context, so do not continue with session key retrieval
		return;
	}
	// Retrieve the Session Key information from Security Context
	SECURITY_STATUS secStatus = QueryContextAttributes(securityContext, SECPKG_ATTR_SESSION_KEY, &m_sessionKey);
	
	// Extract the actual key if the authentication schema uses one
	if (secStatus != SEC_E_OK)
	{
		// xxx
		return;
	}
}

midl_ptr<DRS_MSG_GETCHGREQ_V10> DRSRImporter::CreateGenericReplicateRequest(midl_ptr<DSNAME> &&dsName, const std::vector<ATTRTYP> &partialAttributeSet, ULONG maxBytes, ULONG maxObjects)
{
	// TODO: Add support for Windows Server 2003
	midl_ptr<DRS_MSG_GETCHGREQ_V10> request = make_midl_ptr<DRS_MSG_GETCHGREQ_V10>();
	
	// Inset client ID:
	request->uuidDsaObjDest = m_clientDsa;
	// Insert DSNAME
	request->pNC = dsName.release(); // Note: Request deleter will also delete DSNAME.

	// Insert PAS:

	//auto nativePas = CreateNativePas(partialAttributeSet);
	//request->pPartialAttrSetEx = nativePas.release(); // Note: Request deleter will also delete PAS.

	// Insert response size limits:
	request->cMaxBytes = maxBytes;
	request->cMaxObjects = maxObjects;
	// Set correct flags:
	// TODO: + DRS_OPTIONS::PER_SYNC ?
	request->ulFlags = DRS_OPTIONS::DRS_INIT_SYNC |
		DRS_OPTIONS::DRS_WRIT_REP |
		DRS_OPTIONS::DRS_NEVER_SYNCED;

	return request;
}

midl_ptr<DRS_MSG_GETCHGREQ_V10> DRSRImporter::CreateReplicateAllRequest(const ReplicationCookie & cookie, const std::vector<ATTRTYP> &partialAttributeSet, ULONG maxBytes, ULONG maxObjects)
{
	ULONG dsnamelen = (ULONG)cookie.NamingContext.size();
	midl_ptr<DSNAME> dsname = make_midl_ptr<DSNAME>(dsnamelen);
	wcscpy_s(&dsname->StringName[0], dsnamelen+1, cookie.NamingContext.c_str());

	midl_ptr<DRS_MSG_GETCHGREQ_V10> request = CreateGenericReplicateRequest(move(dsname), partialAttributeSet, maxBytes, maxObjects);
	if (!request)
	{
		return nullptr;
	}

	// Insert replication state from cookie:
	request->usnvecFrom.usnHighObjUpdate = cookie.HighObjUpdate;
	request->usnvecFrom.usnHighPropUpdate = cookie.HighPropUpdate;
	request->usnvecFrom.usnReserved = cookie.Reserved;
	request->uuidInvocIdSrc = cookie.InvocationId;
	request->ulFlags |= DRS_OPTIONS::DRS_GET_NC_SIZE;

	return request;
}

bool DRSRImporter::CreateNamingContext(std::wstring & naming_context)
{
	std::wstring rootname;
	if (m_bRemote)
	{
		rootname = L"LDAP://";
		rootname += m_strRemoteMachine.toStdWString();
		rootname += L"/rootDSE";
	}
	else
	{	
		rootname = L"LDAP://rootDSE";
	}
	
	IADs *pads;
	HRESULT hr = E_FAIL;
	if (m_bRemote && m_bSpecificUser)
	{
		hr = ADsOpenObject(
			rootname.c_str(),
			m_strUser.toStdWString().c_str(),
			m_strPassword.toStdWString().c_str(),
			ADS_SERVER_BIND,
			IID_IADs,
			(void **)&pads
			);
	} 
	if(!SUCCEEDED(hr))
	{
		hr = ADsGetObject(rootname.c_str(), IID_IADs, (void**)&pads);
	}
	if (SUCCEEDED(hr))
	{
		VARIANT var;
		VariantInit(&var);
		hr = pads->Get(CComBSTR("defaultNamingContext"), &var);
		if (SUCCEEDED(hr))
		{
			if (VT_BSTR == var.vt)
			{
				naming_context = var.bstrVal;
			}

			VariantClear(&var);
		}

		pads->Release();
	}
	if (naming_context.empty())
	{
		if (!m_strDomain.isEmpty())
		{
			for (auto part : m_strDomain.split("."))
			{
				if (!naming_context.empty())
				{
					naming_context += L",";
				}
				naming_context += L"dc=";
				naming_context += part.toStdWString();
			}

			return true;
		}

		m_last_error = "Could not get default naming context.";
		return false;
	}

	return true;
}

midl_ptr<DRS_MSG_GETCHGREPLY> DRSRImporter::GetNCChanges(midl_ptr<DRS_MSG_GETCHGREQ_V10> &&request, DWORD *outVersion)
{
	// 
	DWORD version = 5;
	if (m_serverCapabilities & DRS_EXT::DRS_EXT_GETCHGREQ_V8)
	{
		version = 8;
	}
	if (m_serverCapabilities & DRS_EXT::DRS_EXT_GETCHGREQ_V10)
	{
		version = 10;
	}


	const DWORD inVersion = version;
	*outVersion = 0;

	auto reply = make_midl_ptr<DRS_MSG_GETCHGREPLY>();

	// Send message:
	auto result = IDL_DRSGetNCChanges_NoSEH(m_drsHandle, inVersion, (DRS_MSG_GETCHGREQ*)request.get(), outVersion, (DRS_MSG_GETCHGREPLY*)reply.get());
	if (result)
	{
		// TODO: Test extended error code:
		//DWORD extendedError = reply->dwDRSError;
		m_last_error = QString("Could not get drs nc changes: %1").arg(result);// .arg(extendedError);
		return nullptr;
	}

	return reply;
}

static std::vector<byte> GetByteArrayAttribute(ATTRVALBLOCK & AttrVal)
{
	std::vector<byte> out; 
	ULONG totalsize = 0;
	for (ULONG i = 0; i < AttrVal.valCount; i++)
	{
		totalsize += AttrVal.pAVal[i].valLen;
	}
	out.resize(totalsize);
	ULONG currentsize = 0;
	for (ULONG i = 0; i < AttrVal.valCount; i++)
	{
		memcpy(out.data()+currentsize, (const byte *)AttrVal.pAVal[i].pVal, AttrVal.pAVal[i].valLen);
		currentsize += AttrVal.pAVal[i].valLen;
	}
	return out;
}

static QString GetStringAttribute(ATTRVALBLOCK & AttrVal)
{
	QString out;
	for (ULONG i = 0; i < AttrVal.valCount; i++)
	{
		out += QString::fromUtf16((const ushort *)AttrVal.pAVal[i].pVal, AttrVal.pAVal[i].valLen / sizeof(WCHAR));
	}
	return out;
}

static DWORD GetDWORDAttribute(ATTRVALBLOCK & AttrVal)
{
	if (AttrVal.valCount != 1 || AttrVal.pAVal->valLen!=4)
	{
		Q_ASSERT(0);
		return 0;
	}
	return *(DWORD *)AttrVal.pAVal->pVal;
}

static uint64_t GetUINT64Attribute(ATTRVALBLOCK & AttrVal)
{
	if (AttrVal.valCount != 1 || AttrVal.pAVal->valLen != 8)
	{
		Q_ASSERT(0);
		return 0;
	}
	return *(uint64_t *)AttrVal.pAVal->pVal;
}

#pragma pack(push,1)
struct CryptoBuffer
{
	uint32_t Length;
	uint32_t MaximumLength;
	void *Buffer;
};
#pragma pack(pop)


static std::vector<byte> ComputeMD5(const std::vector<byte> &key, const std::vector<byte> &salt, int saltHashRounds = 1)
{
	MD5_CTX ctx;
	MD5_Init(&ctx);
	std::vector<byte> out(16);

	// Hash key
	MD5_Update(&ctx, key.data(), key.size());

	// Hash salt (saltHashRounds-1) times
	for (int i = 0; i < saltHashRounds; i++)
	{
		MD5_Update(&ctx, salt.data(), salt.size());
	}
	MD5_Final(out.data(), &ctx);

	return out;
}

//extern "C" __declspec(dllimport) NTSTATUS __cdecl SystemFunction033(CryptoBuffer *data, CryptoBuffer *key);
//extern "C" __declspec(dllimport) NTSTATUS __stdcall SystemFunction027(unsigned char *encryptedNtOwfPassword, DWORD *index, unsigned char *ntOwfPassword);

static bool DecryptUsingRC4(std::vector<byte> &out, const std::vector<byte> &data, const std::vector<byte> &salt, const std::vector<byte> &decryptionKey, int saltHashRounds = 1)
{
	std::vector<byte> rc4Key = ComputeMD5(decryptionKey, salt, saltHashRounds);
//	std::vector<byte> decryptedData = data;
	std::vector<byte> decryptedData2 = data;

//	CryptoBuffer cbdata = { (uint32_t)decryptedData.size(), (uint32_t)decryptedData.size(), decryptedData.data() };
//	CryptoBuffer cbkey = { (uint32_t)rc4Key.size(), (uint32_t)rc4Key.size(), rc4Key.data() };
//	if (SystemFunction033(&cbdata, &cbkey) != 0)
//	{
		//return false;
	//}

	rc4_state st;
	rc4_init(&st, rc4Key.data(), rc4Key.size());
	rc4_crypt(&st, decryptedData2.data(), decryptedData2.data(), decryptedData2.size());

	//Q_ASSERT(decryptedData == decryptedData2);
		
	out = decryptedData2;
	return true;
}


bool DRSRImporter::DecryptSecret(const std::vector<byte> &blob_in, std::vector<byte> &blob_out)
{
	if (blob_in.size() < 17)
	{
		m_last_error = "Blob was too small";
		return false;
	}

	// Extract salt and the actual encrypted data from the blob
	std::vector<byte> salt(blob_in.begin(), blob_in.begin() + 16);
	std::vector<byte> encryptedSecret(blob_in.begin() + 16, blob_in.end());

	// Perform decryption
	std::vector<byte> sessionkey(m_sessionKey.SessionKeyLength);
	memcpy(sessionkey.data(), m_sessionKey.SessionKey, m_sessionKey.SessionKeyLength);
	
	std::vector<byte> decryptedBlob;
	if (!DecryptUsingRC4(decryptedBlob, encryptedSecret, salt, sessionkey ))
	{
		m_last_error = "Unable to decrypt hash with RC4";
		return false;
	}
	if (decryptedBlob.size() < 4)
	{
		m_last_error = "Decrypted blob was too small";
		return false;
	}

	// The blob is prepended with CRC
	uint32_t expectedCrc = *(uint32_t *)decryptedBlob.data();
	std::vector<byte> decryptedSecret(decryptedBlob.begin() + 4, decryptedBlob.end());

	if (crc(decryptedSecret) != expectedCrc)
	{
		m_last_error = "Decrypted CRC did not match";
		return false;
	}
	
	blob_out = decryptedSecret;

	return true;

}

bool DRSRImporter::DecryptHash(const std::vector<byte> &enchash, DWORD rid, std::vector<byte> &hash)
{
	std::vector<byte> intermediate;
	if (!DecryptSecret(enchash, intermediate))
	{
		return false;
	}

//	hash.resize(16);
//	bool ret = SystemFunction027(intermediate.data(), &rid, hash.data())==0;
//	return ret;

	hash = intermediate;

	BYTE deskey1[8], deskey2[8];
	rid_to_key1(rid, deskey1);
	rid_to_key2(rid, deskey2);
	
	SLOWDES_CTX fdctx;
	slowdes_setkey(&fdctx, deskey1);
	slowdes_dedes(&fdctx, hash.data());
	slowdes_setkey(&fdctx, deskey2);
	slowdes_dedes(&fdctx, hash.data() + 8);

	return true;
}



bool DRSRImporter::ReadAccounts(const REPLENTINFLIST *objects, int objectCount/*, const REPLVALINF_V3 *linkedValues*/, int valueCount, bool &cancelled)
{
	// Read linked values first
	// TODO: Handle the case when linked attributes of an object are split between several responses.
	/*
	std::map<GUID, REPLVALINF_V3> linkedvaluemap;
	for (int i = 0; i < valueCount; i++)
	{
		auto linkedValue = linkedValues[i];
		if (linkedValue.fIsPresent)
		{
			linkedvaluemap[linkedValue.pObject->Guid] = linkedValue;
		}
	}
	*/
	size_t prev_users_imported = m_users_imported;
	m_accountlist->Acquire();

	m_objects_total += objectCount;

	int nPercentage = 0;
	bool success = true;
	QString error;
	int nMachineAccounts = 0;
	int nUserAccounts = 0;
	int remconfig = -1;
	// Now read the replicated objects
	auto currentObject = objects;
	while (currentObject != nullptr)
	{
		nPercentage = (int)(100.0*(double)(m_objects_processed) / (double)(m_objects_total));

		if (m_ctrl && m_ctrl->StopRequested())
		{
			// endbatch and exit to cleanup, success=false, instead of return false
			cancelled = true;
			break;
		}

		if ((m_objects_processed & 0xF) == 0)
		{
			QString str = QString("%1 of %2 objects processed").arg(m_objects_processed).arg(m_objects_total);
			UpdateStatus(str, nPercentage, false);
		}

		///////////////

		bool skip = false;
		LC7Account acct;
		acct.remediations = -1;
		acct.lastchanged = 0;
		acct.flags = 0;
		if (m_bRemote)
		{
			acct.machine = m_strRemoteMachine;
		}
		QString userinfo_name;
		QString userinfo_description;
		bool is_user = false;
		bool is_computer = false;
		DWORD rid;

		std::vector<byte> encnthash;
		std::vector<byte> enclmhash;

		if (currentObject->Entinf.pName->NameLen > 0)
		{
			QString name=QString::fromWCharArray(currentObject->Entinf.pName->StringName, currentObject->Entinf.pName->NameLen);
			QString domain;
			for (auto part : name.split(","))
			{
				if (part.toLower().startsWith("dc="))
				{
					if (domain.size() > 0)
						domain += ".";
					domain += part.mid(3);
				}
			}
			if (domain.size() > 0)
			{
				acct.domain = domain;
			}
		}
		if (currentObject->Entinf.pName->SidLen >= sizeof(DWORD))
		{
			rid = *(DWORD *)&(currentObject->Entinf.pName->Sid.Data[currentObject->Entinf.pName->SidLen - sizeof(DWORD)]);
			acct.userid = QString("%1").arg(rid);
		}

		ULONG attrcnt = currentObject->Entinf.AttrBlock.attrCount;
		for (ULONG attrnum = 0; attrnum < attrcnt; attrnum++)
		{
			auto attr = currentObject->Entinf.AttrBlock.pAttr[attrnum];
			if (attr.AttrVal.valCount == 0)
				continue;
		
			switch (attr.attrTyp)
			{
			case 13: // description
				{
					userinfo_description = GetStringAttribute(attr.AttrVal);
				}
				break;
			case 131120: // isDeleted
				{
					DWORD isdel = GetDWORDAttribute(attr.AttrVal);
					if (isdel)
					{
						skip = true;
					}
				}
				break;
			case 590045: // sAMAccountName
				{
					acct.username = GetStringAttribute(attr.AttrVal);
				}
				break;
			case 590126:	// sAMAccountType
				{
					DWORD sat = GetDWORDAttribute(attr.AttrVal);
					
					if (sat == 0x30000000)
					{
						is_user = true;
					}
					else if (sat == 0x30000001)
					{
						is_computer = true;
					}
				}
				break;
			case 131085:	// display name
				{
					userinfo_name = GetStringAttribute(attr.AttrVal);
				}
				break;
			case 589832:	// user account control
				{
					DWORD uac = GetDWORDAttribute(attr.AttrVal);
					acct.disabled = !!(uac & UAC_DISABLED);
					acct.mustchange = !!(uac & UAC_PASSWORD_EXPIRED);
					acct.lockedout = !!(uac & UAC_LOCKED_OUT);
					acct.neverexpires = !!(uac & UAC_PASSWORD_NEVER_EXPIRES);

					if (!m_bIncludeMachineAccounts && (!(uac & UAC_NORMAL_ACCOUNT)))
					{
						// skip this account
						skip = true;
					}
				}
				break;
			case 589914: // unicodePwd (nt hash)
				{
					encnthash = GetByteArrayAttribute(attr.AttrVal);
				}
				break;
			case 589879: // dBCSPwd (lm hash)
				{
					enclmhash = GetByteArrayAttribute(attr.AttrVal);
				}
				break;
			case 589920: // PwdLastSet
				{
					uint64_t filetime_lastchanged = GetUINT64Attribute(attr.AttrVal);
					acct.lastchanged = (filetime_lastchanged > 0) ? (filetime_lastchanged / 10000000ULL - 11644473600ULL) : 0;
				}
				break;
			case 589917: // PwdProperties
				{
					DWORD prop = GetDWORDAttribute(attr.AttrVal);
					prop = prop;
				}
				break;
				/* This is account expiration not password expiration
			case 589983: // Expires
				{
					uint64_t filetime_expires = GetUINT64Attribute(attr.AttrVal);
					if (filetime_expires == 0)
					{

					}
					else if (filetime_expires == 0x7FFFFFFFFFFFFFFFULL)
					{

					}
					else
					{
						time_t expiretime = (filetime_expires / 10000000ULL - 11644473600ULL);
						time_t now = QDateTime::currentDateTimeUtc().toTime_t();

						if (expiretime <= now)
						{
							acct.expired = true;
						}
					}
				}
				*/
			}

			
			if (skip)
			{
				break;
			}
		}

		if (!is_user && !is_computer)
		{
			// skip non-user/computers
			skip = true;
		}

		if (!m_bIncludeMachineAccounts && is_computer)
		{
			// skip computers
			skip = true;
		}
		
		if (is_computer)
		{
			nMachineAccounts++;
		}
		if (is_user)
		{
			nUserAccounts++;
		}

		if (!skip)
		{
			// add user info
			acct.userinfo = userinfo_name;
			if (userinfo_description.size() > 0)
			{
				if (acct.userinfo.size() > 0)
				{
					acct.userinfo += " ";
				}
				acct.userinfo += QString("(%1)").arg(userinfo_description);
			}
			
			// add nt hash
			if (encnthash.size() > 0)
			{
				std::vector<byte> nthash;
				if (DecryptHash(encnthash, rid, nthash))
				{
					LC7Hash hash= { FOURCC(HASHTYPE_NT), QByteArray(), QString(), CRACKSTATE_NOT_CRACKED, 0, QString() };
					for (size_t i = 0; i < nthash.size(); i++)
					{
						hash.hash += QString().sprintf("%2.2X", nthash[i]);
					}
					if (hash.hash == "31D6CFE0D16AE931B73C59D7E0C089C0")
					{
						hash.crackstate = CRACKSTATE_CRACKED;
						hash.cracktype = "No Password";
					}
					acct.hashes.append(hash);
				}
				else
				{
					Q_ASSERT(0);
				}
			}
			else
			{
				// No NT hash means empty NT hash in this case
				LC7Hash hash = { FOURCC(HASHTYPE_NT), QByteArray(), QString(), CRACKSTATE_NOT_CRACKED, 0, QString() };
				hash.hash = "31D6CFE0D16AE931B73C59D7E0C089C0";
				hash.crackstate = CRACKSTATE_CRACKED;
				hash.cracktype = "No Password";
				acct.hashes.append(hash);
			}
			
			// add lm hash
			if (enclmhash.size() > 0)
			{
				std::vector<byte> lmhash;
				if (DecryptHash(enclmhash, rid, lmhash))
				{
					LC7Hash hash = { FOURCC(HASHTYPE_LM), QByteArray(), QString(), CRACKSTATE_NOT_CRACKED, 0, QString() };
					for (size_t i = 0; i < lmhash.size(); i++)
					{
						hash.hash += QString().sprintf("%2.2X", lmhash[i]);
					}
					if (hash.hash == "AAD3B435B51404EEAAD3B435B51404EE")
					{
						hash.crackstate = CRACKSTATE_CRACKED;
						hash.cracktype = "No Password";
					}
					else if (hash.hash.startsWith("AAD3B435B51404EE"))
					{
						hash.crackstate = CRACKSTATE_FIRSTHALF_CRACKED;
					}
					else if (hash.hash.endsWith("AAD3B435B51404EE"))
					{
						hash.crackstate = CRACKSTATE_SECONDHALF_CRACKED;
					}

					acct.hashes.append(hash);
				}
				else
				{
					Q_ASSERT(0);
				}
			}

			// Add remediation config
			if (remconfig == -1 && !m_remediations.isEmpty())
			{
				remconfig = m_accountlist->AddRemediations(m_remediations);
			}
			acct.remediations = remconfig;

			if (!m_accountlist->AppendAccount(acct))
			{
				m_last_error = "Account limit reached, upgrade your license to import more accounts.";
				success = false;
				break;
			}

			m_users_imported++;
			if (m_account_limit && m_users_imported >= m_account_limit)
			{
				m_last_error = "Account limit reached, upgrade your license to import more accounts.";
				success = false;
				break;
			}
		}

		m_objects_processed++;
		currentObject = currentObject->pNextEntInf; 
	}

	if (prev_users_imported != m_users_imported)
	{
		QString str = QString("%1 users imported").arg(m_users_imported);
		if (!m_bIncludeMachineAccounts && nMachineAccounts != 0)
		{
			str += QString(" (%1 machine accounts removed)").arg(nMachineAccounts);
		}

		UpdateStatus(str, 100);
	}

	m_accountlist->Release();

	return success;
}

bool DRSRImporter::ConnectTCP()
{
	if (!OpenRPCBindingHandle_TCP())
	{
		return false;
	}

	if (!OpenDRSConnection(s_DcPromoGuid2k3))
	{
		FreeRPCBindingHandle();
		return false;
	}

	return true;
}

bool DRSRImporter::ConnectNP()
{
	if (!OpenRPCBindingHandle_NP())
	{
		return false;
	}

	if (!OpenDRSConnection(s_DcPromoGuid2k3))
	{
		FreeRPCBindingHandle();
		return false;
	}

	return true;
}

bool DRSRImporter::DoImport(QString & error, bool & cancelled)
{

	UpdateStatus("Issuing hash dump request...", 0);

	cancelled = false;
	bool ret = true;
	try
	{
		if (m_bSpecificUser)
		{
			if (!WIN_Impersonate(m_strUser, m_strDomain, m_strPassword, error))
			{
				throw QString("Failed to impersonate user: %1").arg(error);
			}
			m_bImpersonated = true;
		}

		if (!ConnectTCP())
		{
			if (!ConnectNP())
			{
				throw m_last_error;
			}
		}

		// Create replication cookie
		std::wstring naming_context;
		if (!CreateNamingContext(naming_context))
		{
			throw m_last_error;
		}

		ReplicationCookie cookie(naming_context);

		// Loop until done
		UpdateStatus("Importing objects...", 0);
		bool hasMoreData = false;
		do
		{

			// Replicate all objects
			std::vector<ATTRTYP> partialAttributeSet;
			midl_ptr<DRS_MSG_GETCHGREQ_V10> request = CreateReplicateAllRequest(cookie, partialAttributeSet, s_defaultMaxBytes, s_defaultMaxObjects);
			if (!request)
			{
				throw m_last_error;
			}

			DWORD reply_version=0;
			auto reply = GetNCChanges(move(request),&reply_version);
			if (!reply)
			{
				throw m_last_error;
			}
			USN_VECTOR usnTo;
			GUID invocationId;
			
			switch (reply_version)
			{
			case 1:
				{
					if (!ReadAccounts(reply->V1.pObjects, reply->V1.cNumObjects/*, reply->rgValues*/, 0, cancelled))
						throw m_last_error;
					if (cancelled)
						break;
					usnTo = reply->V1.usnvecTo;
					invocationId = reply->V1.uuidInvocIdSrc;
					hasMoreData = reply->V1.fMoreData != 0;
				}
				break;
			case 2:
				{
					throw "unsupported replication reply #2";
				}
				break;
			case 6:
				{
					if (!ReadAccounts(reply->V6.pObjects, reply->V6.cNumObjects/*, reply->rgValues*/, reply->V6.cNumValues, cancelled))
						throw m_last_error;
					if (cancelled)
						break;
					usnTo = reply->V6.usnvecTo;
					invocationId = reply->V6.uuidInvocIdSrc;
					hasMoreData = reply->V6.fMoreData != 0;
				}
				break;
			case 7:
				{
					throw "unsupported replication reply #7";
				}
				break;
			case 9:
				{
					if (!ReadAccounts(reply->V9.pObjects, reply->V9.cNumObjects/*, reply->rgValues*/, reply->V9.cNumValues, cancelled))
						throw m_last_error;
					if (cancelled)
						break;
					usnTo = reply->V9.usnvecTo;
					invocationId = reply->V9.uuidInvocIdSrc;
					hasMoreData = reply->V9.fMoreData != 0;
				}
				break;
			}
	
			cookie = ReplicationCookie(cookie.NamingContext, invocationId, usnTo.usnHighObjUpdate, usnTo.usnHighPropUpdate, usnTo.usnReserved);

		} while (hasMoreData);

		UpdateStatus("Import complete. ", 0);
	}
	catch (QString err)
	{
		m_last_error = err;
		error = err;
		ret = false;
	}

	if (m_sessionKey.SessionKey)
	{
		FreeContextBuffer(m_sessionKey.SessionKey);
	}

	if (m_bHasBindingHandle)
	{
		if (!FreeRPCBindingHandle())
		{
			ret = false;
		}
	}
	if (m_bImpersonated)
	{
		if (!RevertToSelf())
		{
			m_last_error = "Could not revert to self.";
			ret = false;
		}
	}
	return ret;
}

QString DRSRImporter::LastError(void)
{
	return m_last_error;
}

quint32 DRSRImporter::GetNumberOfUsers(void)
{
	TR;
	return m_users_imported;
}



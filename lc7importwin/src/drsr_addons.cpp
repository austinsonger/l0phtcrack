#include "stdafx.h"
#include "drsr_addons.h"


extern "C"
{

#define STATUS_POSSIBLE_DEADLOCK 0xC0000194
#define STATUS_INSTRUCTION_MISALIGNMENT 0xC00000AA
#define STATUS_HANDLE_NOT_CLOSABLE 0xC0000235

int MyRpcExceptionFilter(unsigned long ExceptionCode)
{
	switch (ExceptionCode)
	{
	case STATUS_ACCESS_VIOLATION:
	case STATUS_POSSIBLE_DEADLOCK:
	case STATUS_INSTRUCTION_MISALIGNMENT:
	case STATUS_DATATYPE_MISALIGNMENT:
	case STATUS_PRIVILEGED_INSTRUCTION:
	case STATUS_ILLEGAL_INSTRUCTION:
	case STATUS_BREAKPOINT:
	case STATUS_STACK_OVERFLOW:
	case STATUS_HANDLE_NOT_CLOSABLE:
	case STATUS_IN_PAGE_ERROR:
	case STATUS_ASSERTION_FAILURE:
	case STATUS_STACK_BUFFER_OVERRUN:
	case STATUS_GUARD_PAGE_VIOLATION:
	case STATUS_REG_NAT_CONSUMPTION:
		return EXCEPTION_CONTINUE_SEARCH;
	default:
		break;
	}
	return EXCEPTION_EXECUTE_HANDLER;
}

#define SuppressRpcException(Function,...)								\
	RpcTryExcept										\
		return Function(__VA_ARGS__);					\
	RpcExcept(MyRpcExceptionFilter(RpcExceptionCode()))	\
		return RpcExceptionCode();						\
	RpcEndExcept

ULONG IDL_DRSBind_NoSEH(
	/* [in] */ handle_t rpc_handle,
	/* [unique][in] */ UUID *puuidClientDsa,
	/* [unique][in] */ DRS_EXTENSIONS *pextClient,
	/* [out] */ DRS_EXTENSIONS **ppextServer,
	/* [ref][out] */ DRS_HANDLE *phDrs)
{
	SuppressRpcException(IDL_DRSBind, rpc_handle, puuidClientDsa, pextClient, ppextServer, phDrs)
}

ULONG IDL_DRSGetNCChanges_NoSEH(
	/* [ref][in] */ DRS_HANDLE hDrs,
	/* [in] */ DWORD dwInVersion,
	/* [switch_is][ref][in] */ DRS_MSG_GETCHGREQ *pmsgIn,
	/* [ref][out] */ DWORD *pdwOutVersion,
	/* [switch_is][ref][out] */ DRS_MSG_GETCHGREPLY *pmsgOut)
{
	SuppressRpcException(IDL_DRSGetNCChanges, hDrs, dwInVersion, pmsgIn, pdwOutVersion, pmsgOut)
}

ULONG IDL_DRSCrackNames_NoSEH(
	/* [ref][in] */ DRS_HANDLE hDrs,
	/* [in] */ DWORD dwInVersion,
	/* [switch_is][ref][in] */ DRS_MSG_CRACKREQ *pmsgIn,
	/* [ref][out] */ DWORD *pdwOutVersion,
	/* [switch_is][ref][out] */ DRS_MSG_CRACKREPLY *pmsgOut)
{
	SuppressRpcException(IDL_DRSCrackNames, hDrs, dwInVersion, pmsgIn, pdwOutVersion, pmsgOut)
}

ULONG IDL_DRSGetReplInfo_NoSEH(
	/* [ref][in] */ DRS_HANDLE hDrs,
	/* [in] */ DWORD dwInVersion,
	/* [switch_is][ref][in] */ DRS_MSG_GETREPLINFO_REQ *pmsgIn,
	/* [ref][out] */ DWORD *pdwOutVersion,
	/* [switch_is][ref][out] */ DRS_MSG_GETREPLINFO_REPLY *pmsgOut)
{
	SuppressRpcException(IDL_DRSGetReplInfo, hDrs, dwInVersion, pmsgIn, pdwOutVersion, pmsgOut)
}

ULONG IDL_DRSUnbind_NoSEH(
	/* [ref][out][in] */ DRS_HANDLE *phDrs)
{
	SuppressRpcException(IDL_DRSUnbind, phDrs)
}

DRS_EXTENSIONS_INT::DRS_EXTENSIONS_INT()
{
}

DRS_EXTENSIONS_INT::DRS_EXTENSIONS_INT(DRS_EXTENSIONS *genericExtensions)
{
	if (genericExtensions != 0)
	{
		DWORD numDataBytes = genericExtensions->cb;
		DWORD maxDataBytes = cb;
		DWORD bytesToCopy = std::min(numDataBytes, maxDataBytes) + sizeof(DWORD);
		memcpy(this, genericExtensions, bytesToCopy);
	}
}

}


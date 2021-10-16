#define WIN32_NO_STATUS
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#include <windows.h>
#include <winnt.h>
#include <ntsecapi.h>
#include "_ntdll.h"
#include "_epsapi.h"
#include "_ntstatus.h"

NTSTATUS NTAPI
PsaEnumerateProcessModules(IN HANDLE ProcessHandle,
						   IN PPROCMOD_ENUM_ROUTINE Callback,
						   IN OUT PVOID CallbackContext)
{
	NTSTATUS Status;

	/* current process - use direct memory copy */
	/* FIXME - compare process id instead of a handle */
	if(ProcessHandle == NtCurrentProcess())
	{
		PLIST_ENTRY ListHead, Current;

#if 0
		__try
		{
#endif
			ListHead = &(NtCurrentPeb()->Ldr->InLoadOrderModuleList);
			Current = ListHead->Flink;

			while(Current != ListHead)
			{
				PLDR_DATA_TABLE_ENTRY LoaderModule = CONTAINING_RECORD(Current, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

				/* return the current module to the callback */
				Status = Callback(ProcessHandle, LoaderModule, CallbackContext);

				if(!NT_SUCCESS(Status))
				{
					goto Failure;
				}

				Current = LoaderModule->InLoadOrderLinks.Flink;
			}
#if 0
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			return GetExceptionCode();
		}
#endif
	}
	else
	{
		PROCESS_BASIC_INFORMATION BasicInformation;
		PPEB_LDR_DATA LoaderData;
		LDR_DATA_TABLE_ENTRY LoaderModule;
		PLIST_ENTRY ListHead, Current;

		/* query the process basic information (includes the PEB address) */
		Status = ZwQueryInformationProcess(ProcessHandle,
			ProcessBasicInformation,
			&BasicInformation,
			sizeof(BasicInformation),
			NULL);

		if(!NT_SUCCESS(Status))
		{
			goto Failure;
		}

		/* get the address of the PE Loader data */
		Status = ZwReadVirtualMemory(ProcessHandle,
			&(BasicInformation.PebBaseAddress->Ldr),
			&LoaderData,
			sizeof(LoaderData),
			NULL);

		if(!NT_SUCCESS(Status))
		{
			goto Failure;
		}

		/* head of the module list: the last element in the list will point to this */
		ListHead = &LoaderData->InLoadOrderModuleList;

		/* get the address of the first element in the list */
		Status = ZwReadVirtualMemory(ProcessHandle,
			&(LoaderData->InLoadOrderModuleList.Flink),
			&Current,
			sizeof(Current),
			NULL);

		while(Current != ListHead)
		{
			/* read the current module */
			Status = ZwReadVirtualMemory(ProcessHandle,
				CONTAINING_RECORD(Current, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks),
				&LoaderModule,
				sizeof(LoaderModule),
				NULL);

			if(!NT_SUCCESS(Status))
			{
				goto Failure;
			}

			/* return the current module to the callback */
			Status = Callback(ProcessHandle, &LoaderModule, CallbackContext);

			if(!NT_SUCCESS(Status))
			{
				goto Failure;
			}

			/* address of the next module in the list */
			Current = LoaderModule.InLoadOrderLinks.Flink;
		}
	}

	return STATUS_SUCCESS;

Failure:
	return Status;
}

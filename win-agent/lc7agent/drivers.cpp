#define WIN32_NO_STATUS
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#include <windows.h>
#include <winnt.h>
#include <NTSecAPI.h>
#include "_ntdll.h"
#include "_epsapi.h"
#include "_ntstatus.h"

NTSTATUS NTAPI
PsaEnumerateSystemModules(IN PSYSMOD_ENUM_ROUTINE Callback,
						  IN OUT PVOID CallbackContext)
{
	PRTL_PROCESS_MODULES psmModules;
	NTSTATUS Status = STATUS_SUCCESS;

#if 0
	__try
	{
#else
	do
	{
#endif
		/* capture the system modules */
		Status = PsaCaptureSystemModules(&psmModules);

		if(!NT_SUCCESS(Status))
		{
			break;
		}

		/* walk the system modules */
		Status = PsaWalkSystemModules(psmModules, Callback, CallbackContext);
#if 0
	}
	__finally
	{
#else
	} while(0);
#endif
	/* free the capture */
	PsaFreeCapture(psmModules);
#if 0
}
#endif

return Status;
}

NTSTATUS NTAPI
PsaCaptureSystemModules(OUT PRTL_PROCESS_MODULES *SystemModules)
{
	ULONG nSize = 0;
	PRTL_PROCESS_MODULES psmModules = NULL;
	NTSTATUS Status;

#if 0
	__try
	{
#else
	do
	{
#endif
		/* initial probe. We just get the count of system modules */
		Status = ZwQuerySystemInformation(SystemModuleInformation,
			&nSize,
			sizeof(nSize),
			NULL);

		if(!NT_SUCCESS(Status) && (Status != STATUS_INFO_LENGTH_MISMATCH))
		{
			break;
		}

		/* RATIONALE: the loading of a system module is a rare occurrence. To
		minimize memory operations that could be expensive, or fragment the
		pool/heap, we try to determine the buffer size in advance, knowing that
		the number of elements is unlikely to change */
		nSize = sizeof(RTL_PROCESS_MODULES) +
			(nSize * sizeof(RTL_PROCESS_MODULES));

		psmModules = NULL;

		do
		{
			PVOID pTmp;

			/* free the buffer, and reallocate it to the new size. RATIONALE: since we
			ignore the buffer's content at this point, there's no point in a realloc,
			that could end up copying a large chunk of data we'd discard anyway */
			free(psmModules);
			pTmp = malloc(nSize);

			if(pTmp == NULL)
			{
				Status = STATUS_NO_MEMORY;
				break;
			}

			psmModules = (PRTL_PROCESS_MODULES)pTmp;

			/* query the information */
			Status = ZwQuerySystemInformation(SystemModuleInformation,
				psmModules,
				nSize,
				NULL);

			/* double the buffer for the next loop */
			nSize *= 2;
		} while(Status == STATUS_INFO_LENGTH_MISMATCH);

		if(!NT_SUCCESS(Status))
		{
			break;
		}

		*SystemModules = psmModules;

		Status = STATUS_SUCCESS;
#if 0
	}
	__finally
	{
#else
	} while(0);
#endif
	/* in case of failure, free the buffer */
	if(!NT_SUCCESS(Status))
	{
		free(psmModules);
	}
#if 0
}
#endif

return Status;
}

NTSTATUS NTAPI
PsaWalkSystemModules(IN PRTL_PROCESS_MODULES SystemModules,
					 IN PSYSMOD_ENUM_ROUTINE Callback,
					 IN OUT PVOID CallbackContext)
{
	ULONG i;
	NTSTATUS Status;

	/* repeat until all modules have been returned */
	for(i = 0; i < SystemModules->NumberOfModules; i++)
	{
		/* return current module to the callback */
		Status = Callback(&(SystemModules->Modules[i]), CallbackContext);

		if(!NT_SUCCESS(Status))
		{
			return Status;
		}
	}

	return STATUS_SUCCESS;
}

PRTL_PROCESS_MODULE_INFORMATION FASTCALL
PsaWalkFirstSystemModule(IN PRTL_PROCESS_MODULES SystemModules)
{
	return &(SystemModules->Modules[0]);
}

PRTL_PROCESS_MODULE_INFORMATION FASTCALL
PsaWalkNextSystemModule(IN PRTL_PROCESS_MODULES CurrentSystemModule)
{
	return (PRTL_PROCESS_MODULE_INFORMATION)((ULONG_PTR)CurrentSystemModule +
		(FIELD_OFFSET(RTL_PROCESS_MODULES, Modules[1]) -
		FIELD_OFFSET(RTL_PROCESS_MODULES, Modules[0])));
}

/* EOF */

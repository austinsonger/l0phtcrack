
#ifndef NTDLL_H 
#define NTDLL_H 
#ifdef __cplusplus 
extern "C" { 
#endif 

	//#define DEBUG 
#define NTDLL_WRAPPER 

	//Registry

#ifdef NTDLL_WRAPPER 
#define _NTSYSTEM_ 
#endif 

#define NTAPI __stdcall 
#define FASTCALL _fastcall 

#undef NTSYSAPI 
#undef NTHALAPI 
#define NTSYSAPI 
#define NTHALAPI 

#define STATUS_SUCCESS                          ((NTSTATUS)0x00000000L) 
#define STATUS_SEVERITY_WARNING          0x2 
#define STATUS_SEVERITY_SUCCESS          0x0 
#define STATUS_SEVERITY_INFORMATIONAL    0x1 
#define STATUS_SEVERITY_ERROR            0x3 

	typedef LONG NTSTATUS; 
	typedef LONG KPRIORITY; 
	typedef CHAR SCHAR; 
	typedef SHORT CSHORT; 
	typedef SHORT SSHORT; 
	typedef UCHAR KIRQL; 
	typedef KIRQL *PKIRQL; 
	//typedef ULONG ULONG_PTR, *PULONG_PTR; 
	typedef ULONG_PTR ERESOURCE_THREAD; 
	typedef ULONG KPAGE_FRAME; 
	//typedef ULONG KAFFINITY; 
	//typedef KAFFINITY *PKAFFINITY; 
	typedef LONG KPRIORITY; 
	typedef ULONG_PTR KSPIN_LOCK; 
	typedef KSPIN_LOCK *PKSPIN_LOCK; 
	typedef CHAR *PSZ; 
	typedef CONST char *PCSZ; 
	typedef ULONG KPROCESSOR_MODE; 

#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0) 
#define NT_INFORMATION(Status) ((ULONG)(Status) >> 30 == 1) 
#define NT_WARNING(Status) ((ULONG)(Status) >> 30 == 2) 
#define NT_ERROR(Status) ((ULONG)(Status) >> 30 == 3) 
#define APPLICATION_ERROR_MASK       0x20000000 
#define ERROR_SEVERITY_SUCCESS       0x00000000 
#define ERROR_SEVERITY_INFORMATIONAL 0x40000000 
#define ERROR_SEVERITY_WARNING       0x80000000 
#define ERROR_SEVERITY_ERROR         0xC0000000 
#define STATUS_UNSUCCESSFUL              ((NTSTATUS)0xC0000001L) 
#define STATUS_NOT_IMPLEMENTED           ((NTSTATUS)0xC0000002L) 
#define STATUS_INVALID_INFO_CLASS        ((NTSTATUS)0xC0000003L) 
#define STATUS_INFO_LENGTH_MISMATCH      ((NTSTATUS)0xC0000004L) 
//#define STATUS_INVALID_PARAMETER         ((NTSTATUS)0xC000000DL) 
#define STATUS_NO_SUCH_DEVICE            ((NTSTATUS)0xC000000EL) 
#define STATUS_NO_SUCH_FILE              ((NTSTATUS)0xC000000FL) 
#define STATUS_INVALID_DEVICE_REQUEST    ((NTSTATUS)0xC0000010L) 
#define STATUS_END_OF_FILE               ((NTSTATUS)0xC0000011L) 
#define STATUS_NO_MEDIA_IN_DEVICE        ((NTSTATUS)0xC0000013L) 
#define STATUS_UNRECOGNIZED_MEDIA        ((NTSTATUS)0xC0000014L) 
#define STATUS_MORE_PROCESSING_REQUIRED  ((NTSTATUS)0xC0000016L) 
#define STATUS_ACCESS_DENIED             ((NTSTATUS)0xC0000022L) 
#define STATUS_BUFFER_TOO_SMALL          ((NTSTATUS)0xC0000023L) 
#define STATUS_OBJECT_NAME_NOT_FOUND     ((NTSTATUS)0xC0000034L) 

	typedef enum _EVENT_TYPE 
	{ 
		NotificationEvent, 
		SynchronizationEvent 
	} EVENT_TYPE; 

	typedef enum _TIMER_TYPE 
	{ 
		NotificationTimer, 
		SynchronizationTimer 
	} TIMER_TYPE; 

	typedef enum _WAIT_TYPE 
	{ 
		WaitAll, 
		WaitAny 
	} WAIT_TYPE; 

	/*
	typedef struct _STRING 
	{ 
	USHORT Length; 
	USHORT MaximumLength; 
	PCHAR Buffer; 
	} STRING; 
	*/

	typedef STRING *PSTRING; 
	typedef STRING ANSI_STRING; 
	typedef PSTRING PANSI_STRING; 
	typedef STRING OEM_STRING; 
	typedef PSTRING POEM_STRING; 
	/*
	typedef struct _UNICODE_STRING 
	{ 
	USHORT Length; 
	USHORT MaximumLength; 
	PWSTR  Buffer; 
	} UNICODE_STRING; 
	*/	
	typedef UNICODE_STRING *PUNICODE_STRING; 
#define UNICODE_NULL ((WCHAR)0)  

#if _MSC_VER < 1300 
	typedef union _SLIST_HEADER 
	{ 
		ULONGLONG Alignment; 
		struct 
		{ 
			SINGLE_LIST_ENTRY Next; 
			USHORT Depth; 
			USHORT Sequence; 
		}; 
	} SLIST_HEADER, *PSLIST_HEADER; 
#endif 

#define OBJ_INHERIT             0x00000002L 
#define OBJ_PERMANENT           0x00000010L 
#define OBJ_EXCLUSIVE           0x00000020L 
#define OBJ_CASE_INSENSITIVE    0x00000040L 
#define OBJ_OPENIF              0x00000080L 
#define OBJ_OPENLINK            0x00000100L 
#define OBJ_KERNEL_HANDLE       0x00000200L 
#define OBJ_VALID_ATTRIBUTES    0x000003F2L 

	typedef struct _OBJECT_ATTRIBUTES 
	{ 
		ULONG Length; 
		HANDLE RootDirectory; 
		PUNICODE_STRING ObjectName; 
		ULONG Attributes; 
		SECURITY_DESCRIPTOR *SecurityDescriptor; 
		SECURITY_QUALITY_OF_SERVICE *SecurityQualityOfService; 
	} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES; 

	typedef enum _OBJECT_INFORMATION_CLASS { 
		ObjectBasicInformation, 
		ObjectNameInformation, 
		ObjectTypeInformation, 
		ObjectTypesInformation, 
		ObjectHandleFlagInformation 
	} OBJECT_INFORMATION_CLASS; 

	typedef struct _KSYSTEM_TIME 
	{ 
		ULONG LowPart; 
		LONG High1Time; 
		LONG High2Time; 
	} KSYSTEM_TIME, *PKSYSTEM_TIME; 

	typedef enum _NT_PRODUCT_TYPE 
	{ 
		NtProductWinNt = 1, 
		NtProductLanManNt, 
		NtProductServer 
	} NT_PRODUCT_TYPE, *PNT_PRODUCT_TYPE; 

	typedef enum _ALTERNATIVE_ARCHITECTURE_TYPE 
	{ 
		StandardDesign, 
		NEC98x86, 
		EndAlternatives 
	} ALTERNATIVE_ARCHITECTURE_TYPE; 

#define PROCESSOR_FEATURE_MAX 64 

#define SYSTEM_FLAG_REMOTE_BOOT_CLIENT 0x00000001 
#define SYSTEM_FLAG_DISKLESS_CLIENT    0x00000002 
	typedef struct _KUSER_SHARED_DATA 
	{ 
		volatile ULONG TickCountLow; 
		ULONG TickCountMultiplier; 
		volatile KSYSTEM_TIME InterruptTime; 
		volatile KSYSTEM_TIME SystemTime; 
		volatile KSYSTEM_TIME TimeZoneBias; 
		USHORT ImageNumberLow; 
		USHORT ImageNumberHigh; 
		WCHAR NtSystemRoot[260]; 
		ULONG MaxStackTraceDepth; 
		ULONG CryptoExponent; 
		ULONG TimeZoneId; 
		ULONG Reserved2[8]; 
		NT_PRODUCT_TYPE NtProductType; 
		BOOLEAN ProductTypeIsValid; 
		ULONG NtMajorVersion; 
		ULONG NtMinorVersion; 
		BOOLEAN ProcessorFeatures[PROCESSOR_FEATURE_MAX]; 
		ULONG Reserved1; 
		ULONG Reserved3; 
		volatile ULONG TimeSlip; 
		ALTERNATIVE_ARCHITECTURE_TYPE AlternativeArchitecture; 
		LARGE_INTEGER SystemExpirationDate; 
		ULONG SuiteMask; 
		BOOLEAN KdDebuggerEnabled; 
		volatile ULONG ActiveConsoleId; 
		volatile ULONG DismountCount; 
		ULONG ComPlusPackage; 
		ULONG LastSystemRITEventTickCount; 
		ULONG NumberOfPhysicalPages; 
		BOOLEAN SafeBootMode; 
		ULONG TraceLogging; 
		ULONGLONG Fill; 
		ULONGLONG SystemCall[4]; 
	} KUSER_SHARED_DATA, *PKUSER_SHARED_DATA; 

#define PF_FLOATING_POINT_PRECISION_ERRATA  0 
#define PF_FLOATING_POINT_EMULATED          1 
#define PF_COMPARE_EXCHANGE_DOUBLE          2 
#define PF_MMX_INSTRUCTIONS_AVAILABLE       3 
#define PF_PPC_MOVEMEM_64BIT_OK             4 
#define PF_ALPHA_BYTE_INSTRUCTIONS          5 
#define PF_XMMI_INSTRUCTIONS_AVAILABLE      6 
#define PF_3DNOW_INSTRUCTIONS_AVAILABLE     7 
#define PF_RDTSC_INSTRUCTION_AVAILABLE      8 
#define PF_PAE_ENABLED                      9 

	typedef struct _CLIENT_ID 
	{ 
		HANDLE UniqueProcess; 
		HANDLE UniqueThread; 
	} CLIENT_ID, *PCLIENT_ID; 


	typedef DWORD KWAIT_REASON; 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwSetInformationObject( 
		IN HANDLE Handle, 
		IN OBJECT_INFORMATION_CLASS ObjectInformationClass, 
		IN PVOID ObjectInformation, 
		IN ULONG ObjectInformationLength 
		); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwQueryObject( 
		IN HANDLE Handle, 
		IN OBJECT_INFORMATION_CLASS ObjectInformationClass, 
		OUT PVOID ObjectInformation, 
		IN ULONG Length, 
		OUT PULONG ReturnLength OPTIONAL 
		); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwQuerySecurityObject( 
		IN HANDLE Handle, 
		IN SECURITY_INFORMATION SecurityInformation, 
		OUT PSECURITY_DESCRIPTOR SecurityDescriptor, 
		IN ULONG Length, 
		OUT PULONG LengthNeeded 
		); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwQueryDirectoryObject( 
		IN HANDLE DirectoryHandle, 
		OUT PVOID Buffer, 
		IN ULONG Length, 
		IN BOOLEAN ReturnSingleEntry, 
		IN BOOLEAN RestartScan, 
		IN OUT PULONG Context, 
		OUT PULONG ReturnLength OPTIONAL 
		); 

	typedef enum _SYSTEM_INFORMATION_CLASS { 
		SystemBasicInformation, // 0 
		SystemProcessorInformation, // 1 
		SystemPerformanceInformation, // 2 
		SystemTimeOfDayInformation, // 3 
		SystemPathInformation, // 4 
		SystemProcessInformation, // 5 
		SystemCallCountInformation, // 6 
		SystemDeviceInformation, // 7 
		SystemProcessorPerformanceInformation, // 8 
		SystemFlagsInformation, // 9 
		SystemCallTimeInformation, // 10 
		SystemModuleInformation, // 11 
		SystemLocksInformation, // 12 
		SystemStackTraceInformation, // 13 
		SystemPagedPoolInformation, // 14 
		SystemNonPagedPoolInformation, // 15 
		SystemHandleInformation, // 16 
		SystemObjectInformation, // 17 
		SystemPageFileInformation, // 18 
		SystemVdmInstemulInformation, // 19 
		SystemVdmBopInformation, // 20 
		SystemFileCacheInformation, // 21 
		SystemPoolTagInformation, // 22 
		SystemInterruptInformation, // 23 
		SystemDpcBehaviorInformation, // 24 
		SystemFullMemoryInformation, // 25 
		SystemLoadGdiDriverInformation, // 26 
		SystemUnloadGdiDriverInformation, // 27 
		SystemTimeAdjustmentInformation, // 28 
		SystemSummaryMemoryInformation, // 29 
		SystemNextEventIdInformation, // 30 
		SystemEventIdsInformation, // 31 
		SystemCrashDumpInformation, // 32 
		SystemExceptionInformation, // 33 
		SystemCrashDumpStateInformation, // 34 
		SystemKernelDebuggerInformation, // 35 
		SystemContextSwitchInformation, // 36 
		SystemRegistryQuotaInformation, // 37 
		SystemExtendServiceTableInformation, // 38 
		SystemPrioritySeperation, // 39 
		SystemPlugPlayBusInformation, // 40 
		SystemDockInformation, // 41 
		SystemPwrInformation, // 42 
		SystemProcessorSpeedInformation, // 43 
		SystemCurrentTimeZoneInformation, // 44 
		SystemLookasideInformation // 45 
	} SYSTEM_INFORMATION_CLASS; 

#define SystemProcessesAndThreadsInformation SystemProcessInformation 
#define SystemLoadImage SystemLoadGdiDriverInformation 
#define SystemUnloadImage SystemUnloadGdiDriverInformation 
#define SystemLoadAndCallImage SystemExtendServiceTableInformation 

#if _MSC_VER < 1300 
	typedef struct _IO_COUNTERS 
	{ 
		ULONGLONG  ReadOperationCount; 
		ULONGLONG  WriteOperationCount; 
		ULONGLONG  OtherOperationCount; 
		ULONGLONG ReadTransferCount; 
		ULONGLONG WriteTransferCount; 
		ULONGLONG OtherTransferCount; 
	} IO_COUNTERS; 
#endif 

	typedef struct _VM_COUNTERS 
	{ 
		SIZE_T PeakVirtualSize; 
		SIZE_T VirtualSize; 
		ULONG PageFaultCount; 
		SIZE_T PeakWorkingSetSize; 
		SIZE_T WorkingSetSize; 
		SIZE_T QuotaPeakPagedPoolUsage; 
		SIZE_T QuotaPagedPoolUsage; 
		SIZE_T QuotaPeakNonPagedPoolUsage; 
		SIZE_T QuotaNonPagedPoolUsage; 
		SIZE_T PagefileUsage; 
		SIZE_T PeakPagefileUsage; 
	} VM_COUNTERS; 


	typedef struct _SYSTEM_THREAD_INFORMATION {
		ULONGLONG KernelTime;
		ULONGLONG UserTime;
		ULONGLONG CreateTime;
		ULONG WaitTime;
		ULONG Reserved1;
		PVOID StartAddress;
		CLIENT_ID ClientId;
		KPRIORITY Priority;
		LONG BasePriority;
		ULONG ContextSwitchCount;
		ULONG State;
		KWAIT_REASON WaitReason;
	} SYSTEM_THREAD_INFORMATION, *PSYSTEM_THREAD_INFORMATION;

	typedef struct _SYSTEM_PROCESS_INFORMATION {
		ULONG NextEntryOffset;
		ULONG NumberOfThreads;
		ULONGLONG Reserved[3];
		ULONGLONG CreateTime;
		ULONGLONG UserTime;
		ULONGLONG KernelTime;
		UNICODE_STRING ImageName;
		KPRIORITY BasePriority;
		HANDLE ProcessId;
		HANDLE InheritedFromProcessId;
		ULONG HandleCount;
		ULONG Reserved2[2];
		ULONG PrivatePageCount;  // Garbage
		VM_COUNTERS VirtualMemoryCounters;
		IO_COUNTERS IoCounters;
		SYSTEM_THREAD_INFORMATION Threads[1];
	} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;

	typedef struct _SYSTEM_FLAGS_INFORMATION 
	{ 
		ULONG Flags; 
	} SYSTEM_FLAGS_INFORMATION, *PSYSTEM_FLAGS_INFORMATION; 

	typedef struct _PROCESS_BASIC_INFORMATION 
	{ 
		NTSTATUS ExitStatus; 
		struct _PEB *PebBaseAddress; 
		ULONG_PTR AffinityMask; 
		KPRIORITY BasePriority; 
		ULONG_PTR UniqueProcessId; 
		ULONG_PTR InheritedFromUniqueProcessId; 
	} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION; 

	typedef enum _PROCESSINFOCLASS 
	{ 
		ProcessBasicInformation, 
		ProcessQuotaLimits, 
		ProcessIoCounters, 
		ProcessVmCounters, 
		ProcessTimes, 
		ProcessBasePriority, 
		ProcessRaisePriority, 
		ProcessDebugPort, 
		ProcessExceptionPort, 
		ProcessAccessToken, 
		ProcessLdtInformation, 
		ProcessLdtSize, 
		ProcessDefaultHardErrorMode, 
		ProcessIoPortHandlers, 
		ProcessPooledUsageAndLimits, 
		ProcessWorkingSetWatch, 
		ProcessUserModeIOPL, 
		ProcessEnableAlignmentFaultFixup, 
		ProcessPriorityClass, 
		ProcessWx86Information, 
		ProcessHandleCount, 
		ProcessAffinityMask, 
		ProcessPriorityBoost, 
		ProcessDeviceMap, 
		ProcessSessionInformation, 
		ProcessForegroundInformation, 
		ProcessWow64Information, 
		MaxProcessInfoClass 
	} PROCESSINFOCLASS; 

	typedef struct _RTL_PROCESS_MODULE_INFORMATION 
	{ 
		HANDLE Section; 
		PVOID MappedBase; 
		PVOID ImageBase; 
		ULONG ImageSize; 
		ULONG Flags; 
		USHORT LoadOrderIndex; 
		USHORT InitOrderIndex; 
		USHORT LoadCount; 
		USHORT OffsetToFileName; 
		UCHAR  FullPathName[256]; 
	} RTL_PROCESS_MODULE_INFORMATION, *PRTL_PROCESS_MODULE_INFORMATION, PROCESS_MODULE, *PPROCESS_MODULE, SYSTEM_MODULE, *PSYSTEM_MODULE; 

	typedef struct _RTL_HEAP_TAG 
	{ 
		ULONG NumberOfAllocations; 
		ULONG NumberOfFrees; 
		ULONG BytesAllocated; 
		USHORT TagIndex; 
		USHORT CreatorBackTraceIndex; 
		WCHAR TagName[24]; 
	} RTL_HEAP_TAG, *PRTL_HEAP_TAG; 

	typedef struct _RTL_HEAP_ENTRY 
	{ 
		ULONG Size; 
		USHORT Flags; 
		USHORT AllocatorBackTraceIndex; 
		union 
		{ 
			struct 
			{ 
				ULONG Settable; 
				ULONG Tag; 
			} s1; 
			struct 
			{ 
				ULONG CommittedSize; 
				PVOID FirstBlock; 
			} s2; 
		} u; 
	} RTL_HEAP_ENTRY, *PRTL_HEAP_ENTRY; 


	typedef struct _RTL_HEAP_INFORMATION 
	{ 
		PVOID BaseAddress; 
		ULONG Flags; 
		USHORT EntryOverhead; 
		USHORT CreatorBackTraceIndex; 
		ULONG BytesAllocated; 
		ULONG BytesCommitted; 
		ULONG NumberOfTags; 
		ULONG NumberOfEntries; 
		ULONG NumberOfPseudoTags; 
		ULONG PseudoTagGranularity; 
		ULONG Reserved[5]; 
		PRTL_HEAP_TAG Tags; 
		PRTL_HEAP_ENTRY Entries; 
	} RTL_HEAP_INFORMATION, *PRTL_HEAP_INFORMATION; 

	typedef struct _RTL_PROCESS_HEAPS 
	{ 
		ULONG NumberOfHeaps; 
		RTL_HEAP_INFORMATION Heaps[1]; 
	} RTL_PROCESS_HEAPS, *PRTL_PROCESS_HEAPS; 

	typedef struct _RTL_PROCESS_MODULES 
	{ 
		ULONG NumberOfModules; 
		RTL_PROCESS_MODULE_INFORMATION Modules[1]; 
	} RTL_PROCESS_MODULES, *PRTL_PROCESS_MODULES, SYSTEM_MODULES, *PSYSTEM_MODULES; 

	typedef struct _PROCESS_ACCESS_TOKEN { 
		HANDLE Token; 
		HANDLE Thread; 
	} PROCESS_ACCESS_TOKEN, *PPROCESS_ACCESS_TOKEN; 

	typedef struct _PEB_LDR_DATA 
	{ 
		ULONG Length; 
		BOOLEAN Initialized; 
		HANDLE SsHandle; 
		LIST_ENTRY InLoadOrderModuleList; 
		LIST_ENTRY InMemoryOrderModuleList; 
		LIST_ENTRY InInitializationOrderModuleList; 
	} PEB_LDR_DATA, *PPEB_LDR_DATA; 

	typedef struct _LDR_DATA_TABLE_ENTRY 
	{ 
		LIST_ENTRY InLoadOrderLinks; 
		LIST_ENTRY InMemoryOrderLinks; 
		LIST_ENTRY InInitializationOrderLinks; 
		PVOID DllBase; 
		PVOID EntryPoint; 
		ULONG SizeOfImage; 
		UNICODE_STRING FullDllName; 
		UNICODE_STRING BaseDllName; 
		ULONG Flags; 
		USHORT LoadCount; 
		USHORT TlsIndex; 
		union 
		{ 
			LIST_ENTRY HashLinks; 
			struct 
			{ 
				PVOID SectionPointer; 
				ULONG CheckSum; 
			}; 
		}; 
		ULONG   TimeDateStamp; 
	} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY; 

	typedef struct _PEB 
	{ 
		BOOLEAN InheritedAddressSpace; 
		BOOLEAN ReadImageFileExecOptions; 
		BOOLEAN BeingDebugged; 
		BOOLEAN Unused; 
		HANDLE Mutant; 
		PVOID ImageBaseAddress; 
		PPEB_LDR_DATA Ldr; 
		struct _RTL_USER_PROCESS_PARAMETERS *ProcessParameters; 
	} PEB, *PPEB; 

	typedef struct _IO_STATUS_BLOCK 
	{ 
		union 
		{ 
			NTSTATUS Status; 
			PVOID Pointer; 
		}; 
		ULONG_PTR Information; 
	} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK; 

#ifndef PIO_APC_ROUTINE_DEFINED 
	typedef 
		VOID 
		(NTAPI *PIO_APC_ROUTINE) ( 
		IN PVOID ApcContext, 
		IN PIO_STATUS_BLOCK IoStatusBlock, 
		IN ULONG Reserved 
		); 
#define PIO_APC_ROUTINE_DEFINED 
#endif 


	typedef 
		VOID 
		(NTAPI *PIO_APC_ROUTINE) ( 
		IN PVOID ApcContext, 
		IN PIO_STATUS_BLOCK IoStatusBlock, 
		IN ULONG Reserved 
		); 


	typedef enum _FILE_INFORMATION_CLASS { 
		// end_wdm 
		FileDirectoryInformation         = 1, 
		FileFullDirectoryInformation,   // 2 
		FileBothDirectoryInformation,   // 3 
		FileBasicInformation,           // 4  wdm 
		FileStandardInformation,        // 5  wdm 
		FileInternalInformation,        // 6 
		FileEaInformation,              // 7 
		FileAccessInformation,          // 8 
		FileNameInformation,            // 9 
		FileRenameInformation,          // 10 
		FileLinkInformation,            // 11 
		FileNamesInformation,           // 12 
		FileDispositionInformation,     // 13 
		FilePositionInformation,        // 14 wdm 
		FileFullEaInformation,          // 15 
		FileModeInformation,            // 16 
		FileAlignmentInformation,       // 17 
		FileAllInformation,             // 18 
		FileAllocationInformation,      // 19 
		FileEndOfFileInformation,       // 20 wdm 
		FileAlternateNameInformation,   // 21 
		FileStreamInformation,          // 22 
		FilePipeInformation,            // 23 
		FilePipeLocalInformation,       // 24 
		FilePipeRemoteInformation,      // 25 
		FileMailslotQueryInformation,   // 26 
		FileMailslotSetInformation,     // 27 
		FileCompressionInformation,     // 28 
		FileObjectIdInformation,        // 29 
		FileCompletionInformation,      // 30 
		FileMoveClusterInformation,     // 31 
		FileQuotaInformation,           // 32 
		FileReparsePointInformation,    // 33 
		FileNetworkOpenInformation,     // 34 
		FileAttributeTagInformation,    // 35 
		FileTrackingInformation,        // 36 
		FileIdBothDirectoryInformation, // 37 
		FileIdFullDirectoryInformation, // 38 
		FileValidDataLengthInformation, // 39 
		FileShortNameInformation,       // 40 
		FileMaximumInformation 
		// begin_wdm 
	} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS; 



	typedef struct _FILE_DIRECTORY_INFORMATION {  
		ULONG NextEntryOffset; 
		ULONG Unknown; 
		LARGE_INTEGER CreationTime; 
		LARGE_INTEGER LastAccessTime; 
		LARGE_INTEGER LastWriteTime; 
		LARGE_INTEGER ChangeTime; 
		LARGE_INTEGER EndOfFile; 
		LARGE_INTEGER AllocationSize;  
		ULONG FileAttributes; 
		ULONG FileNameLength; 
		WCHAR FileName[1]; 
	} FILE_DIRECTORY_INFORMATION, *PFILE_DIRECTORY_INFORMATION; 

	typedef struct _FILE_FULL_DIRECTORY_INFORMATION { 
		ULONG NextEntryOffset; 
		ULONG Unknown; 
		LARGE_INTEGER CreationTime; 
		LARGE_INTEGER LastAccessTime; 
		LARGE_INTEGER LastWriteTime; 
		LARGE_INTEGER ChangeTime; 
		LARGE_INTEGER EndOfFile; 
		LARGE_INTEGER AllocationSize; 
		ULONG FileAttributes; 
		ULONG FileNameLength; 
		ULONG EaInformationLength; 
		WCHAR FileName[1]; 
	} FILE_FULL_DIRECTORY_INFORMATION, *PFILE_FULL_DIRECTORY_INFORMATION; 


	typedef struct _FILE_BOTH_DIRECTORY_INFORMATION {  
		ULONG NextEntryOffset; 
		ULONG Unknown; 
		LARGE_INTEGER CreationTime; 
		LARGE_INTEGER LastAccessTime; 
		LARGE_INTEGER LastWriteTime; 
		LARGE_INTEGER ChangeTime; 
		LARGE_INTEGER EndOfFile; 
		LARGE_INTEGER AllocationSize; 
		ULONG FileAttributes; 
		ULONG FileNameLength; 
		ULONG EaInformationLength; 
		UCHAR AlternateNameLength; 
		WCHAR AlternateName[12]; 
		WCHAR FileName[1]; 
	} FILE_BOTH_DIRECTORY_INFORMATION, *PFILE_BOTH_DIRECTORY_INFORMATION;  

	typedef struct _FILE_NAMES_INFORMATION { 
		ULONG NextEntryOffset; 
		ULONG Unknown; 
		ULONG FileNameLength; 
		WCHAR FileName[1]; 
	} FILE_NAMES_INFORMATION, *PFILE_NAMES_INFORMATION; 



	typedef UNICODE_STRING *PUNICODE_STRING; 
	typedef const UNICODE_STRING *PCUNICODE_STRING; 


	typedef struct _RTL_DEBUG_INFORMATION 
	{ 
		HANDLE SectionHandleClient; 
		PVOID ViewBaseClient; 
		PVOID ViewBaseTarget; 
		ULONG ViewBaseDelta; 
		HANDLE EventPairClient; 
		HANDLE EventPairTarget; 
		HANDLE TargetProcessId; 
		HANDLE TargetThreadHandle; 
		ULONG Flags; 
		ULONG OffsetFree; 
		ULONG CommitSize; 
		ULONG ViewSize; 
		struct _RTL_PROCESS_MODULES *Modules; 
		struct _RTL_PROCESS_BACKTRACES *BackTraces; 
		struct _RTL_PROCESS_HEAPS *Heaps; 
		struct _RTL_PROCESS_LOCKS *Locks; 
		PVOID SpecificHeap; 
		HANDLE TargetProcessHandle; 
		PVOID Reserved[6]; 
	} RTL_DEBUG_INFORMATION, *PRTL_DEBUG_INFORMATION; 

	typedef ERESOURCE_THREAD *PERESOURCE_THREAD; 

	typedef struct _PROTOTYPE_PTE_ENTRY 
	{ 
		ULONG Present : 1; 
		ULONG AddressLow : 7; 
		ULONG ReadOnly : 1; 
		ULONG WhichPool : 1; 
		ULONG Prototype : 1; 
		ULONG AddressHigh : 21; 
	} PROTOTYPE_PTE_ENTRY; 

	typedef struct _GDT_ENTRY 
	{ 
		USHORT  LimitLow; 
		USHORT  BaseLow; 
		union { 
			struct { 
				UCHAR   BaseMid; 
				UCHAR   Flags1; 
				UCHAR   Flags2; 
				UCHAR   BaseHi; 
			} Bytes; 
			struct { 
				ULONG   BaseMid : 8; 
				ULONG   Type : 5; 
				ULONG   Dpl : 2; 
				ULONG   Pres : 1; 
				ULONG   LimitHi : 4; 
				ULONG   Sys : 1; 
				ULONG   Reserved_0 : 1; 
				ULONG   Default_Big : 1; 
				ULONG   Granularity : 1; 
				ULONG   BaseHi : 8; 
			} Bits; 
		} HighWord; 
	} GDT_ENTRY; 

	typedef struct _IO_ACCESS_MAP 
	{ 
		UCHAR DirectionMap[32]; 
		UCHAR IoMap[8196]; 
	} IO_ACCESS_MAP; 

#define MIN_TSS_SIZE FIELD_OFFSET(TSS_ENTRY, IoMaps) 
	typedef struct _TSS_ENTRY 
	{ 
		USHORT  Backlink; 
		USHORT  Reserved0; 
		ULONG   Esp0; 
		USHORT  Ss0; 
		USHORT  Reserved1; 
		ULONG   NotUsed1[4]; 
		ULONG   CR3; 
		ULONG   Eip; 
		ULONG   NotUsed2[9]; 
		USHORT  Es; 
		USHORT  Reserved2; 
		USHORT  Cs; 
		USHORT  Reserved3; 
		USHORT  Ss; 
		USHORT  Reserved4; 
		USHORT  Ds; 
		USHORT  Reserved5; 
		USHORT  Fs; 
		USHORT  Reserved6; 
		USHORT  Gs; 
		USHORT  Reserved7; 
		USHORT  LDT; 
		USHORT  Reserved8; 
		USHORT  Flags; 
		USHORT  IoMapBase; 
		IO_ACCESS_MAP IoMaps[1]; 
		UCHAR IntDirectionMap[32]; 
	} TSS_ENTRY; 

	typedef struct _TSS16_ENTRY 
	{ 
		USHORT  Backlink; 
		USHORT  Sp0; 
		USHORT  Ss0; 
		USHORT  Sp1; 
		USHORT  Ss1; 
		USHORT  Sp2; 
		USHORT  Ss3; 
		USHORT  Ip; 
		USHORT  Flags; 
		USHORT  Ax; 
		USHORT  Cx; 
		USHORT  Dx; 
		USHORT  Bx; 
		USHORT  Sp; 
		USHORT  Bp; 
		USHORT  Si; 
		USHORT  Di; 
		USHORT  Es; 
		USHORT  Cs; 
		USHORT  Ss; 
		USHORT  Ds; 
		USHORT  LDT; 
	} TSS16_ENTRY; 

	typedef struct _KSERVICE_TABLE_DESCRIPTOR 
	{ 
		PULONG_PTR Base; 
		PULONG Count; 
		ULONG Limit; 
		PUCHAR Number; 
	} KSERVICE_TABLE_DESCRIPTOR, *PKSERVICE_TABLE_DESCRIPTOR; 


	//////////////////////////////////////////////////////////////////////////////// 
	// Function prototypes 
	//////////////////////////////////////////////////////////////////////////////// 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwSetSystemInformation ( 
		IN SYSTEM_INFORMATION_CLASS SystemInformationClass, 
		IN PVOID SystemInformation, 
		IN ULONG SystemInformationLength 
		); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwQuerySystemInformation( 
		IN SYSTEM_INFORMATION_CLASS SystemInformationClass,  
		IN OUT PVOID SystemInformation,  
		IN ULONG SystemInformationLength,  
		OUT PULONG ReturnLength 
		); 


	NTSYSAPI 
		NTSTATUS  
		NTAPI ZwQueryDirectoryFile( 
		IN HANDLE FileHandle, 
		IN HANDLE Event OPTIONAL, 
		IN PIO_APC_ROUTINE ApcRoutine OPTIONAL, 
		IN PVOID ApcContext OPTIONAL, 
		OUT PIO_STATUS_BLOCK IoStatusBlock, 
		OUT PVOID FileInformation, 
		IN ULONG FileInformationLength, 
		IN FILE_INFORMATION_CLASS FileInformationClass, 
		IN BOOLEAN ReturnSingleEntry, 
		IN PUNICODE_STRING FileName OPTIONAL, 
		IN BOOLEAN RestartScan 
		); 

	// 
	// LPC 
	// 

#define LPC_REQUEST             1 
#define LPC_REPLY               2 
#define LPC_DATAGRAM            3 
#define LPC_LOST_REPLY          4 
#define LPC_PORT_CLOSED         5 
#define LPC_CLIENT_DIED         6 
#define LPC_EXCEPTION           7 
#define LPC_DEBUG_EVENT         8 
#define LPC_ERROR_EVENT         9 
#define LPC_CONNECTION_REQUEST 10 

	typedef struct _PORT_MESSAGE { 
		union { 
			struct { 
				CSHORT DataLength; 
				CSHORT TotalLength; 
			} s1; 
			ULONG Length; 
		} u1; 
		union { 
			struct { 
				CSHORT Type; 
				CSHORT DataInfoOffset; 
			} s2; 
			ULONG ZeroInit; 
		} u2; 
		union { 
			CLIENT_ID ClientId; 
			double DoNotUseThisField; 
		}; 
		ULONG MessageId; 
		union { 
			SIZE_T ClientViewSize; 
			ULONG CallbackId; 
		}; 
		//  UCHAR Data[]; 
	} PORT_MESSAGE, *PPORT_MESSAGE; 

	typedef struct _PORT_VIEW { 
		ULONG Length; 
		HANDLE SectionHandle; 
		ULONG SectionOffset; 
		SIZE_T ViewSize; 
		PVOID ViewBase; 
		PVOID ViewRemoteBase; 
	} PORT_VIEW, *PPORT_VIEW; 

	typedef struct _REMOTE_PORT_VIEW { 
		ULONG Length; 
		SIZE_T ViewSize; 
		PVOID ViewBase; 
	} REMOTE_PORT_VIEW, *PREMOTE_PORT_VIEW; 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwCreatePort( 
		OUT PHANDLE PortHandle, 
		IN POBJECT_ATTRIBUTES ObjectAttributes, 
		IN ULONG MaxConnectionInfoLength, 
		IN ULONG MaxMessageLength, 
		IN ULONG MaxPoolUsage 
		); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwConnectPort( 
		OUT PHANDLE PortHandle, 
		IN PUNICODE_STRING PortName, 
		IN PSECURITY_QUALITY_OF_SERVICE SecurityQos, 
		IN OUT PPORT_VIEW ClientView OPTIONAL, 
		IN OUT PREMOTE_PORT_VIEW ServerView OPTIONAL, 
		OUT PULONG MaxMessageLength OPTIONAL, 
		IN OUT PVOID ConnectionInformation OPTIONAL, 
		IN OUT PULONG ConnectionInformationLength OPTIONAL 
		); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwSecureConnectPort( 
		OUT PHANDLE PortHandle, 
		IN PUNICODE_STRING PortName, 
		IN PSECURITY_QUALITY_OF_SERVICE SecurityQos, 
		IN OUT PPORT_VIEW ClientView OPTIONAL, 
		IN PSID RequiredServerSid, 
		OUT PREMOTE_PORT_VIEW ServerView OPTIONAL, 
		OUT PULONG MaxMessageLength OPTIONAL, 
		IN OUT PVOID ConnectionInformation OPTIONAL, 
		IN OUT PULONG ConnectionInformationLength OPTIONAL 
		); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwListenPort( 
		IN HANDLE PortHandle, 
		OUT PPORT_MESSAGE ConnectionRequest 
		); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwAcceptConnectPort( 
		OUT PHANDLE PortHandle, 
		IN PVOID PortContext, 
		IN PPORT_MESSAGE ConnectionRequest, 
		IN BOOLEAN AcceptConnection, 
		IN OUT PPORT_VIEW ServerView OPTIONAL, 
		OUT PREMOTE_PORT_VIEW ClientView OPTIONAL 
		); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwCompleteConnectPort( 
		IN HANDLE PortHandle 
		); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwRequestPort( 
		IN HANDLE PortHandle, 
		IN PPORT_MESSAGE RequestMessage 
		); 


	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwRequestWaitReplyPort( 
		IN HANDLE PortHandle, 
		IN PPORT_MESSAGE RequestMessage, 
		OUT PPORT_MESSAGE ReplyMessage 
		); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwReplyPort( 
		IN HANDLE PortHandle, 
		IN PPORT_MESSAGE ReplyMessage 
		); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwReplyWaitReplyPort( 
		IN HANDLE PortHandle, 
		IN OUT PPORT_MESSAGE ReplyMessage 
		); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwReplyWaitReceivePort( 
		IN HANDLE PortHandle, 
		OUT PVOID *PortContext OPTIONAL, 
		IN PPORT_MESSAGE ReplyMessage OPTIONAL, 
		OUT PPORT_MESSAGE ReceiveMessage 
		); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwImpersonateClientOfPort( 
		IN HANDLE PortHandle, 
		IN PPORT_MESSAGE Message 
		); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwReadRequestData( 
		IN HANDLE PortHandle, 
		IN PPORT_MESSAGE Message, 
		IN ULONG DataEntryIndex, 
		OUT PVOID Buffer, 
		IN ULONG BufferSize, 
		OUT PULONG NumberOfBytesRead OPTIONAL 
		); 


	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwWriteRequestData( 
		IN HANDLE PortHandle, 
		IN PPORT_MESSAGE Message, 
		IN ULONG DataEntryIndex, 
		IN PVOID Buffer, 
		IN ULONG BufferSize, 
		OUT PULONG NumberOfBytesWritten OPTIONAL 
		); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwCreateProcess( 
		OUT PHANDLE ProcessHandle, 
		IN ACCESS_MASK DesiredAccess, 
		IN POBJECT_ATTRIBUTES ObjectAttributes, 
		IN HANDLE InheritFromProcessHandle, 
		IN BOOLEAN InheritHandles, 
		IN HANDLE SectionHandle OPTIONAL, 
		IN HANDLE DebugPort OPTIONAL, 
		IN HANDLE ExceptionPort OPTIONAL	 
		); 
	typedef struct _TEB 
	{ 
		NT_TIB NtTib; 
		PVOID  EnvironmentPointer; 
		CLIENT_ID ClientId; 
		PVOID ActiveRpcHandle; 
		PVOID ThreadLocalStoragePointer; 
		struct _PEB *ProcessEnvironmentBlock; 
	} TEB, *PTEB; 

	typedef struct _THREAD_BASIC_INFORMATION { 
		NTSTATUS ExitStatus; 
		PTEB TebBaseAddress; 
		CLIENT_ID ClientId; 
		KAFFINITY AffinityMask; 
		KPRIORITY Priority; 
		LONG BasePriority; 
	} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION; 

	typedef enum _THREADINFOCLASS 
	{ 
		ThreadBasicInformation, 
		ThreadTimes, 
		ThreadPriority, 
		ThreadBasePriority, 
		ThreadAffinityMask, 
		ThreadImpersonationToken, 
		ThreadDescriptorTableEntry, 
		ThreadEnableAlignmentFaultFixup, 
		ThreadEventPair_Reusable, 
		ThreadQuerySetWin32StartAddress, 
		ThreadZeroTlsCell, 
		ThreadPerformanceCount, 
		ThreadAmILastThread, 
		ThreadIdealProcessor, 
		ThreadPriorityBoost, 
		ThreadSetTlsArrayAddress, 
		ThreadIsIoPending, 
		ThreadHideFromDebugger, 
		MaxThreadInfoClass 
	} THREADINFOCLASS; 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwSetInformationProcess( 
		IN HANDLE	ProcessHandle, 
		IN PROCESSINFOCLASS ProcessInformationClass, 
		IN PVOID	ProcessInformation, 
		IN ULONG ProcessInformationLength); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwQueryInformationProcess( 
		IN HANDLE ProcessHandle,  
		IN PROCESSINFOCLASS ProcessInformationClass,  
		OUT PVOID ProcessInformation,  
		IN ULONG ProcessInformationLength,  
		OUT PULONG ReturnLength OPTIONAL); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwSetInformationThread( 
		IN HANDLE ThreadHandle, 
		IN THREADINFOCLASS ThreadInformationClass, 
		IN PVOID ThreadInformation, 
		IN ULONG ThreadInformationLength 
		); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwQueryInformationThread( 
		IN HANDLE ThreadHandle, 
		IN THREADINFOCLASS ThreadInformationClass, 
		OUT PVOID ThreadInformation, 
		IN ULONG ThreadInformationLength, 
		OUT PULONG ReturnLength OPTIONAL 
		); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwOpenThread( 
		OUT PHANDLE ThreadHandle, 
		IN ACCESS_MASK DesiredAccess, 
		IN POBJECT_ATTRIBUTES ObjectAttributes, 
		IN PCLIENT_ID	ClientId 
		); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwAlertResumeThread( 
		IN HANDLE ThreadHandle,  
		OUT PULONG PreviousSuspendCount OPTIONAL 
		); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwAlertThread( 
		IN HANDLE ThreadHandle 
		); 

	NTSYSAPI 
		PRTL_DEBUG_INFORMATION 
		NTAPI 
		RtlCreateQueryDebugBuffer( 
		IN ULONG MaximumCommit OPTIONAL, 
		IN BOOLEAN UseEventPair 
		); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		RtlDestroyQueryDebugBuffer( 
		IN PRTL_DEBUG_INFORMATION Buffer 
		); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		RtlQueryProcessDebugInformation( 
		IN HANDLE UniqueProcessId, 
		IN ULONG Flags, 
		IN OUT PRTL_DEBUG_INFORMATION Buffer 
		); 

#define RTL_QUERY_PROCESS_MODULES       0x00000001 
#define RTL_QUERY_PROCESS_HEAP_SUMMARY  0x00000004 
#define RTL_QUERY_PROCESS_HEAP_TAGS     0x00000008 
#define RTL_QUERY_PROCESS_HEAP_ENTRIES  0x00000010 
	NTSTATUS 
		NTAPI 
		RtlQueryProcessModuleInformation( 
		IN OUT PRTL_DEBUG_INFORMATION Buffer 
		); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		RtlQueryProcessHeapInformation( 
		IN OUT PRTL_DEBUG_INFORMATION Buffer 
		); 

	typedef struct _SECTION_IMAGE_INFORMATION { 
		PVOID TransferAddress; 
		ULONG ZeroBits; 
		SIZE_T MaximumStackSize; 
		SIZE_T CommittedStackSize; 
		ULONG SubSystemType; 
		union { 
			struct { 
				USHORT SubSystemMinorVersion; 
				USHORT SubSystemMajorVersion; 
			}; 
			ULONG SubSystemVersion; 
		}; 
		ULONG GpValue; 
		USHORT ImageCharacteristics; 
		USHORT DllCharacteristics; 
		USHORT Machine; 
		BOOLEAN ImageContainsCode; 
		BOOLEAN Spare1; 
		ULONG LoaderFlags; 
		ULONG Reserved[ 2 ]; 
	} SECTION_IMAGE_INFORMATION, *PSECTION_IMAGE_INFORMATION; 

	typedef struct _RTL_USER_PROCESS_INFORMATION { 
		ULONG Length; 
		HANDLE Process; 
		HANDLE Thread; 
		CLIENT_ID ClientId; 
		SECTION_IMAGE_INFORMATION ImageInformation; 
	} RTL_USER_PROCESS_INFORMATION, *PRTL_USER_PROCESS_INFORMATION; 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwAllocateVirtualMemory( 
		IN HANDLE ProcessHandle,  
		IN OUT PVOID *BaseAddress,  
		IN ULONG ZeroBits,  
		IN OUT PULONG AllocationSize,  
		IN ULONG AllocateType,  
		IN ULONG Protect); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwFreeVirtualMemory( 
		IN HANDLE ProcessHandle,  
		IN OUT PVOID *BaseAddress, 
		IN OUT PULONG FreeSize, 
		IN ULONG FreeType); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwCreateSection( 
		OUT PHANDLE SectionHandle,  
		IN ACCESS_MASK DesiredAccess,  
		IN POBJECT_ATTRIBUTES ObjectAttributes,  
		IN PLARGE_INTEGER SectionSize OPTIONAL,  
		IN ULONG Protect,  
		IN ULONG Attributes,  
		IN HANDLE FileHandle); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwOpenSection( 
		OUT PHANDLE SectionHandle, 
		IN ACCESS_MASK DesiredAccess, 
		IN POBJECT_ATTRIBUTES ObjectAttributes 
		); 

#define VIEW_SHARE 1 
#define VIEW_UNMAP 2 
#define ViewShare VIEW_SHARE 
#define ViewUnmap VIEW_UNMAP 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwMapViewOfSection( 
		IN HANDLE SectionHandle, 
		IN HANDLE ProcessHandle, 
		IN OUT PVOID *BaseAddress, 
		IN ULONG ZeroBits, 
		IN ULONG CommitSize, 
		IN OUT PLARGE_INTEGER SectionOffset OPTIONAL, 
		IN OUT PULONG ViewSize, 
		IN ULONG InheritDisposition, 
		IN ULONG AllocationType, 
		IN ULONG Protect); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwUnmapViewOfSection( 
		IN HANDLE ProcessHandle,  
		IN PVOID BaseAddress); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwCreateEvent( 
		OUT PHANDLE	EventHandle, 
		IN ACCESS_MASK DesiredAccess, 
		IN POBJECT_ATTRIBUTES ObjectAttributes, 
		IN ULONG	EventType, 
		IN BOOLEAN	InitialState); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwCreateFile( 
		OUT PHANDLE FileHandle, 
		IN ACCESS_MASK DesiredAccess, 
		IN POBJECT_ATTRIBUTES ObjectAttributes, 
		OUT PIO_STATUS_BLOCK IoStatusBlock, 
		IN PLARGE_INTEGER AllocationSize OPTIONAL, 
		IN ULONG FileAttributes, 
		IN ULONG ShareAccess, 
		IN ULONG CreateDisposition, 
		IN ULONG CreateOptions, 
		IN PVOID EaBuffer OPTIONAL, 
		IN ULONG EaLength 
		); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwOpenFile( 
		OUT PHANDLE FileHandle, 
		IN ACCESS_MASK DesiredAccess, 
		IN POBJECT_ATTRIBUTES ObjectAttributes, 
		OUT PIO_STATUS_BLOCK IoStatusBlock, 
		IN ULONG ShareAccess, 
		IN ULONG OpenOptions 
		); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwQueryInformationFile( 
		IN HANDLE FileHandle, 
		OUT PIO_STATUS_BLOCK IoStatusBlock, 
		OUT PVOID FileInformation, 
		IN ULONG Length, 
		IN FILE_INFORMATION_CLASS FileInformationClass 
		); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwSetInformationFile( 
		IN HANDLE FileHandle, 
		OUT PIO_STATUS_BLOCK IoStatusBlock, 
		IN PVOID FileInformation, 
		IN ULONG Length, 
		IN FILE_INFORMATION_CLASS FileInformationClass 
		); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwReadFile( 
		IN HANDLE FileHandle, 
		IN HANDLE Event OPTIONAL, 
		IN PIO_APC_ROUTINE ApcRoutine OPTIONAL, 
		IN PVOID ApcContext OPTIONAL, 
		OUT PIO_STATUS_BLOCK IoStatusBlock, 
		OUT PVOID Buffer, 
		IN ULONG Length, 
		IN PLARGE_INTEGER ByteOffset OPTIONAL, 
		IN PULONG Key OPTIONAL 
		); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwWriteFile( 
		IN HANDLE FileHandle, 
		IN HANDLE Event OPTIONAL, 
		IN PIO_APC_ROUTINE ApcRoutine OPTIONAL, 
		IN PVOID ApcContext OPTIONAL, 
		OUT PIO_STATUS_BLOCK IoStatusBlock, 
		IN PVOID Buffer, 
		IN ULONG Length, 
		IN PLARGE_INTEGER ByteOffset OPTIONAL, 
		IN PULONG Key OPTIONAL 
		); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwClose( 
		IN HANDLE Handle 
		); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwFsControlFile( 
		IN HANDLE	FileHandle, 
		IN HANDLE	Event OPTIONAL, 
		IN PIO_APC_ROUTINE	ApcRoutine OPTIONAL, 
		IN PVOID	ApcContext OPTIONAL, 
		OUT PIO_STATUS_BLOCK	IoStatusBlock, 
		IN ULONG	FsControlCode, 
		IN PVOID	InputBuffer OPTIONAL, 
		IN ULONG	InputBufferLength, 
		OUT PVOID	OutputBuffer OPTIONAL, 
		IN ULONG	OutputBufferLength); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwOpenDirectoryObject( 
		OUT	HANDLE	DirectoryHandle, 
		IN	ACCESS_MASK DesiredAccess, 
		IN	POBJECT_ATTRIBUTES ObjectAttributes); 


	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwQueryDirectoryObject( 
		IN HANDLE	DirectoryHandle, 
		OUT	PVOID	Buffer, 
		IN	ULONG	BufferLength, 
		IN	BOOLEAN	ReturnSingleEntry, 
		IN	BOOLEAN	RestartScan, 
		IN OUT PULONG Context, 
		OUT PULONG	ReturnLength OPTIONAL); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwCreateToken( 
		OUT PHANDLE TokenHandle, 
		IN ACCESS_MASK DesiredAccess, 
		IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL, 
		IN TOKEN_TYPE TokenType, 
		IN PLUID AuthenticationId, 
		IN PLARGE_INTEGER ExpirationTime, 
		IN PTOKEN_USER User, 
		IN PTOKEN_GROUPS Groups, 
		IN PTOKEN_PRIVILEGES Privileges, 
		IN PTOKEN_OWNER Owner OPTIONAL, 
		IN PTOKEN_PRIMARY_GROUP PrimaryGroup, 
		IN PTOKEN_DEFAULT_DACL DefaultDacl OPTIONAL, 
		IN PTOKEN_SOURCE TokenSource 
		); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwSetInformationToken( 
		IN HANDLE	TokenHandle, 
		IN TOKEN_INFORMATION_CLASS TokenInformationClass, 
		IN PVOID	TokenInformation, 
		IN ULONG TokenInformationLength); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwQueryInformationToken( 
		IN HANDLE TokenHandle,  
		IN TOKEN_INFORMATION_CLASS ProcessInformationClass,  
		OUT PVOID ProcessInformation,  
		IN ULONG ProcessInformationLength,  
		OUT PULONG ReturnLength OPTIONAL); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwImpersonateThread( 
		IN HANDLE ServerThreadHandle, 
		IN HANDLE ClientThreadHandle, 
		IN PSECURITY_QUALITY_OF_SERVICE SecurityQos 
		); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwCreateSymbolicLinkObject( 
		OUT PHANDLE LinkHandle, 
		IN ACCESS_MASK DesiredAccess, 
		IN POBJECT_ATTRIBUTES ObjectAttributes, 
		IN PUNICODE_STRING LinkTarget 
		); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwOpenSymbolicLinkObject( 
		OUT PHANDLE LinkHandle, 
		IN ACCESS_MASK DesiredAccess, 
		IN POBJECT_ATTRIBUTES ObjectAttributes 
		); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI 
		ZwQuerySymbolicLinkObject( 
		IN HANDLE LinkHandle, 
		IN OUT PUNICODE_STRING LinkTarget, 
		OUT PULONG ReturnedLength OPTIONAL 
		); 

	NTSYSAPI 
		VOID 
		NTAPI 
		ZwYieldExecution(); 

	NTSYSAPI 
		VOID 
		NTAPI 
		RtlInitUnicodeString( 
		PUNICODE_STRING DestinationString, 
		PCWSTR SourceString 
		); 
	NTSYSAPI 
		VOID 
		NTAPI 
		RtlFreeUnicodeString( 
		PUNICODE_STRING UnicodeString 
		); 
	NTSYSAPI  
		NTSTATUS  
		NTAPI  
		ZwLoadDriver(IN PUNICODE_STRING UnicodeString); 

	NTSYSAPI 
		PVOID 
		NTAPI 
		RtlDestroyHeap( 
		IN PVOID HeapHandle 
		); 
	NTSYSAPI 
		PVOID 
		NTAPI 
		RtlAllocateHeap( 
		IN PVOID HeapHandle, 
		IN ULONG Flags, 
		IN ULONG Size 
		); 

	NTSYSAPI 
		BOOLEAN 
		NTAPI 
		RtlFreeHeap( 
		IN PVOID HeapHandle, 
		IN ULONG Flags, 
		IN PVOID BaseAddress 
		); 

	KUSER_SHARED_DATA *GetSharedData(); 
	BOOL GetNtGlobalFlag(DWORD *OutFlags); 
	BOOL SetNtGlobalFlag(DWORD Flags); 


	typedef enum _SHUTDOWN_ACTION 
	{ 
		ShutdownNoReboot, 
		ShutdownReboot, 
		ShutdownPowerOff 
	} SHUTDOWN_ACTION; 

	NTSYSAPI  
		NTSTATUS  
		NTAPI  
		ZwShutdownSystem( 
		IN SHUTDOWN_ACTION Action 
		); 

	NTSYSAPI  
		NTSTATUS 
		NTAPI  
		ZwSetSystemEnviromentValue( 
		IN PUNICODE_STRING Name,  
		IN PUNICODE_STRING Value 
		); 


	NTSYSAPI  
		NTSTATUS  
		NTAPI  
		ZwSetSystemTime( 
		IN PLARGE_INTEGER NewTime,  
		OUT PLARGE_INTEGER OldTime OPTIONAL 
		); 


	NTSYSAPI NTSTATUS NTAPI ZwDeviceIoControlFile( 

		IN HANDLE FileHandle, 
		IN HANDLE Event OPTIONAL, 
		IN PIO_APC_ROUTINE ApcRoutine OPTIONAL, 
		IN PVOID ApcContext OPTIONAL, 
		OUT PIO_STATUS_BLOCK IoStatusBlock, 
		IN ULONG IoControlCode, 
		IN PVOID InputBuffer OPTIONAL, 
		IN ULONG InputBufferLength, 
		OUT PVOID OutputBuffer OPTIONAL, 
		IN ULONG OutputBufferLength); 

	// 
	// Key query structures 
	// 

	typedef struct _KEY_BASIC_INFORMATION { 
		LARGE_INTEGER LastWriteTime; 
		ULONG   TitleIndex; 
		ULONG   NameLength; 
		WCHAR   Name[1];            // Variable length string 
	} KEY_BASIC_INFORMATION, *PKEY_BASIC_INFORMATION; 

	typedef struct _KEY_NODE_INFORMATION { 
		LARGE_INTEGER LastWriteTime; 
		ULONG   TitleIndex; 
		ULONG   ClassOffset; 
		ULONG   ClassLength; 
		ULONG   NameLength; 
		WCHAR   Name[1];            // Variable length string 
		//          Class[1];           // Variable length string not declared 
	} KEY_NODE_INFORMATION, *PKEY_NODE_INFORMATION; 

	typedef struct _KEY_FULL_INFORMATION { 
		LARGE_INTEGER LastWriteTime; 
		ULONG   TitleIndex; 
		ULONG   ClassOffset; 
		ULONG   ClassLength; 
		ULONG   SubKeys; 
		ULONG   MaxNameLen; 
		ULONG   MaxClassLen; 
		ULONG   Values; 
		ULONG   MaxValueNameLen; 
		ULONG   MaxValueDataLen; 
		WCHAR   Class[1];           // Variable length 
	} KEY_FULL_INFORMATION, *PKEY_FULL_INFORMATION; 

	// end_wdm 
	typedef struct _KEY_NAME_INFORMATION { 
		ULONG   NameLength; 
		WCHAR   Name[1];            // Variable length string 
	} KEY_NAME_INFORMATION, *PKEY_NAME_INFORMATION; 

	typedef struct _KEY_CACHED_INFORMATION { 
		LARGE_INTEGER LastWriteTime; 
		ULONG   TitleIndex; 
		ULONG   SubKeys; 
		ULONG   MaxNameLen; 
		ULONG   Values; 
		ULONG   MaxValueNameLen; 
		ULONG   MaxValueDataLen; 
		ULONG   NameLength; 
		WCHAR   Name[1];            // Variable length string 
	} KEY_CACHED_INFORMATION, *PKEY_CACHED_INFORMATION; 

	typedef struct _KEY_FLAGS_INFORMATION { 
		ULONG   UserFlags; 
	} KEY_FLAGS_INFORMATION, *PKEY_FLAGS_INFORMATION; 

	// begin_wdm 
	typedef enum _KEY_INFORMATION_CLASS { 
		KeyBasicInformation, 
		KeyNodeInformation, 
		KeyFullInformation 
		// end_wdm 
		, 
		KeyNameInformation, 
		KeyCachedInformation, 
		KeyFlagsInformation 
		// begin_wdm 
	} KEY_INFORMATION_CLASS; 

	typedef struct _KEY_WRITE_TIME_INFORMATION { 
		LARGE_INTEGER LastWriteTime; 
	} KEY_WRITE_TIME_INFORMATION, *PKEY_WRITE_TIME_INFORMATION; 

	typedef struct _KEY_USER_FLAGS_INFORMATION { 
		ULONG   UserFlags; 
	} KEY_USER_FLAGS_INFORMATION, *PKEY_USER_FLAGS_INFORMATION; 

	typedef enum _KEY_SET_INFORMATION_CLASS { 
		KeyWriteTimeInformation, 
		KeyUserFlagsInformation 
	} KEY_SET_INFORMATION_CLASS; 

	// 
	// Value entry query structures 
	// 

	typedef struct _KEY_VALUE_BASIC_INFORMATION { 
		ULONG   TitleIndex; 
		ULONG   Type; 
		ULONG   NameLength; 
		WCHAR   Name[1];            // Variable size 
	} KEY_VALUE_BASIC_INFORMATION, *PKEY_VALUE_BASIC_INFORMATION; 

	typedef struct _KEY_VALUE_FULL_INFORMATION { 
		ULONG   TitleIndex; 
		ULONG   Type; 
		ULONG   DataOffset; 
		ULONG   DataLength; 
		ULONG   NameLength; 
		WCHAR   Name[1];            // Variable size 
		//          Data[1];            // Variable size data not declared 
	} KEY_VALUE_FULL_INFORMATION, *PKEY_VALUE_FULL_INFORMATION; 

	typedef struct _KEY_VALUE_PARTIAL_INFORMATION { 
		ULONG   TitleIndex; 
		ULONG   Type; 
		ULONG   DataLength; 
		UCHAR   Data[1];            // Variable size 
	} KEY_VALUE_PARTIAL_INFORMATION, *PKEY_VALUE_PARTIAL_INFORMATION; 

	typedef struct _KEY_VALUE_PARTIAL_INFORMATION_ALIGN64 { 
		ULONG   Type; 
		ULONG   DataLength; 
		UCHAR   Data[1];            // Variable size 
	} KEY_VALUE_PARTIAL_INFORMATION_ALIGN64, *PKEY_VALUE_PARTIAL_INFORMATION_ALIGN64; 

	typedef struct _KEY_VALUE_ENTRY { 
		PUNICODE_STRING ValueName; 
		ULONG           DataLength; 
		ULONG           DataOffset; 
		ULONG           Type; 
	} KEY_VALUE_ENTRY, *PKEY_VALUE_ENTRY; 

	typedef enum _KEY_VALUE_INFORMATION_CLASS { 
		KeyValueBasicInformation, 
		KeyValueFullInformation, 
		KeyValuePartialInformation, 
		KeyValueFullInformationAlign64, 
		KeyValuePartialInformationAlign64 
	} KEY_VALUE_INFORMATION_CLASS; 



	NTSYSAPI 
		NTSTATUS 
		NTAPI ZwEnumerateKey( 
		IN HANDLE KeyHandle, 
		IN ULONG Index, 
		IN KEY_INFORMATION_CLASS KeyInformationClass,  
		OUT PVOID KeyInformation, 
		IN ULONG KeyInformationLength, 
		OUT PULONG ResultLength 
		); 

	NTSYSAPI 
		NTSTATUS 
		NTAPI  

		ZwEnumerateValueKey( 
		IN HANDLE KeyHandle, 
		IN ULONG Index, 
		IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass, 
		OUT PVOID KeyValueInformation, 
		IN ULONG KeyValueInformationLength, 
		OUT PULONG ResultLength 
		); 


	NTSYSAPI NTSTATUS NTAPI 
		ZwSaveKey(IN HANDLE KeyHandle, 
		IN HANDLE FileHandle); 

	NTSYSAPI NTSTATUS NTAPI ZwReadVirtualMemory(
		IN HANDLE ProcessHandle,
		IN PVOID BaseAddress,
		OUT PVOID Buffer,
		IN ULONG BufferLength,
		OUT PULONG ReturnLength OPTIONAL
		); 


	/* =========================== Utils =========================== */
#ifdef _WIN64
#define TEB_PEB               0x0060
#define PEB_NT5_SESSION_ID    0x02C0
#else
#define TEB_PEB               0x0030
#define PEB_NT5_SESSION_ID    0x01D4
#endif
#define NtCurrentPeb() ((PEB *)(*(PBYTE*)(((PBYTE)NtCurrentTeb())+TEB_PEB)))
#define PEB32_NT4_SESSION_ID    0x014C

#define UserSharedData() ((volatile BYTE*)0x7FFE0000)
#define KUSER_TICK_COUNT        0x0000
#define KUSER_IMAGE_NUMBER_LOW  0x002C
#define KUSER_SYSTEM_ROOT       0x0030
#define KUSER_NT_MAJOR_VERSION  0x026C
#define KUSER_ACTIVE_CONSOLE_ID 0x02D8
#define KUSER_TICK_COUNT_NEW    0x0320

#define GetNtMajorVersion() (*(PBYTE)(UserSharedData()+KUSER_NT_MAJOR_VERSION)) /* NT3 == 0 */
#define GetSystemRootDir() ((PWSTR)(UserSharedData()+KUSER_SYSTEM_ROOT))
#define GetConsoleSessionId() (*(PULONG)(UserSharedData()+KUSER_ACTIVE_CONSOLE_ID))
#define AmIi386() (*(PWORD)(UserSharedData()+KUSER_IMAGE_NUMBER_LOW) == IMAGE_FILE_MACHINE_I386)
#ifdef _WIN64
#define AmIWow64() 0
#define GetCurrentSessionId() (*(PULONG)(NtCurrentPeb()+(PEB_NT5_SESSION_ID)))
#else
#define AmIWow64() (!AmIi386())
#define GetCurrentSessionId() (*(PULONG)(NtCurrentPeb()+(GetNtMajorVersion() > 4 ? PEB_NT5_SESSION_ID : PEB32_NT4_SESSION_ID)))
#endif

#define NtCurrentProcess() ( (HANDLE) -1 )
#define NtCurrentThread() ( (HANDLE) -2 )

#ifdef __cplusplus 
} 
#endif 
#endif // NTDLL_H


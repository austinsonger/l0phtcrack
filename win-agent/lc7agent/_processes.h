#ifndef __INC_PROCESSES_H
#define __INC_PROCESSES_H

/* TYPES */
typedef NTSTATUS (NTAPI *PPROC_ENUM_ROUTINE)(IN PSYSTEM_PROCESS_INFORMATION CurrentProcess,
                                             IN OUT PVOID CallbackContext);

typedef NTSTATUS (NTAPI *PTHREAD_ENUM_ROUTINE)(IN PSYSTEM_THREAD_INFORMATION CurrentThread,
                                               IN OUT PVOID CallbackContext);

typedef NTSTATUS (NTAPI *PSYSMOD_ENUM_ROUTINE)(IN PRTL_PROCESS_MODULE_INFORMATION CurrentModule,
                                               IN OUT PVOID CallbackContext);

typedef NTSTATUS (NTAPI *PPROCMOD_ENUM_ROUTINE)(IN HANDLE ProcessHandle,
                                                IN PLDR_DATA_TABLE_ENTRY CurrentModule,
                                                IN OUT PVOID CallbackContext);

/* PROTOTYPES */
/* Processes and threads */
/* enumeration */
NTSTATUS NTAPI
PsaEnumerateProcessesAndThreads(IN PPROC_ENUM_ROUTINE ProcessCallback,
                                IN OUT PVOID ProcessCallbackContext,
                                IN PTHREAD_ENUM_ROUTINE ThreadCallback,
                                IN OUT PVOID ThreadCallbackContext);

NTSTATUS NTAPI
PsaEnumerateProcesses(IN PPROC_ENUM_ROUTINE Callback,
                      IN OUT PVOID CallbackContext);

NTSTATUS NTAPI
PsaEnumerateThreads(IN PTHREAD_ENUM_ROUTINE Callback,
                    IN OUT PVOID CallbackContext);

/* capturing & walking */
NTSTATUS NTAPI
PsaCaptureProcessesAndThreads(OUT PSYSTEM_PROCESS_INFORMATION * ProcessesAndThreads);

NTSTATUS NTAPI
PsaWalkProcessesAndThreads(IN PSYSTEM_PROCESS_INFORMATION ProcessesAndThreads,
                           IN PPROC_ENUM_ROUTINE ProcessCallback,
                           IN OUT PVOID ProcessCallbackContext,
                           IN PTHREAD_ENUM_ROUTINE ThreadCallback,
                           IN OUT PVOID ThreadCallbackContext);

NTSTATUS NTAPI
PsaWalkProcesses(IN PSYSTEM_PROCESS_INFORMATION ProcessesAndThreads,
                 IN PPROC_ENUM_ROUTINE Callback,
                 IN OUT PVOID CallbackContext);

NTSTATUS NTAPI
PsaWalkThreads(IN PSYSTEM_PROCESS_INFORMATION ProcessesAndThreads,
               IN PTHREAD_ENUM_ROUTINE Callback,
               IN OUT PVOID CallbackContext);

PSYSTEM_PROCESS_INFORMATION FASTCALL
PsaWalkFirstProcess(IN PSYSTEM_PROCESS_INFORMATION ProcessesAndThreads);

PSYSTEM_PROCESS_INFORMATION FASTCALL
PsaWalkNextProcess(IN PSYSTEM_PROCESS_INFORMATION CurrentProcess);

PSYSTEM_THREAD_INFORMATION FASTCALL
PsaWalkFirstThread(IN PSYSTEM_PROCESS_INFORMATION CurrentProcess);

PSYSTEM_THREAD_INFORMATION FASTCALL
PsaWalkNextThread(IN PSYSTEM_THREAD_INFORMATION CurrentThread);

/* System modules */
/* enumeration */
NTSTATUS NTAPI
PsaEnumerateSystemModules(IN PSYSMOD_ENUM_ROUTINE Callback,
                          IN OUT PVOID CallbackContext);

/* capturing & walking */
NTSTATUS NTAPI
PsaCaptureSystemModules(OUT PRTL_PROCESS_MODULES * SystemModules);

NTSTATUS NTAPI
PsaWalkSystemModules(IN PRTL_PROCESS_MODULES SystemModules,
                     IN PSYSMOD_ENUM_ROUTINE Callback,
                     IN OUT PVOID CallbackContext);

PRTL_PROCESS_MODULE_INFORMATION FASTCALL
PsaWalkFirstSystemModule(IN PRTL_PROCESS_MODULES SystemModules);

PRTL_PROCESS_MODULE_INFORMATION FASTCALL
PsaWalkNextSystemModule(IN PRTL_PROCESS_MODULES CurrentSystemModule);

/* Process modules */
NTSTATUS NTAPI
PsaEnumerateProcessModules(IN HANDLE ProcessHandle,
                           IN PPROCMOD_ENUM_ROUTINE Callback,
                           IN OUT PVOID CallbackContext);

/* Miscellaneous */
VOID NTAPI
PsaFreeCapture(IN PVOID Capture);

/* MACROS */
#define DEFINE_DBG_MSG(__str__) "PSAPI: " __str__ "\n"


#endif
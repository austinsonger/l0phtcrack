#ifndef __INC_WIN_H
#define __INC_WIN_H

#include<exception>
#include<QString>

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

#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)

#define WIN_SC_HANDLE				SC_HANDLE
#define WIN_SERVICE_HANDLE			SC_HANDLE
#define INVALID_WIN_SC_HANDLE		NULL
#define INVALID_WIN_SERVICE_HANDLE	NULL
#define WIN_HANDLE					HANDLE
#define INVALID_WIN_HANDLE			INVALID_HANDLE_VALUE

#else

#include<libmsrpc.h>

#endif

class WIN
{
private:

	bool m_bNetUseIsDefault;

public:
	WIN();
	~WIN();
	
	bool _PathFileExists(QString str);
	void _ReportStatusError(QString errortext, bool silent);
	
	bool _RemoveNetUses(QString remotemachinewithslashes, QString & error);
	bool _ConnectNetUse(QString share, QString user, QString password, QString domain, QString & error);
	bool _ConnectNetUseDefault(QString share, QString & error);
	bool _StopService(WIN_SC_HANDLE remote_sc, QByteArray servicename, bool &service_exists, QString &error);
	void _DeleteFile(QString filename);
	bool _WriteDataToFile(QString targetfile, QByteArray data, QString &error);
	bool _ReadDataFromFile(QString targetfile, QByteArray &data, QString &error);
	bool _StartService(WIN_SC_HANDLE remote_sc, QByteArray servicename, QString servicepath, QString remoteservicepath, QString servicetitle, QString servicedescription,
					   WIN_SERVICE_HANDLE & hLcAgent, QString &error);
	bool _Impersonate(QString user, QString domain, QString password, QString & error);
	WIN_SC_HANDLE _OpenSCManager(QString remotemachinewithslashes);
	WIN_HANDLE _OpenSharedFile(QString file);
	bool _ReadSharedFile(WIN_HANDLE h, void *buf, quint32 len, quint32 *bytesread);
	void _SeekSharedFile(WIN_HANDLE h, quint32 pos);
	void _CloseSharedFile(WIN_HANDLE h);
	bool _CopyFile(QString src, QString dest);
			
};

extern WIN g_win;

#define WIN_PathFileExists g_win._PathFileExists
#define WIN_IsWow64 g_win._IsWow64
#define WIN_ReportStatusError g_win._ReportStatusError
#define WIN_RemoveNetUses g_win._RemoveNetUses
#define WIN_ConnectNetUse g_win._ConnectNetUse
#define WIN_ConnectNetUseDefault g_win._ConnectNetUseDefault
#define WIN_StopService g_win._StopService
#define WIN_DeleteFile g_win._DeleteFile
#define WIN_WriteDataToFile g_win._WriteDataToFile
#define WIN_ReadDataFromFile g_win._ReadDataFromFile
#define WIN_StartService g_win._StartService
#define WIN_Impersonate g_win._Impersonate
#define WIN_OpenSCManager g_win._OpenSCManager
#define WIN_OpenSharedFile g_win._OpenSharedFile
#define WIN_ReadSharedFile g_win._ReadSharedFile
#define WIN_SeekSharedFile g_win._SeekSharedFile
#define WIN_CloseSharedFile g_win._CloseSharedFile
#define WIN_CopyFile g_win._CopyFile

#endif

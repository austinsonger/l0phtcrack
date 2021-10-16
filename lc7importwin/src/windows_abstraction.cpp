#include<stdafx.h>


#ifdef __APPLE__
#define _WIN32
#include<wchar.h>
#define _wcsnicmp wcsncasecmp
#define LOGON32_LOGON_INTERACTIVE 2
#define LOGON32_PROVIDER_DEFAULT 0
#endif

#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)
#include<Windows.h>
#include<lm.h>
#include<wincrypt.h>
#include<Shlwapi.h>
#endif


/////////////////////////

WIN g_win;

/////////////////////////

WIN::WIN()
{TR;
#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)
	m_bNetUseIsDefault=false;
#else
	// Initialize samba
#endif
}

WIN::~WIN()
{TR;
#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)
#else
	// Terminate samba

#endif
}
	
bool WIN::_PathFileExists(QString str)
{TR;
#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)
	QString nativepath=QDir::toNativeSeparators(str);
	return ::PathFileExistsW((LPCWSTR)nativepath.constData())?true:false;
#else
	UNIMPLEMENTED;
#endif
}


void WIN::_ReportStatusError(QString errortext, bool silent)
{TR;
#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)
	LPWSTR lpMsgBuf;
	DWORD nStatus=GetLastError();
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, nStatus,	MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPWSTR)&lpMsgBuf,	0,	NULL );
	QString errormsg((const QChar *)lpMsgBuf);
	LocalFree(lpMsgBuf);
	
	QString outmsg=QString("%1\n\nDetails: %2").arg(errortext).arg(errormsg);
	
	TRDBG(outmsg.toUtf8());

#else
	UNIMPLEMENTED;
#endif

	if(!silent)
	{
		QMessageBox msgBox;
		msgBox.setText("An error has occurred");
		msgBox.setInformativeText(outmsg);
		msgBox.setStandardButtons(QMessageBox::Ok);
		msgBox.setDefaultButton(QMessageBox::Ok);
		int ret = msgBox.exec();
	}
}

bool WIN::_RemoveNetUses(QString remotemachinewithslashes, QString & error)
{TR;
#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)

	// If they already have a use record to the same machine
	// with different credentials than the admin/domain admin ones we want
	// it to have, they will get the dreaded "credentials conflict with existing credentials" error
	// We should almost definitely handle this case.
	QStringList removelist;
	USE_INFO_2 *buf, *cur;
	DWORD read, total, resumeh, rc, i;
	resumeh = 0;
	do
	{
		buf = NULL;
		rc = NetUseEnum( NULL, 2, (BYTE **) &buf, 2048, &read, &total, &resumeh );
		if ( rc != ERROR_MORE_DATA && rc != ERROR_SUCCESS )
		{
			break;
		}
		for ( i = 0, cur = buf; i < read; ++ i, ++ cur )
		{
			QString sharename((QChar *)cur->ui2_remote);
			if(sharename.startsWith(remotemachinewithslashes,Qt::CaseInsensitive))
			{
				removelist.append(sharename);
			}
		}

		if ( buf != NULL )
		{
			NetApiBufferFree( buf );
		}
	} while ( rc == ERROR_MORE_DATA );

	foreach(QString toremove, removelist)
	{
//		size_t mbuflen=(*iter).size()+1;
//		WCHAR *mbuf=(WCHAR *)malloc(mbuflen*sizeof(WCHAR));
//		wcsncpy_s(mbuf,mbuflen,iter->c_str(),mbuflen);
		NetUseDel(NULL,(LMSTR)toremove.constData(),USE_LOTS_OF_FORCE);
//		free(mbuf);
	}
	return true;
#else
	UNIMPLEMENTED;
#endif
}


bool WIN::_ConnectNetUseDefault(QString share, QString & error)
{TR;
#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)
	m_bNetUseIsDefault=true;
	return true;
#else
	UNIMPLEMENTED;
#endif
}

bool WIN::_ConnectNetUse(QString share, QString user, QString password, QString domain, QString & error)
{TR;
#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)

	// set up a use record with the (domain) admin creds so we can connect to the 
	// target machine's SCM
	USE_INFO_2 ui;
	ZeroMemory(&ui, sizeof ui);

	// although if you don't have WINNT defined this struct will have all plain ascii members,
	// if you use the non unicode version, the api call WILL NOT WORK
	ui.ui2_local = NULL; // we're not mapping a drive
	ui.ui2_password = (WCHAR *)password.data();
	ui.ui2_username = (WCHAR *)user.data();
	ui.ui2_domainname = (WCHAR *)domain.data();
	ui.ui2_asg_type = USE_WILDCARD;
	ui.ui2_remote = (WCHAR *)share.data();

	DWORD dwParmError = 0;
	NET_API_STATUS Status = NetUseAdd(NULL, 2, (LPBYTE)&ui, &dwParmError);

	if(Status != NERR_Success && Status != ERROR_SESSION_CREDENTIAL_CONFLICT)
	{
		if (Status == ERROR_LOGON_FAILURE || Status == ERROR_ACCESS_DENIED)
		{
			error="Access denied connecting to target machine";
		}
		else
		{
			error="Failed to connect to target machine";
		}
		return false;
	}

	m_bNetUseIsDefault=false;
	return true;
#else
	UNIMPLEMENTED;
#endif
}

bool WIN::_StopService(WIN_SC_HANDLE remote_sc, QByteArray servicename, bool &service_exists, QString &error)
{TR;
#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)

	SC_HANDLE hService = OpenServiceA(remote_sc, servicename, GENERIC_ALL);
	if(!hService)
	{
		DWORD dwErr=GetLastError();
		service_exists=(dwErr!=ERROR_SERVICE_DOES_NOT_EXIST);
		
		error=QString("Couldn't open LC7 agent service");
		return false;
	}

	BOOL bStopped = FALSE;
	SERVICE_STATUS st;
	if(QueryServiceStatus(hService,&st) && st.dwCurrentState!=SERVICE_STOPPED)
	{
		BOOL bRet = ControlService(hService, SERVICE_CONTROL_STOP, &st);

		for (int number_tries = 0 ; ((bStopped == FALSE) && (number_tries < 5)); number_tries++)
		{
			// Give it a second.. (literally)
			Sleep(1000);

			// See if it stopped
			QueryServiceStatus(hService, &st);

			if (st.dwCurrentState != SERVICE_STOPPED)
			{
				bRet = ControlService(hService, SERVICE_CONTROL_STOP, &st);				
			}
			else
			{
				bStopped = TRUE;
			}
		} 
	}
	else
	{
		bStopped=TRUE;
	}

	// Don't delete the service, since actually clearing it out requires a reboot, just replace the service later on.
	// DeleteService(hService);
	CloseServiceHandle(hService);

	service_exists=true;

	if(!bStopped)
	{
		error=QString("Couldn't stop LcService service");
		return false;
	}

	return true;
#else
	UNIMPLEMENTED;
#endif
}

void WIN::_DeleteFile(QString filename)
{TR;
#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)
	QString nativefilename=QDir::toNativeSeparators(filename);
	DeleteFileW((const WCHAR *)nativefilename.data());
#else
	UNIMPLEMENTED;
#endif
}

bool WIN::_WriteDataToFile(QString targetfile, QByteArray data, QString &error)
{TR;
#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)
	QString targetfilenative=QDir::toNativeSeparators(targetfile);
	HANDLE h=CreateFileW((const WCHAR *)targetfilenative.data(),GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if(h==INVALID_HANDLE_VALUE)
	{
		error=QString("Couldn't create target file: %1").arg(targetfile);
		return false;
	}

	DWORD dw;
	if(!WriteFile(h,data.data(),data.length(),&dw,NULL) || dw!=data.length())
	{
		error=QString("Couldn't write to target file: %1").arg(targetfile);
		CloseHandle(h);
		return false;
	}

	CloseHandle(h);
	return true;
#else
	UNIMPLEMENTED;
#endif
}

bool WIN::_ReadDataFromFile(QString targetfile, QByteArray &data, QString &error)
{TR;
#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)
	QString targetfilenative=QDir::toNativeSeparators(targetfile);
	HANDLE h=CreateFileW((const WCHAR *)targetfilenative.data(),GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if(h==INVALID_HANDLE_VALUE)
	{
		error=QString("Couldn't open target file: %1").arg(targetfile);
		return false;
	}

	DWORD size=GetFileSize(h,NULL);
	data.resize(size);

	DWORD dw;
	if(!ReadFile(h,data.data(),size,&dw,NULL) || dw!=size)
	{
		error=QString("Couldn't read from target file: %1").arg(targetfile);
		CloseHandle(h);
		return false;
	}

	CloseHandle(h);
	return true;
#else
	UNIMPLEMENTED;
#endif
}

bool WIN::_StartService(WIN_SC_HANDLE remote_sc, QByteArray servicename, QString servicepath, QString remoteservicepath, QString servicetitle, QString servicedescription, WIN_SERVICE_HANDLE & hLcAgent, QString &error)
{TR;
#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)
	servicepath=QDir::toNativeSeparators(servicepath);
	remoteservicepath=QDir::toNativeSeparators(remoteservicepath);

	hLcAgent = OpenService(remote_sc, (const WCHAR *)servicetitle.data(), GENERIC_ALL);
	if(hLcAgent == NULL)
	{
		DWORD dwErr = GetLastError();

		DWORD dwKeyOffset = 0;
		DWORD dwKeySizeOffset = 0;

		switch(dwErr)
		{
		case ERROR_ACCESS_DENIED:
			error="Access denied starting LC Agent service";
			return false;

		case ERROR_SERVICE_DOES_NOT_EXIST: 
			{
				hLcAgent = CreateService(remote_sc, (const WCHAR *)servicetitle.data(), (const WCHAR *)servicedescription.data(),
					GENERIC_ALL, SERVICE_WIN32_OWN_PROCESS,
					SERVICE_DEMAND_START, SERVICE_ERROR_IGNORE,
					(const WCHAR *)remoteservicepath.data(),
					NULL, NULL, NULL, NULL, NULL);
				if (hLcAgent == NULL)
				{
					error="Could not create LC Agent service";
					return false;
				}

				break;
			}

		default:
			error="Unknown service error";
			return false;
		}
	}
	else
	{
		char buf[8192];
		QUERY_SERVICE_CONFIGW *cfgw = (QUERY_SERVICE_CONFIGW *)buf;
		DWORD bufneeded = 0;
		if (!QueryServiceConfig(hLcAgent, cfgw, sizeof(buf), &bufneeded))
		{
			error = "Can not query service configuration";
			return false;
		}

		if (wcscmp(cfgw->lpBinaryPathName, (const WCHAR *)remoteservicepath.data()) != 0)
		{
			if (!ChangeServiceConfig(hLcAgent,
				SERVICE_NO_CHANGE,
				SERVICE_NO_CHANGE,
				SERVICE_NO_CHANGE,
				(const WCHAR *)remoteservicepath.data(),
				NULL,
				NULL, 
				NULL, 
				NULL,
				NULL,
				NULL))
			{
				error = "Could not configure LC Agent service";
				return false;
			}
		}
	}

	// try to start LC Agent service
	BOOL bRet = StartService(hLcAgent, 0, NULL);
	SERVICE_STATUS ServiceStatus;
	if(bRet)
	{
		BOOL bStarted = FALSE;
		for (int number_tries = 0 ; ((bStarted == FALSE) && (number_tries < 5)); number_tries++)
		{
			Sleep(1000);
			QueryServiceStatus(hLcAgent, &ServiceStatus);
			if (ServiceStatus.dwCurrentState == SERVICE_RUNNING)
			{
				bStarted = TRUE;
			}
		} 
	}
	else
	{
		DWORD dwErr=GetLastError();

		BOOL bStarted = FALSE;

		// in practice, i see ERROR_IO_PENDING here sometimes
		// but the service seems to be started fine when that happens.

		// see below, when this happens we have to wait for the service
		// to come up and set its status to running. otherwise, sending
		// it the control code to dump the hashes will fail.

		switch(dwErr)
		{
		case ERROR_SERVICE_ALREADY_RUNNING: 
			break; // that's fine, it's already running

		case ERROR_EXE_MACHINE_TYPE_MISMATCH:
			error="LC Agent is the wrong architecture.";
			CloseServiceHandle(hLcAgent);
			return false;

		case ERROR_IO_PENDING: 

			// wait until service comes up...
			for (int number_tries = 0 ; ((bStarted == FALSE) && (number_tries < 5)); number_tries++)
			{
				Sleep(1000);
				QueryServiceStatus(hLcAgent, &ServiceStatus);
				if (ServiceStatus.dwCurrentState == SERVICE_RUNNING)
				{
					bStarted = TRUE;
				}
			} 

			if (bStarted == FALSE)
			{
				error="Timeout waiting for service to start.";
				CloseServiceHandle(hLcAgent);
				return false;
			}

			break; 

		case ERROR_SERVICE_DISABLED:
			// Display the string.

			error="The LC Agent service on the target computer is disabled, or it is scheduled for update.\nYou may need to enable the service manually, and reboot the machine to get it to clean up the Service Control Manager.";
			return false;

		default:
			CloseServiceHandle(hLcAgent);
			return false;
		}	
	}

	return true;
#else
	UNIMPLEMENTED;
#endif
}
	

bool WIN::_Impersonate(QString user, QString domain, QString password, QString & error)
{TR;
#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)
	HANDLE hToken;

	const WCHAR * wchr_user = nullptr;
	const WCHAR * wchr_domain = nullptr;
	const WCHAR * wchr_passwd = nullptr;
	QString error_message;

	if(domain.length()==0 && user.contains("@"))
	{
		// UPN Format name
		wchr_user = (const WCHAR *)user.data();
		wchr_domain = nullptr;
		wchr_passwd = (const WCHAR *)password.data();
		error_message = "Couldn't log on as specified user. User principal name or password are not valid.";
	}
	else if(domain.length()==0)
	{
		// Local name only, no domain
		wchr_user = (const WCHAR *)user.data();
		wchr_domain = (const WCHAR *)QString(".").constData();
		wchr_passwd = (const WCHAR *)password.data();
		error_message="Couldn't log on as specified user. Username and password are not valid for local user.";
	}
	else
	{
		wchr_user = (const WCHAR *)user.data();
		wchr_domain = (const WCHAR *)domain.data();
		wchr_passwd = (const WCHAR *)password.data();
		error = "Couldn't log on as specified user. Username and password are not valid in this domain.";
	}

	// Try local login
	if (!LogonUserW(wchr_user, wchr_domain, wchr_passwd, LOGON32_LOGON_BATCH, LOGON32_PROVIDER_DEFAULT, &hToken))
	{
		// Use network-only login
		if (!LogonUserW(wchr_user, wchr_domain, wchr_passwd, LOGON32_LOGON_NEW_CREDENTIALS, LOGON32_PROVIDER_WINNT50, &hToken))
		{
			error = error_message;
			return false;
		}
	}

	if (!ImpersonateLoggedOnUser(hToken))
	{
		error="Couldn't impersonate specified user.";
		CloseHandle(hToken);
		return false;
	}

	CloseHandle(hToken);
	return true;
#else
	UNSUPPORTED;
#endif
}

WIN_SC_HANDLE WIN::_OpenSCManager(QString remotemachinewithslashes)
{TR;
#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)
	SC_HANDLE hRemoteSC;
	if(remotemachinewithslashes.size()==0)
	{
		hRemoteSC = OpenSCManagerW(NULL,SERVICES_ACTIVE_DATABASEW, SC_MANAGER_ALL_ACCESS);
	}
	else
	{
		hRemoteSC = OpenSCManagerW((const WCHAR *)remotemachinewithslashes.data(),SERVICES_ACTIVE_DATABASEW, SC_MANAGER_ALL_ACCESS);
	}

	return hRemoteSC;
#else
	UNIMPLEMENTED;
#endif
}


WIN_HANDLE WIN::_OpenSharedFile(QString path)
{TR;
#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)
	QString nativepath=QDir::toNativeSeparators(path);
	return CreateFileW((const WCHAR *)nativepath.data(),FILE_GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
#else
	UNIMPLEMENTED;
#endif
}

bool WIN::_ReadSharedFile(WIN_HANDLE h, void *buf, quint32 len, quint32 *bytesread)
{TR;
#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)
	return ReadFile(h,buf,len, (DWORD *)bytesread,NULL)==TRUE;
#else
	UNIMPLEMENTED;
#endif
}

void WIN::_SeekSharedFile(WIN_HANDLE h, quint32 pos)
{TR;
#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)
	SetFilePointer(h,pos,NULL,FILE_BEGIN);
#else
	UNIMPLEMENTED;
#endif
}

void WIN::_CloseSharedFile(WIN_HANDLE h)
{TR;
#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)
	CloseHandle(h);
#else
	UNIMPLEMENTED;
#endif
}

bool WIN::_CopyFile(QString src, QString dest)
{TR;
#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)
	QString srcpath=QDir::toNativeSeparators(src);
	QString destpath=QDir::toNativeSeparators(dest);
	return CopyFileW((const WCHAR *)srcpath.data(),(const WCHAR *)destpath.data(),FALSE)!=FALSE;
#else
	UNIMPLEMENTED;
#endif
}

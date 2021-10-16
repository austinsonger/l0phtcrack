#include"stdafx.h"
#include"CImportWindows.h"
#include <QProcessEnvironment> 

//#define _DEBUG_DUMP

// service control codes for LcAgent remote agent
#define SERVICE_GET_HASH_COUNT 137
#define SERVICE_DUMP_HASHES 138
#define SERVICE_REMEDIATE 139

//#define NO_SYMMETRIC_ENCRYPTION 1

extern int prng_idx, hash_idx;

#define BUFFER_SIZE (65536)
#define KEY_BYTE_LENGTH (32)
#define HEADER_BYTE_LENGTH (1024)


CImportWindows::CImportWindows(ILC7AccountList *accountlist, ILC7CommandControl *ctrl)
{TR;
	if (ctrl)
		m_ctrl = ctrl->GetSubControl("Windows Agent Importer: ");
	else
		m_ctrl = nullptr;

	m_accountlist=accountlist;

	m_bRemote=false;
	m_bSpecificUser=false;
	m_bIs64Bit=false;
	m_bIncludeMachineAccounts=false;

	m_nNumImported=-1;
	m_estimated_total=0;
	m_number_of_users=0;

	m_account_limit = 0;

	m_remedation_command_file = NULL;
}

CImportWindows::~CImportWindows()
{TR;
	Q_ASSERT(m_remedation_command_file == NULL);
	
	if (m_ctrl)
		m_ctrl->ReleaseSubControl();
}


void CImportWindows::SetAccountLimit(quint32 alim)
{
	m_account_limit = alim;
}

void CImportWindows::SetRemoteMachine(QString remote_machine)
{TR;

	// Remove leading and trailing backslashes
	while(remote_machine.at(0)=='\\')
	{
		remote_machine.remove(0,1);
	}
	while(remote_machine.at(remote_machine.length()-1)=='\\')
	{
		remote_machine.remove(remote_machine.length()-1,1);
	}

	m_bRemote=true;
	m_strRemoteMachine=remote_machine;
	m_strRemoteMachineWithSlashes=QString("\\\\")+m_strRemoteMachine;
	m_strRemoteAdmin=m_strRemoteMachineWithSlashes+QString("\\ADMIN$");
	m_strRemoteAdminTemp=m_strRemoteMachineWithSlashes+QString("\\ADMIN$\\Temp");
	m_strRemoteWOW64=m_strRemoteMachineWithSlashes+QString("\\ADMIN$\\SysWOW64");
}

void CImportWindows::SetSpecificUser(QString user, QString password, QString domain)
{TR;
	m_bSpecificUser=true;
	m_strUser=user;
	m_strPassword=password;
	m_strDomain=domain;
}

void CImportWindows::SetIncludeMachineAccounts(bool include)
{TR;
	m_bIncludeMachineAccounts=include;
}

void CImportWindows::SetRemediations(const LC7Remediations &remediations)
{
	TR;
	m_remediations = remediations;
}


#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)

bool IsWow64()
{
	BOOL bIsWow64 = FALSE;

	typedef BOOL(APIENTRY *LPFN_ISWOW64PROCESS)
		(HANDLE, PBOOL);

	LPFN_ISWOW64PROCESS fnIsWow64Process;

	HMODULE module = GetModuleHandle(L"kernel32");
	const char funcName[] = "IsWow64Process";
	fnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress(module, funcName);

	if (NULL != fnIsWow64Process)
	{
		if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64))
		{
			throw std::exception("Unknown error");
		}
	}
	return bIsWow64 != FALSE;
}

#endif

void CImportWindows::CheckIs64Bit(void)
{TR;
	if(m_bRemote)
	{
		if(WIN_PathFileExists(m_strRemoteWOW64))
		{
			m_bIs64Bit=true;
			return;
		}
		m_bIs64Bit=false;
		return;
	}

#if (PLATFORM == PLATFORM_WIN64)
	// 64-bit programs run only on Win64
	m_bIs64Bit=true;  
#elif (PLATFORM == PLATFORM_WIN32)
	// 32-bit programs run on both 32-bit and 64-bit Windows
	m_bIs64Bit=IsWow64();
#else
#error "64-bit detect"
#endif

}

void CImportWindows::UpdateStatus(QString statustext, quint32 cur, bool statuslog)
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


void CImportWindows::GetAllUsefulPathnames(void)
{TR;
	QDir agentdir(g_pLinkage->GetStartupDirectory());
		
	agentdir.cd("lcplugins");
	agentdir.cd("lc7importwin{17324176-3fa7-4c1a-9204-3f391b6b3599}");
	agentdir.cd("lcagent");
	m_strSource=agentdir.filePath(m_bIs64Bit?"lc7agent64.exe":"lc7agent.exe");
	m_strSourceDLL=agentdir.filePath(m_bIs64Bit?"lc7dump64.dll":"lc7dump.dll");
	
	if(m_bRemote)
	{
		m_strTarget = m_strRemoteAdmin+QString("\\")+QString("lc7agent.exe");
		m_strTargetDLL = m_strRemoteAdmin+QString("\\")+QString("lc7dump.dll");
		m_strRemoteDMP = m_strRemoteAdminTemp+QString("\\lc7agent.dmp");
		m_strRemoteDMPStatus = m_strRemoteAdminTemp+QString("\\lc7agent.dmp.status");
		m_strRemoteREMIn = m_strRemoteAdminTemp + QString("\\lc7agent.rem.in");
		m_strRemoteREMOut = m_strRemoteAdminTemp + QString("\\lc7agent.rem.out");
		m_strRemoteREMStatus = m_strRemoteAdminTemp + QString("\\lc7agent.rem.status");
	}
	else
	{
#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)

		QDir targetdir;
		QDir agentdir;

		WCHAR wsvWindowsDir[MAX_PATH];
		GetWindowsDirectoryW(wsvWindowsDir,sizeof(wsvWindowsDir));

		agentdir = QDir(QString::fromWCharArray(wsvWindowsDir));
		
		targetdir = QDir(QString::fromWCharArray(wsvWindowsDir));
		targetdir.cd("Temp");
	
		m_strTarget = agentdir.filePath("lc7agent.exe");
		m_strTargetDLL = agentdir.filePath("lc7dump.dll");
		m_strRemoteDMP = targetdir.filePath("lc7agent.dmp");
		m_strRemoteDMPStatus = targetdir.filePath("lc7agent.dmp.status");
		m_strRemoteREMIn = targetdir.filePath("lc7agent.rem.in");
		m_strRemoteREMOut = targetdir.filePath("lc7agent.rem.out");
		m_strRemoteREMStatus = targetdir.filePath("lc7agent.dmp.status");
#else
		UNSUPPORTED;
#endif
	}
}

void CImportWindows::CreateAgentAndDLL(QByteArray & agent, QByteArray & agentdll)
{TR;
	// Open the exe to insert encryption key
	QFile agentfile(m_strSource);
	if(!agentfile.open(QIODevice::ReadOnly))
	{
		throw "Couldn't open LC Agent";
	}

	// Read agent in completely
	agent=agentfile.readAll();

	// Open the dll
	QFile agentdllfile(m_strSourceDLL);
	if(!agentdllfile.open(QIODevice::ReadOnly))
	{
		throw "Couldn't open LC Agent DLL";
	}

	// Read agent in completely
	agentdll=agentdllfile.readAll();

	// Find place to store key in agent
	quint32 dwKeyLenOffset, dwKeyOffset;
	FindAgentKeyOffset(agentdll, &dwKeyLenOffset, &dwKeyOffset);

	// Write key length and key into agent
	quint32 keybuflen=(quint32)(m_public_key.length() + 1);
	memcpy(agentdll.data() + dwKeyLenOffset, &keybuflen, sizeof(quint32));
	memcpy(agentdll.data() + dwKeyOffset, m_public_key.c_str(), keybuflen);
}

void CImportWindows::GetOrCreatePrivateKey(bool & created)
{TR;
	// check to see if there is already a key pair for this machine
	ILC7Settings *settings=g_pLinkage->GetSettings();
	QString keykeypub,keykeypriv;
	keykeypub = QString(UUID_IMPORTWINPLUGIN.toString() + ":AGENT_KEYPUB");
	keykeypriv = QString(UUID_IMPORTWINPLUGIN.toString() + ":AGENT_KEYPRIV");

	if(!settings->contains(keykeypub) || !settings->contains(keykeypriv))
	{		
		EVP_PKEY *keypair = NULL;
		bool keypair_ok = false;
		// Init RSA
		EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
		if (EVP_PKEY_keygen_init(ctx) > 0) {
			if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048) > 0) {
				if (EVP_PKEY_keygen(ctx, &keypair) > 0) {
					// Key generated successfully
					keypair_ok = true;
				}
			}
			EVP_PKEY_CTX_free(ctx);
		}
		if (!keypair_ok) {
			Q_ASSERT(false);
			throw "Failed to generate keypair";
		}

		BIO *bio_private;
		bio_private = BIO_new(BIO_s_mem());
		if (PEM_write_bio_PrivateKey(bio_private, keypair, NULL, NULL, 0, NULL, NULL) <= 0) {
			Q_ASSERT(false);
			throw "Failed to write private key";
		}

		BIO *bio_public;
		bio_public = BIO_new(BIO_s_mem());
		if (PEM_write_bio_PUBKEY(bio_public, keypair) <= 0) {
			Q_ASSERT(false);
			throw "Failed to write public key";
		}

		int bio_private_len = BIO_pending(bio_private);
		m_private_key.resize(bio_private_len);
		if (BIO_read(bio_private, &m_private_key[0], bio_private_len) <= 0) {
			Q_ASSERT(false);
			throw "Failed to write export private key";
		}
		
		int bio_public_len = BIO_pending(bio_public);
		m_public_key.resize(bio_public_len);
		if (BIO_read(bio_public, &m_public_key[0], bio_public_len) <= 0) {
			Q_ASSERT(false);
			throw "Failed to write export public key";
		}
		
		// Save keypair into settings for remote machine
		settings->setValue(keykeypub,QByteArray(m_public_key.c_str()));
		settings->setValue(keykeypriv,QByteArray(m_private_key.c_str()));
		settings->sync();
		created=true;
	}
	else
	{
		// Get keypair from settings key
		QByteArray kpub=settings->value(keykeypub).toByteArray();
		QByteArray kpriv=settings->value(keykeypriv).toByteArray();
		m_public_key = kpub.toStdString();
		m_private_key = kpriv.toStdString();
	
		created=false;
	}
}

void CImportWindows::CopyAgent(QByteArray agent, QByteArray agentdll)
{TR;
	// Write agent to target
	QString error;
	if(!WIN_WriteDataToFile(m_strTarget, agent, error))
	{
		throw QString("Couldn't write data to agent exe file: %1").arg(error);
	}

	// Write dll to target
	if(!WIN_WriteDataToFile(m_strTargetDLL,agentdll,error))
	{
		throw QString("Couldn't write data to agent dll file: %1").arg(error);
	}
}

void CImportWindows::FindAgentKeyOffset(QByteArray agent, quint32 *pdwKeyLenOffset, quint32 *pdwKeyOffset)
{TR;
	char tag[] = "LC7CDCFTWLC7";

	*pdwKeyOffset=0;
	const char *data=agent.data();
	quint32 dwTagOffset = 0;

	// find our tag in memory
	for (quint32 i = 0 ; i < (quint32)agent.length(); i++)
	{
		// there is probably a much better/faster way
		// of doing this 
		if (memcmp(data+i, tag, strlen(tag)) == 0)
		{
			// we found the tag !
			dwTagOffset = i;
			break;
		}
	}

	if (dwTagOffset == 0) // we didn't find the tag ?!?
	{
		FAILURE;
	}

	*pdwKeyLenOffset = dwTagOffset + 12;
	*pdwKeyOffset = dwTagOffset + 16;
}


void CImportWindows::IssueHashDumpRequest(WIN_SERVICE_HANDLE hLcAgent)
{TR;
	UpdateStatus("Issuing hash dump request...",0);

	SERVICE_STATUS ServiceStatus;
	BOOL bRet = ControlService(hLcAgent, SERVICE_DUMP_HASHES, &ServiceStatus);

	quint32 dwErr = GetLastError();
	if ((bRet == FALSE) && (dwErr != ERROR_SERVICE_REQUEST_TIMEOUT) && (dwErr != ERROR_IO_PENDING))
	{
		throw "Couldn't issue hash dump request";
	}
}

void CImportWindows::IssueRemediationRequest(WIN_SERVICE_HANDLE hLcAgent)
{
	TR;
	UpdateStatus("Issuing remediation request...", 0);

	SERVICE_STATUS ServiceStatus;
	BOOL bRet = ControlService(hLcAgent, SERVICE_REMEDIATE, &ServiceStatus);

	quint32 dwErr = GetLastError();
	if ((bRet == FALSE) && (dwErr != ERROR_SERVICE_REQUEST_TIMEOUT) && (dwErr != ERROR_IO_PENDING))
	{
		throw "Couldn't issue remediation request";
	}
}



void CImportWindows::WaitForCompletion(QString strStatusFile, HANDLE hProcess, bool & cancelled)
{TR;
	// See if the file has shown up yet, keep checking this until we're done
	WIN_HANDLE hStatusFile;
	int numtries = 0;
	do
	{
		hStatusFile=WIN_OpenSharedFile(strStatusFile);
		if(hStatusFile!=INVALID_WIN_HANDLE)
		{
			break;
		}

		QThread::sleep(1);
		numtries++;
	} while(numtries<10);

	if(hStatusFile==INVALID_WIN_HANDLE)
	{
		throw "Couldn't open target hash dump status file.";
	}

	// Dump file showed up, cycle on it and report status
	m_estimated_total=0;
	m_number_of_users=0;

	int nPercentage=0;

	bool bFinished=false;
	do
	{
		if (m_ctrl && m_ctrl->StopRequested())
		{
			cancelled = true;
			WIN_CloseSharedFile(hStatusFile);
			return;
		}

		quint32 dwBytesRead=0;
		quint32 dwbuf[3];
		QDateTime starttime = QDateTime::currentDateTime();
		while(WIN_ReadSharedFile(hStatusFile,dwbuf,sizeof(quint32)*3,&dwBytesRead))
		{

			if (m_ctrl && m_ctrl->StopRequested())
			{
				cancelled = true;
				WIN_CloseSharedFile(hStatusFile);
				return;
			}

			// It make take a while for the file to have something in it
			// if the server is heavily loaded, wait for something to happen.
			if(dwBytesRead==0)
			{
				UpdateStatus("Waiting for server...",nPercentage,false);
				QThread::sleep(1);

				DWORD exitcode=STILL_ACTIVE;
				if (hProcess && GetExitCodeProcess(hProcess, &exitcode) && exitcode != STILL_ACTIVE)
				{
					if (((int32_t)exitcode)<0)
					{
						cancelled = true;
						WIN_CloseSharedFile(hStatusFile);
						return;
					}
				}

				if (starttime.secsTo(QDateTime::currentDateTime()) >= 30)
				{
					throw "Timed out reading from hash dump agent.\nYou may have a virus scanner that is installed that is interfering with L0phtCrack 7's operation.\nPlease disable any virus scanners or whitelist the C:\\Windows\\lc7agent.exe executable.";
				}
				continue;
			}
			// reset wait timer
			starttime = QDateTime::currentDateTime();

			if(dwBytesRead!=sizeof(quint32)*3)
			{
				WIN_CloseSharedFile(hStatusFile);
				throw "Couldn't read status from hash dump agent.\nYou may have a virus scanner that is installed that is interfering with L0phtCrack 7's operation.\nPlease disable any virus scanners or whitelist the C:\\Windows\\lc7agent.exe executable.";
			}

			// Go back to the beginning and keep reading first few bytes
			WIN_SeekSharedFile(hStatusFile,0);

			// dwbuf[0]=estimated number of users, dwbuf[1]=number of users dumped so far, dwbuf[2]=(bool) is done
			m_estimated_total=dwbuf[0];
			m_number_of_users=dwbuf[1];
			nPercentage=(int)(100.0*(double)(m_number_of_users)/(double)(m_estimated_total));

			UpdateStatus(QString("%1 of %2 users processed").arg(m_number_of_users).arg(m_estimated_total),nPercentage,false);
			
			if(dwbuf[2]==1)
			{
				bFinished=true;
				break;
			}

			QThread::sleep(2);
		}

		if(bFinished)
		{
			break;
		}

		// Reopen dump file if it closed on us and we weren't finished
		WIN_CloseSharedFile(hStatusFile);
		int numtries = 0;
		do
		{
			hStatusFile=WIN_OpenSharedFile(strStatusFile);
			if (hStatusFile != INVALID_WIN_HANDLE || (hProcess && WaitForSingleObject(hProcess, 0) == WAIT_OBJECT_0))
			{
				break;
			}

			numtries++;
			QThread::sleep(2);
			numtries++;
		} while(numtries<5);
		if(hStatusFile==INVALID_HANDLE_VALUE)
		{
			break;
		}
	}
	while(1);

	WIN_CloseSharedFile(hStatusFile); 

	if(!bFinished)
	{
		throw "Timed out trying to issue control request to service.";
	}
	
	UpdateStatus(QString("%1 of %2 users processed").arg(m_number_of_users).arg(m_number_of_users),100);
}

static void copy_data(void *outdata, CPubKeyFile &pkfile, const char *errormsg, size_t &dechashbufoff, size_t sizeof_data) {
	quint32 bytesread;
	if(!pkfile.Read(outdata, (quint32)sizeof_data, &bytesread)) {
		throw errormsg;
	}
	dechashbufoff += bytesread;
}

#define	COPY_DATA(XXX,ERRORMSG)										\
	copy_data(&XXX, encryptedfile, ERRORMSG, dechashbufoff, sizeof(XXX));

#define	COPY_DATA_MAX(XXX,LEN,ERRORMSG)						\
	copy_data(&XXX, encryptedfile, ERRORMSG, dechashbufoff, LEN);


void CImportWindows::DecryptAndImportHashes(QString strDumpFile, bool & cancelled)
{TR;
	// now we have the ENCRYPTED file with the userentry structs in it on the local machine
	CPubKeyFile encryptedfile;	
	if(!encryptedfile.Open(strDumpFile, false))
	{
		throw "Couldn't open dump file";
	}

	// Set decryption key
	encryptedfile.SetKey(QByteArray(m_private_key.c_str()), false);

	UpdateStatus("Decrypting hashes...",0);

	size_t dechashbufoff = 0;
	
	quint32 dwDomainLen;
	ushort wsvDomainBuf[1025];
	ushort wsvUsernameBuf[1025];
	ushort wsvFullnameBuf[1025];
	ushort wsvCommentBuf[1025];
	ushort wsvHomedirBuf[1025];

	COPY_DATA(dwDomainLen,"Invalid hash data format\nNo domain name length");
	if(dwDomainLen>2048)
	{
		throw "Invalid hash data format\nDomain length too long";
	}
	COPY_DATA_MAX(wsvDomainBuf,dwDomainLen+2,"Invalid hash data format\nDomain name unavailable");

	if(m_number_of_users==0)
	{
		throw "No users were imported!";
	}

	m_accountlist->Acquire();
	int starting_count=m_accountlist->GetAccountCount();

	int dwRealImportedCount=0;
	int nImported;
	int nPercentage=0;
	bool success=true;
	QString error;

	int nMachineAccounts=0;

	int remconfig = -1;

	for(nImported=0;nImported<(int)m_number_of_users;nImported++)
	{
		nPercentage=(int)(100.0*(double)(nImported)/(double)(m_number_of_users));

		if (m_ctrl && m_ctrl->StopRequested())
		{
			// endbatch and exit to cleanup, success=false, instead of return false
			cancelled=true;
			break;
		}

		if((nImported & 0xF)==0)
		{
			QString str=QString("%1 of %2 users imported").arg(nImported).arg(m_number_of_users);
			UpdateStatus(str,nPercentage,false);
		}

		quint32 dwUsernameLen;
		COPY_DATA(dwUsernameLen,"Invalid hash data format\nNo username length");
		if(dwUsernameLen > 2048)
		{
			error = QString("Invalid hash data format\nUsername length too long: %1").arg(dwUsernameLen);
			success=false;
			break;
		}
		COPY_DATA_MAX(wsvUsernameBuf,dwUsernameLen + 2,"Invalid hash data format\nUsername unavailable");

		quint32 dwFullnameLen;
		COPY_DATA(dwFullnameLen, "Invalid hash data format\nNo fullname length");
		if (dwFullnameLen > 2048)
		{
			error = QString("Invalid hash data format\nFullname length too long: %1").arg(dwFullnameLen);
			success = false;
			break;
		}
		COPY_DATA_MAX(wsvFullnameBuf, dwFullnameLen + 2, "Invalid hash data format\nFullname unavailable");

		quint32 dwCommentLen;
		COPY_DATA(dwCommentLen, "Invalid hash data format\nNo comment length");
		if (dwCommentLen > 2048)
		{
			error = QString("Invalid hash data format\nComment length too long: %1").arg(dwCommentLen);
			success = false;
			break;
		}
		COPY_DATA_MAX(wsvCommentBuf, dwCommentLen + 2, "Invalid hash data format\nComment unavailable");

		quint32 dwHomedirLen;
		COPY_DATA(dwHomedirLen, "Invalid hash data format\nNo homedir length");
		if (dwHomedirLen > 2048)
		{
			error = QString("Invalid hash data format\nHomedir length too long: %1").arg(dwHomedirLen);
			success = false;
			break;
		}
		COPY_DATA_MAX(wsvHomedirBuf, dwHomedirLen + 2, "Invalid hash data format\nHomedir unavailable");

		quint32 dwRID;
		COPY_DATA(dwRID,"Invalid hash data format\nNo user id");

		unsigned char has_nt_hash=0;
		COPY_DATA(has_nt_hash,"Invalid hash data format\nNo NTLM hash boolean");

		unsigned char ntlm_hash[16];
		if(has_nt_hash)
		{
			COPY_DATA(ntlm_hash,"Invalid hash data format\nNo NTLM hash");
		}

		unsigned char has_lm_hash=0;
		COPY_DATA(has_lm_hash,"Invalid hash data format\nNo LM hash boolean");

		unsigned char lm_hash[16];
		if(has_lm_hash)
		{
			COPY_DATA(lm_hash,"Invalid hash data format\nNo LM hash");
		}

		ULONGLONG ullAge;
		COPY_DATA(ullAge,"Invalid hash data format\nNo password age");

		bool bLockedOut;
		COPY_DATA(bLockedOut,"Invalid hash data format\nNo 'lockedout' flag");
		bool bDisabled;
		COPY_DATA(bDisabled,"Invalid hash data format\nNo 'disabled' flag");
		bool bNeverExpires;
		COPY_DATA(bNeverExpires,"Invalid hash data format\nNo 'neverexpires' flag");
		bool bExpired;
		COPY_DATA(bExpired,"Invalid hash data format\nNo 'expired' flag");

		if(!m_bIncludeMachineAccounts && dwUsernameLen>0 && wsvUsernameBuf[(dwUsernameLen/2)-1]=='$')
		{
			nMachineAccounts++;
			continue;
		}

		LC7Account acct;

		bool emptyhash = false;

		if (has_lm_hash)
		{
			LC7Hash hash;
			hash.hashtype = FOURCC(HASHTYPE_LM);
			hash.crackstate = 0;
			hash.cracktime = 0;
			for (int i = 0; i < sizeof(lm_hash); i++)
			{
				hash.hash += QString().sprintf("%2.2X", lm_hash[i]);
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

		if (has_nt_hash)
		{
			LC7Hash hash;
			hash.hashtype = FOURCC(HASHTYPE_NT);
			hash.crackstate = 0;
			hash.cracktime = 0;
			for (int i = 0; i < sizeof(ntlm_hash); i++)
			{
				hash.hash += QString().sprintf("%2.2X", ntlm_hash[i]);
			}
		
			if (hash.hash == "31D6CFE0D16AE931B73C59D7E0C089C0")
			{
				hash.crackstate = CRACKSTATE_CRACKED;
				hash.cracktype = "No Password";
			}

			acct.hashes.append(hash);
		}
		
		acct.userid=QString("%1").arg(dwRID);
		acct.domain=QString::fromUtf16(wsvDomainBuf);
		acct.username=QString::fromUtf16(wsvUsernameBuf);
		QString fullname = QString::fromUtf16(wsvFullnameBuf);
		QString comment = QString::fromUtf16(wsvCommentBuf);
		//QString homedir = QString::fromUtf16(wsvHomedirBuf);
		acct.userinfo = fullname;
		if (comment.size() > 0)
		{
			if (acct.userinfo.size() > 0)
			{
				acct.userinfo += " ";
			}
			acct.userinfo += QString("(%1)").arg(comment);
		}
		//if (homedir.size() > 0)
		//{
		//	acct.userinfo += QString(" [%1]").arg(homedir);
		//}
		acct.lastchanged = (ullAge > 0) ? (ullAge / 10000000ULL - 11644473600ULL) : 0;
		acct.lockedout=bLockedOut?1:0;
		acct.disabled=bDisabled?1:0;
		acct.neverexpires=bNeverExpires?1:0;
		acct.mustchange=bExpired?1:0;

		// Add user entry
		if(m_bRemote)
		{
			acct.machine=m_strRemoteMachine;
		}

		// Add remediation config
		if (remconfig == -1 && !m_remediations.isEmpty())
		{
			remconfig = m_accountlist->AddRemediations(m_remediations);
		}
		acct.remediations = remconfig;

		if(!m_accountlist->AppendAccount(acct))
		{
			error="Account limit reached, upgrade your license to import more accounts.";
			success=false;
			break;
		}
		
		dwRealImportedCount++;
		if (m_account_limit && dwRealImportedCount >= m_account_limit)
		{
			break;
		}
	};
	
	QString str=QString("%1 of %2 users imported").arg(dwRealImportedCount).arg(m_number_of_users);
	if(nMachineAccounts!=0)
	{
		str+=QString(" (%1 machine accounts removed)").arg(nMachineAccounts);
	}

	UpdateStatus(str,100);

	if(dwRealImportedCount<m_number_of_users)
	{
		std::set<int> positions;
		for (int x=starting_count+dwRealImportedCount;x<(starting_count+m_number_of_users);x++)
		{
			positions.insert(x);
		}
		m_accountlist->RemoveAccounts(positions);
	}
	m_accountlist->Release();
	
	if (!success)
	{
		throw error;
	}
}

bool CImportWindows::WarnMITM(bool changed)
{TR;
	bool res;
	if(changed)
	{
		res=g_pLinkage->GetGUILinkage()->YesNoBox("Caution!","The LC7Agent on the remote machine has been changed.\n\n"
			"This could be the result of a new version of LC7 being used, or multiple copies of LC7 being used on the same remote server.\n"
			"Also, there is a possibility of a man-in-the-middle attack unless you are using NTLMv2 between your machine and the server.\n"
			"If this is not acceptable for your environment, please install the LC7 Agent by hand on the target machine, following the directions in the documentation.\nAre you sure you want to proceed?");
	}
	else
	{
		res=g_pLinkage->GetGUILinkage()->YesNoBox("Copying LC7 Agent","The LC7Agent on the remote machine will be installed.\n\n"
			"There is a possibility of a man-in-the-middle attack unless you are using NTLMv2 between your machine and the server.\n"
			"If this is not acceptable for your environment, please install the LC7 Agent by hand on the target machine, following the directions in the documentation.\nAre you sure you want to proceed?");				
	}
	return res;
}


bool CImportWindows::DoImport(QString & error, bool & cancelled)
{TR;
	bool bRet = true;
	try
	{
		if (m_ctrl)
			m_ctrl->SetStatusText("Starting import...");

		if (m_bRemote)
		{
			DoImportSMB(cancelled);
		}
		else
		{
			DoImportLocal(cancelled);
		}
	}
	catch (const char *err)
	{
		error = QString::fromLatin1(err);
		bRet = false;
	}
	catch (QString err)
	{
		error = err;
		bRet = false;
	}

	return bRet;
}

void CImportWindows::TouchEmptyFile(QString path)
{
	QFile f(path);
	if (!f.open(QIODevice::WriteOnly))
	{
		f.close();
		throw QString("Couldn't open file for writing: %1").arg(path);
	}
	f.resize(0);
	f.close();
}



void CImportWindows::DoImportLocal(bool & cancelled)
{
	QString error;
	bool bRet = true;

	QString tempdirname = g_pLinkage->NewTemporaryDir();
	try
	{
		CheckIs64Bit();

		// we need the target file name, not just the target dir
		GetAllUsefulPathnames();

		QByteArray agent, agentdll;
		bool agent_exists=false;
		bool install_agent = false;
		if (NeedsAgentUpdate(agent, agentdll, agent_exists, cancelled))
		{
			install_agent = true;
		}

		// Write out agent and communicate with it
		QDir tempdir(tempdirname);
		QString myagentpath = tempdir.absoluteFilePath(m_bIs64Bit ? "lc7agent64.exe" : "lc7agent.exe");
		QString myagentdllpath = tempdir.absoluteFilePath(m_bIs64Bit ? "lc7dump64.dll" : "lc7dump.dll");
		WriteAgentFile(myagentpath, agent);
		WriteAgentFile(myagentdllpath, agentdll);
		
		// Make local dump file we can read
		QString mydumppath = tempdir.absoluteFilePath("lc7agent.dmp");
		QString mystatuspath = tempdir.absoluteFilePath("lc7agent.dmp.status");
		TouchEmptyFile(mydumppath);
		TouchEmptyFile(mystatuspath);
			
		// Run dump command
		QString commandline = QString("\"%1\" %2 /dump \"%3\"").arg(QDir::toNativeSeparators(myagentpath)).arg(install_agent ? "/install " : "").arg(QDir::toNativeSeparators(mydumppath));
		
		STARTUPINFOW si;
		memset(&si, 0, sizeof(si));
		si.cb = sizeof(STARTUPINFOW);
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;

		PROCESS_INFORMATION pi;

		if (m_bSpecificUser)
		{
			WCHAR cmdlinebuf[1024];
			wcscpy_s(cmdlinebuf, 1024, commandline.toStdWString().c_str());

			if (!CreateProcessWithLogonW(
				m_strUser.toStdWString().c_str(),
				m_strDomain.toStdWString().c_str(),
				m_strPassword.toStdWString().c_str(),
				0,
				NULL,
				cmdlinebuf,
				0,
				NULL,
				NULL,
				&si,
				&pi))
			{
				throw "Couldn't create process with specified credentials. Username or password may be incorrect.";
			}
		}
		else
		{
			WCHAR cmdlinebuf[1024];
			wcscpy_s(cmdlinebuf, 1024, commandline.toStdWString().c_str());
			if (!CreateProcessW(
				NULL,
				cmdlinebuf,
				NULL,
				NULL,
				FALSE,
				0,
				NULL,
				NULL,
				&si,
				&pi))
			{
				throw "Couldn't create process as specified user.";
			}
		}
		CloseHandle(pi.hThread);

		// Sit around waiting and update status
		WaitForCompletion(mystatuspath, pi.hProcess, cancelled);
		
		CloseHandle(pi.hProcess);

		if (cancelled)
		{
			throw "Operation cancelled";
		}

		DecryptAndImportHashes(mydumppath, cancelled);
		if (cancelled)
		{
			throw "Operation cancelled";
		}
	}
	catch (...)
	{
		// Delete remote file and local dump file as well if they exist
		// Ignore if this fails.
		QDir(tempdirname).removeRecursively();
		throw;
	}

	// Delete remote file and local dump file as well if they exist
	// Ignore if this fails.
	QDir(tempdirname).removeRecursively();
}

void CImportWindows::ApplyCredentials()
{
	QString error;
	// Remove any old uses and try to establish a connection if we're doing this remotely
	// If we're doing this locally, log on and impersonate the requested user
	if (m_bRemote)
	{
		if (!WIN_RemoveNetUses(m_strRemoteMachineWithSlashes, error))
		{
			throw QString("Failed to remove net uses: %1").arg(error);
		}
		UpdateStatus("Accessing remote admin share...", 0);
		if (m_bSpecificUser)
		{
			if (!WIN_ConnectNetUse(m_strRemoteAdmin, m_strUser, m_strPassword, m_strDomain, error))
			{
				throw QString("Failed to connect net use: %1").arg(error);
			}
		}
		else
		{
			if (!WIN_ConnectNetUseDefault(m_strRemoteAdmin, error))
			{
				throw QString("Failed to connect net use with default credentials: %1").arg(error);
			}
		}
		m_bEstablishedConnection = true;
	}
	else
	{
		if (m_bSpecificUser)
		{
			if (!WIN_Impersonate(m_strUser, m_strDomain, m_strPassword, error))
			{
				throw QString("Failed to impersonate user: %1").arg(error);
			}
			m_bImpersonated = true;
		}
	}
}

bool CImportWindows::NeedsAgentUpdate(QByteArray & agent, QByteArray & agentdll, bool &agent_exists, bool & cancelled)
{
	// Get the private key
	bool pkcreated = false;
	GetOrCreatePrivateKey(pkcreated);

	// Create agent and dll
	CreateAgentAndDLL(agent, agentdll);
	
	// See if the remote agent exists
	agent_exists = WIN_PathFileExists(m_strTarget) && WIN_PathFileExists(m_strTargetDLL);

	// If it's a new private key then redistribute the agent
	if(!pkcreated && agent_exists)
	{
		// If there's an agent there already then compare against this agent to ensure it's the same key
		
		QByteArray targetagent;
		QString error;
		if (!WIN_ReadDataFromFile(m_strTarget, targetagent, error))
		{
			throw QString("Unable to read data from file: %1").arg(error);
		}
		if (targetagent.length() == agent.length() &&
			memcmp(agent.data(), targetagent.data(), agent.length()) == 0)
		{
			QByteArray targetagentdll;
			if (!WIN_ReadDataFromFile(m_strTargetDLL, targetagentdll, error))
			{
				throw QString("Unable to read data from file: %1").arg(error);
			}
			if (targetagentdll.length() == agentdll.length() &&
				memcmp(agentdll.data(), targetagentdll.data(), agentdll.length()) == 0)
			{
				// Same, don't redistribute
				return false;
			}
		}
	}

	// Redistribute the agent
	return true;
}


void CImportWindows::DoImportSMB(bool & cancelled)
{TR;
	QString error;
	bool bRet = true;

	UpdateStatus("Attempting agent connection...", 0);

	m_hRemoteSC=INVALID_WIN_SC_HANDLE;
	m_hLcAgent=INVALID_WIN_SERVICE_HANDLE;
	m_bEstablishedConnection=false;
	m_bImpersonated=false;
	m_bAgentStarted = false;

	// Get temp path for local hash file
	QString strLocalDMP = g_pLinkage->NewTemporaryFile();

	try
	{
		
		// Apply credentials
		ApplyCredentials();

		// Once established, check to see if the remote is 64-bit
		CheckIs64Bit();

		// connect to remote machine's SCM
		UpdateStatus(m_bRemote?"Connecting to remote machine...":"Connecting to local machine...",0);	

		m_hRemoteSC = WIN_OpenSCManager(m_bRemote?m_strRemoteMachineWithSlashes:"");
		if (m_hRemoteSC == NULL)
		{
			throw "Failed to connect to service control manager. The target machine may not have its firewall configured properly.\nEither install the remote agent, or configure the firewall manually. Refer to the LC7 documentation for details.";
		}

		// we need the target file name, not just the target dir
		GetAllUsefulPathnames();

		// Ensure we are running the latest agent with the proper key
		QByteArray agent, agentdll;
		bool agent_exists=false;
		if (NeedsAgentUpdate(agent, agentdll, agent_exists, cancelled))
		{
			if (m_bRemote && !WarnMITM(agent_exists))
			{
				cancelled = true;
				throw "";
			}

			// Stop remote agent
			bool bAgentExists = false;
			if (!WIN_StopService(m_hRemoteSC, "LC7Agent", bAgentExists, error) && bAgentExists)
			{
				throw QString("Unable to stop service: %1").arg(error);
			}

			CopyAgent(agent, agentdll);
		}
		if (cancelled)
		{
			throw "";
		}
	
		// If this is the right agent binary now, in the right place
		// install it if necessary, then start it.
		UpdateStatus("Starting LC Agent service...",0);
	
		WIN_SERVICE_HANDLE hLcAgent;
		if(!WIN_StartService(m_hRemoteSC,"LC7Agent",m_strTarget,"%SystemRoot%\\lc7agent.exe","LC7Agent","LC7 Remote Agent",hLcAgent,error))
		{
			throw "Unable to start remote agent";
		}
		m_bAgentStarted=TRUE;

		// Delete dump file if it exists, and can be deleted. If it's owned by another process, this will silently fail and be picked up later
		// when we check to see if the file is still there.
		WIN_DeleteFile(m_strRemoteDMP);
		WIN_DeleteFile(m_strRemoteDMPStatus);

		// See if the dump file already exists, if so, bail
		if(WIN_PathFileExists(m_strRemoteDMP))
		{
			throw "A hash dump operation is already in progress on the target machine\n"
				  "Please wait until it completes before issuing another request.\n"
				  "If no dump is currently in progress, remove file %SystemRoot%\\Temp\\lc7agent.dmp from the target system.\n";
		}

		// tell the agent to dump the hashes
		IssueHashDumpRequest(hLcAgent);

		// Sit around waiting and update status
		WaitForCompletion(m_strRemoteDMPStatus, NULL, cancelled);
		if (cancelled)
		{
			throw "";
		}

		// snag the file with the hashes in it from the remote machine
		UpdateStatus("Copying hashes from remote machine...",0);
		bool bRet = WIN_CopyFile(m_strRemoteDMP, strLocalDMP);
		if(!bRet)
		{
			throw QString("Couldn't copy hash data from %1 to %2").arg(m_strRemoteDMP, strLocalDMP);
		}

		DecryptAndImportHashes(strLocalDMP, cancelled);
		if (cancelled)
		{
			throw "";
		}
	}
	catch(const char *err)
	{
		error = QString::fromLatin1(err);
		if (!cancelled)
			bRet=false;
	}
	catch (QString err)
	{
		error = err;
		if (!cancelled)
			bRet = false;
	}

	// Delete remote file and local dump file as well if they exist
	// Ignore if this fails.
	WIN_DeleteFile(m_strRemoteDMP);
	WIN_DeleteFile(m_strRemoteDMPStatus);
	WIN_DeleteFile(strLocalDMP);

	if(m_hLcAgent)
	{
		CloseServiceHandle(m_hLcAgent);
	}
	if (m_bAgentStarted)
	{
		bool bAgentExists=false;
		if (!WIN_StopService(m_hRemoteSC, "LC7Agent", bAgentExists, error))
		{
			bRet=false;
		}
	}
	if (m_hRemoteSC != NULL)
	{
		CloseServiceHandle(m_hRemoteSC);
	}
	if (m_bImpersonated)
	{
		RevertToSelf();
	}
	if (m_bEstablishedConnection)
	{
		if(!WIN_RemoveNetUses(m_strRemoteMachineWithSlashes,error))
		{
			bRet=false;
		}	
	}

	if (!bRet)
	{
		throw error;
	}
}

void CImportWindows::WriteAgentFile(QString filepath, QByteArray filedata)
{
	QFile f(filepath);
	if(!f.open(QIODevice::ReadWrite))
	{
		throw "";
	}
	if(!f.write(filedata))
	{
		throw "";
	}
	f.close();
}

bool CImportWindows::CreateRemoteAgent(QString folder, QString & error)
{TR;	
	QString tempdir;
	QDir agentfolder(folder);
	
	bool bRet=true;
	try
	{
		// Get the private key
		bool pkcreated=false;
		GetOrCreatePrivateKey(pkcreated);

		// Get agent source location
		QDir agentdir(g_pLinkage->GetStartupDirectory());		
		agentdir.cd("lcplugins");
		agentdir.cd("lc7importwin{17324176-3fa7-4c1a-9204-3f391b6b3599}");
		agentdir.cd("lcagent");
	
		// Create 32-bit agent and dll
		QByteArray agent32;
		QByteArray agent32dll;
		m_strSource=agentdir.absoluteFilePath("lc7agent.exe");
		m_strSourceDLL=agentdir.absoluteFilePath("lc7dump.dll");
		CreateAgentAndDLL(agent32, agent32dll);
		
		WriteAgentFile(agentfolder.absoluteFilePath("lc7agent.exe"),agent32);
		WriteAgentFile(agentfolder.absoluteFilePath("lc7dump.dll"),agent32dll);

		// Create 64-bit agent and dll
		QByteArray agent64;
		QByteArray agent64dll;
		m_strSource=agentdir.absoluteFilePath("lc7agent64.exe");
		m_strSourceDLL=agentdir.absoluteFilePath("lc7dump64.dll");
		CreateAgentAndDLL(agent64, agent64dll);
		
		WriteAgentFile(agentfolder.absoluteFilePath("lc7agent64.exe"),agent64);
		WriteAgentFile(agentfolder.absoluteFilePath("lc7dump64.dll"),agent64dll);

	}	
	catch(const char *err)
	{
		error = QString::fromLatin1(err);
		bRet=false;
	}
	catch (QString err)
	{
		error = err;
		bRet = false;
	}


	return bRet;	
}

quint32 CImportWindows::GetNumberOfUsers(void)
{TR;
	return m_number_of_users;
}

void showInGraphicalShell(QWidget *parent, const QString &pathIn)
{
	const QFileInfo fileInfo(pathIn);
	// Mac, Windows support folder or file.
#ifdef WIN32
	QStringList param;
	if (!fileInfo.isDir())
		param += QLatin1String("/select,");
	param += QDir::toNativeSeparators(fileInfo.canonicalFilePath());
	QProcess::startDetached("explorer.exe", param);
#elif defined(APPLE)
	
#else
	
#endif
}

bool CImportWindows::CreateRemoteAgentInstaller(QString target, QString &error)
{
	QString temp = g_pLinkage->NewTemporaryDir();
	
	if (!CreateRemoteAgent(temp, error))
	{
		return false;
	}

	// Get agent dest location
	QDir tempdir(temp);

	// Get agent source location
	QDir agentdir(g_pLinkage->GetStartupDirectory());
	agentdir.cd("lcplugins");
	agentdir.cd("lc7importwin{17324176-3fa7-4c1a-9204-3f391b6b3599}");
	agentdir.cd("lcagent");

	// Write installer
	QFile::copy(agentdir.absoluteFilePath("lc7remoteagent.exe"), tempdir.absoluteFilePath("lc7remoteagent.exe"));

	showInGraphicalShell(nullptr, temp);

	return true;
}


void CImportWindows::SetAccountsToRemediate(QList<int> accounts_to_force_change, QList<int> accounts_to_disable)
{
	m_accounts_to_force_change = accounts_to_force_change;
	m_accounts_to_disable = accounts_to_disable;
}

void CImportWindows::OpenRemediationCommandFile(QString myremediationpath, quint32 remsize)
{
	Q_ASSERT(m_remedation_command_file == NULL);
	if (m_remedation_command_file != NULL)
	{
		CloseRemediationCommandFile();
	}

	m_remedation_command_file = new QFile(myremediationpath);
	if (!m_remedation_command_file->open(QIODevice::WriteOnly))
	{
		throw QString("Unable to open file for writing: %1").arg(myremediationpath);
	}

	if (m_remedation_command_file->write((const char *)&remsize, sizeof(remsize)) != sizeof(remsize))
	{
		throw "error writing remediation count";
	}
}

void CImportWindows::CloseRemediationCommandFile()
{
	Q_ASSERT(m_remedation_command_file != NULL);
	if (m_remedation_command_file)
	{
		m_remedation_command_file->close();
		delete m_remedation_command_file;
		m_remedation_command_file = NULL;
	}
}


void CImportWindows::WriteRemediationCommand(quint32 command, QList<int> accountnumbers)
{
	if (m_remedation_command_file->write((const char *)&command, sizeof(command)) != sizeof(command))
	{
		throw "error writing command";
	}
	
	quint32 acctnums = accountnumbers.size();
	if (m_remedation_command_file->write((const char *)&acctnums, sizeof(acctnums)) != sizeof(acctnums))
	{
		throw "error writing account count";
	}
	
	m_accountlist->Acquire();

	foreach(int acctnum, accountnumbers)
	{
		const LC7Account *acct = m_accountlist->GetAccountAtConstPtrFast(acctnum);

		const ushort *username16 = acct->username.utf16();
		QByteArray username((const char *)username16,acct->username.size() * 2);

		quint32 usernamesize = username.size();
		if (m_remedation_command_file->write((const char *)&usernamesize, sizeof(usernamesize)) != sizeof(usernamesize))
		{
			m_accountlist->Release();
			throw "error writing username length";
		}
		if (m_remedation_command_file->write(username) != username.size())
		{
			m_accountlist->Release();
			throw "error writing username";
		}
	}

	m_accountlist->Release();
}

void CImportWindows::PerformRemediations(QString remediationpath, bool & cancelled)
{
	// See what failed and what succeeded
	QFile remresults(remediationpath);
	if (!remresults.open(QIODevice::ReadOnly))
	{
		throw QString("Unable to get remediation results for '%1'").arg(m_bRemote ? m_strRemoteMachine : "Local Machine");
	}
	quint32 successcnt[2] = { 0, 0 };
	quint32 failurecnt[2] = { 0, 0 };

	m_accountlist->Acquire();

	// Build reverse hash
	QMap<QString, int> name_to_index;
	foreach(int idx, m_accounts_to_force_change)
	{
		const LC7Account *acct = m_accountlist->GetAccountAtConstPtrFast(idx);
		if (!name_to_index.contains(acct->username))
		{
			name_to_index[acct->username] = idx;
		}
	}
	foreach(int idx, m_accounts_to_disable)
	{
		const LC7Account *acct = m_accountlist->GetAccountAtConstPtrFast(idx);
		if (!name_to_index.contains(acct->username))
		{
			name_to_index[acct->username] = idx;
		}
	}

	while (!remresults.atEnd())
	{
		quint32 cmd;
		if (remresults.read((char *)&cmd, sizeof(quint32)) != sizeof(quint32) || cmd < 1 || cmd>2)
		{
			throw "Unable to read command from results file";
		}

		quint32 usernamelen;
		if (remresults.read((char *)&usernamelen, sizeof(quint32)) != sizeof(quint32) || usernamelen > (256 * 2) || ((usernamelen & 1) != 0))
		{
			throw "Unable to read username length from results file";
		}

		wchar_t username[257];
		if (remresults.read((char *)username, usernamelen) != usernamelen)
		{
			throw "Unable to read username from results file";
		}
		username[usernamelen / 2] = 0;

		quint32 result;
		if (remresults.read((char *)&result, sizeof(quint32)) != sizeof(quint32))
		{
			throw "Unable to read result from results file";
		}

		if (result == 1)
		{
			successcnt[cmd - 1]++;

			int idx = name_to_index.value(QString::fromWCharArray(username), -1);
			if (idx != -1)
			{
				LC7Account acct = *m_accountlist->GetAccountAtConstPtrFast(idx);
				if (cmd == 1)
				{
					acct.disabled = true;
				}
				else if (cmd == 2)
				{
					acct.mustchange = true;
				}
				m_accountlist->ReplaceAccountAt(idx, acct);
			}
		}
		else
		{
			failurecnt[cmd - 1]++;
		}
	}
	m_accountlist->Release();

	if (m_ctrl)
	{
		m_ctrl->AppendToActivityLog(QString("Remediation Status For '%1'\n").arg(m_bRemote ? m_strRemoteMachine : "Local Machine"));
		m_ctrl->AppendToActivityLog("-------------------------------------\n");
		if (m_accounts_to_disable.size() > 0)
		{
			if (successcnt[0] != 0)
				m_ctrl->AppendToActivityLog(QString("Successfully disabled %1 accounts\n").arg(successcnt[0]));
			if (failurecnt[0] != 0)
				m_ctrl->AppendToActivityLog(QString("Failed to disable %1 accounts\n").arg(failurecnt[0]));
		}
		if (m_accounts_to_force_change.size() > 0)
		{
			if (successcnt[1] != 0)
				m_ctrl->AppendToActivityLog(QString("Successfully expired %1 accounts (forced password change)\n").arg(successcnt[1]));
			if (failurecnt[1] != 0)
				m_ctrl->AppendToActivityLog(QString("Failed to expire %1 accounts (could not force password change)\n").arg(failurecnt[1]));
		}
	}
}

void CImportWindows::WriteRemediationCommandFile(QString myremediationpath)
{
	OpenRemediationCommandFile(myremediationpath, m_accounts_to_force_change.size() + m_accounts_to_disable.size());

	if (m_accounts_to_disable.size() > 0)
	{
		WriteRemediationCommand(1, m_accounts_to_disable);
	}
	if (m_accounts_to_force_change.size() > 0)
	{
		WriteRemediationCommand(2, m_accounts_to_force_change);
	}

	CloseRemediationCommandFile();
}

void CImportWindows::DoRemediateLocal(bool & cancelled)
{
	QString error;
	bool bRet = true;

	if (m_accounts_to_force_change.size() == 0 && m_accounts_to_disable.size() == 0)
	{
		return;
	}

	QString tempdirname = g_pLinkage->NewTemporaryDir();
	try
	{
		CheckIs64Bit();

		// we need the target file name, not just the target dir
		GetAllUsefulPathnames();

		QByteArray agent, agentdll;
		bool agent_exists = false;
		bool install_agent = false;
		if (NeedsAgentUpdate(agent, agentdll, agent_exists, cancelled))
		{
			install_agent = true;
		}

		// Write out agent and communicate with it
		QDir tempdir(tempdirname);
		QString myagentpath = tempdir.absoluteFilePath(m_bIs64Bit ? "lc7agent64.exe" : "lc7agent.exe");
		QString myagentdllpath = tempdir.absoluteFilePath(m_bIs64Bit ? "lc7dump64.dll" : "lc7dump.dll");
		WriteAgentFile(myagentpath, agent);
		WriteAgentFile(myagentdllpath, agentdll);

		// Make local dump file we can read
		QString myremediationpath = tempdir.absoluteFilePath("lc7agent.rem");
		QString mystatuspath = tempdir.absoluteFilePath("lc7agent.rem.status");
		TouchEmptyFile(myremediationpath);
		TouchEmptyFile(mystatuspath);

		// Run remediation command
		QString commandline = QString("\"%1\"").arg(QDir::toNativeSeparators(myagentpath));
		if (install_agent)
		{
			commandline += " /install";
		}

		WriteRemediationCommandFile(myremediationpath);

		commandline += QString(" /remediate \"%1\"").arg(QDir::toNativeSeparators(myremediationpath));

		STARTUPINFOW si;
		memset(&si, 0, sizeof(si));
		si.cb = sizeof(STARTUPINFOW);
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;

		PROCESS_INFORMATION pi;

		if (m_bSpecificUser)
		{
			WCHAR cmdlinebuf[1024];
			wcscpy_s(cmdlinebuf, 1024, commandline.toStdWString().c_str());

			if (!CreateProcessWithLogonW(
				m_strUser.toStdWString().c_str(),
				m_strDomain.toStdWString().c_str(),
				m_strPassword.toStdWString().c_str(),
				0,
				NULL,
				cmdlinebuf,
				0,
				NULL,
				NULL,
				&si,
				&pi))
			{
				throw "Couldn't create process with specified credentials. Username or password may be incorrect.";
			}
		}
		else
		{
			WCHAR cmdlinebuf[1024];
			wcscpy_s(cmdlinebuf, 1024, commandline.toStdWString().c_str());
			if (!CreateProcessW(
				NULL,
				cmdlinebuf,
				NULL,
				NULL,
				FALSE,
				0,
				NULL,
				NULL,
				&si,
				&pi))
			{
				throw "Couldn't create process as specified user.";
			}
		}
		CloseHandle(pi.hThread);

		// Sit around waiting and update status
		WaitForCompletion(mystatuspath, pi.hProcess, cancelled);

		CloseHandle(pi.hProcess);
		if (cancelled)
		{
			throw "";
		}

		// Apply remediations
		PerformRemediations(myremediationpath, cancelled);
		if (cancelled)
		{
			throw "";
		}
	}
	catch (...)
	{
		// Delete remote file and local dump file as well if they exist
		// Ignore if this fails.
		QDir(tempdirname).removeRecursively();
		throw;
	}

	// Delete remote file and local dump file as well if they exist
	// Ignore if this fails.
	QDir(tempdirname).removeRecursively();


}

void CImportWindows::DoRemediateSMB(bool & cancelled)
{
	QString error;
	bool bRet = true;

	m_hRemoteSC = INVALID_WIN_SC_HANDLE;
	m_hLcAgent = INVALID_WIN_SERVICE_HANDLE;
	m_bEstablishedConnection = false;
	m_bImpersonated = false;
	m_bAgentStarted = false;

	QString strLocalREMIn = g_pLinkage->NewTemporaryFile();
	QString strLocalREMOut = g_pLinkage->NewTemporaryFile();

	try
	{

		// Apply credentials
		ApplyCredentials();

		// Once established, check to see if the remote is 64-bit
		CheckIs64Bit();

		// connect to remote machine's SCM
		UpdateStatus(m_bRemote ? "Connecting to remote machine..." : "Connecting to local machine...", 0);

		m_hRemoteSC = WIN_OpenSCManager(m_bRemote ? m_strRemoteMachineWithSlashes : "");
		if (m_hRemoteSC == NULL)
		{
			throw "Failed to connect to service control manager. The target machine may not have its firewall configured properly.\nEither install the remote agent, or configure the firewall manually. Refer to the LC7 documentation for details.";
		}

		// we need the target file name, not just the target dir
		GetAllUsefulPathnames();

		// Ensure we are running the latest agent with the proper key
		QByteArray agent, agentdll;
		bool agent_exists = false;
		if (NeedsAgentUpdate(agent, agentdll, agent_exists, cancelled))
		{
			if (m_bRemote && !WarnMITM(agent_exists))
			{
				cancelled = true;
				throw "";
			}

			// Stop remote agent
			bool bAgentExists = false;
			if (!WIN_StopService(m_hRemoteSC, "LC7Agent", bAgentExists, error) && bAgentExists)
			{
				throw QString("Unable to stop service: %1").arg(error);
			}

			CopyAgent(agent, agentdll);
		}
		if (cancelled)
		{
			throw "";
		}

		// If this is the right agent binary now, in the right place
		// install it if necessary, then start it.
		UpdateStatus("Starting LC Agent service...", 0);

		WIN_SERVICE_HANDLE hLcAgent;
		if (!WIN_StartService(m_hRemoteSC, "LC7Agent", m_strTarget, "%SystemRoot%\\lc7agent.exe", "LC7Agent", "LC7 Remote Agent", hLcAgent, error))
		{
			throw "Can't start remote agent";
		}
		m_bAgentStarted = TRUE;

		// Delete dump file if it exists, and can be deleted. If it's owned by another process, this will silently fail and be picked up later
		// when we check to see if the file is still there.
		WIN_DeleteFile(m_strRemoteREMIn);
		WIN_DeleteFile(m_strRemoteREMOut);
		WIN_DeleteFile(m_strRemoteREMStatus);

		// See if the dump file already exists, if so, bail
		if (WIN_PathFileExists(m_strRemoteREMIn) || WIN_PathFileExists(m_strRemoteREMOut) || WIN_PathFileExists(m_strRemoteREMStatus))
		{
			throw "A remediation operation is already in progress on the target machine\n"
				"Please wait until it completes before issuing another request.\n"
				"If no remediation is currently in progress, remove files %SystemRoot%\\Temp\\lc7agent.rem.* from the target system.\n";
		}
		
		// Write out local remediations file
		WriteRemediationCommandFile(strLocalREMIn);

		// Send the remediations to the remote machine
		UpdateStatus("Sending remediations to remote machine...", 0);
		if (!WIN_CopyFile(strLocalREMIn, m_strRemoteREMIn))
		{
			throw "Couldn't copy remediation commands";
		}
		
		// tell the agent to dump the hashes
		IssueRemediationRequest(hLcAgent);

		// Sit around waiting and update status
		WaitForCompletion(m_strRemoteREMStatus, NULL, cancelled);
		if (cancelled)
		{
			throw "";
		}

		// snag the file with the remediation results in it from the remote machine
		UpdateStatus("Getting results from remote machine...", 0);
		if (!WIN_CopyFile(m_strRemoteREMOut, strLocalREMOut))
		{
			throw "Couldn't copy remediation results";
		}

		PerformRemediations(strLocalREMOut, cancelled);
		if (cancelled)
		{
			throw "";
		}
	}
	catch (const char *err)
	{
		error = QString::fromLatin1(err);
		if (!cancelled)
			bRet = false;
	}
	catch (QString err)
	{
		error = err;
		if (!cancelled)
			bRet = false;
	}

	// Delete remote file and local dump file as well if they exist
	// Ignore if this fails.
	WIN_DeleteFile(m_strRemoteREMIn);
	WIN_DeleteFile(m_strRemoteREMOut);
	WIN_DeleteFile(m_strRemoteREMStatus);
	WIN_DeleteFile(strLocalREMOut);
	WIN_DeleteFile(strLocalREMIn);

	if (m_hLcAgent)
	{
		CloseServiceHandle(m_hLcAgent);
	}
	if (m_bAgentStarted)
	{
		bool bAgentExists = false;
		if (!WIN_StopService(m_hRemoteSC, "LC7Agent", bAgentExists, error))
		{
			bRet = false;
		}
	}
	if (m_hRemoteSC != NULL)
	{
		CloseServiceHandle(m_hRemoteSC);
	}
	if (m_bImpersonated)
	{
		RevertToSelf();
	}
	if (m_bEstablishedConnection)
	{
		if (!WIN_RemoveNetUses(m_strRemoteMachineWithSlashes, error))
		{
			bRet = false;
		}
	}

	if (!bRet)
	{
		throw error;
	}

}

bool CImportWindows::DoRemediate(QString & error, bool & cancelled)
{
	bool bRet = true;
	try
	{
		if (m_ctrl)
			m_ctrl->SetStatusText("Starting remediation...");

		if (m_bRemote)
		{
			DoRemediateSMB(cancelled);
		}
		else
		{
			DoRemediateLocal(cancelled);
		}

	}
	catch (const char *err)
	{
		error = QString::fromLatin1(err);
		bRet = false;
	}
	catch (QString err)
	{
		error = err;
		bRet = false;
	}

	return bRet;
}

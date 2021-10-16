#include<stdafx.h>

#if defined(_WIN32)

#include<ws2def.h>
#include<ws2ipdef.h>
#include<ws2tcpip.h>
#define _last_sock_err() WSAGetLastError()

#else 

#include<errno.h>
#include<sys/socket.h>
#include<arpa/inet.h>

#define _last_sock_err() errno

#endif

UnixSSHImporter::UnixSSHImporter(ILC7AccountList *accountlist, ILC7CommandControl *ctrl)
{
	TR;

	if (ctrl)
		m_ctrl = ctrl->GetSubControl("SSH Importer: ");
	else
		m_ctrl = nullptr;

	m_accountlist = accountlist;
	m_account_limit = 0;

	// Init libssh2 and make session

	libssh2_init(0);
	m_socket = NULL;
	m_ssh = NULL;
	m_channel = NULL;
}

UnixSSHImporter::~UnixSSHImporter()
{
	TR;
	if (m_ctrl)
		m_ctrl->ReleaseSubControl();
	
	if (m_channel) 
	{
		libssh2_channel_free(m_channel);
	}
	if (m_ssh)
	{
		libssh2_session_free(m_ssh);
	}
	if (m_socket)
	{
		closesocket(m_socket);
	}
}


void UnixSSHImporter::SetAccountLimit(quint32 alim)
{
	m_account_limit = alim;
}

void UnixSSHImporter::UpdateStatus(QString statustext, quint32 cur, bool statuslog)
{
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

void UnixSSHImporter::setHost(QString host)
{
	m_port = 22;
	m_host = host;

	if (m_host.contains(":"))
	{
		QStringList parts = m_host.split(":");
		if (parts.size() == 2)
		{
			bool ok = true;
			int port = parts[1].toInt(&ok);
			if (ok && port >= 1 && port<=65535)
			{
				m_port = port;
				m_host = parts[0];
			}
		}
	}
}

void UnixSSHImporter::setIncludeNonLogin(bool include_non_login)
{
	m_include_non_login = include_non_login;
}

void UnixSSHImporter::setAuthType(UnixSSHImporter::AUTHTYPE authtype)
{
	m_authtype = authtype;
}

void UnixSSHImporter::setUsername(QString username)
{
	m_username = username;
}

void UnixSSHImporter::setPassword(QString password)
{
	m_password = password;
}

void UnixSSHImporter::setPrivateKeyFile(QString privatekeyfile)
{
	m_privkeyfile = privatekeyfile;
}

void UnixSSHImporter::setPrivateKeyPassword(QString password)
{
	m_privkeypassword = password;
}

void UnixSSHImporter::setElevType(UnixSSHImporter::ELEVTYPE elevtype)
{
	m_elevtype = elevtype;
}

void UnixSSHImporter::setSUDOPassword(QString sudopassword)
{
	m_sudopassword = sudopassword;
}

void UnixSSHImporter::setSUPassword(QString supassword)
{
	m_supassword = supassword;
}

void UnixSSHImporter::gracefulTerminate()
{
	if (m_channel)
	{
		if (libssh2_channel_send_eof(m_channel))
		{
			throw FailException("Error sending EOF");
		}

		if (libssh2_channel_close(m_channel))
		{
			throw FailException("Error closing channel");
		}

		libssh2_channel_free(m_channel);
		m_channel = NULL;
	}

	if (m_ssh) {		
		libssh2_session_disconnect(m_ssh, "graceful termination");
		libssh2_session_free(m_ssh);
		m_ssh = NULL;
	}

	if (m_socket) {
		closesocket(m_socket);
		m_socket = NULL;
	}
}


void UnixSSHImporter::connectAndAuthenticate(void)
{
	// Resolve host
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_port = htons(m_port);
	bool sin_valid = false;

	struct sockaddr_in6 sin6;
	memset(&sin6, 0, sizeof(sin6));
	sin6.sin6_port = htons(m_port);
	bool sin6_valid = false;

	
	QByteArray host = m_host.toLatin1();
	if (inet_pton(AF_INET, host.constData(), &(sin.sin_addr)) > 0) {
		sin.sin_family = AF_INET;
		sin_valid = true;
	}
	else if (inet_pton(AF_INET6, host.constData(), &(sin6.sin6_addr)) > 0) {
		sin6.sin6_family = AF_INET6;
		sin6_valid = true;
	}
	else {
		struct addrinfo hints;
		memset(&hints, 0, sizeof(struct addrinfo));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM; 
		hints.ai_flags = AI_ALL;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_canonname = NULL;
		hints.ai_addr = NULL;
		hints.ai_next = NULL;

		struct addrinfo *result;
		int s = getaddrinfo(host.constData(), NULL, &hints, &result);
		if (s != 0 || result == nullptr)
		{
			throw FailException("Error resolving host");
		}
		
		bool ok = false;
		for (auto rp = result; rp != nullptr; rp = rp->ai_next) {
			if (result->ai_family == AF_INET) {
				sin.sin_family = result->ai_family;
				sin.sin_addr = ((struct sockaddr_in *)result->ai_addr)->sin_addr;
				sin_valid = true;
				ok = true;
				break;
			}
			else if (result->ai_family == AF_INET6) {
				sin6.sin6_family = result->ai_family;
				sin6.sin6_addr = ((struct sockaddr_in6 *)result->ai_addr)->sin6_addr;
				sin6_valid = true;
				ok = true;
				break;
			}
		}
		if (!ok) {
			throw FailException("Wrong address type");
		}
	}

	// Set host and port
	SOCKET sock;
	if (sin_valid) {
		sock = socket(sin.sin_family, SOCK_STREAM, IPPROTO_TCP);
		if (connect(sock, (struct sockaddr*)(&sin),
			sizeof(struct sockaddr_in)) != 0)
		{
			throw FailException(QString("IP4 Connection error: %1").arg(_last_sock_err()));
		}
	}
	else if (sin6_valid) {
		sock = socket(sin6.sin6_family, SOCK_STREAM, IPPROTO_TCP);
		if (connect(sock, (struct sockaddr*)(&sin6),
			sizeof(struct sockaddr_in6)) != 0)
		{
			throw FailException(QString("IP6 Connection error: %1").arg(_last_sock_err()));
		}
	}
	else {
		throw FailException("No valid address found");
	}
	

	// Init session
	m_ssh = libssh2_session_init();

	// Wait up to 30 seconds for receives
	libssh2_session_set_timeout(m_ssh, 30000);

	if (libssh2_session_handshake(m_ssh, sock))
	{
		throw FailException("SSH session handshake failed");
	}

	if (m_authtype == PASSWORD)
	{
		if (libssh2_userauth_password(m_ssh, m_username.toLatin1(), m_password.toLatin1())) {
			throw FailException("\tAuthentication by password failed!\n");
		}
	}
	else if (m_authtype == PUBLICKEY)
	{
		if (libssh2_userauth_publickey_fromfile(m_ssh, m_username.toLatin1(), NULL, m_privkeyfile.toUtf8(), m_privkeypassword.toLatin1())) {
			throw FailException("\tAuthentication by password failed!\n");
		}
	}
	else
	{
		throw FailException("Unknown authentication type");
	}
}

void UnixSSHImporter::openChannel()
{
	//  Open a session channel.  (It is possible to have multiple
	//  session channels open simultaneously.)
	m_channel = libssh2_channel_open_session(m_ssh);
	if (!m_channel) {
		throw FailException("Error opening session channel");
	}

	const char *termType = "dumb";
	int widthInChars = 120;
	int heightInChars = 40;
	//  Use 0 for pixWidth and pixHeight when the dimensions
	//  are set in number-of-chars.
	int pixWidth = 0;
	int pixHeight = 0;
	
	/*
	if (!m_ssh.SetTtyMode("ECHO", 0))
	{
		throw FailException("Failed to turn off echo");
	}
	*/
	
	if (libssh2_channel_request_pty_ex(m_channel, termType, (unsigned int)strlen(termType), NULL, 0, widthInChars, heightInChars, pixWidth, pixHeight)) {
		throw FailException("Failed requesting pty");
	}
	

	/*
	//  Start a shell on the channel:
	if (!m_ssh.SendReqShell(m_channelNum)) 
	{
		throw FailException("Error requesting shell");
	}
	*/

}

void UnixSSHImporter::write_string(QByteArray str, const char *error) 
{
	if (libssh2_channel_write(m_channel, str.constData(), str.length()) != str.length())
	{
		throw FailException(QString("%1: %2").arg(error).arg(getLastError()));
	}
}

QString UnixSSHImporter::receive_until_match(QByteArrayList matches, const char *error) 
{
	QByteArray out;
	QList<QRegExp> rxlist;
	for (auto match : matches) {
		rxlist.push_front(QRegExp(match, Qt::CaseSensitive, QRegExp::Wildcard));
	}

	char buf[1024];
	bool done = false;
	while (!done) {
		ssize_t readlen = libssh2_channel_read(m_channel, buf, sizeof(buf));
		if (readlen == LIBSSH2_ERROR_CHANNEL_CLOSED || readlen == 0) {
			break;
		}
		if (readlen == LIBSSH2_ERROR_EAGAIN) {
			QThread::sleep(0);
			continue;
			
		}
		if (readlen < 0) {
			throw FailException(error);
		}
		
		out += QByteArray(buf, readlen);
		// If out matches one the patterns in the byte array list, return the string
		for (auto rx : rxlist) {
			if (rx.exactMatch(out)) {
				done = true;
				break;
			}
		}
	}

	return QString::fromUtf8(out);
}

QString UnixSSHImporter::receiveSUCommandOutput(QString command)
{
	openChannel();
	//m_ssh.ChannelPoll(m_channelNum, 2000);

	QString fullcommand = QString("su root -c 'echo \"-\"\"--START---\" && %1 && echo \"-\"\"--SUCCESS---\" || echo \"-\"\"--BADCOMMAND---\"' || echo \"-\"\"--BADPASS---\"\n").arg(command);

	if(libssh2_channel_exec(m_channel, fullcommand.toUtf8().constData()))
	{
		throw FailException("Error sending su command");
	}

	//  Retrieve the output.
	QByteArrayList matches;
//	matches.append("*---START---*");
	matches.append("*---SUCCESS---*");
	matches.append("*---BADCOMMAND---*");
	matches.append("*---BADPASS---*");
	matches.append("*Password: *");
	matches.append("*Password:*");
//	matches.append("Sorry");
	
	QString response = receive_until_match(matches, "Unexpected end of data").replace("\r\n", "\n");
	if (response.contains("Sorry"))
	{
		throw FailException("Can't 'su' to root. User is denied access.");
	}

	if (response.endsWith("Password: ") || response.endsWith("Password:"))
	{
		// Send password
		QByteArray passwordstr = QString("%1\n").arg(m_supassword).toUtf8();
		write_string(passwordstr, "Error sending 'su' command");
		
		response = receive_until_match(matches, "Unexpected prompt").replace("\r\n", "\n");
	}
	
	if (!response.endsWith("---SUCCESS---\n"))
	{
		if (response.endsWith("---BADPASS---\n"))
		{
			throw FailException("Incorrect 'su' password.");
		}
		else if (response.endsWith("---BADCOMMAND---\n"))
		{
			throw FailException("Command is unvailable.");
		}
		else
		{
			throw FailException("Unknown command output.");
		}
	}

	int start = response.indexOf("---START---\n");
	int end = response.indexOf("---SUCCESS---\n");

	QString commandoutput = response.mid(start + 12, end - (start + 12));
	return commandoutput;
}

QString UnixSSHImporter::receiveSUDOCommandOutput(QString command) 
{
	openChannel();

	//m_ssh.ChannelPoll(m_channelNum, 2000);

	QString fullcommand = QString("sudo sh -c 'echo \"-\"\"--START---\" && %1 && echo \"-\"\"--SUCCESS---\" || echo \"-\"\"--BADCOMMAND---\"' || echo \"-\"\"--BADPASS---\"\n").arg(command);
	if (libssh2_channel_exec(m_channel, fullcommand.toUtf8().constData()))
	{
		throw FailException("Error sending sudo command");
	}
	
	//  Retrieve the output.
	QByteArrayList matches;
	//	matches.append("*---START---*");
	matches.append("*---SUCCESS---*");
	matches.append("*---BADCOMMAND---*");
	matches.append("*---BADPASS---*");
	matches.append("*Sorry*");
	matches.append(QString("*%1: *").arg(m_username).toUtf8().constData());
	matches.append(QString("*%1:*").arg(m_username).toUtf8().constData());
	matches.append("*Password: *");
	matches.append("*Password:*");
	QString response = receive_until_match(matches, "Unexpected end of data").replace("\r\n", "\n");
	if (response.endsWith(": ") || response.endsWith(":"))
	{
		// Send password
		QByteArray passwordstr = QString("%1\n").arg(m_sudopassword).toUtf8();
		write_string(passwordstr, "Error sending 'sudo' command");

		response = receive_until_match(QByteArrayList(), "Unexpected prompt").replace("\r\n", "\n");
	}

	if (!response.endsWith("---SUCCESS---\n"))
	{
		if (response.endsWith("---BADPASS---\n") || response.contains("Sorry"))
		{
			if (response.contains("sudoers"))
			{
				throw FailException("User is not in the 'sudoers' file.");
			}
			if (response.contains("not found"))
			{
				throw FailException("'sudo' command is not available on this system.");
			}
			throw FailException("Incorrect 'sudo' password.");
		}
		else if (response.endsWith("---BADCOMMAND---\n"))
		{
			throw FailException("Command is unvailable.");
		}
		else
		{
			throw FailException("Unknown command output.");
		}
	}

	int start = response.indexOf("---START---\n");
	int end = response.indexOf("---SUCCESS---\n");

	QString commandoutput = response.mid(start + 12, end - (start + 12));
	return commandoutput; 
}


QString UnixSSHImporter::receiveElevatedCommandOutput(QString command)
{
	if (m_elevtype == ELEVTYPE::SU)
	{
		return receiveSUCommandOutput(command);
	}
	if (m_elevtype == ELEVTYPE::SUDO)
	{
		return receiveSUDOCommandOutput(command);
	}
	return receiveCommandOutput(command);
}

QString UnixSSHImporter::getLastError(void)
{
	if (!m_ssh) {
		return QString("Session could not be created");
	}
	char *errmsg;
	int errmsg_len;
	if (libssh2_session_last_error(m_ssh, &errmsg, &errmsg_len, 0)) {
		return QString("<unknown>");
	}
	return QString::fromUtf8(errmsg);
}

QString UnixSSHImporter::receiveCommandOutput(QString command) 
{
	openChannel();
	
//	QThread::sleep(2);
	//m_ssh.ChannelPoll(m_channelNum, 2000);

	QString fullcommand = QString("echo \"-\"\"--START---\" && %1 && echo \"-\"\"--SUCCESS---\" || echo \"-\"\"--FAILURE---\"\n").arg(command);
	if (libssh2_channel_exec(m_channel, fullcommand.toUtf8().constData()))
	{
		QString error = getLastError();
		throw FailException(QString("Error sending command: %1").arg(error));
	}

	//  Retrieve the output.
	QString response = receive_until_match(QByteArrayList(), "error reading stream").replace("\r\n", "\n");
	if (!response.contains("---START---\n"))
	{
		throw FailException("Malformed command output");
	}
	if (!response.endsWith("---SUCCESS---\n"))
	{
		if (response.contains("Permission denied"))
		{
			throw FailException("Permission denied");
		}
		throw FailException("Malformed command output");
	}
	int start = response.indexOf("---START---\n");
	int end = response.indexOf("---SUCCESS---\n");

	QString commandoutput = response.mid(start + 12, end-(start+12));
	return commandoutput;
}

void UnixSSHImporter::getSystemType()
{
	QString uname = receiveCommandOutput("uname");

	m_systemvarianttype = ST_DEFAULT;
	if (uname.contains("Linux"))
	{
		m_systemtype = ST_LINUX;
	}
	else if (uname.contains("BSD"))
	{
		m_systemtype = ST_BSD;
		if (uname.contains("OpenBSD"))
		{
			m_systemvarianttype = ST_OPENBSD;
		}
		else if (uname.contains("FreeBSD"))
		{
			m_systemvarianttype = ST_FREEBSD;
		}
	}
	else if (uname.contains("SunOS"))
	{
		m_systemtype = ST_SOLARIS;
	}
	else if (uname.contains("AIX"))
	{
		m_systemtype = ST_AIX;
	}
	else 
	{
		m_systemtype = ST_UNKNOWN;
		throw FailException("Unknown system type");
	}
}

bool UnixSSHImporter::TestCredentials(QList<FOURCC> &hashtypes, QString &error, bool &cancelled)
{
	TR;

	cancelled = false;

	try
	{
		connectAndAuthenticate();
		getSystemType();

		ShadowImporter imp(NULL, NULL);
		QStringList filecontents;
		QStringList files;
		QList<bool> filevalid;
		ILC7UnixImporter *pimp = NULL;

		if (m_systemtype == ST_LINUX)
		{
			QString shadow = receiveElevatedCommandOutput("cat /etc/shadow");
			filecontents.append(shadow);

			pimp=imp.GetPasswdImporter("linuxshadow");
			if (!pimp)
			{
				throw FailException("Missing importer module");
			}
		}
		else if (m_systemtype == ST_SOLARIS)
		{
			QString shadow = receiveElevatedCommandOutput("cat /etc/shadow");
			filecontents.append(shadow);

			pimp = imp.GetPasswdImporter("solarisshadow");
			if (!pimp)
			{
				throw FailException("Missing importer module");
			}
		}
		else if (m_systemtype == ST_BSD)
		{
			QString shadow = receiveElevatedCommandOutput("cat /etc/master.passwd");
			filecontents.append(shadow);

			pimp = imp.GetPasswdImporter("bsdmasterpasswd");
			if (!pimp)
			{
				throw FailException("Missing importer module");
			}
		}
		else if (m_systemtype == ST_AIX)
		{
			QString shadow = receiveElevatedCommandOutput("cat /etc/security/passwd");
			filecontents.append(shadow);

			pimp = imp.GetPasswdImporter("aixsecuritypasswd");
			if (!pimp)
			{
				throw FailException("Missing importer module");
			}
		}
		else
		{
			throw FailException("Unknown system type");
		}

		foreach(QString filecontent, filecontents)
		{
			QString temp = g_pLinkage->NewTemporaryFile();
			QFile tempfile(temp);
			if (!tempfile.open(QIODevice::WriteOnly))
			{
				throw FailException("Failed opening temporary file");
			}
			tempfile.write(filecontent.toUtf8().constData());
			tempfile.close();

			files.append(temp);
			filevalid.append(true);
		}
		
		if (!pimp->CheckHashTypes(files, filevalid, hashtypes))
		{
			throw FailException("Hash type check failed");
		}

		foreach(QString file, files)
		{
			QFile::remove(file);
		}

		gracefulTerminate();
	}
	catch (FailException e)
	{
		QString internalerror = QString("%1: %2").arg(e.what()).arg(getLastError());
		error = e.Error();
		gracefulTerminate();
		return false;
	}


	return true;
}

void UnixSSHImporter::setRemediations(const LC7Remediations & remediations)
{
	m_remediations = remediations;
}

void UnixSSHImporter::setAccountsToRemediate(QList<int> accounts_to_disable, QList<int> accounts_to_force_change, QList<int> accounts_to_lockout)
{
	m_accounts_to_disable = accounts_to_disable;
	m_accounts_to_force_change = accounts_to_force_change;
	m_accounts_to_lockout = accounts_to_lockout;
}


bool UnixSSHImporter::DoImport(FOURCC hashtype, QString & error, bool & cancelled)
{
	TR;

	cancelled = false;
	bool ret = true;

	try
	{
		connectAndAuthenticate();
		getSystemType();

		ShadowImporter imp(m_accountlist, m_ctrl);
		imp.SetAccountLimit(m_account_limit);

		if (!m_remediations.isEmpty())
		{
			// xxx: remove disabled remediation for aix and solaris, bit of a hack, sorry.
			if (m_systemtype == ST_SOLARIS || m_systemtype == ST_AIX)
			{
				if (m_remediations[0].command == "disable")
				{
					m_remediations.removeFirst();
				}
			}
			imp.SetRemediations(m_remediations);
		}

		QStringList filecontents;
		QStringList files;
		QList<bool> filevalid;
		ILC7UnixImporter *pimp = NULL;
		QString importername;

		if (m_systemtype == ST_LINUX)
		{
			QString passwd = receiveElevatedCommandOutput("cat /etc/passwd");
			filecontents.append(passwd);
			QString shadow = receiveElevatedCommandOutput("cat /etc/shadow");
			filecontents.append(shadow);

			importername = "linuxpasswdshadow";
		}
		else if (m_systemtype == ST_SOLARIS)
		{
			QString passwd = receiveElevatedCommandOutput("cat /etc/passwd");
			filecontents.append(passwd);
			QString shadow = receiveElevatedCommandOutput("cat /etc/shadow");
			filecontents.append(shadow);

			importername = "solarispasswdshadow";
		}
		else if (m_systemtype == ST_BSD)
		{
			QString passwd = receiveElevatedCommandOutput("cat /etc/passwd");
			filecontents.append(passwd);
			QString shadow = receiveElevatedCommandOutput("cat /etc/master.passwd");
			filecontents.append(shadow);

			importername = "bsdpasswdmasterpasswd";
		}
		else if (m_systemtype == ST_AIX)
		{
			QString passwd = receiveElevatedCommandOutput("cat /etc/passwd");
			filecontents.append(passwd);
			QString securitypasswd = receiveElevatedCommandOutput("cat /etc/security/passwd");
			filecontents.append(securitypasswd);
			QString securityuser= receiveElevatedCommandOutput("cat /etc/security/user");
			filecontents.append(securityuser);

			importername = "aixpasswdsecuritypasswdsecurityuser";
		}
		else
		{
			throw FailException("Unknown system type");
		}

		foreach(QString filecontent, filecontents)
		{
			QString temp = g_pLinkage->NewTemporaryFile();
			QFile tempfile(temp);
			if (!tempfile.open(QIODevice::WriteOnly))
			{
				throw FailException("Failed opening temporary file");
			}
			tempfile.write(filecontent.toUtf8().constData());
			tempfile.close();

			files.append(temp);
			filevalid.append(true);
		}
		
		ret = imp.DoImport(importername, files, hashtype, m_include_non_login ? ShadowImporter::exclude_none :
			(ShadowImporter::IMPORT_FLAGS) (ShadowImporter::exclude_disabled | ShadowImporter::exclude_lockedout | ShadowImporter::exclude_expired), error, cancelled);

		foreach(QString file, files)
		{
			QFile::remove(file);
		}

		gracefulTerminate();
	}
	catch (FailException e)
	{
		QString internalerror = QString("%1: %2").arg(e.what()).arg(getLastError());
		error = e.Error();
		gracefulTerminate();
		return false;
	}


	return ret;
}


QString UnixSSHImporter::getRemediationLine(int remediationtype, QString username)
{
	QString remline;

	if (m_systemtype == ST_LINUX)
	{
		if (remediationtype == 1)
		{
			// disable
			remline = QString("chage -E 1 %1").arg(username);
		}
		else if (remediationtype == 2)
		{
			// expire password
			remline = QString("chage -M 0 %1").arg(username);
		}
		else if (remediationtype == 3)
		{
			// lock out account
			remline = QString("usermod -L %1").arg(username);
		}
	}
	else if (m_systemtype == ST_SOLARIS)
	{
		if (remediationtype == 1)
		{
			// disable
			throw FailException("'Disable' remediation not supported on this operating system. Use lockout instead.");
		}
		else if (remediationtype == 2)
		{
			// expire password
			remline = QString("passwd -f %1").arg(username);
		}
		else if (remediationtype == 3)
		{
			// lock out account
			remline = QString("passwd -l %1").arg(username);
		}
	}
	else if (m_systemtype == ST_BSD)
	{
		if (m_systemvarianttype == ST_OPENBSD)
		{
			if (remediationtype == 1)
			{
				// disable
				remline = QString("usermod -e 1 %1").arg(username);
			}
			else if (remediationtype == 2)
			{
				// expire password
				remline = QString("usermod -f 1 %1").arg(username);
			}
			else if (remediationtype == 3)
			{
				// lock out account
				remline = QString("usermod -Z %1").arg(username);
			}
		}
		else if (m_systemvarianttype == ST_FREEBSD)
		{
			if (remediationtype == 1)
			{
				// disable
				remline = QString("pw usermod %1 -e 1").arg(username);
			}
			else if (remediationtype == 2)
			{
				// expire password
				remline = QString("pw usermod %1 -p 1").arg(username);
			}
			else if (remediationtype == 3)
			{
				// lock out account
				remline = QString("pw lock %1").arg(username);
			}
		}
		else
		{
			throw FailException("Remediation not supported on this BSD variant");
		}
	}
	else if (m_systemtype == ST_AIX)
	{
		if (remediationtype == 1)
		{
			// disable
			throw FailException("'Disable' remediation not supported on this operating system. Use lockout instead.");
		}
		else if (remediationtype == 2)
		{
			// expire password
			remline = QString("pwdadm -f ADMCHG %1").arg(username);
		}
		else if (remediationtype == 3)
		{
			// lock out account
			remline = QString("chuser account_locked=true %1").arg(username);
		}
	}
	else
	{
		throw FailException("Remediation is not supported on this operating system.");
	}

	remline += QString(" && echo 1,%1,%2 || echo 0,%1,%2\n").arg(remediationtype).arg(username);

	return remline;
}


void UnixSSHImporter::generateRemediationScript(QString & remediationscript)
{
	remediationscript = "#!/bin/sh\n";
	m_accountlist->Acquire();
	try
	{
		foreach(int acct, m_accounts_to_disable)
		{
			const LC7Account * lc7acct = m_accountlist->GetAccountAtConstPtrFast(acct);

			QString remline = getRemediationLine(1, lc7acct->username);

			remediationscript += remline;
		}
		foreach(int acct, m_accounts_to_force_change)
		{
			const LC7Account * lc7acct = m_accountlist->GetAccountAtConstPtrFast(acct);

			QString remline = getRemediationLine(2, lc7acct->username);

			remediationscript += remline;
		}
		foreach(int acct, m_accounts_to_lockout)
		{
			const LC7Account * lc7acct = m_accountlist->GetAccountAtConstPtrFast(acct);

			QString remline = getRemediationLine(3, lc7acct->username);

			remediationscript += remline;
		}
	}
	catch (...)
	{
		m_accountlist->Release();
		throw;
	}
	m_accountlist->Release();
}

void UnixSSHImporter::writeRemoteFile(QString remotefilename, QString contents)
{
	openChannel();

	//	QThread::sleep(2);
	//m_ssh.ChannelPoll(m_channelNum, 2000);

	QString command = QString("cat > %1").arg(remotefilename);

	QString fullcommand = QString("echo \"-\"\"--START---\" && ( %1 ) && echo \"-\"\"--SUCCESS---\" || echo \"-\"\"--FAILURE---\"\n").arg(command);
	if (libssh2_channel_exec(m_channel, fullcommand.toUtf8().constData()))
	{
		throw FailException("Error sending sudo command");
	}

	QByteArray cmdstr = contents.toUtf8();
	cmdstr.append((char)4);

	write_string(cmdstr.constData(), "Error sending remote file");

	//  Retrieve the output.
	QString response = receive_until_match(QByteArrayList(), "error reading stream").replace("\r\n", "\n");
	if (!response.contains("---START---\n"))
	{
		throw FailException("Malformed command output");
	}
	if (!response.endsWith("---SUCCESS---\n"))
	{
		if (response.contains("Permission denied"))
		{
			throw FailException("Permission denied");
		}
		throw FailException("Malformed command output");
	}
}

bool UnixSSHImporter::DoRemediate(QString & error, bool & cancelled)
{
	TR;

	cancelled = false;
	bool ret = true;

	try
	{
		QString remscriptlocal = g_pLinkage->NewTemporaryFile();

		connectAndAuthenticate();
		getSystemType();

		QString remediationscript;
		generateRemediationScript(remediationscript);
		
		QString tmpnum = QUuid::createUuid().toString().remove("{").remove("}").remove("-");
		QString remotescriptname = QString("/tmp/.lc7.remediate.%1.sh").arg(tmpnum);

		writeRemoteFile(remotescriptname, remediationscript);

		QString scriptoutput = receiveElevatedCommandOutput(QString("/bin/sh %1").arg(remotescriptname));

		receiveCommandOutput(QString("rm %1").arg(remotescriptname));

		// Import remediation output
		quint32 successcnt[3] = { 0, 0, 0 };
		quint32 failurecnt[3] = { 0, 0, 0 };

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
		foreach(int idx, m_accounts_to_lockout)
		{
			const LC7Account *acct = m_accountlist->GetAccountAtConstPtrFast(idx);
			if (!name_to_index.contains(acct->username))
			{
				name_to_index[acct->username] = idx;
			}
		}
		foreach(QString line, scriptoutput.split('\n'))
		{
			QStringList lineparts = line.split(",");
			if (lineparts.size() != 3)
			{
				continue;
			}

			quint32 result = lineparts[0].toUInt();
			quint32 cmd = lineparts[1].toUInt();
			QString username = lineparts[2];

			if (result == 1)
			{
				successcnt[cmd - 1]++;

				int idx = name_to_index.value(username, -1);
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
					else if (cmd == 3)
					{
						acct.lockedout = true;
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
			m_ctrl->AppendToActivityLog(QString("Remediation Status For '%1'\n").arg(m_host));
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
					m_ctrl->AppendToActivityLog(QString("Failed to expire %1 accounts (could not force password change, account password may be set to not expire)\n").arg(failurecnt[1]));
			}
			if (m_accounts_to_lockout.size() > 0)
			{
				if (successcnt[2] != 0)
					m_ctrl->AppendToActivityLog(QString("Successfully locked out %1 accounts\n").arg(successcnt[2]));
				if (failurecnt[2] != 0)
					m_ctrl->AppendToActivityLog(QString("Failed to locked out %1 accounts\n").arg(failurecnt[2]));
			}
		}

		gracefulTerminate();
	}
	catch (FailException e)
	{
		QString internalerror = QString("%1: %2").arg(e.what()).arg(getLastError());
		error = e.Error();
		gracefulTerminate();
		return false;
	}


	return ret;
}


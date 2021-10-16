#ifndef __INC_JTREXE_CRASHDUMP_H
#define __INC_JTREXE_CRASHDUMP_H

#include"platform_specific.h"
#include"appversion.h"
#include"CrashRpt.h"

int g_CrashRptInstalled = 0;

void JTREXE_INSTALL_CRASHDUMP(void)
{
#if PLATFORM == PLATFORM_WIN64
	_set_FMA3_enable(0);
#endif

#ifndef _DEBUG
	// Install crash reporting

	CR_INSTALL_INFO info;
	memset(&info, 0, sizeof(CR_INSTALL_INFO));
	info.cb = sizeof(CR_INSTALL_INFO);				// Size of the structure
	info.pszAppName = "L0phtCrack 7 JTRDLL";				// App name
	info.pszAppVersion = VERSION_STRING;				// App version
	info.dwFlags = CR_INST_AUTO_THREAD_HANDLERS | CR_INST_ALL_POSSIBLE_HANDLERS;
	info.pszEmailSubject = "L0phtCrack 7 JTRDLL v"VERSION_STRING" Error Report"; // Email subject
	info.uMiniDumpType = (MINIDUMP_TYPE)(MiniDumpWithDataSegs | MiniDumpWithHandleData | MiniDumpWithProcessThreadData | MiniDumpWithThreadInfo);
	info.pszEmailTo = "crashdump@l0phtcrack.com";   // Email recipient address
	info.pszSmtpProxy = "mailproxy.l0phtcrack.com:465"; // SMTP Server 
	info.uPriorities[CR_HTTP] = CR_NEGATIVE_PRIORITY;
	info.uPriorities[CR_SMTP] = 2;
	info.uPriorities[CR_SMAPI] = 1;

	// Install crash handlers
	int nInstResult = crInstall(&info);

	// Check result
	if (nInstResult == 0)
	{
		g_CrashRptInstalled = true;
	}
#endif

}

void JTREXE_UNINSTALL_CRASHDUMP(void)
{

#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)
	if (!g_CrashRptInstalled)
	{
		return;
	}
	int nUninstRes = crUninstall(); // Uninstall exception handlers
#endif

}

#endif
#define _WIN32_WINNT _WIN32_WINNT_WINXP
#include<Windows.h>
#include<stdio.h>

#define _WIN32_WINNT_NT4                    0x0400
#define _WIN32_WINNT_WIN2K                  0x0500
#define _WIN32_WINNT_WINXP                  0x0501
#define _WIN32_WINNT_WS03                   0x0502
#define _WIN32_WINNT_WIN6                   0x0600
#define _WIN32_WINNT_VISTA                  0x0600
#define _WIN32_WINNT_WS08                   0x0600
#define _WIN32_WINNT_LONGHORN               0x0600
#define _WIN32_WINNT_WIN7                   0x0601
#define _WIN32_WINNT_WIN8                   0x0602
#define _WIN32_WINNT_WINBLUE                0x0603
#define _WIN32_WINNT_WINTHRESHOLD           0x0A00 /* ABRACADABRA_THRESHOLD*/
#define _WIN32_WINNT_WIN10                  0x0A00 /* ABRACADABRA_THRESHOLD*/

BOOL IsWindowsVersionOrGreater(WORD wMajorVersion, WORD wMinorVersion, WORD wServicePackMajor)
{
	OSVERSIONINFOEXW osvi = { sizeof(osvi), 0, 0, 0, 0, { 0 }, 0, 0 };
	DWORDLONG        const dwlConditionMask = VerSetConditionMask(
		VerSetConditionMask(
		VerSetConditionMask(
		0, VER_MAJORVERSION, VER_GREATER_EQUAL),
		VER_MINORVERSION, VER_GREATER_EQUAL),
		VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);

	osvi.dwMajorVersion = wMajorVersion;
	osvi.dwMinorVersion = wMinorVersion;
	osvi.wServicePackMajor = wServicePackMajor;

	return VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR, dwlConditionMask) != FALSE;
}

BOOL
IsWindowsXPOrGreater()
{
	return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WINXP), LOBYTE(_WIN32_WINNT_WINXP), 0);
}

BOOL
IsWindowsXPSP1OrGreater()
{
	return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WINXP), LOBYTE(_WIN32_WINNT_WINXP), 1);
}

BOOL
IsWindowsXPSP2OrGreater()
{
	return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WINXP), LOBYTE(_WIN32_WINNT_WINXP), 2);
}

BOOL
IsWindowsXPSP3OrGreater()
{
	return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WINXP), LOBYTE(_WIN32_WINNT_WINXP), 3);
}

BOOL
IsWindowsVistaOrGreater()
{
	return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_VISTA), LOBYTE(_WIN32_WINNT_VISTA), 0);
}

BOOL
IsWindowsVistaSP1OrGreater()
{
	return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_VISTA), LOBYTE(_WIN32_WINNT_VISTA), 1);
}

BOOL
IsWindowsVistaSP2OrGreater()
{
	return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_VISTA), LOBYTE(_WIN32_WINNT_VISTA), 2);
}

BOOL
IsWindows7OrGreater()
{
	return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WIN7), LOBYTE(_WIN32_WINNT_WIN7), 0);
}

BOOL
IsWindows7SP1OrGreater()
{
	return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WIN7), LOBYTE(_WIN32_WINNT_WIN7), 1);
}

BOOL
IsWindows8OrGreater()
{
	return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WIN8), LOBYTE(_WIN32_WINNT_WIN8), 0);
}

BOOL
IsWindows8Point1OrGreater()
{
	return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WINBLUE), LOBYTE(_WIN32_WINNT_WINBLUE), 0);
}

BOOL
IsWindowsThresholdOrGreater()
{
	return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WINTHRESHOLD), LOBYTE(_WIN32_WINNT_WINTHRESHOLD), 0);
}

BOOL
IsWindows10OrGreater()
{
	return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WINTHRESHOLD), LOBYTE(_WIN32_WINNT_WINTHRESHOLD), 0);
}

BOOL
IsWindowsServer()
{
	OSVERSIONINFOEXW osvi = { sizeof(osvi), 0, 0, 0, 0, { 0 }, 0, 0, 0, VER_NT_WORKSTATION };
	DWORDLONG        const dwlConditionMask = VerSetConditionMask(0, VER_PRODUCT_TYPE, VER_EQUAL);

	return !VerifyVersionInfoW(&osvi, VER_PRODUCT_TYPE, dwlConditionMask);
}

//////////////////////////////////////////////////////////////////////////////////////////

int configuresmb(void)
{
	if (!IsWindowsXPOrGreater())
	{
		printf("No configuration required on this operating system\r\n");
		return 0;
	}

	if (IsWindowsVistaOrGreater())
	{
		// UAC

		HKEY k;
		if (RegOpenKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\system", &k) != ERROR_SUCCESS)
		{
			fprintf(stderr, "Failed to open UAC policy registry key\r\n");
			return 1;
		}

		DWORD value = 1;
		if (RegSetValueEx(k, L"LocalAccountTokenFilterPolicy", 0, REG_DWORD, (const BYTE *)&value, sizeof(value)) != ERROR_SUCCESS)
		{
			fprintf(stderr, "Failed to set UAC policy registry key value\r\n");
			return 2;
		}

		RegCloseKey(k);

		// AFP 

		STARTUPINFO si;
		ZeroMemory(&si, sizeof(STARTUPINFO));
		si.cb = sizeof(STARTUPINFO);
		si.wShowWindow = SW_HIDE;
		si.dwFlags = STARTF_USESHOWWINDOW;

		PROCESS_INFORMATION pi;
		if (!CreateProcess(NULL, L"netsh advfirewall firewall set rule group=\"File and Printer Sharing\" new enable=Yes", NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
		{
			fprintf(stderr, "Failed to create process to set advanced firewall rule\r\n");
			return 3;
		}
		if (WaitForSingleObject(pi.hProcess, 5000)!=WAIT_OBJECT_0)
		{
			fprintf(stderr, "Failed waiting for advanced firewall rule process to complete\r\n");
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			return 4;
		}

		DWORD exitcode;
		if (!GetExitCodeProcess(pi.hProcess, &exitcode))
		{
			fprintf(stderr, "Failed getting exit code of advanced firewall rule process\r\n");
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			return 5;
		}

		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		if (exitcode != 0)
		{
			fprintf(stderr, "Could not set advanced firewall rule\r\n");
			return 6;
		}
	}
	else
	{
		// RFG

		HKEY k;
		if (RegOpenKey(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Lsa", &k) != ERROR_SUCCESS)
		{
			fprintf(stderr, "Failed to open LSA registry key\r\n");
			return 1;
		}

		DWORD value = 0;
		if (RegSetValueEx(k, L"forceguest", 0, REG_DWORD, (const BYTE *)&value, sizeof(value)) != ERROR_SUCCESS)
		{
			fprintf(stderr, "Failed to set LSA registry key value\r\n");
			return 2;
		}

		RegCloseKey(k);

		// FFP 

		STARTUPINFO si;
		ZeroMemory(&si, sizeof(STARTUPINFO));
		si.cb = sizeof(STARTUPINFO);
		si.wShowWindow = SW_HIDE;
		si.dwFlags = STARTF_USESHOWWINDOW;

		PROCESS_INFORMATION pi;
		if (!CreateProcess(NULL, L"netsh firewall set service type = FILEANDPRINT mode = ENABLE", NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
		{
			fprintf(stderr, "Failed to create process to set basic firewall rules\r\n");
			return 3;
		}
		if (WaitForSingleObject(pi.hProcess, 5000) != WAIT_OBJECT_0)
		{
			fprintf(stderr, "Failed waiting for basic firewall rule process to complete\r\n");
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			return 4;
		}

		DWORD exitcode;
		if (!GetExitCodeProcess(pi.hProcess, &exitcode))
		{
			fprintf(stderr, "Failed getting exit code of basic firewall rule process\r\n");
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			return 5;
		}

		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		if (exitcode != 0)
		{
			fprintf(stderr, "Could not set basic firewall rule\r\n");
			return 6;
		}

	}

	return 0;
}


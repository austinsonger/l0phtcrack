#include<stdafx.h>

ILC7Linkage *g_pLinkage=NULL;
CLC7ReportsPlugin *g_pReportsPlugin=NULL;

extern "C"
{

DLLEXPORT bool Register(ILC7Linkage *pLinkage)
{
	g_pLinkage=pLinkage;

	g_pReportsPlugin=new CLC7ReportsPlugin();
	g_pLinkage->GetPluginRegistry()->RegisterPlugin(g_pReportsPlugin);
	
	return true;
}

DLLEXPORT bool Unregister(void)
{
	g_pLinkage->GetPluginRegistry()->UnregisterPlugin(g_pReportsPlugin);
	delete g_pReportsPlugin;
	g_pReportsPlugin=NULL;

	g_pLinkage=NULL;

	return true;
}

}


#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)

BOOL WINAPI DllMain(
  HANDLE hinstDLL,
  DWORD dwReason,
  LPVOID lpvReserved
)
{
	if(dwReason==DLL_PROCESS_ATTACH)
	{
	
	}

	return TRUE;
}

#endif
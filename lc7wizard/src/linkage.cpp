#include<stdafx.h>

ILC7Linkage *g_pLinkage=NULL;
CLC7WizardPlugin *g_pWizardPlugin = NULL;

extern "C"
{

DLLEXPORT bool Register(ILC7Linkage *pLinkage)
{
	g_pLinkage=pLinkage;

	g_pWizardPlugin = new CLC7WizardPlugin();
	g_pLinkage->GetPluginRegistry()->RegisterPlugin(g_pWizardPlugin);

	return true;
}

DLLEXPORT bool Unregister(void)
{
	g_pLinkage->GetPluginRegistry()->UnregisterPlugin(g_pWizardPlugin);
	delete g_pWizardPlugin;
	g_pWizardPlugin = NULL;

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
	return TRUE;
}

#else



#endif
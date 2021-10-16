#pragma once

#ifdef _WIN32
#include <winsock2.h>
#endif

#include <QtWidgets>
#include <QApplication>
#include <QResource>
#include <qdatetime.h>

#include"../../lc7core/include/platform_specific.h"

#include"../../lc7/include/appversion.h"
#include"lc7api.h"

#include"linkage.h"

#include"windows_abstraction.h"
#include"uuids.h"

#include"PubKeyFile.h"
#include"CLC7ImportWinPlugin.h"
#include"CImportWindows.h"
#include"PWDumpImporter.h"
#include"SAMImporter.h"
#include"NTDSImporter.h"
#include"DRSRImporter.h"
#include"CImportPWDump.h"
#include"CImportPWDumpGUI.h"
#include"CImportSAM.h"
#include"CImportSAMGUI.h"
#include"CImportNTDS.h"
#include"CImportNTDSGUI.h"
#include"CImportWindowsLocal.h"
#include"CImportWindowsLocalGUI.h"
#include"CImportWindowsRemote.h"
#include"CImportWindowsRemoteGUI.h"
#include"GenerateRemoteAgentDlg.h"
#include"CWindowsImportSettings.h"

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

#include"uuids.h"

#include"ILC7UnixImporter.h"
#include"importshadowconfig.h"
#include"importunixsshconfig.h"
#include"UnixSSHImporter.h"
#include"CLC7ImportUnixPlugin.h"
#include"CLC7UnixImporter.h"
#include"CLC7UnixImporter_OLDPASSWD.h"
#include"CLC7UnixImporter_LINUX.h"
#include"CLC7UnixImporter_SOLARIS.h"
#include"CLC7UnixImporter_AIX.h"
#include"CLC7UnixImporter_BSD.h"
#include"ShadowImporter.h"
#include"CImportShadow.h"
#include"CImportShadowGUI.h"
#include"CImportUnixSSH.h"
#include"CImportUnixSSHGUI.h"

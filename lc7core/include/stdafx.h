#pragma once

#include"platform_specific.h"

////////////////////////////////////////
// workaround for https://codereview.qt-project.org/#/c/256188/

#pragma message("Remove when Qt releases a fix")
#define QT_NO_FLOAT16_OPERATORS
#include"qfloat16_qt5.12.2_workaround.h"

#include <QtWidgets>
#include <QApplication>
#include <QResource>
#include <QtSql>
#include <qdatetime.h>
#include <quuid.h>

#include"../../lc7/include/appversion.h"

#include"lc7api.h"

#include"CLC7SecureStringSerializer.h"
#include"CLC7CPUInformation.h"
#include"CLC7SystemMonitor.h"
#include"CLC7Linkage.h"
#include"CLC7Controller.h"
#include"CLC7PluginLibrary.h"
#include"CLC7PluginRegistry.h"
#include"CLC7WorkQueue.h"
#include"CLC7WorkQueueFactory.h"
#include"CLC7Task.h"
#include"CLC7HistoricalData.h"
#include"CLC7Settings.h"
#include"CLC7CommandControl.h"
#include"CLC7CoreSettings.h"
#include"CLC7Preset.h"
#include"CLC7PresetGroup.h"
#include"CLC7PresetManager.h"
#include"CLC7ThermalWatchdog.h"

#include"ITaskScheduler_BASE.h"
#if (PLATFORM==PLATFORM_WIN32) || (PLATFORM==PLATFORM_WIN64)
#include"CTaskScheduler_WIN32v1.h"
#include"CTaskScheduler_WIN32v2.h"
#else
#error Header files plz
#endif

#include"CLC7TaskScheduler.h"

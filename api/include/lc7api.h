#ifndef __INC_LC7API_H
#define __INC_LC7API_H

#define LC7API_VERSION 1

// Interface base
#include "core/ILC7Interface.h"

// Debug
#include "debug/trdebug.h"
#include "debug/failexception.h"

// Core
#include "core/LC7SecureString.h"
#include "core/ILC7CPUInformation.h"
#include "core/ILC7Action.h"
#include "core/ILC7ActionCategory.h"
#include "core/ILC7CommandControl.h"
#include "core/ILC7Component.h"
#include "core/ILC7HistoricalData.h"
#include "core/ILC7Linkage.h"
#include "core/ILC7Plugin.h"
#include "core/ILC7PluginLibrary.h"
#include "core/ILC7PluginRegistry.h"
#include "core/ILC7SessionHandler.h"
#include "core/ILC7SessionHandlerFactory.h"
#include "core/ILC7Settings.h"
#include "core/ILC7WorkQueue.h"
#include "core/ILC7TaskScheduler.h"
#include "core/ILC7Task.h"
#include "core/ILC7ThermalWatchdog.h"
#include "core/ILC7Preset.h"
#include "core/ILC7PresetGroup.h"
#include "core/ILC7PresetManager.h"
#include "core/ILC7SystemMonitor.h"

// GUI
#include "gui/ILC7ColorManager.h"
#include "gui/ILC7GUILinkage.h"
#include "gui/ILC7ProgressBox.h"
#include "gui/ILC7WorkQueueWidget.h"

// Password Auditing
#include "password/LC7GPUInfo.h"
#include "password/LC7Remediation.h"
#include "password/ILC7CalibrationTable.h"
#include "password/ILC7GPUManager.h"
#include "password/ILC7PasswordEngine.h"
#include "password/ILC7AccountList.h"
#include "password/LC7Account.h"
#include "password/LC7HashType.h"
#include "password/ILC7PasswordLinkage.h"

#endif

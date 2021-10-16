CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

PROJECT(lc7password)

INCLUDE(../cmake/PCH.cmake)

SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTORCC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5Core REQUIRED)
FIND_PACKAGE(Qt5PrintSupport REQUIRED)

SET(lc7password_SOURCES 
	src/stdafx.cpp
	src/accountslistmodel.cpp
	src/sortedaccountslistmodel.cpp
	src/CAccountsPage.cpp
	src/CAuditPage.cpp
	src/CImportPage.cpp
	src/CLC7AccountList.cpp
	src/CLC7AccountListFactory.cpp
	src/CLC7PasswordGUIPlugin.cpp
	src/CLC7PasswordPlugin.cpp
	src/CPasswordUIOptions.cpp
	src/CLC7PasswordLinkage.cpp
	src/CLC7ReportExportAccounts.cpp
	src/CLC7ReportExportAccountsGUI.cpp
	src/CLC7FileImportAccounts.cpp
	src/CLC7FileImportAccountsGUI.cpp
	src/CAccountsExport.cpp
	src/CAccountsImport.cpp
	src/exportaccountsconfig.cpp
	src/CLC7CalibrationTable.cpp
	src/CLC7CalibrateGUI.cpp
	src/CFireWidget.cpp
	src/waitingspinnerwidget.cpp
	src/linkage.cpp
)

SET(lc7password_FORMS 
	resources/accounts.ui
	resources/audit.ui
	resources/import.ui
	resources/exportaccountsconfig.ui
	resources/calibrationdialog.ui
)

SET(lc7password_RESOURCES
	resources/lc7password.qrc
)

SET(lc7password_HEADERS
	include/accountslistmodel.h
	include/sortedaccountslistmodel.h
	include/CAccountsPage.h
	include/CAuditPage.h
	include/CImportPage.h
	include/CLC7AccountList.h
	include/CLC7AccountListFactory.h
	include/CLC7PasswordGUIPlugin.h
	include/CLC7PasswordPlugin.h
	include/CPasswordUIOptions.h
	include/CLC7PasswordLinkage.h
	include/CLC7ReportExportAccounts.h
	include/CLC7ReportExportAccountsGUI.h
	include/CLC7FileImportAccounts.h
	include/CLC7FileImportAccountsGUI.h
	include/CAccountsExport.h
	include/CAccountsImport.h
	include/exportaccountsconfig.h
	include/CFireWidget.h
	include/waitingspinnerwidget.h
	include/CLC7CalibrationTable.h
	include/CLC7CalibrateGUI.h
	include/linkage.h
	include/stdafx.h
	include/uuids.h
)

#MESSAGE(lc7password_SOURCES=${lc7password_SOURCES})
#MESSAGE(lc7password_FORMS=${lc7password_FORMS})
#MESSAGE(lc7password_HEADERS=${lc7password_HEADERS})

QT5_WRAP_UI(lc7password_UI_HEADERS
    ${lc7password_FORMS}
)

QT5_ADD_RESOURCES(ResourceSources ${lc7password_RESOURCES})

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} "include" "../api/include" "../external/qtpropertybrowser/src")
ADD_LIBRARY(lc7password SHARED ${lc7password_SOURCES} ${lc7password_HEADERS} ${lc7password_UI_HEADERS} ${lc7password_RESOURCES} ${ResourceSources})
TARGET_LINK_LIBRARIES(lc7password qtpropertybrowser)

qt5_use_modules(lc7password Widgets PrintSupport)

ADD_PRECOMPILED_HEADER(lc7password stdafx lc7password_SOURCES)

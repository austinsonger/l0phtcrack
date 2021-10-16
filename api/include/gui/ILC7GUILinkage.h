#ifndef __INC_ILC7GUILINKAGE_H
#define __INC_ILC7GUILINKAGE_H

#include"core/ILC7Interface.h"

#include <qmainwindow.h>
#include <qstring.h>
#include <qwidget.h>

class ILC7ProgressBox;
class ILC7WorkQueueWidget;
class ILC7ColorManager;

class ILC7GUILinkage: public ILC7Interface
{
public:
	typedef void (GUITHREAD_CALLBACK_TYPE)(QList<QVariant> args);

protected:

	virtual ~ILC7GUILinkage() {}

public:

	virtual QMainWindow *GetMainWindow()=0;
	virtual ILC7ColorManager *GetColorManager() = 0;
	virtual ILC7WorkQueueWidget *GetWorkQueueWidget() = 0;
	virtual QAbstractButton *GetHelpButton(void) = 0;

	virtual bool AddMainMenuTab(QString strTabTitle, QString strInternalName, QWidget *pMainMenuTab)=0;
	virtual bool RemoveMainMenuTab(QWidget *pMainMenuTab)=0;
	virtual QWidget *GetMainMenuTab(QString strInternalName)=0;
	virtual bool SwitchToMainMenuTab(QString strInternalName) = 0;

	virtual void AddTopMenuItem(ILC7Action *act)=0;
	virtual void RemoveTopMenuItem(ILC7Action *act)=0;

	virtual void AddStartupDialogItem(ILC7Action *act)=0;
	virtual void RemoveStartupDialogItem(ILC7Action *act)=0;

	virtual void ErrorMessage(QString title,QString msg)=0;
	virtual void WarningMessage(QString title,QString msg)=0;
	virtual void InfoMessage(QString title,QString msg)=0;
	virtual bool YesNoBox(QString title, QString msg)=0;
	virtual bool YesNoBoxWithNeverAgain(QString title, QString msg, QString neveragainkey)=0;
	virtual bool AskForPassword(QString title, QString msg, QString & passwd) = 0;
	virtual bool AskForUsernameAndPassword(QString title, QString msg, QString &username, QString & passwd) = 0;
	virtual bool OpenFileDialog(QString title, QString starting_folder, QString filter, QString &filepath) = 0;
	virtual bool OpenFileDialogMultiple(QString title, QString starting_folder, QString filter, QStringList &paths) = 0;
	virtual bool SaveFileDialog(QString title, QString starting_folder, QString filter, QString &filepath) = 0;
	virtual bool GetDirectoryDialog(QString title, QString starting_folder, QString &dirpath) = 0;

	virtual bool RequestNewSession() = 0;
	virtual bool RequestOpenSession(QString requested_session = QString()) = 0;
	virtual bool RequestCloseSession(bool force=false, bool allow_startup_dialog=true) = 0;
	virtual void Exit(bool warn) = 0;

	virtual bool PauseBackgroundSession(QString sessionfile)=0;
	virtual void ShowStartupDialog(bool show) = 0;
	virtual void Callback(GUITHREAD_CALLBACK_TYPE *callback, QList<QVariant> args) = 0;

	virtual QWidget *CreatePresetWidget(QWidget *page, QWidget *configwidget, QString preset_group) = 0;
	virtual ILC7ProgressBox *CreateProgressBox(QString title, QString status, quint32 progresscur, quint32 progressmax, bool can_cancel)=0;

	virtual void AppendToActivityLog(QString text)=0;
	
	virtual void UpdateUI()=0;
	virtual void ShadeUI(bool shade) = 0;
};

#endif
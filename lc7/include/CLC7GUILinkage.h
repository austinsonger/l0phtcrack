#ifndef __INC_CLC7GUILINKAGE_H
#define __INC_CLC7GUILINKAGE_H

class CLC7WorkQueueWidget;

class CLC7GUILinkage:public QObject, public ILC7GUILinkage
{
	Q_OBJECT

signals:
	void sig_asyncProc(QSemaphore *s, QString function, QVariant args);
	
		
private:

	LC7Main *m_pMainWnd;
	QSettings *m_pSettings;
	
public:
	CLC7GUILinkage(LC7Main *pMainWnd);
	virtual ~CLC7GUILinkage();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	virtual QMainWindow *GetMainWindow();
	virtual ILC7ColorManager *GetColorManager();
	virtual ILC7WorkQueueWidget *GetWorkQueueWidget();
	virtual QAbstractButton *GetHelpButton(void);

	virtual bool AddMainMenuTab(QString strTabTitle, QString strInternalName, QWidget *pMainMenuTab);
	virtual bool RemoveMainMenuTab(QWidget *pMainMenuTab);
	virtual QWidget *GetMainMenuTab(QString strInternalName);
	virtual bool SwitchToMainMenuTab(QString strInternalName);
	
	virtual void AddTopMenuItem(ILC7Action *act);
	virtual void RemoveTopMenuItem(ILC7Action *act);

	virtual void AddStartupDialogItem(ILC7Action *act);
	virtual void RemoveStartupDialogItem(ILC7Action *act);

	virtual void ErrorMessage(QString title,QString msg);
	virtual void WarningMessage(QString title,QString msg);
	virtual void InfoMessage(QString title,QString msg);
	virtual bool YesNoBox(QString title, QString msg);
	virtual bool YesNoBoxWithNeverAgain(QString title, QString msg, QString neveragainkey);
	virtual bool AskForPassword(QString title, QString msg, QString & passwd);
	virtual bool AskForUsernameAndPassword(QString title, QString msg, QString &username, QString & passwd);
	virtual bool OpenFileDialog(QString title, QString starting_folder, QString filter, QString &filepath);
	virtual bool OpenFileDialogMultiple(QString title, QString starting_folder, QString filter, QStringList &paths);
	virtual bool SaveFileDialog(QString title, QString starting_folder, QString filter, QString &filepath);
	virtual bool GetDirectoryDialog(QString title, QString starting_folder, QString &dirpath);

	virtual bool RequestNewSession();
	virtual bool RequestOpenSession(QString requested_session = QString());
	virtual bool RequestCloseSession(bool force=false, bool allow_startup_dialog=true);
	virtual void Exit(bool warn);
	
	virtual bool PauseBackgroundSession(QString sessionfile);
	virtual void ShowStartupDialog(bool show);
	virtual void Callback(GUITHREAD_CALLBACK_TYPE *callback, QList<QVariant> args);
	
	virtual QWidget *CreatePresetWidget(QWidget *page, QWidget *configwidget, QString preset_group);

	virtual void AppendToActivityLog(QString text);

	virtual void UpdateUI();
	virtual void ShadeUI(bool shade);

	virtual ILC7ProgressBox *CreateProgressBox(QString title, QString status, UINT32 progresscur, UINT32 progressmax, bool can_cancel=true);


};

#endif
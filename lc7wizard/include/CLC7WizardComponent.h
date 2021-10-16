#ifndef __INC_CLC7WIZARDCOMPONENT_H
#define __INC_CLC7WIZARDCOMPONENT_H

class CLC7WizardComponent :public QObject, public ILC7Component
{
	Q_OBJECT;

private:
	ILC7WorkQueue *m_batch_workqueue;

	bool RunWizard(QMap<QString, QVariant> & config, bool & cancelled, QString & error);
	void NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler);
	bool QueueBatchCommand(QUuid uuid, QMap<QString, QVariant> & config, QString & error);
	void ShowHideColumn(QString colname, bool show);

public:
	CLC7WizardComponent();
	virtual ~CLC7WizardComponent();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	// ILC7Component
	virtual QUuid GetID();
	virtual RETURNCODE ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl = NULL);
	virtual bool ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error);
};


#endif
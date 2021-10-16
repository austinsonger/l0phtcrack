#ifndef __INC_CLC7COMMANDCONTROL_H
#define __INC_CLC7COMMANDCONTROL_H

class CLC7WorkQueue;

class CLC7CommandControl:public QObject, public ILC7CommandControl
{
	Q_OBJECT;

private:

	QString m_statustext;
	CLC7CommandControl *m_parent;
	CLC7WorkQueue *m_workqueue;
	
	int m_sub_count;

	virtual void ReleaseSubControl(ILC7CommandControl *ctrl);

public:

	CLC7CommandControl(CLC7WorkQueue *workqueue);
	CLC7CommandControl(CLC7CommandControl *parent, QString statustext);
	virtual ~CLC7CommandControl();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	virtual ILC7CommandControl *GetSubControl(QString statustext);
	virtual void ReleaseSubControl();

	virtual void AppendToActivityLog(QString text);
	virtual void SetStatusText(QString text);
	virtual void UpdateCurrentProgressBar(quint32 cur);
	virtual bool PauseRequested();
	virtual bool StopRequested();
	virtual void SaveCheckpointConfig(QMap<QString, QVariant> checkpoint_config);
};

#endif
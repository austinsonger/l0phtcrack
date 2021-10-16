#ifndef __INC_ILC7COMMANDCONTROL_H
#define __INC_ILC7COMMANDCONTROL_H

#include "core/ILC7Interface.h"

class ILC7CommandControl:public ILC7Interface
{

protected:
	virtual ~ILC7CommandControl() {}

public:

	virtual ILC7CommandControl *GetSubControl(QString statustext) = 0;
	virtual void ReleaseSubControl() = 0;

	virtual void AppendToActivityLog(QString text)=0;
	virtual void SetStatusText(QString text)=0;
	virtual void UpdateCurrentProgressBar(quint32 cur)=0;
	virtual bool PauseRequested()=0;
	virtual bool StopRequested()=0;
	virtual void SaveCheckpointConfig(QMap<QString, QVariant> checkpoint_config)=0;
};

class LC7SubControl
{
private:
	ILC7CommandControl *m_subctrl;
protected:
	LC7SubControl() = default;
	LC7SubControl(LC7SubControl const &) = delete;
	void operator=(LC7SubControl const &) = delete;

public:

	inline LC7SubControl(ILC7CommandControl *ctrl, QString statustext) { m_subctrl = ctrl->GetSubControl(statustext); }
	inline ~LC7SubControl() { m_subctrl->ReleaseSubControl(); }
	inline operator ILC7CommandControl*() { return m_subctrl; }
};

#endif
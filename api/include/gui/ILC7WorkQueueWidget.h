#ifndef __INC_ILC7WORKQUEUEWIDGET_H
#define __INC_ILC7WORKQUEUEWIDGET_H

#include"core/ILC7Interface.h"

#include<qstring.h>

class ILC7WorkQueueWidget:public ILC7Interface
{
protected:
	virtual ~ILC7WorkQueueWidget() {}
	
public:
	
	virtual void UpdateCurrentActivity(QString text) = 0;
	virtual void ClearActivityLog() = 0;
	virtual void ScrollToBottom() = 0;
	virtual void AppendToActivityLog(QString text) = 0;
	virtual void SetStatusText(QString text) = 0;
	virtual void UpdateCurrentProgressBar(quint32 cur) = 0;
	virtual void UpdateTotalProgressBar(quint32 cur) = 0;
	
	virtual bool Save(QDataStream & ds) =0;
	virtual bool Load(QDataStream & ds) = 0;

	virtual void Reset(void) =0;
};

#endif
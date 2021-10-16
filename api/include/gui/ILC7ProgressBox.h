#ifndef __INC_ILC7PROGRESSBOX_H
#define __INC_ILC7PROGRESSBOX_H

#include"core/ILC7Interface.h"

#include "qstring.h"

class ILC7ProgressBox: public ILC7Interface
{
protected:
	virtual ~ILC7ProgressBox() {}
	
public:
	virtual void UpdateStatusText(QString text)=0;
	virtual void UpdateProgressBar(quint32 cur)=0;
	virtual bool IsCancelled()=0;
	virtual void Release()=0;
};

class LC7ProgressBoxWrapper
{
private:
	ILC7ProgressBox *m_cancelbox;
public:
	inline LC7ProgressBoxWrapper(ILC7ProgressBox *cancelbox)
	{
		m_cancelbox=cancelbox;
	}
	inline ILC7ProgressBox * operator->()
	{
		return m_cancelbox;
	}
	inline ILC7ProgressBox * get()
	{
		return m_cancelbox;
	}
	inline ~LC7ProgressBoxWrapper()
	{
		m_cancelbox->Release();
	}
};


#endif
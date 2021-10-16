#ifndef __INC_ILC7GUICOMPONENT_H
#define __INC_ILC7GUICOMPONENT_H

class ILC7GUIComponent:public ILC7Component
{
public:
	virtual QIcon GetIcon()=0;
	virtual QString GetTitle()=0;
	virtual QString GetDescription()=0;
	virtual void Enter(QWidget *parent)=0;
	virtual void Exit(void)=0;
};


#endif
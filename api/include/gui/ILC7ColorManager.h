#ifndef __INC_ILC7COLORMANAGER_H
#define __INC_ILC7COLORMANAGER_H

#include"core/ILC7Interface.h"

#include<qobject.h>
#include<qcolor.h>
#include<qstring.h>
#include<qpixmap.h>
#include<qcommandlinkbutton.h>

class ILC7ColorManager:public ILC7Interface
{
protected:
	virtual ~ILC7ColorManager() {}

public:

	virtual void ReloadSettings(void)=0;

	virtual QMap<QString,int> GetDefaultShades()=0;
	virtual QColor GetDefaultBaseColor()=0;
	virtual QColor GetDefaultHighlightColor()=0;

	virtual void SetBaseColor(QColor color)=0;
	virtual void SetHighlightColor(QColor color)=0;
	virtual QColor GetBaseColor()=0;
	virtual QColor GetHighlightColor()=0;
	
	virtual QString GetStyleSheet(void)=0;
	virtual QString GetTextColor()=0;
	virtual QString GetInverseTextColor()=0;
	virtual QString GetHighlightShade(QString shade)=0;
	virtual QString GetBaseShade(QString shade)=0;
	
	virtual QPixmap GetHueColorPixmap(QPixmap pixmap, QColor huecolor=QColor()) = 0;
	virtual QPixmap GetHueColorPixmapResource(QString resource_path, QColor huecolor=QColor())=0;
	virtual QPixmap GetMonoColorPixmap(QPixmap pixmap, QColor basecolor = QColor()) = 0;
	virtual QPixmap GetMonoColorPixmapResource(QString resource_path, QColor basecolor=QColor()) = 0;
	virtual QIcon GetMonoColorIcon(QString resource, QColor normal = QColor(), QColor active = QColor(), QColor disabled = QColor()) = 0;

	virtual QGraphicsEffect *CreateShadowEffect(void)=0;
	virtual void StyleCommandLinkButton(QCommandLinkButton *commandlinkbutton)=0;

	virtual int GetSizeRatio(void) = 0;

	virtual void RegisterRecolorCallback(QObject *slot_obj, void (QObject::*slot_method)(void))=0;
};

#endif
#ifndef __INC_TECHNIQUEJTRSINGLEGUI_H
#define __INC_TECHNIQUEJTRSINGLEGUI_H

#include"../../lc7/include/ILC7GUIComponent.h"

class CTechniqueJTRSingleGUI :public QObject, public ILC7Component
{
	Q_OBJECT;

signals:
	void sig_isValid(bool);

public:
	CTechniqueJTRSingleGUI();
	virtual ~CTechniqueJTRSingleGUI();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	// ILC7Component
	virtual QUuid GetID();
	virtual RETURNCODE ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl = NULL);
	virtual bool ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error);
};


#endif
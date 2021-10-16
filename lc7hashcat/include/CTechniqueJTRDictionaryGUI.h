#ifndef __INC_TECHNIQUEJTRDICTIONARYGUI_H
#define __INC_TECHNIQUEJTRDICTIONARYGUI_H

#include"../../lc7/include/ILC7GUIComponent.h"

class CTechniqueJTRDictionaryGUI:public QObject, public ILC7Component
{
	Q_OBJECT;
	
public:
	CTechniqueJTRDictionaryGUI();
	virtual ~CTechniqueJTRDictionaryGUI();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	// ILC7Component
	virtual QUuid GetID();
	virtual RETURNCODE ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl = NULL);
	virtual bool ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error);
};


#endif
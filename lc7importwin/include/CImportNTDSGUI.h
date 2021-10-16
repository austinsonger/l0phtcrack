#ifndef __INC_IMPORTNTDSGUI_H
#define __INC_IMPORTNTDSGUI_H

#include"../../lc7/include/ILC7GUIComponent.h"

class CImportNTDSGUI :public QObject, public ILC7Component
{
	Q_OBJECT;

private:

public:
	CImportNTDSGUI();
	virtual ~CImportNTDSGUI();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	// ILC7Component
	virtual QUuid GetID();
	virtual ILC7Component::RETURNCODE ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl = NULL);
	virtual bool ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error);
};



#endif
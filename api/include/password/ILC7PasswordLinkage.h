#ifndef __INC_LC7PASSWORDLINKAGE
#define __INC_LC7PASSWORDLINKAGE

#include"core/ILC7Interface.h"
#include"password/ILC7PasswordEngine.h"

#include "lc7hashtype.h"

#ifndef UUID_LC7PASSWORDLINKAGE
#define UUID_LC7PASSWORDLINKAGE QUuid("{1510c910-516b-497c-af07-a4c27459595b}")
#endif

#define GET_ILC7PASSWORDLINKAGE(linkage) (static_cast<ILC7PasswordLinkage *>((linkage)->FindComponentByID(UUID_LC7PASSWORDLINKAGE)))

class ILC7PasswordLinkage: public ILC7Component
{
protected:
	virtual ~ILC7PasswordLinkage() {}

public:

	virtual void RegisterPasswordEngine(ILC7PasswordEngine *engine) = 0;
	virtual void UnregisterPasswordEngine(ILC7PasswordEngine *engine) = 0;
	virtual QList<ILC7PasswordEngine *> ListPasswordEngines() = 0;
	virtual ILC7PasswordEngine *GetPasswordEngineByID(QUuid id) = 0;

	virtual void RegisterHashType(fourcc fcc, QString name, QString description, QString category, QString platform, QUuid plugin)=0;
	virtual void UnregisterHashType(fourcc fcc, QString category, QUuid plugin) = 0;
	virtual bool LookupHashType(fourcc fcc, LC7HashType & hashtype, QString & error) = 0;
	virtual QList<fourcc> ListHashTypes() = 0;

	virtual ILC7CalibrationTable *NewCalibrationTable() = 0;
	virtual ILC7CalibrationTable *LoadCalibrationTable(QString table_key) = 0;
	virtual bool SaveCalibrationTable(QString table_key, ILC7CalibrationTable *table) = 0;

};

#endif
#ifndef __INC_ILC7PASSWORDENGINE_H
#define __INC_ILC7PASSWORDENGINE_H

#include"core/ILC7Interface.h"
#include<QVector>

class ILC7PasswordEngine:public ILC7Interface
{
protected:
	virtual ~ILC7PasswordEngine() {}

public:

	// Calibration
	enum CALIBRATION_ACTIVITY
	{
		CAL_STARTED = 0,
		CAL_STOPPED = 1,
		CAL_BEGIN_CELL = 2,
		CAL_END_CELL = 3,
		CAL_ERROR = 4,
		CAL_RESULTS = 5
	};
	struct CALIBRATION_CALLBACK_ARGUMENTS
	{
		// The status of the calibration request
		CALIBRATION_ACTIVITY activity;

		// What this callback belongs to
		ILC7CalibrationTable *table;
		QVariant rowId;
		QVariant colId;

		// Calibration Data
		bool valid;
		quint64 cps;
		QVariant data;

		// Extra details for error
		QString details;
	};
	typedef void (QObject::*CALIBRATION_CALLBACK)(const CALIBRATION_CALLBACK_ARGUMENTS &args);
	struct CALIBRATION_DEFAULT_SET
	{
		QUuid id;
		QString name;
		QMap<QVariant, QVariant> default_col_by_row;
	};
	struct CALIBRATION_COL_INFO
	{
		QString name;
		QString description;
	};
	struct CALIBRATION_ROW_INFO
	{
		QString engine;
		QString platform;
		QString hashtype;
		QString audittype;
		QString description;
	};

public:

	// Identification
	virtual QUuid GetID()=0;
	virtual QString GetDisplayName()=0;
	virtual QString GetDescription()=0;
    
	virtual QList<CALIBRATION_DEFAULT_SET> GetCalibrationDefaultSets()=0;
	virtual void ResetCalibrationTable(ILC7CalibrationTable *table, QUuid default_set_id=QUuid())=0;
	virtual bool GetCalibrationRowInfo(ILC7CalibrationTable *table, QVariant rowId, CALIBRATION_ROW_INFO &rowInfo, QString &error) = 0;
	virtual bool GetCalibrationColInfo(ILC7CalibrationTable *table, QVariant colId, CALIBRATION_COL_INFO &colInfo, QString &error) = 0;
	virtual bool RunCalibration(ILC7CalibrationTable *table, QVariant rowId, QVariant colId, QObject *callback_object, CALIBRATION_CALLBACK callback) = 0;
	virtual void StopCalibration(bool force=false) = 0;
	virtual bool IsCalibrationRunning() = 0;
	virtual bool IsCalibrationStopping() = 0;
	virtual QString GetCalibrationKey(void) = 0 ;

	virtual ILC7GPUManager *GetGPUManager(void) = 0;

	virtual ILC7Component::RETURNCODE Crack(QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl) = 0;
	virtual bool ValidateCrack(QMap<QString, QVariant> & state, QMap<QString, QVariant> & config, QString & error) = 0;
};



#endif
#ifndef __INC_CLC7JTRPASSWORDENGINE_H
#define __INC_CLC7JTRPASSWORDENGINE_H

#include "CLC7ExecuteJTR.h"
#include "CLC7JTRGPUManager.h"

class CLC7JTRCalibrationController;

class CLC7JTRCalibrationThread : public QThread
{
	Q_OBJECT
public:

	CLC7JTRCalibrationController *m_controller;
	CLC7ExecuteJTR *m_exejtr;
	quint64 m_cps;
	QString m_jtrversion;
	QString m_algo;
	bool m_mask;
	LC7GPUInfo m_gpuinfo;
	
public:
	CLC7JTRCalibrationThread(CLC7JTRCalibrationController *controller, QString algo, QString jtrversion, bool mask, const LC7GPUInfo &gpuinfo);
	virtual ~CLC7JTRCalibrationThread();
	virtual void run(void);
	void abort(void);
	void terminate(void);
};

class CLC7JTRCalibrationController:public QObject
{
	Q_OBJECT
private:
	ILC7CalibrationTable *m_table;
	QVariant m_rowId;
	QVariant m_colId;
	QVariant m_rowId_filter;
	QVariant m_colId_filter;
	QObject *m_callback_object;
	ILC7PasswordEngine::CALIBRATION_CALLBACK m_callback;
	
	enum class INTERRUPT_TYPE
	{
		NONE=0,
		ABORT,
		TERMINATE
	};
	
	INTERRUPT_TYPE m_interrupt_type;
protected:

	void report_started();
	void report_begin_cell();
	void report_results();
	void report_end_cell();
	void report_stopped();
	void report_error(const QString & details);

	bool run_calibration_threads(fourcc fcc, GPUPLATFORM gpuplatform, QString cpuins, bool mask, quint64 &cps);

public:
	CLC7JTRCalibrationController(ILC7CalibrationTable *table, QVariant rowId, QVariant colId, QObject *callback_object, ILC7PasswordEngine::CALIBRATION_CALLBACK callback);
	virtual ~CLC7JTRCalibrationController();
	bool calibrate(void);
	void abort(void);
	void terminate(void);
	bool is_stopping(void);

signals:
	void sig_calibrationCallback(const ILC7PasswordEngine::CALIBRATION_CALLBACK_ARGUMENTS &args);
};


class CLC7JTRPasswordEngine : public QObject, public ILC7PasswordEngine
{
	Q_OBJECT
private:

	ILC7AccountList *m_accountlist;

	CLC7JTRCalibrationController *m_controller;
	CLC7JTRGPUManager m_gpu_manager;
	bool m_running;
	QMutex m_mutex;

	virtual void NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler);
	
	static bool hashTypeLessThan(fourcc fcc1, fourcc fcc2);

public:


	CLC7JTRPasswordEngine();
	virtual ~CLC7JTRPasswordEngine();

	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);
	virtual QUuid GetID();
	virtual QString GetDisplayName();
	virtual QString GetDescription();

	virtual QList<CALIBRATION_DEFAULT_SET> GetCalibrationDefaultSets();
	virtual void ResetCalibrationTable(ILC7CalibrationTable *table, QUuid default_set_id = QUuid());
	virtual bool GetCalibrationRowInfo(ILC7CalibrationTable *table, QVariant rowId, CALIBRATION_ROW_INFO &rowInfo, QString &error);
	virtual bool GetCalibrationColInfo(ILC7CalibrationTable *table, QVariant colId, CALIBRATION_COL_INFO &colInfo, QString &error);
	virtual bool RunCalibration(ILC7CalibrationTable *table, QVariant rowId, QVariant colId, QObject *callback_object, CALIBRATION_CALLBACK callback);
	virtual void StopCalibration(bool force = false);
	virtual bool IsCalibrationRunning();
	virtual bool IsCalibrationStopping();
	virtual QString GetCalibrationKey(void);

	virtual ILC7GPUManager *GetGPUManager(void);

	virtual ILC7Component::RETURNCODE Crack(QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl);
	virtual bool ValidateCrack(QMap<QString, QVariant> & state, QMap<QString, QVariant> & config, QString & error);
};




#endif
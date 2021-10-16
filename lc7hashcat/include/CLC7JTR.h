#ifndef __INC_CLC7JTR_H
#define __INC_CLC7JTR_H

#include"jtrworker.h"
#include"CLC7Calibrate.h"

class ILC7CommandControl;
class ILC7AccountList;

class CLC7JTR: public QObject
{
	Q_OBJECT

private:
	friend class CJTRWorker;

	ILC7CommandControl *m_ctrl;
	QMap<QString,QVariant> m_config;
	ILC7AccountList *m_accountlist;
	QMultiMap<QByteArray, QPair<int, int> > m_accts_by_firsthalf;		// pair of account number and hash number within account
	QMultiMap<QByteArray, QPair<int, int> > m_accts_by_secondhalf;
	QMultiMap<QByteArray, QPair<int, int> > m_accts_by_hash;
	QString m_error;
	CLC7Calibration m_cal;
	JTRWORKERCTX *m_ctx;
	
	CJTRWorker *m_pJTRWorker;
	QMutex m_statusmutex;

	QMap<QUuid, QTextCodec *> m_encodingcache;

	bool m_bStopRequested;
	bool m_bReset;
	bool m_is_error;

	QString GenerateCommandLine(const JTRPASS & pass, bool restore, QString potfilename, QString sessionfilename);
	bool AddPasses(fourcc hashtype, int durationblock);
	bool AddNTTogglePass(int durationblock);
	bool ProcessHashes(quint32 hashtype, bool create_index, bool create_hash_file);
	bool ConfigurePassNodes(JTRPASS & pass);
	QByteArray ConvertToEncoding(QString str, QUuid encoding = QUuid());
	//bool PreflightNode(JTRPASS & pass, int node);

public:

	CLC7JTR(ILC7AccountList *accountlist, ILC7CommandControl *ctrl);
	virtual ~CLC7JTR();

	static QStringList GetSupportedInstructionSets(bool include_disabled = false);
	static void DisableInstructionSet(QString jtrdllversion);
	static QString GetCPUNodeAlgorithm(fourcc hashtype);
	static QString GetOpenCLNodeAlgorithm(fourcc hashtype);
	//static QString GetCUDANodeAlgorithm(fourcc hashtype);

	virtual bool Configure(QMap<QString,QVariant> config);

	virtual QMap<QString, QVariant> GetConfig();
	virtual QMap<QString, QVariant> GetCheckpointConfig();

	virtual void StartCracking(void);
	virtual void StopCracking(void);
	virtual void ResetCracking(void);
	virtual bool CheckCrackingFinished(bool & success);	
	virtual void ProcessStatus(void);
	virtual void Cleanup(void);
	virtual bool IsReset(void);
	virtual QString LastError(void);

};

#endif
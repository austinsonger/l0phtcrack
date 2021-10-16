#ifndef __INC_JTRWORKER_H
#define __INC_JTRWORKER_H

#include<QThread>

#include "CLC7JTRGPUManager.h"


struct JTRSTATUS 
{
	QString status;
	QString details;
	float percent_done;
	unsigned int secs_total;

	JTRSTATUS() { status = "Initializing"; details = "Initializing status..."; percent_done = 0.0f; secs_total = 0; }
};

struct JTRPASSNODE
{
	int node;
	int nodecount;

	LC7GPUInfo gpuinfo;
	
	QString jtrversion;
	QString node_algorithm;
	QString preflight_node_algorithm;

	JTRPASSNODE() { node = 0xCCCCCCCC; nodecount = 0xCCCCCCCC; }
	void Save(QVariant & v);
	bool Load(const QVariant & v);
};

struct JTRPASS
{
	int passnumber;
	QString passdescription;

	fourcc hashtype;
	QString jtrmode;
	int durationblock; // which time slice this pass is in (index into ctx.m_duration_block_seconds_left)

	// wordlist mode (and mask mode)
	QString wordlist_file;
	quint64 wordlist_count;
	QString encoding; 
	QString rule;
	bool leet;

	// incremental mode (and mask mode)
	quint32 num_chars_min;
	quint32 num_chars_max;
	QString character_set;
	QString mask;

	// cpu/gpu configuration
	QVector<JTRPASSNODE> nodes;

	// preflight for progress estimation
	//quint64 candidates_done;
	//quint64 candidates_total;

	JTRPASS() { passnumber = 0xCCCCCCCC; hashtype = 0xCCCCCCCC; durationblock = 0xCCCCCCCC; wordlist_count = 0xCCCCCCCCCCCCCCCCull; num_chars_min = 0xCCCCCCCC; num_chars_max = 0xCCCCCCCC; /* candidates_done = 0xCCCCCCCCCCCCCCCCull; candidates_total = 0xCCCCCCCCCCCCCCCCull; */}
	void Save(QVariant & v);
	bool Load(const QVariant & v);
};
	
struct JTRWORKERCTX
{
	bool m_restore;
	QString m_temporary_dir;
	QList<JTRPASS> m_jtrpasses;	
	int m_current_pass;
	QUuid m_current_input_encoding;

	// duration
	bool m_duration_unlimited;
	quint32 m_duration_seconds;

	quint32 m_elapsed_seconds;
	quint32 m_elapsed_seconds_at_start_of_pass;

	QList<quint32> m_duration_block_seconds_left;

	JTRWORKERCTX();
	~JTRWORKERCTX();
	void Save(QVariant & v);
	bool Load(const QVariant & v);
	void RewriteFilePaths(QString filename, QByteArray & contents);

};

class CLC7JTR;
class CJTRNodeWorker;
class ILC7CommandControl;

class CJTRWorker : public QThread 
{
    Q_OBJECT

public:
	CJTRWorker(CLC7JTR *lc7jtr, JTRWORKERCTX *ctx, ILC7CommandControl *ctrl, ILC7AccountList *accountlist);
    virtual ~CJTRWorker();

	virtual void run();
	void stop();
	bool lasterror(QString &err);
	JTRSTATUS get_status();
	QMap<QByteArray,QString> get_cracked();

private:

	QString m_error;
	bool m_is_error;
	CLC7JTR *m_lc7jtr;
	JTRWORKERCTX *m_ctx;
	QList<CJTRNodeWorker *> m_nodeworkers;
	size_t m_potsize;
	ILC7CommandControl *m_ctrl;
	bool m_status_available;
	JTRSTATUS m_last_status;
	JTRSTATUS m_previous_last_status;
	ILC7AccountList *m_accountlist;
	
	HANDLE m_workerDoneEvent;

protected:
	QString format_speed(unsigned long long count);
	void StartNode(JTRPASS &pass, int passnode);
	void set_error(QString err);
	
	bool RunPass(JTRPASS &pass, QString & error);
	void CleanUpNodeWorkers();
	bool GenerateCrackedWordlist(QString &wordlist_file, QString &error);

protected slots:

	void slot_workerDone(void);


};

#endif
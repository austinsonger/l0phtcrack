#ifndef __INC_SAMIMPORTER_H
#define __INC_SAMIMPORTER_H


class SAMImporter
{
private:
	ILC7AccountList *m_accountlist;
	ILC7CommandControl *m_ctrl;
	int m_account_limit;

	void UpdateStatus(QString statustext, quint32 cur, bool statuslog = true);

	bool DecryptHash(quint32 rid, QByteArray hash_data, quint8 out_hash[16], quint8 hbootkey[16], bool isNT);
	bool AddEntry(int ofs_entrykey, quint32 rid, struct hive *sam_hive, quint8 hbootkey[16], QString &error);
	quint32 FindControlSet(struct hive *system_hive);
	unsigned char *GetBootKey(struct hive *system_hive, size_t &bootkeylen);
	bool GetHBootKey(struct hive *sam_hive, unsigned char *bootkey, size_t bootkeylen, quint8 hbootkey[16]);

public:

	SAMImporter(ILC7AccountList *accountlist, ILC7CommandControl *ctrl);
	~SAMImporter();

	void SetAccountLimit(quint32 alim);
	bool DoImport(QString sam_filename, QString system_filename, QString & error, bool & cancelled);
};


#endif
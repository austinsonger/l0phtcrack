#ifndef __INC_CACCOUNTSEXPORT_H
#define __INC_CACCOUNTSEXPORT_H

class QPrinter;

class CAccountsExport:public QObject
{
private:
	Q_OBJECT;

	ILC7AccountList *m_accountlist;
	ILC7CommandControl *m_ctrl;
	ILC7PasswordLinkage *m_plink;

	bool m_alt;
	bool m_include_style;
	int m_current_page;
	int m_page_total;
	QMap<QString, bool> m_column_enabled;
	QString m_filename;
	QString m_format;
	QList<fourcc> m_hashtypes;

protected:

	void DetermineHashes(void);
	QStringList GenerateColumnNames(void);
	QStringList GenerateColumns(const LC7Account &acct);

	void StartDocument_CSV(QIODevice & outfile);
//	void StartDocument_PDF(QIODevice & outfile);
	void StartDocument_HTML(QIODevice & outfile);
	void StartDocument_XML(QIODevice & outfile);

	void EmitAccount_CSV(QIODevice & outfile, const LC7Account &acct);
//	void EmitAccount_PDF(QIODevice & outfile, const LC7Account &acct);
	void EmitAccount_HTML(QIODevice & outfile, const LC7Account &acct);
	void EmitAccount_XML(QIODevice & outfile, const LC7Account &acct);

	void EndDocument_CSV(QIODevice & outfile);
//	void EndDocument_PDF(QIODevice & outfile);
	void EndDocument_HTML(QIODevice & outfile);
	void EndDocument_XML(QIODevice & outfile);

	bool EmitDocument_PDF(QString &error);
	bool EmitPage_PDF(QPainter *painter, QPrinter *printer, int page, int count, QString &error);

public:

	CAccountsExport(ILC7AccountList *accountlist, ILC7CommandControl *ctrl);
	~CAccountsExport();

	void setFilename(QString filename);
	void setFormat(QString format);
	void setIncludeStyle(bool style);
	void enableColumn(QString colname, bool enable);

	bool DoExport(QString &error, bool &cancelled);
};

#endif
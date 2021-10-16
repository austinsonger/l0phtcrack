#ifndef IMPORTSHADOWCONFIG_H
#define IMPORTSHADOWCONFIG_H

#include <QWidget>
#include"ui_importshadowconfig.h"
#include"ShadowImporter.h"

class ImportShadowConfig : public QWidget
{
	Q_OBJECT

public:
	ImportShadowConfig(QWidget *parent, QWidget *page, const QMap<QString, QVariant> & def_config, bool simple=false);
	~ImportShadowConfig();

	QString GetFileFormat();
	QString GetFile1Name();
	QString GetFile2Name();
	QString GetFile3Name();
	bool GetKeepCurrentAccounts();
	bool GetIncludeNonLogin();
	FOURCC GetHashType();
	bool GetLimitAccounts();
	quint32 GetAccountLimit();

private:
	Ui::ImportShadowConfig ui;
	QWidget *m_page;

	bool m_keep_current_accounts;
	bool m_include_non_login;
	QString m_fileformat;
	int m_fileformatindex;
	QString m_file1name;
	QString m_file2name;
	QString m_file3name;
	FOURCC m_hashtype;
	bool m_file1valid;
	bool m_file2valid;
	bool m_file3valid;
	bool m_limit_accounts;
	quint32 m_account_limit;

	QString m_file1type;
	QString m_file2type;
	QString m_file3type;

	ShadowImporter m_shadowimporter;
	QMap<QString, ILC7UnixImporter *> m_importers;

	void CheckFileFormat();
	void CheckHashTypes();
	void RefreshContent();
	void UpdateUI();

private slots:

	void slot_currentIndexChanged_fileFormatComboBox(int index);
	void slot_textChanged_file1Edit(const QString & str);
	void slot_textChanged_file2Edit(const QString & str);
	void slot_textChanged_file3Edit(const QString & str);
	void slot_clicked_browseFile1(bool checked);
	void slot_clicked_browseFile2(bool checked);
	void slot_clicked_browseFile3(bool checked);
	void slot_clicked_KeepCurrentAccounts(bool checked);
	void slot_currentRowChanged_hashTypeList(int row);
	void slot_clicked_includeNonLoginCheckBox(bool checked);
	void slot_clicked_LimitAccounts(bool checked);
	void slot_textChanged_AccountLimit(const QString &str);

signals:
	void sig_isValid(bool valid);

};

#endif // IMPORTSHADOWCONFIG_H

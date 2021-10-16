#ifndef CLC7UPDATEDIALOG_H	
#define CLC7UPDATEDIALOG_H

#include <QDialog>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QProgressDialog>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>

#include "ui_LC7UpdateDialog.h"

class CLC7UpdateDialog : public QDialog
{
	Q_OBJECT

public:
	
	CLC7UpdateDialog(ILC7Controller *ctrl, QString url, QString current_version, QString new_version, QString changes);
	~CLC7UpdateDialog();

public:
	void startDownload();
	void UpdateUI(void);
	
	bool getSkipThisUpdate();
	bool getRemindMeTomorrow();
	QString getFilePath();

	
private slots:
	void slot_skipThisUpdateButton_clicked(bool checked);
	void slot_remindMeLaterButton_clicked(bool checked);
	void slot_downloadAndUpdateNowButton_clicked(bool checked);
	void slot_cancelButton_clicked(bool checked);

	void slot_httpReadyRead();
	void slot_fileDownloadFinished();
	void slot_updateDownloadProgress(qint64, qint64);

protected:
	void closeEvent(QCloseEvent * e);


private:
	Ui::LC7UpdateDialog ui;

	ILC7Controller *m_ctrl;
	
	QUrl m_url;
	QString m_current_version;
	QString m_new_version;
	QString m_changes;
	bool m_skipThisUpdate;
	bool m_remindMeTomorrow;
	QString m_filepath;

	QNetworkAccessManager *m_manager;
	QNetworkReply *m_reply;
	QFile *m_file;
	bool m_httpRequestAborted;
	qint64 m_fileSize;

};

#endif // CLC7UpdateDialog_H
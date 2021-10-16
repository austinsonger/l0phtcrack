#include "stdafx.h"

CLC7UpdateDialog::CLC7UpdateDialog(ILC7Controller *ctrl, QString url, QString current_version, QString new_version, QString changes) : QDialog(NULL, Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
	ui.setupUi(this);

	m_ctrl = ctrl;

	m_skipThisUpdate = false;
	m_remindMeTomorrow = false;

	m_manager = NULL;
	m_reply = NULL;
	m_file = NULL;

	m_url = url;
	m_current_version = current_version;
	m_new_version = new_version;
	m_changes = changes;

	ui.progressWidget->setVisible(false);
	ui.changesEdit->setWordWrapMode(QTextOption::WordWrap);
	ui.currentVersionEdit->setText(current_version);
	ui.newVersionEdit->setText(new_version);
	ui.changesEdit->setText(changes);
	ui.downloadAndUpdateNowButton->setDefault(true);

	connect(ui.skipThisUpdateButton, &QAbstractButton::clicked, this, &CLC7UpdateDialog::slot_skipThisUpdateButton_clicked);
	connect(ui.remindMeLaterButton, &QAbstractButton::clicked, this, &CLC7UpdateDialog::slot_remindMeLaterButton_clicked);
	connect(ui.downloadAndUpdateNowButton, &QAbstractButton::clicked, this, &CLC7UpdateDialog::slot_downloadAndUpdateNowButton_clicked);
	connect(ui.cancelButton, &QAbstractButton::clicked, this, &CLC7UpdateDialog::slot_cancelButton_clicked);

	adjustSize();

	UpdateUI();
}

CLC7UpdateDialog::~CLC7UpdateDialog()
{TR;
}

void CLC7UpdateDialog::UpdateUI()
{TR;
	bool changed = false;

	if (m_reply != NULL && ui.updateWidget->isVisible())
	{
		ui.updateWidget->setVisible(false);
		ui.progressWidget->setVisible(true);
		setWindowTitle("Downloading Update");
		changed = true;
	}
	if (m_reply == NULL && ui.progressWidget->isVisible())
	{
		ui.updateWidget->setVisible(true);
		ui.progressWidget->setVisible(false);
		setWindowTitle("Update Available");
		changed = true;
	}

	if (changed)
	{
		adjustSize();
	}
}

bool CLC7UpdateDialog::getSkipThisUpdate()
{TR;
	return m_skipThisUpdate;
}

bool CLC7UpdateDialog::getRemindMeTomorrow()
{
	TR;
	return m_remindMeTomorrow;
}

QString CLC7UpdateDialog::getFilePath()
{TR;
	return m_filepath;
}

void CLC7UpdateDialog::closeEvent(QCloseEvent * e)
{TR;
	if (m_reply)
	{
		slot_cancelButton_clicked(false);
	}
	else
	{
		slot_remindMeLaterButton_clicked(false);
	}
}

void CLC7UpdateDialog::slot_skipThisUpdateButton_clicked(bool checked)
{TR;
	m_skipThisUpdate = true;

	reject();
}

void CLC7UpdateDialog::slot_remindMeLaterButton_clicked(bool checked)
{TR;
	m_remindMeTomorrow = true;

	reject();
}

void CLC7UpdateDialog::slot_downloadAndUpdateNowButton_clicked(bool checked)
{TR;
	// Ensure things are closed down
	if (!m_ctrl->GetGUILinkage()->RequestCloseSession(false, false))
	{
		return;
	}

	startDownload();
}


void CLC7UpdateDialog::startDownload()
{TR;
	QString tempfile;
#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)
	QDir tempdir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
	tempfile = tempdir.absoluteFilePath(QUuid::createUuid().toString()+".exe");
#else
	QDir tempdir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
	tempfile = tempdir.absoluteFilePath(QUuid::createUuid().toString());
#endif

	m_file = new QFile(tempfile);
	if (!m_file->open(QIODevice::WriteOnly))
	{
		delete m_file;
		m_file = NULL;

		m_ctrl->GetGUILinkage()->ErrorMessage("Error downloading file", "Couldn't create file to download update. You may be out of disk space or do not have permissions to perform this action.");

		return;
	}
	
	// This will be set true when canceled from progress dialog
	m_httpRequestAborted = false;
	
	m_manager = new QNetworkAccessManager(this);

	// get() method posts a request to obtain the contents of the target request
	// and returns a new QNetworkReply object opened for reading which emits the readyRead() signal whenever new data arrives.
	m_reply = m_manager->get(QNetworkRequest(m_url));

	if (!m_reply)
	{
		delete m_manager;
		m_manager = NULL;
		m_file->close();
		delete m_file;
		m_file = NULL;

		m_ctrl->GetGUILinkage()->ErrorMessage("Error creating network request", "Couldn't request the update URL. The host may be unreachable.");

		return;
	}

	// Whenever more data is received from the network,this readyRead() signal is emitted
	connect(m_reply, &QNetworkReply::readyRead, this, &CLC7UpdateDialog::slot_httpReadyRead);

	// Also, downloadProgress() signal is emitted when data is received
	connect(m_reply, &QNetworkReply::downloadProgress, this, &CLC7UpdateDialog::slot_updateDownloadProgress);

	// This signal is emitted when the reply has finished processing.
	// After this signal is emitted, there will be no more updates to the reply's data or metadata.
	connect(m_reply, &QNetworkReply::finished, this, &CLC7UpdateDialog::slot_fileDownloadFinished);

	UpdateUI();
}


void CLC7UpdateDialog::slot_httpReadyRead()
{TR;
	// this slot gets called every time the QNetworkReply has new data.
	// We read all of its new data and write it into the file.
	// That way we use less RAM than when reading it at the finished()
	// signal of the QNetworkReply
	m_file->write(m_reply->readAll());
}

void CLC7UpdateDialog::slot_updateDownloadProgress(qint64 bytesRead, qint64 totalBytes)
{TR;
	if (m_httpRequestAborted)
		return;

	ui.progressBar->setMaximum(totalBytes);
	ui.progressBar->setValue(bytesRead);
	ui.currentBytes->setText(QString("%1").arg(bytesRead));
	ui.totalBytes->setText(QString("%1").arg(totalBytes));

	UpdateUI();
}

// During the download progress, it can be canceled
void CLC7UpdateDialog::slot_cancelButton_clicked(bool checked)
{TR;
	m_httpRequestAborted = true;
	m_reply->abort();

	UpdateUI();
}

// When download finished or canceled, this will be called
void CLC7UpdateDialog::slot_fileDownloadFinished()
{TR;
	// when canceled
	if (m_httpRequestAborted) 
	{
		if (m_file) 
		{
			m_file->close();
			m_file->remove();
			delete m_file;
			m_file = NULL;
		}
		m_reply->deleteLater();
		m_manager->deleteLater();
		m_reply = NULL;
		m_manager = NULL;

		UpdateUI();
		return;
	}

	m_file->flush();
	m_file->close();

	if (m_reply->error())
	{
		m_file->remove();
		QMessageBox::information(this, tr("Network Error"), tr("Download failed: %1.").arg(m_reply->errorString()));
	}
	
	// get redirection url if we are redirected
	QVariant redirectionTarget = m_reply->attribute(QNetworkRequest::RedirectionTargetAttribute);

	// Clean up
	m_filepath = m_file->fileName();

	delete m_file;
	m_file = NULL;
	m_reply->deleteLater();
	m_reply = NULL;
	m_manager->deleteLater();
	m_manager = NULL;

	// Redirect if we have to
	if (!redirectionTarget.isNull()) 
	{
		m_url = m_url.resolved(redirectionTarget.toUrl());

		startDownload();
		return;
	}
	
	accept();
}

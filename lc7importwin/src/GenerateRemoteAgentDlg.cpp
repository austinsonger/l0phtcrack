#include<stdafx.h>

GenerateRemoteAgentDlg::GenerateRemoteAgentDlg(QWidget *parent) :QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
{
	ui.setupUi(this);


	QString lastagentloc = g_pLinkage->GetSettings()->value(
		QString("%1:remoteagentloc").arg(UUID_IMPORTWINDOWSREMOTEGUI.toString()),
		QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).toString();

	ui.remoteAgentEdit->setText(QDir::toNativeSeparators(lastagentloc));

	connect(ui.browseButton, &QAbstractButton::clicked, this, &GenerateRemoteAgentDlg::slot_browseButton_clicked);
	connect(ui.generateButton, &QAbstractButton::clicked, this, &GenerateRemoteAgentDlg::slot_generateButton_clicked);
	connect(ui.remoteAgentEdit, &QLineEdit::textChanged, this, &GenerateRemoteAgentDlg::slot_remoteAgentEdit_textChanged);
	connect(ui.cancelButton, &QAbstractButton::clicked, this, &GenerateRemoteAgentDlg::reject);

	UpdateUI();
}

GenerateRemoteAgentDlg::~GenerateRemoteAgentDlg()
{

}

void GenerateRemoteAgentDlg::UpdateUI(void)
{
	QString location = QDir::fromNativeSeparators(ui.remoteAgentEdit->text());

	if (QFileInfo(location).isDir())
	{
		ui.generateButton->setEnabled(true);
	}
	else
	{
		ui.generateButton->setEnabled(false);
	}
}

void GenerateRemoteAgentDlg::slot_browseButton_clicked(int checked)
{
	QString location = QDir::fromNativeSeparators(ui.remoteAgentEdit->text());

	QString dirpath;
	if (!g_pLinkage->GetGUILinkage()->GetDirectoryDialog("Choose Remote Agent Output Location", location, dirpath))
	{
		return;
	}

	ui.remoteAgentEdit->setText(QDir::toNativeSeparators(dirpath));
}

void GenerateRemoteAgentDlg::slot_generateButton_clicked(int checked)
{
	QString location = QDir::fromNativeSeparators(ui.remoteAgentEdit->text());

	g_pLinkage->GetSettings()->setValue(QString("%1:remoteagentloc").arg(UUID_IMPORTWINDOWSREMOTEGUI.toString()), location);

	DoGenerateRemoteAgent(location);
	
	accept();
}

void GenerateRemoteAgentDlg::slot_remoteAgentEdit_textChanged(const QString &text)
{
	UpdateUI();
}


void GenerateRemoteAgentDlg::DoGenerateRemoteAgent(QString location)
{
	CImportWindows impwin(NULL,NULL);

	QDir instloc(location);
	QString instfile = instloc.absoluteFilePath("LC7 Remote Agent Installer.exe");
	QString error;
	if (!impwin.CreateRemoteAgentInstaller(instfile, error))
	{
		g_pLinkage->GetGUILinkage()->ErrorMessage("Error Generating Remote Agent", error);
		return;
	}
	g_pLinkage->GetGUILinkage()->InfoMessage("Success", "Remote agent generated successfully.");
}
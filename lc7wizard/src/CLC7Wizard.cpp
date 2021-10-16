#include"stdafx.h"

CLC7Wizard::CLC7Wizard(QWidget *parent)
	: QWizard(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{

	setDefaultProperty("QGroupBox", "checked", SIGNAL(toggled(bool)));

	ILC7ColorManager *colman = g_pLinkage->GetGUILinkage()->GetColorManager();

	//setOption(QWizard::ExtendedWatermarkPixmap, true);
	QLabel *sidewidget = new QLabel();
	sidewidget->setStyleSheet(
		"background: qlineargradient( x1:0 y1:0, x2:0 y2:1, stop:0 #AAAAAA, stop:1 #444444);"
		"margin: 0px;"
		"padding: 0px;"
		);
	sidewidget->setPixmap(colman->GetHueColorPixmapResource(":/lc7/lc7logo_small.png"));
	sidewidget->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
	setSideWidget(sidewidget);

	setPage(Page_Intro, new CPageIntro);
	setPage(Page_Windows_Or_Unix, new CPageWindowsOrUnix);
	setPage(Page_Windows_Import, new CPageWindowsImport);
	setPage(Page_Windows_Import_Local, new CPageWindowsImportLocal);
	setPage(Page_Windows_Import_Remote_SMB, new CPageWindowsImportRemoteSMB);
	setPage(Page_Windows_Import_PWDump, new CPageWindowsImportPWDump);
	setPage(Page_Unix_Import, new CPageUnixImport);
	setPage(Page_Unix_Import_File, new CPageUnixImportFile);
	setPage(Page_Unix_Import_SSH, new CPageUnixImportSSH);
	setPage(Page_Audit_Type, new CPageAuditType);
	setPage(Page_Reporting, new CPageReporting);
	setPage(Page_Scheduling, new CPageScheduling);
	setPage(Page_Summary, new CPageSummary);

	setStartId(Page_Intro);

//#ifndef Q_WS_MAC
	setWizardStyle(ModernStyle);
//#endif

	resize(700,550);

	setPixmap(QWizard::LogoPixmap, colman->GetHueColorPixmapResource(":/lc7wizard/wizardlogo.png"));
	setPixmap(QWizard::WatermarkPixmap, colman->GetHueColorPixmapResource(":/lc7wizard/wizardwatermark.png"));
	//setPixmap(QWizard::BackgroundPixmap, colman->GetHueColorPixmapResource(":/lc7wizard/wizardwatermark.png"));

//	connect(this, SIGNAL(helpRequested()), this, SLOT(showHelp()));

	setWindowTitle(tr("LC7 Password Auditing Wizard"));

	connect(this, &CLC7Wizard::window_loaded, this, &CLC7Wizard::slot_window_loaded, Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));
}

void CLC7Wizard::showHelp()
{	TR;
	QString message;

	switch (currentId()) {
	case Page_Intro:
		message = tr("The decision you make here will affect which page you "
			"get to see next.");
		break;
	default:
		message = tr("This help is likely not to be of any help.");
	}

	QMessageBox::information(this, tr("LC7 Password Auditing Wizard Help"), message);
}

void CLC7Wizard::showEvent(QShowEvent *ev)
{
	QWizard::showEvent(ev);
	emit window_loaded();
}

void CLC7Wizard::slot_window_loaded(void)
{
	TR;
	g_pLinkage->GetGUILinkage()->UpdateUI();
}
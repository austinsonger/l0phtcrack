#include "stdafx.h"

CLC7StartupWidget::CLC7StartupWidget(QList<ILC7Action *> custom_actions, QWidget *parent, ILC7Controller *ctrl):QFrame(parent)
{TR;
	m_ctrl = ctrl;
	ui.setupUi(this);

	ILC7ColorManager *colman=CLC7App::getInstance()->GetMainWindow()->GetColorManager();
	colman->RegisterRecolorCallback(this, (void(QObject::*)(void))&CLC7StartupWidget::RecolorCallback);
	RecolorCallback();

	ILC7Settings *settings=CLC7App::getInstance()->GetController()->GetSettings();
	bool show_startup_dialog=settings->value("_ui_:showstartupdialog",true).toBool();
	ui.ShowDialogCheckbox->setChecked(show_startup_dialog);

	QStringList recentsessionfiles=CLC7App::getInstance()->GetController()->GetRecentSessionsList();
	foreach(QString recentsessionfile, recentsessionfiles)
	{
		QListWidgetItem *item=new QListWidgetItem();
		item->setText(QFileInfo(recentsessionfile).fileName());
		item->setData(1,recentsessionfile);
		ui.recentSessionsList->addItem(item);
	}
	
	connect(ui.ShowDialogCheckbox,&QCheckBox::stateChanged,this,&CLC7StartupWidget::onShowDialogCheckbox);
	connect(ui.newSessionButton,&QAbstractButton::clicked,this,&CLC7StartupWidget::onNewSessionButton);
	connect(ui.openSessionButton,&QAbstractButton::clicked,this,&CLC7StartupWidget::onOpenSessionButton);
	connect(ui.recentSessionsList,&QListWidget::itemDoubleClicked,this,&CLC7StartupWidget::onRecentSessionsDoubleClicked);
	connect(ui.closeButton,&QAbstractButton::clicked,this,&CLC7StartupWidget::onCloseButton);

	colman->StyleCommandLinkButton(ui.newSessionButton);
	colman->StyleCommandLinkButton(ui.openSessionButton);

	// Add wizard plugins
	foreach(ILC7Action *act, custom_actions)
	{
		addWizardAction(act);
	}
}

CLC7StartupWidget::~CLC7StartupWidget()
{TR;

}

void CLC7StartupWidget::RecolorCallback(void)
{TR;
	ILC7ColorManager *colman = CLC7App::getInstance()->GetMainWindow()->GetColorManager();
	ui.InnerFrame->setMinimumSize(580 * colman->GetSizeRatio(), 360 * colman->GetSizeRatio());

	ui.newSessionButton->setIcon(QIcon(colman->GetHueColorPixmapResource(":/qss_icons/rc/commandlink.png")));
	ui.openSessionButton->setIcon(QIcon(colman->GetHueColorPixmapResource(":/qss_icons/rc/commandlink.png")));
	ui.logo->setPixmap(colman->GetHueColorPixmapResource(QString(":/lc7/lc7icon_%1.png").arg(256 * colman->GetSizeRatio())));
	ui.closeButton->setIcon(colman->GetMonoColorIcon(QString(":/lc7/cancel.png"),
		QColor(colman->GetBaseShade("BUTTON_BKGD_0")),
		QColor(colman->GetBaseShade("BUTTON_BKGD_PRESSED_0")),
		QColor(colman->GetBaseShade("BUTTON_COLOR_DISABLED"))
		));
	ui.closeButton->setIconSize(QSize(16, 16)*colman->GetSizeRatio());

}

void CLC7StartupWidget::onShowDialogCheckbox(int)
{TR;
	ILC7Settings *settings=CLC7App::getInstance()->GetController()->GetSettings();
	settings->setValue("_ui_:showstartupdialog",ui.ShowDialogCheckbox->isChecked());
}

void CLC7StartupWidget::onNewSessionButton(bool checked)
{TR;
	emit sig_newSession();
}

void CLC7StartupWidget::onOpenSessionButton(bool checked)
{TR;
	QString sessiondir = CLC7App::getInstance()->GetMainWindow()->GetRecentSessionDirectory();
	QString sessionfile;
	if(!m_ctrl->GetGUILinkage()->OpenFileDialog("Choose session file", sessiondir, "LC7 Session Files (*.lc7)", sessionfile))
	{
		return;
	}

	emit sig_openSession(sessionfile);
}

void CLC7StartupWidget::onCloseButton(bool checked)
{TR;
	emit sig_closeWindow();
}

void CLC7StartupWidget::onRecentSessionsDoubleClicked(QListWidgetItem *item)
{TR;
	QString sessionfile=item->data(1).toString();
	emit sig_openSession(sessionfile);
}

void CLC7StartupWidget::keyPressEvent(QKeyEvent *event)
{TR;
	if(event->type()==QKeyEvent::KeyPress && (event->matches(QKeySequence::Close) || event->key()==Qt::Key_Escape))
	{
		emit sig_closeWindow();
	}
	
}

void CLC7StartupWidget::addWizardAction(ILC7Action *act)
{TR;
	QCommandLinkButton *wizardbutt = new QCommandLinkButton();
	wizardbutt->setText(act->Name());
	//wizardbutt->setDescription(act->Desc());
	wizardbutt->setToolTip(act->Desc());
	wizardbutt->setProperty("action", QVariant((qulonglong)act));

	ILC7ColorManager *colman = CLC7App::getInstance()->GetMainWindow()->GetColorManager();
	colman->StyleCommandLinkButton(wizardbutt);

	connect(wizardbutt, &QCommandLinkButton::clicked, this, &CLC7StartupWidget::slot_wizardButton_clicked);

	ui.verticalLayout->insertWidget(0, wizardbutt);
}
 

void CLC7StartupWidget::slot_wizardButton_clicked(bool checked)
{TR;
	hide();

	ILC7Action *act = (ILC7Action *)(sender()->property("action").toULongLong());

	ILC7Component *comp = m_ctrl->FindComponentByID(act->ComponentId());
	QMap<QString, QVariant> config;
	QString error;
	if (comp->ExecuteCommand(act->Command(), act->Args(), config, error) != ILC7Component::SUCCESS)
	{
		show();

		m_ctrl->GetGUILinkage()->ErrorMessage("Error running wizard", error);
		return;
	}

	if (config["cancelled"].toBool() && !m_ctrl->IsSessionOpen())
	{
		show();
	}
	else
	{
		emit sig_closeWindow();
	}
}
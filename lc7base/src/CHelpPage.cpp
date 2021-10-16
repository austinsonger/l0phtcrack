#include<stdafx.h>

CHelpPage::CHelpPage(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	m_enable_ui=true;
	m_first_page = true;
	
	connect(ui.BackButton, &QAbstractButton::clicked, this, &CHelpPage::slot_backButton_clicked);
	connect(ui.ForwardButton, &QAbstractButton::clicked, this, &CHelpPage::slot_forwardButton_clicked);
	connect(ui.HomeButton, &QAbstractButton::clicked, this, &CHelpPage::slot_homeButton_clicked);
	connect(ui.SearchButton, &QAbstractButton::clicked, this, &CHelpPage::slot_searchButton_clicked);
	connect(ui.SearchEdit, &QLineEdit::textChanged, this, &CHelpPage::slot_SearchEdit_textChanged);

	m_helpbutton = g_pLinkage->GetGUILinkage()->GetHelpButton();
	connect(m_helpbutton, &QAbstractButton::clicked, this, &CHelpPage::slot_helpButtonClicked);

	///
	QDir helpdir(g_pLinkage->GetPluginsDirectory());
	helpdir.cd("lc7base{2f63a714-8518-4ab6-ba7d-5440888dfc8a}");
	helpdir.cd("help");
	m_helpEngine = new QHelpEngine(helpdir.absoluteFilePath("L0phtCrack Password Auditor v7.qhc"), this);
	m_helpEngine->setupData();

	m_contentWidget = m_helpEngine->contentWidget();
	//m_indexWidget = m_helpEngine->indexWidget();
	m_tabWidget = new QTabWidget(this);
	m_tabWidget->addTab(m_contentWidget, "Contents");
	//m_tabWidget->addTab(m_indexWidget, "Index");
	
	HelpBrowser *m_helpBrowser = new HelpBrowser(m_helpEngine);
	m_helpBrowser->setSource(
		QUrl("qthelp://l0phtcrack.password.auditor.v7/doc/Introduction.html"));
	m_helpBrowser->setStyleSheet("background: black;");

	connect(m_contentWidget, &QHelpContentWidget::linkActivated, m_helpBrowser, &HelpBrowser::setSource);
	//connect(m_indexWidget, &QHelpIndexWidget::linkActivated, m_helpBrowser, &HelpBrowser::setSource);

	QObject::connect(m_contentWidget, &QHelpContentWidget::clicked, m_contentWidget, &QHelpContentWidget::activated);
	QObject::connect(m_contentWidget, &QHelpContentWidget::clicked, this, &CHelpPage::slot_contentWidget_expandItem);

	m_horizSplitter = new QSplitter(Qt::Horizontal, this);
	m_horizSplitter->insertWidget(0, m_tabWidget);
	m_horizSplitter->insertWidget(1, m_helpBrowser);
	m_horizSplitter->setSizes(QList<int>() << 100 << width());
	m_tabWidget->setMinimumWidth(260);
	m_horizSplitter->setCollapsible(0, false);
	m_horizSplitter->setCollapsible(1, false);

	m_tabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	m_helpBrowser->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	m_horizSplitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	ui.frame->layout()->addWidget(m_horizSplitter);

	///
	
	//browseToUrl(QUrl("help:///"));
	
	delete ui.toolbarLayout;
	delete ui.ForwardButton;
	delete ui.BackButton;
	delete ui.TitleEdit;
	delete ui.HomeButton;
	delete ui.SearchEdit;
	delete ui.SearchButton;

	ILC7ColorManager *colman=g_pLinkage->GetGUILinkage()->GetColorManager();
	colman->RegisterRecolorCallback(this, (void(QObject::*)(void))&CHelpPage::RecolorCallback);
	RecolorCallback();
}


CHelpPage::~CHelpPage()
{

}


CHelpPage *CreateHelpPage()
{TR;
	return new CHelpPage();
}


void CHelpPage::slot_contentWidget_expandItem(const QModelIndex &index)
{
	m_contentWidget->isExpanded(index) ? m_contentWidget->collapse(index) : m_contentWidget->expand(index);
}
void CHelpPage::slot_uiEnable(bool enable)
{TR;
	m_enable_ui=enable;
	UpdateUI();
}

void CHelpPage::RecolorCallback(void)
{TR;
	ILC7ColorManager *colman = g_pLinkage->GetGUILinkage()->GetColorManager();
	int sizeratio = colman->GetSizeRatio();
	/*
	ui.BackButton->setIcon(colman->GetMonoColorIcon(":/base/back.png",
		QColor(colman->GetBaseShade("SLIDER_COLOR_0")),
		QColor(colman->GetBaseShade("SLIDER_COLOR_1")),
		QColor(colman->GetBaseShade("BUTTON_COLOR_DISABLED"))));
	ui.BackButton->setIconSize(QSize(16, 16)*sizeratio);

	ui.ForwardButton->setIcon(colman->GetMonoColorIcon(":/base/forward.png",
		QColor(colman->GetBaseShade("SLIDER_COLOR_0")),
		QColor(colman->GetBaseShade("SLIDER_COLOR_1")),
		QColor(colman->GetBaseShade("BUTTON_COLOR_DISABLED"))));
	ui.ForwardButton->setIconSize(QSize(16, 16)*sizeratio);

	ui.HomeButton->setIcon(colman->GetMonoColorIcon(":/base/home.png",
		QColor(colman->GetBaseShade("SLIDER_COLOR_0")),
		QColor(colman->GetBaseShade("SLIDER_COLOR_1")),
		QColor(colman->GetBaseShade("BUTTON_COLOR_DISABLED"))));
	ui.HomeButton->setIconSize(QSize(16, 16)*sizeratio);

	ui.SearchButton->setIcon(colman->GetMonoColorIcon(":/base/search.png",
		QColor(colman->GetBaseShade("SLIDER_COLOR_0")),
		QColor(colman->GetBaseShade("SLIDER_COLOR_1")),
		QColor(colman->GetBaseShade("BUTTON_COLOR_DISABLED"))));
	ui.SearchButton->setIconSize(QSize(16, 16)*sizeratio);
	*/
}

void CHelpPage::UpdateUI(void)
{TR;
	ui.help->setVisible(m_helpbutton->isChecked());
//	ui.SearchButton->setEnabled(!m_searchtext.isEmpty());
//	ui.ForwardButton->setEnabled(ui.HelpWebView->history()->canGoForward());
//	ui.BackButton->setEnabled(ui.HelpWebView->history()->canGoBack());
}

void CHelpPage::slot_backButton_clicked(bool toggled)
{TR;

//	ui.HelpWebView->back();

	UpdateUI();
}

void CHelpPage::slot_forwardButton_clicked(bool toggled)
{TR;

//	ui.HelpWebView->forward();

	UpdateUI();
}

void CHelpPage::slot_homeButton_clicked(bool toggled)
{TR;
//	ui.HelpWebView->setUrl(QUrl("help:///index.html"));
	UpdateUI();
}

void CHelpPage::slot_searchButton_clicked(bool toggled)
{TR;
	DoSearch(m_searchtext);

	UpdateUI();
}

void CHelpPage::slot_SearchEdit_textChanged(const QString &text)
{TR;
	m_searchtext = text;
	UpdateUI();
}

void CHelpPage::slot_HelpWebView_titleChanged(const QString &text)
{TR;
	ui.TitleEdit->setText(text);
	UpdateUI();
}

void CHelpPage::slot_HelpWebView_loadFinished(bool ok)
{TR;
	if (ok && m_first_page)
	{
//		ui.HelpWebView->history()->clear();
		m_first_page = false;
	}

	UpdateUI();
}

void CHelpPage::slot_helpButtonClicked()
{
	TR;
	UpdateUI();
}


void CHelpPage::DoSearch(QString query)
{TR;
/*
	QString path = g_pLinkage->GetStartupDirectory() + "/help";
	
	QStringList slNameFilters;
	slNameFilters << "*.html";	
	QDirIterator iter(path, slNameFilters, QDir::Files, QDirIterator::Subdirectories);

	QMap<int, QString> matches;

	QStringList words = query.split(QRegExp("\\s+"));

	while (iter.hasNext())
	{
		QString item = iter.next();

		QFile f(item);
		if (!f.open(QIODevice::ReadOnly))
		{
			continue;
		}
		QString text = QString::fromUtf8(f.readAll());
			
		int cnt = 0;
		foreach(QString word, words)
		{
			cnt += text.count(word, Qt::CaseInsensitive);
		}

		if (cnt > 0)
		{
			matches.insertMulti(cnt, item);
		}
	}
	
*/


}
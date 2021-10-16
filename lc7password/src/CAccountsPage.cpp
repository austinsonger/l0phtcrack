#include<stdafx.h>

CAccountsPage::CAccountsPage(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	m_enable_ui=true;

	ILC7ColorManager *colman = g_pLinkage->GetGUILinkage()->GetColorManager();
	int sizeratio = colman->GetSizeRatio();

	m_previous_sort = -1;

	// Set empty model to start
	ui.accountsTableView->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui.accountsTableView, &QTableView::customContextMenuRequested, this, &CAccountsPage::slot_accountsListContextMenu);

	UpdateAccountList(NULL);

	// Hook up to account list notifications
	g_pLinkage->RegisterNotifySessionActivity(ACCOUNTLIST_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CAccountsPage::NotifySessionActivity);
	g_pLinkage->RegisterNotifySessionActivity(SINGLE_WORKQUEUE_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CAuditPage::NotifySessionActivity);

	// Reshape table view
	QHeaderView *horizontalHeader = ui.accountsTableView->horizontalHeader();
	horizontalHeader->setSectionResizeMode(QHeaderView::ResizeToContents);
	horizontalHeader->setMaximumSectionSize(640 * sizeratio);
	horizontalHeader->setResizeContentsPrecision(30);

	//horizontalHeader->setMinimumSectionSize(90 * g_pLinkage->GetGUILinkage()->GetColorManager()->GetSizeRatio());

	QHeaderView *verticalHeader = ui.accountsTableView->verticalHeader();
	verticalHeader->setSectionResizeMode(QHeaderView::Fixed);
	verticalHeader->setDefaultSectionSize(24 * sizeratio);
	verticalHeader->setVisible(true);

	// Set up corner button

	QList<QAbstractButton *> buttonlist = ui.accountsTableView->findChildren<QAbstractButton *>();
	QAbstractButton *cornerbutton = buttonlist.first();
	
	ui.toolsMenuButton->setParent(NULL);
	ui.toolsMenuButton->setObjectName("toolsMenuButton");
	ui.toolsMenuButton->setIcon(colman->GetMonoColorIcon(":/qss_icons/rc/down_arrow.png",
		QColor(colman->GetTextColor()), QColor(colman->GetHighlightColor()), QColor(colman->GetInverseTextColor())));
	ui.toolsMenuButton->setIconSize(QSize(9, 6)*sizeratio);
	ui.toolsMenuButton->setStyleSheet("left: 0px; top: 0px; margin: 0px; padding: 0px;");
	//ui.toolsMenuButton->setFixedSize(QSize(22, 22)*sizeratio);
	ui.toolsMenuButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

//	ui.accountsTableView->layout()->replaceWidget(cornerbutton, ui.toolsMenuButton);
//	ui.gridLayout_2->addWidget(ui.toolsMenuButton,0,0,1,1,Qt::AlignLeft|Qt::AlignTop);

	QLayout *layout = new QGridLayout();
	layout->setMargin(0);
	layout->setContentsMargins(0,0,0,0);
	cornerbutton->setLayout(layout);
	cornerbutton->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(ui.toolsMenuButton);
	
	connect(ui.toolsMenuButton,SIGNAL(clicked()),this,SLOT(slot_toolsMenuButtonPopup()));
	
	// Set up right click
 	m_helpbutton=g_pLinkage->GetGUILinkage()->GetHelpButton();
	connect(m_helpbutton, &QAbstractButton::clicked, this, &CAccountsPage::slot_helpButtonClicked);
	
	connect(ui.totalCountLabel, &QLabel::linkActivated, this, &CAccountsPage::slot_linkActivated);
	connect(ui.crackedLabel, &QLabel::linkActivated, this, &CAccountsPage::slot_linkActivated);
	connect(ui.partiallyCrackedLabel, &QLabel::linkActivated, this, &CAccountsPage::slot_linkActivated);
	connect(ui.lockedOutLabel, &QLabel::linkActivated, this, &CAccountsPage::slot_linkActivated);
	connect(ui.disabledLabel, &QLabel::linkActivated, this, &CAccountsPage::slot_linkActivated);
	connect(ui.expiredLabel, &QLabel::linkActivated, this, &CAccountsPage::slot_linkActivated);
	connect(ui.nonExpiringLabel, &QLabel::linkActivated, this, &CAccountsPage::slot_linkActivated);

	m_colman = g_pLinkage->GetGUILinkage()->GetColorManager();
	colman->RegisterRecolorCallback(this, (void (QObject::*)(void))&CAccountsPage::slot_recolorCallback);
	slot_recolorCallback();

	UpdateTableHeaders();

	m_selected_count = 0;
	
	m_current_status_rows = 1;

	UpdateStatusBar();

	UpdateUI();
}


CAccountsPage::~CAccountsPage()
{TR;
	g_pLinkage->UnregisterNotifySessionActivity(ACCOUNTLIST_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CAccountsPage::NotifySessionActivity);
	g_pLinkage->UnregisterNotifySessionActivity(SINGLE_WORKQUEUE_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CAuditPage::NotifySessionActivity);
}

void CAccountsPage::slot_helpButtonClicked()
{TR;
	UpdateUI();
}

static QString restyleLink(QString linktext, QString style)
{
	// Remove style
	linktext.remove(QRegExp("\\s*style\\s*=\\s*\"[^\"]*\""));
	linktext.replace(QRegExp("<a\\s*"), QString("<a style=\"%1\" ").arg(style));
	return linktext;
}

void CAccountsPage::slot_recolorCallback()
{	

	QString style;
	
	if (isEnabled())
	{
		style = QString("color: %1").arg(m_colman->GetTextColor());
	}
	else
	{
		style = QString("color: %1").arg(m_colman->GetBaseShade("TEXT_DISABLED"));
	}

	ui.totalCountLabel->setText(restyleLink(ui.totalCountLabel->text(),style));
	ui.crackedLabel->setText(restyleLink(ui.crackedLabel->text(), style));
	ui.partiallyCrackedLabel->setText(restyleLink(ui.partiallyCrackedLabel->text(), style));
	ui.lockedOutLabel->setText(restyleLink(ui.lockedOutLabel->text(), style));
	ui.disabledLabel->setText(restyleLink(ui.disabledLabel->text(), style));
	ui.expiredLabel->setText(restyleLink(ui.expiredLabel->text(), style));
	ui.nonExpiringLabel->setText(restyleLink(ui.nonExpiringLabel->text(), style));
}

void CAccountsPage::slot_selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{TR;
	SortedAccountsListModel *sortedmodel = (SortedAccountsListModel *)ui.accountsTableView->model();
	QItemSelection sel = ui.accountsTableView->selectionModel()->selection();

	std::set<int> selset;
	foreach(QItemSelectionRange isr, sel)
	{
		int first = isr.top();
		int last = isr.bottom();
		
		for (int n = first; n <= last; n++)
		{
			selset.insert(n);
		}
	}

	m_selected.clear();
	foreach(int selrow, selset)
	{
		int acct = sortedmodel->mapToSource(sortedmodel->index(selrow, 0)).row();
		m_selected.append(acct);
	}
	m_selected_count = m_selected.size();

	UpdateStatusBar();

}

void CAccountsPage::showEvent(QShowEvent *evt)
{TR;
	QWidget::showEvent(evt);
}

void CAccountsPage::hideEvent(QHideEvent *evt)
{TR;
	QWidget::hideEvent(evt);
}

void CAccountsPage::keyPressEvent(QKeyEvent *event)
{TR;
	if(event->type()==QKeyEvent::KeyPress && event->matches(QKeySequence::Copy))
	{
		slot_copyToClipboard(false);
	}
	if(event->type() == QKeyEvent::KeyPress && event->matches(QKeySequence::Delete) && m_enable_ui)
	{
		slot_removeAccounts(false);
	}
}

void CAccountsPage::resizeEvent(QResizeEvent * event)
{TR;
	ILC7ColorManager *colman = g_pLinkage->GetGUILinkage()->GetColorManager();
	int sizeratio = colman->GetSizeRatio();

	QSize sz = size();

	if (sz.width() < (848 * sizeratio))
	{
		set_status_rows(2);
	}
	else
	{
		set_status_rows(1);
	}
}



 
void CAccountsPage::slot_accountsListContextMenu(const QPoint &pos)
{TR;
	QAbstractItemModel * model = ui.accountsTableView->model();
	if (!model)
	{
		return;
	}
	if (m_accountlist->GetAccountCount() == 0)
	{
		return;
	}

	// Build context menu
	QMenu menu;

	QAction *copy = new QAction("&Copy To Clipboard", &menu);
	connect(copy, &QAction::triggered, this, &CAccountsPage::slot_copyToClipboard);
	menu.addAction(copy);

	QAction *remove = new QAction("&Remove Accounts", &menu);
	connect(remove, &QAction::triggered, this, &CAccountsPage::slot_removeAccounts);
	menu.addAction(remove);
	remove->setEnabled(m_enable_ui);

	// Add remediations
	if (m_selected.isEmpty())
	{
		return;
	}

	QLabel *label1 = new QLabel("Available Remediations", &menu);
	label1->setAlignment(Qt::AlignLeft);
	//	label1->setStyleSheet(QString("color: %1; border-top: 1px solid %1;").arg(m_colman->GetBaseShade("TEXT_DISABLED")).arg(m_colman->GetBaseShade("BORDER_COLOR")));
	label1->setStyleSheet(QString("color: %1; background-color: %2; border-bottom: 2px solid %3;")
		.arg(m_colman->GetTextColor())
		.arg(m_colman->GetBaseShade("TOOLTIP_BKGD"))
		.arg(m_colman->GetBaseShade("WIDGET_BKGD"))
		);
	QWidgetAction *act = new QWidgetAction(label1);
	label1->setMargin(4);
	act->setDefaultWidget(label1);

	menu.addAction(act);


	std::set<qint32> remnums;

	QList<QAction *> remactions;
	
	m_accountlist->Acquire();
	bool no_remediations = false;
	foreach(int acct, m_selected)
	{
		const LC7Account *lc7acct = m_accountlist->GetAccountAtConstPtrFast(acct);
		if (lc7acct->remediations != -1)
		{
			remnums.insert(lc7acct->remediations);
		}
		else
		{
			// Since one account has no remediations, we must disable them all
			no_remediations = true;
		}
	}

	// Build intersection set of remediation actions
	// Same displayed text is the same action for this purpose
	bool first = true;
	foreach(int remnum, remnums)
	{
		LC7Remediations rems;
		if (!m_accountlist->GetRemediations(remnum, rems))
		{
			continue;
		}

		if (first)
		{
			// Just add the remediation to the empty action set, enabled
			foreach(const LC7Remediation & rem, rems)
			{
				QAction *remaction = new QAction(rem.description, &menu);
				remactions.append(remaction);
			}

			first = false;
		}
		else
		{
			// If any action is not in this remediation set, disable it
			foreach(QAction *remaction, remactions)
			{
				if (!remaction->isEnabled())
				{
					continue;
				}
				bool found = false;
				foreach(const LC7Remediation & rem, rems)
				{
					if (rem.description == remaction->text())
					{
						found = true;
						break;
					}
				}
				if (!found)
				{
					remaction->setEnabled(false);
				}
			}

			// If any remediation is not in the action set, add it, disabled
			foreach(const LC7Remediation & rem, rems)
			{
				bool found = false;
				foreach(QAction * remaction, remactions)
				{
					if (rem.description == remaction->text())
					{
						found = true;
						break;
					}
				}
				if (!found)
				{
					QAction *remaction = new QAction(rem.description, &menu);
					remactions.append(remaction);
					remaction->setEnabled(false);
				}
			}
		}
	}

	// Disable all remediations if necessary
	if (no_remediations || !m_enable_ui)
	{
		foreach(QAction *remaction, remactions)
		{
			remaction->setEnabled(false);
		}
	}

	// If we have any remediation actions to add to the menu, add them
	foreach(QAction *remaction, remactions)
	{
		menu.addAction(remaction);
		connect(remaction, &QAction::triggered, this, &CAccountsPage::slot_remediationAction);
	}

	m_accountlist->Release();

	menu.exec(ui.accountsTableView->viewport()->mapToGlobal(pos));
}

void CAccountsPage::slot_remediationAction(void)
{
	if (!m_enable_ui)
	{
		return;
	}

	QAction *remaction = (QAction *)sender();

	// Tally up all the remediations we need to perform
	// and which accounts get which remediations

	QModelIndexList mapped_indexes;

	QMap<qint32, QList<int>> accounts_by_remediation;
	m_accountlist->Acquire();
	foreach(int acct, m_selected)
	{
		const LC7Account *lc7acct = m_accountlist->GetAccountAtConstPtrFast(acct);
		if (lc7acct->remediations == -1)
		{
			// This should have been made impossible by the context menu code
			Q_ASSERT(0);
			m_accountlist->Release();
			return;
		}

		accounts_by_remediation[lc7acct->remediations].append(acct);
	}

	// Start a new work queue
	m_single_workqueue->ResetWorkQueueState();
	m_single_workqueue->ClearWorkQueue();

	// Now, for each remediation, pass it the list of accounts to affect
	foreach(qint32 remnum, accounts_by_remediation.keys())
	{
		LC7Remediations remediations;
		if (!m_accountlist->GetRemediations(remnum, remediations))
		{
			Q_ASSERT(0);
			m_accountlist->Release();
			return;
		}

		bool found = false;
		LC7Remediation remediation; 
		foreach(remediation, remediations)
		{
			if (remediation.description == remaction->text())
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			Q_ASSERT(0);
			m_accountlist->Release();
			return;
		}
	
		QList<QVariant> accounts_to_remediate;
		foreach(int acct, accounts_by_remediation[remnum])
		{
			accounts_to_remediate.append(acct);
		}
				
		ILC7Component *comp = g_pLinkage->FindComponentByID(remediation.component);
		QMap<QString, QVariant> wqiconfig(remediation.config);
		wqiconfig["accounts_to_remediate"] = accounts_to_remediate;
		LC7WorkQueueItem wqi(remediation.component, remediation.command, QStringList(), wqiconfig, remediation.description, true, false);
		
		m_single_workqueue->AppendWorkQueueItem(wqi);

	}

	m_accountlist->Release();

	// Execute work queue
	QString error;
	int failed_item;
	if (!m_single_workqueue->Validate(error, failed_item))
	{
		g_pLinkage->GetGUILinkage()->ErrorMessage("Unable to perform remediation", QString("Remedation actions can not be validated: %1").arg(error));
		return;
	}
	m_single_workqueue->StartRequest();
}


void CAccountsPage::slot_copyToClipboard(bool checked)
{TR;
	QAbstractItemModel * model = ui.accountsTableView->model();
	if(!model)
	{
		return;
	}
	QItemSelectionModel * selection = ui.accountsTableView->selectionModel();
	if(!selection)
	{
		return;
	}
	QModelIndexList indexes = selection->selectedIndexes();
	if(indexes.isEmpty())
	{
		return;
	}
	
	// QModelIndex::operator < sorts first by row, then by column. 
	// this is what we need
	qSort(indexes.begin(), indexes.end());

	// You need a pair of indexes to find the row changes
	QString selected_text;

	QModelIndex previous = indexes.first();
	indexes.removeFirst();
	if (indexes.isEmpty())
	{
		selected_text.append(model->data(previous, Qt::AccessibleTextRole).toString());
		selected_text.append(QLatin1Char('\n'));
	}
	else
	{
		QModelIndex current;
		foreach(current, indexes)
		{
			QVariant data = model->data(previous, Qt::AccessibleTextRole);
			QString text = data.toString();
			// At this point `text` contains the text in one cell
			selected_text.append(text);
			// If you are at the start of the row the row number of the previous index
			// isn't the same.  Text is followed by a row separator, which is a newline.
			if (current.row() != previous.row())
			{
				selected_text.append(QLatin1Char('\n'));
			}
			// Otherwise it's the same row, so append a column separator, which is a tab.
			else
			{
				selected_text.append(QLatin1Char('\t'));
			}
			previous = current;
		}

		// add last element
		selected_text.append(model->data(current, Qt::AccessibleTextRole).toString());
		selected_text.append(QLatin1Char('\n'));
	}
  
	qApp->clipboard()->setText(selected_text);
}

template<class T> 
void DeleteBackground(T * obj)
{
	class Deleter :public QThread
	{
	private:
		T * m_obj;
	public:
		Deleter(T* obj) : m_obj(obj) {}
		void run(void)
		{
			setPriority(QThread::IdlePriority);

			delete m_obj;
		}
	};
	Deleter *t = new Deleter(obj);
	QObject::connect(t, &Deleter::finished, t, &Deleter::deleteLater, Qt::QueuedConnection);
	t->start();
}

void fastMapSelectionToSource(QSortFilterProxyModel *proxymodel, const QModelIndexList &proxySelectedIndexes, QModelIndexList & sourceSelectedIndexes)
{
	for (int i = 0; i < proxySelectedIndexes.size(); ++i) {
		const QModelIndex proxyIdx = proxymodel->mapToSource(proxySelectedIndexes.at(i));
		if (!proxyIdx.isValid())
			continue;
		sourceSelectedIndexes.append(proxyIdx);
	}
}

void CAccountsPage::slot_removeAccounts(bool checked)
{TR;
	SortedAccountsListModel *sortedmodel = (SortedAccountsListModel *)ui.accountsTableView->model();
	if (!sortedmodel)
	{
		return;
	}
	AccountsListModel * model = (AccountsListModel *)sortedmodel->sourceModel();

	QModelIndexList *selectedIndexes = new QModelIndexList();
	*selectedIndexes = ui.accountsTableView->selectionModel()->selectedIndexes();
	if (selectedIndexes->isEmpty())
	{
		delete selectedIndexes;
		return;
	}

	QModelIndexList *sourceSelectedIndexes = new QModelIndexList();
	fastMapSelectionToSource(sortedmodel, *selectedIndexes, *sourceSelectedIndexes);
	
	ui.accountsTableView->setUpdatesEnabled(false);
	m_accountlist->Acquire();

	// QModelIndex::operator < sorts first by row, then by column. 
	// this is what we need
	qSort(sourceSelectedIndexes->begin(), sourceSelectedIndexes->end());
	QModelIndex current;
	std::set<int> positions;
	foreach(current, *sourceSelectedIndexes)
	{
		int currentrow=current.row();
		positions.insert(currentrow);
	}

	ILC7ProgressBox *box = NULL;
	if (positions.size() > 50)
	{
		box = g_pLinkage->GetGUILinkage()->CreateProgressBox("Removing Accounts",
			QString("Removing %1 accounts").arg((int)positions.size()),
			0, (int)positions.size(), false);

		QCoreApplication::processEvents();

		model->setProgressBox(box);
	}
	
	m_accountlist->RemoveAccounts(positions);

	m_accountlist->Release();
	ui.accountsTableView->setUpdatesEnabled(true);

	if (box)
	{
		box->UpdateStatusText("Cleaning up");
		QCoreApplication::processEvents();
	}

	delete sourceSelectedIndexes; //DeleteBackground(sourceSelectedIndexes);
	delete selectedIndexes; //DeleteBackground(selectedIndexes);

	if (box)
	{
		model->setProgressBox(NULL);
		box->Release();
		QCoreApplication::processEvents();
	}
}





void CAccountsPage::NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler)
{TR;
	switch(activity)
	{
	case ILC7Linkage::SESSION_OPEN_POST:
	case ILC7Linkage::SESSION_NEW_POST:

		if (handler && handler->GetId() == SINGLE_WORKQUEUE_HANDLER_ID)
		{
			m_single_workqueue = (ILC7WorkQueue *)handler;
		}
		if (handler && handler->GetId() == ACCOUNTLIST_HANDLER_ID)
		{
			UpdateAccountList((CLC7AccountList *)handler);

			UpdateTableHeaders();
			UpdateStatusBar();
			UpdateUI();
		}

		UpdateUI();

		break;

	case ILC7Linkage::SESSION_CLOSE_PRE:
		
		if (handler && handler->GetId() == SINGLE_WORKQUEUE_HANDLER_ID)
		{
			m_single_workqueue = NULL;
		}
		if (handler && handler->GetId() == ACCOUNTLIST_HANDLER_ID)
		{
			UpdateAccountList(NULL);

			UpdateTableHeaders();
			UpdateStatusBar();
			UpdateUI();
		}

		UpdateUI();

		break;
	}
}

void CAccountsPage::slot_uiEnable(bool enable)
{TR;
	m_enable_ui=enable;
	UpdateUI();
}


void CAccountsPage::setSorting(int column, Qt::SortOrder order)
{
	if (ui.accountsTableView->horizontalHeader()->sortIndicatorOrder() == Qt::AscendingOrder && 
		ui.accountsTableView->horizontalHeader()->isSortIndicatorShown() && 
		m_previous_sort == column)
	{
		ui.accountsTableView->horizontalHeader()->setSortIndicator(column, Qt::DescendingOrder);
		ui.accountsTableView->horizontalHeader()->setSortIndicatorShown(false);
		column = -1;
	}
	else
	{
		ui.accountsTableView->horizontalHeader()->setSortIndicatorShown(true);
	}
	m_previous_sort = column;
	emit sig_doSort(column, order);
}

void CAccountsPage::UpdateUI()
{TR;
	ui.toolsMenuButton->setEnabled(ui.accountsTableView->model()->columnCount() > 0);
	ui.help->setVisible(m_helpbutton->isChecked());

	setEnabled(m_accountlist != NULL);
		
	slot_recolorCallback();
}

void CAccountsPage::UpdateTableHeaders()
{TR;
	// Initialize settings
	ILC7Settings *settings = g_pLinkage->GetSettings();
	for (int col = 0; col<ui.accountsTableView->model()->columnCount(); col++)
	{
		QString colname = ui.accountsTableView->model()->headerData(col, Qt::Horizontal, Qt::UserRole).toString();
		QString name = QString(UUID_PASSWORDGUIPLUGIN.toString() + ":show_column_%1").arg(colname);
		bool s = settings->value(name, QVariant(true)).toBool();
		settings->setValue(name, s);
	}
	
	for(int col=0;col<ui.accountsTableView->model()->columnCount();col++)
	{
		QString colname = ui.accountsTableView->model()->headerData(col, Qt::Horizontal, Qt::UserRole).toString();

		QString name=QString(UUID_PASSWORDGUIPLUGIN.toString()+":show_column_%1").arg(colname);
		bool s=settings->value(name,QVariant(true)).toBool();

		if(s)
		{
			ui.accountsTableView->horizontalHeader()->showSection(col);
		}
		else
		{
			ui.accountsTableView->horizontalHeader()->hideSection(col);
		}
	}

	// Header width calculation parameter
	/*
	QHeaderView *horizontalHeader = ui.accountsTableView->horizontalHeader();
	if (m_accountlist->GetAccountCount()>=500 && horizontalHeader->resizeContentsPrecision()!=50)
	{
		horizontalHeader->setResizeContentsPrecision(30);
	}
	else
	{
		horizontalHeader->setResizeContentsPrecision(-1);
	}
	*/
}


void CAccountsPage::slot_toolsMenuButtonPopup(void)
{  
	if (ui.accountsTableView->model()->columnCount() == 0)
	{
		return;
	}
	
	ILC7Settings *settings=g_pLinkage->GetSettings();
	QPoint pt = ui.toolsMenuButton->mapToGlobal(QPoint(0,0));
	
	QMenu *menu=new QMenu(this);
//	QAction *resetact = menu->addAction("Reset Sorting");
//	QObject::connect(resetact, &QAction::triggered, this, &CAccountsPage::slot_resetSortingClicked);
		
	QLabel *label1 = new QLabel("Show/Hide Columns", menu);
	label1->setAlignment(Qt::AlignCenter);
//	label1->setStyleSheet(QString("color: %1; border-top: 1px solid %1;").arg(m_colman->GetBaseShade("TEXT_DISABLED")).arg(m_colman->GetBaseShade("BORDER_COLOR")));
	label1->setStyleSheet(QString("color: %1; background-color: %2; border-bottom: 2px solid %3;")
		.arg(m_colman->GetTextColor())
		.arg(m_colman->GetBaseShade("TOOLTIP_BKGD"))
		.arg(m_colman->GetBaseShade("WIDGET_BKGD"))
		);
	QWidgetAction *act = new QWidgetAction(label1);
	label1->setMargin(4);
	act->setDefaultWidget(label1);

	menu->addAction(act);
	
	for (int col = 0; col<ui.accountsTableView->model()->columnCount(); col++)
	{
		//QString header = ui.accountsTableView->model()->headerData(col,Qt::Horizontal,Qt::DisplayRole).toString();
		QString header = ui.accountsTableView->model()->headerData(col, Qt::Horizontal, Qt::UserRole).toString();

		QCheckBox *checkBox = new QCheckBox(header,menu);
		QWidgetAction *act = new QWidgetAction(menu);
		act->setDefaultWidget(checkBox);
		checkBox->setProperty("column",col);

		QString colname = ui.accountsTableView->model()->headerData(col, Qt::Horizontal, Qt::UserRole).toString();

		checkBox->setChecked(settings->value(QString(UUID_PASSWORDGUIPLUGIN.toString() + ":show_column_%1").arg(colname), QVariant(true)).toBool());
		QObject::connect(checkBox,SIGNAL(toggled(bool)),this,SLOT(slot_toolsMenuToggled(bool)));

		menu->addAction(act);
	}

    menu->popup(pt);
}


void CAccountsPage::slot_resetSortingClicked(bool checked)
{
	SortedAccountsListModel *sortedmodel = new SortedAccountsListModel(ui.accountsTableView);
}


void CAccountsPage::slot_toolsMenuToggled(bool)
{TR;
	QCheckBox *checkBox = (QCheckBox *)sender();
	int col=checkBox->property("column").toInt();
		
	ILC7Settings *settings=g_pLinkage->GetSettings();
	QString colname = ui.accountsTableView->model()->headerData(col, Qt::Horizontal, Qt::UserRole).toString();

	QString name=QString(UUID_PASSWORDGUIPLUGIN.toString()+":show_column_%1").arg(colname);
	bool s=settings->value(name,QVariant(true)).toBool();
	s=!s;
	settings->setValue(name,s);
	
	UpdateTableHeaders();
}

void CAccountsPage::slot_changeNotify(void)
{TR;
	UpdateTableHeaders();
	UpdateStatusBar();
	UpdateUI();
}

void CAccountsPage::UpdateStatusBar(void)
{TR;
	if (!m_accountlist)
	{
		ui.totalCountEdit->setText("");
		ui.crackedEdit->setText("");
		ui.partiallyCrackedEdit->setText("");
		ui.selectedEdit->setText("");
		ui.lockedOutEdit->setText("");
		ui.disabledEdit->setText("");
		ui.expiredEdit->setText("");
		ui.nonExpiringEdit->setText("");
		return;
	}

	ILC7AccountList::STATS stats = m_accountlist->GetStats();

	ui.totalCountEdit->setText(QString("%1").arg(stats.total));
	ui.crackedEdit->setText(QString("%1").arg(stats.cracked));
	ui.partiallyCrackedEdit->setText(QString("%1").arg(stats.partially_cracked));
	ui.selectedEdit->setText(QString("%1").arg(m_selected_count));
	ui.lockedOutEdit->setText(QString("%1").arg(stats.locked_out));
	ui.disabledEdit->setText(QString("%1").arg(stats.disabled));
	ui.expiredEdit->setText(QString("%1").arg(stats.must_change));
	ui.nonExpiringEdit->setText(QString("%1").arg(stats.non_expiring));
}

void CAccountsPage::UpdateAccountList(CLC7AccountList *acctlist)
{TR;
	m_accountlist = acctlist;

	// Clear the model
	ui.accountsTableView->setModel(NULL);
	
	SortedAccountsListModel *sortedmodel = new SortedAccountsListModel(ui.accountsTableView);
	AccountsListModel *acctmodel = new AccountsListModel(sortedmodel, m_accountlist);

	sortedmodel->setSourceModel(acctmodel);

	// Update the ui when the model changes
	connect(acctmodel, &AccountsListModel::sig_modelchanged, this, &CAccountsPage::slot_changeNotify, Qt::QueuedConnection);

	// Enable sorting only when we're not making changes to the model for speed
	connect(acctmodel, &AccountsListModel::sig_enable_sorting, sortedmodel, &SortedAccountsListModel::slot_enable_sorting, Qt::DirectConnection);

	// Enable tri-state sorting by outsourcing the whole sort thing
	connect(this, &CAccountsPage::sig_doSort, sortedmodel, &SortedAccountsListModel::doSort);
	connect(sortedmodel, &SortedAccountsListModel::askOrder, this, &CAccountsPage::setSorting);

	ui.accountsTableView->setModel(sortedmodel);

	connect(ui.accountsTableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CAccountsPage::slot_selectionChanged);
}

static bool is_cracked(const LC7Account *acct)
{
	foreach(const LC7Hash &hash, acct->hashes)
	{
		if (hash.crackstate != CRACKSTATE_CRACKED)
		{
			return false;
		}
	}
	return true;
}

static bool is_partially_cracked(const LC7Account *acct)
{
	int is_cracked = 0;
	int is_not_cracked = 0;
	foreach(const LC7Hash &hash, acct->hashes)
	{
		if (hash.crackstate == CRACKSTATE_CRACKED)
		{
			is_cracked++; 
		}
		else if (hash.crackstate == CRACKSTATE_NOT_CRACKED)
		{
			is_not_cracked++;
		}
		else
		{
			return true;
		}
	}
	if (is_cracked > 0 && is_not_cracked > 0)
	{
		return true;
	}
	return false;
}

static bool is_locked_out(const LC7Account *acct)
{
	return acct->lockedout;
}

static bool is_disabled(const LC7Account *acct)
{
	return acct->disabled;
}

static bool is_expired(const LC7Account *acct)
{
	return acct->mustchange;
}

static bool is_non_expiring(const LC7Account *acct)
{
	return acct->neverexpires;
}



void CAccountsPage::buildSelection(SELECTION_FILTER *filter, QItemSelection & sel)
{	
	SortedAccountsListModel *model = (SortedAccountsListModel *)ui.accountsTableView->model();

	m_accountlist->Acquire();
	int lastcolumn = model->columnCount() - 1;

	for (int row = 0; row < model->rowCount(); row++)
	{
		int acctnum = model->mapToSource(model->index(row, 0)).row();
		
		const LC7Account *acct = m_accountlist->GetAccountAtConstPtrFast(acctnum);
		if (acct == NULL)
		{
			continue;
		}

		if (filter(acct))
		{
			sel.select(model->index(row, 0), model->index(row, lastcolumn));
		}
	}
	
	m_accountlist->Release();
}

void CAccountsPage::slot_linkActivated(const QString & link)
{
	QItemSelection sel;

	if (link == "#all_accounts")
	{
		QAbstractItemModel *model = ui.accountsTableView->model();
		sel.append(QItemSelectionRange(model->index(0, 0), model->index(model->rowCount() - 1, model->columnCount() - 1)));
	}
	else if (link == "#cracked")
	{
		buildSelection(is_cracked, sel);
	}
	else if (link == "#partially_cracked")
	{
		buildSelection(is_partially_cracked, sel);
	}
	else if (link == "#locked_out")
	{
		buildSelection(is_locked_out, sel);
	}
	else if (link == "#disabled")
	{
		buildSelection(is_disabled, sel);
	}
	else if (link == "#expired")
	{
		buildSelection(is_expired, sel);
	}
	else if (link == "#non_expiring")
	{
		buildSelection(is_non_expiring, sel);
	}
	else
	{
		Q_ASSERT(0);
	}

	ui.accountsTableView->selectionModel()->select(sel,QItemSelectionModel::ClearAndSelect);
}

void CAccountsPage::set_status_rows(int rows)
{
	if (rows == 1 && m_current_status_rows==2)
	{
		ui.statusLayout->removeWidget(ui.totalCountLabel);
		ui.statusLayout->removeWidget(ui.totalCountEdit);
		ui.statusLayout->removeWidget(ui.crackedLabel);
		ui.statusLayout->removeWidget(ui.crackedEdit);
		ui.statusLayout->removeWidget(ui.partiallyCrackedLabel);
		ui.statusLayout->removeWidget(ui.partiallyCrackedEdit);
		ui.statusLayout->removeWidget(ui.selectedLabel);
		ui.statusLayout->removeWidget(ui.selectedEdit);
		ui.statusLayout->removeWidget(ui.lockedOutLabel);
		ui.statusLayout->removeWidget(ui.lockedOutEdit);
		ui.statusLayout->removeWidget(ui.disabledLabel);
		ui.statusLayout->removeWidget(ui.disabledEdit);
		ui.statusLayout->removeWidget(ui.expiredLabel);
		ui.statusLayout->removeWidget(ui.expiredEdit);
		ui.statusLayout->removeWidget(ui.nonExpiringLabel);
		ui.statusLayout->removeWidget(ui.nonExpiringEdit);
				
		ui.statusLayout->addWidget(ui.totalCountLabel,0, 0);
		ui.statusLayout->addWidget(ui.totalCountEdit, 0, 1);
		ui.statusLayout->addWidget(ui.crackedLabel, 0, 2);
		ui.statusLayout->addWidget(ui.crackedEdit, 0, 3);
		ui.statusLayout->addWidget(ui.partiallyCrackedLabel, 0, 4);
		ui.statusLayout->addWidget(ui.partiallyCrackedEdit, 0, 5);
		ui.statusLayout->addWidget(ui.selectedLabel, 0, 6);
		ui.statusLayout->addWidget(ui.selectedEdit, 0, 7);
		ui.statusLayout->addWidget(ui.lockedOutLabel, 0, 8);
		ui.statusLayout->addWidget(ui.lockedOutEdit, 0, 9);
		ui.statusLayout->addWidget(ui.disabledLabel, 0, 10);
		ui.statusLayout->addWidget(ui.disabledEdit, 0, 11);
		ui.statusLayout->addWidget(ui.expiredLabel, 0, 12);
		ui.statusLayout->addWidget(ui.expiredEdit, 0, 13);
		ui.statusLayout->addWidget(ui.nonExpiringLabel, 0, 14);
		ui.statusLayout->addWidget(ui.nonExpiringEdit, 0, 15);

		m_current_status_rows = 1;
	}
	else if (rows == 2 && m_current_status_rows == 1)
	{
		ui.statusLayout->removeWidget(ui.totalCountLabel);
		ui.statusLayout->removeWidget(ui.totalCountEdit);
		ui.statusLayout->removeWidget(ui.crackedLabel);
		ui.statusLayout->removeWidget(ui.crackedEdit);
		ui.statusLayout->removeWidget(ui.partiallyCrackedLabel);
		ui.statusLayout->removeWidget(ui.partiallyCrackedEdit);
		ui.statusLayout->removeWidget(ui.selectedLabel);
		ui.statusLayout->removeWidget(ui.selectedEdit);
		ui.statusLayout->removeWidget(ui.lockedOutLabel);
		ui.statusLayout->removeWidget(ui.lockedOutEdit);
		ui.statusLayout->removeWidget(ui.disabledLabel);
		ui.statusLayout->removeWidget(ui.disabledEdit);
		ui.statusLayout->removeWidget(ui.expiredLabel);
		ui.statusLayout->removeWidget(ui.expiredEdit);
		ui.statusLayout->removeWidget(ui.nonExpiringLabel);
		ui.statusLayout->removeWidget(ui.nonExpiringEdit);

		ui.statusLayout->addWidget(ui.totalCountLabel, 0, 0);
		ui.statusLayout->addWidget(ui.totalCountEdit, 0, 1);
		ui.statusLayout->addWidget(ui.crackedLabel, 0, 2);
		ui.statusLayout->addWidget(ui.crackedEdit, 0, 3);
		ui.statusLayout->addWidget(ui.partiallyCrackedLabel, 0, 4);
		ui.statusLayout->addWidget(ui.partiallyCrackedEdit, 0, 5);
		ui.statusLayout->addWidget(ui.selectedLabel, 0, 6);
		ui.statusLayout->addWidget(ui.selectedEdit, 0, 7);
		ui.statusLayout->addWidget(ui.lockedOutLabel, 1, 0);
		ui.statusLayout->addWidget(ui.lockedOutEdit, 1, 1);
		ui.statusLayout->addWidget(ui.disabledLabel, 1, 2);
		ui.statusLayout->addWidget(ui.disabledEdit, 1, 3);
		ui.statusLayout->addWidget(ui.expiredLabel, 1, 4);
		ui.statusLayout->addWidget(ui.expiredEdit, 1, 5);
		ui.statusLayout->addWidget(ui.nonExpiringLabel, 1, 6);
		ui.statusLayout->addWidget(ui.nonExpiringEdit, 1, 7);

		m_current_status_rows = 2;
	}
}


CAccountsPage *CreateAccountsPage()
{TR;
	return new CAccountsPage();
}




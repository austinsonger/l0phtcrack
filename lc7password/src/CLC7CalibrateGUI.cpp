#include <stdafx.h>

#define FONT_SIZE 13
#define FONTTEXT(x) QString("<span style=\"font-size:%1px\">%2</span>").arg(m_colman->GetSizeRatio() * FONT_SIZE).arg(x)
#define SMALL_FONT_SIZE 10
#define SMALLFONTTEXT(x) QString("<span style=\"font-size:%1px\">%2</span>").arg(m_colman->GetSizeRatio() * SMALL_FONT_SIZE).arg(x)

uint qHash(const QVariant & var)
{
	if (!var.isValid() || var.isNull())
		return 0;

	switch (var.type())
	{
	case QVariant::Int:
		return qHash(var.toInt());
	case QVariant::UInt:
		return qHash(var.toUInt());
	case QVariant::Bool:
		return qHash(var.toUInt());
	case QVariant::Double:
		return qHash(var.toUInt());
	case QVariant::LongLong:
		return qHash(var.toLongLong());
	case QVariant::ULongLong:
		return qHash(var.toULongLong());
	case QVariant::String:
		return qHash(var.toString());
	case QVariant::Char:
		return qHash(var.toChar());
	case QVariant::StringList:
		return qHash(var.toString());
	case QVariant::ByteArray:
		return qHash(var.toByteArray());
	case QVariant::Date:
	case QVariant::Time:
	case QVariant::DateTime:
	case QVariant::Url:
	case QVariant::Locale:
	case QVariant::RegExp:
		return qHash(var.toString());
	case QVariant::Map:
	case QVariant::List:
	case QVariant::BitArray:
	case QVariant::Size:
	case QVariant::SizeF:
	case QVariant::Rect:
	case QVariant::LineF:
	case QVariant::Line:
	case QVariant::RectF:
	case QVariant::Point:
	case QVariant::PointF:
	case QVariant::UserType:
	case QVariant::Invalid:
	default:
		Q_ASSERT(0);
		break;
	}

	// could not generate a hash for the given variant
	return -1;
}

inline bool operator==(const CLC7CalibrationDialog::TABLE_KEY &e1, const CLC7CalibrationDialog::TABLE_KEY &e2)
{
	return e1.table == e2.table && e1.colnum==e2.colnum && e1.rownum==e2.rownum;
}

inline uint qHash(const CLC7CalibrationDialog::TABLE_KEY &key, uint seed)
{
	return qHash(key.table, seed) ^ qHash(key.rownum, seed) ^ qHash(key.colnum, seed);
}

inline bool operator==(const CLC7CalibrationDialog::ID_KEY &e1, const CLC7CalibrationDialog::ID_KEY &e2)
{
	return e1.table == e2.table && e1.id == e2.id;
}

inline uint qHash(const CLC7CalibrationDialog::ID_KEY &key, uint seed)
{
	return qHash(key.table, seed) ^ qHash(key.id, seed);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CLC7CalibrationThread::CLC7CalibrationThread()
{

}

CLC7CalibrationThread::~CLC7CalibrationThread()
{

}

void CLC7CalibrationThread::run()
{
	emit started();
}

void CLC7CalibrationThread::abort()
{

}

void CLC7CalibrationThread::terminate()
{

}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


CLC7CalibrationDialog::CLC7CalibrationDialog(QWidget *parent) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
{
	ui.setupUi(this);
	m_colman = g_pLinkage->GetGUILinkage()->GetColorManager();

	QVBoxLayout *presetsvboxlayout = new QVBoxLayout();
	ui.presetsScrollArea->widget()->setLayout(presetsvboxlayout);
	ui.presetsScrollArea->widget()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	QVBoxLayout *calibrationvboxlayout = new QVBoxLayout();
	ui.calibrationTableScrollArea->widget()->setLayout(calibrationvboxlayout);
	ui.calibrationTableScrollArea->widget()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	m_passlink = GET_ILC7PASSWORDLINKAGE(g_pLinkage);
	for (auto engine : m_passlink->ListPasswordEngines())
	{
		m_engines.push_back(engine);

		// Get default sets for engine
		QList<ILC7PasswordEngine::CALIBRATION_DEFAULT_SET> sets = engine->GetCalibrationDefaultSets();
		m_engine_default_sets[engine] = sets;

		// Create tables for each engine
		ILC7CalibrationTable *table = m_passlink->LoadCalibrationTable(engine->GetCalibrationKey());
		if (!table || !tableIsCompatibleWithEngine(table, engine))
		{
			// If no table or table is incompatible, go with best default set
			if (!table)
				table = m_passlink->NewCalibrationTable();
			engine->ResetCalibrationTable(table, sets.back().id);
		}

		m_calibration_table_by_engine[engine] = table;
		m_engine_by_calibration_table[table] = engine;
	}

	for (auto engine : m_passlink->ListPasswordEngines())
	{
		AddEnginePresetsBox(engine, presetsvboxlayout);
		AddEngineCalibrationTable(engine, calibrationvboxlayout);
	}
	calibrationvboxlayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
	
	connect(ui.okButton, &QAbstractButton::clicked, this, &CLC7CalibrationDialog::accept);
	connect(ui.cancelButton, &QAbstractButton::clicked, this, &CLC7CalibrationDialog::reject);

	resize(size()*m_colman->GetSizeRatio());
	
	connect(ui.calibrateButton, &QAbstractButton::clicked, this, &CLC7CalibrationDialog::slot_calibrateButton_clicked);

	connect(this, &CLC7CalibrationDialog::sig_setProgressBarValue, ui.calibrationProgress, &QProgressBar::setValue, Qt::BlockingQueuedConnection);
	connect(this, &CLC7CalibrationDialog::sig_setProgressBarRange, ui.calibrationProgress, &QProgressBar::setRange, Qt::BlockingQueuedConnection);

	connect(&m_calibration_thread, &CLC7CalibrationThread::started, this, &CLC7CalibrationDialog::slot_calibrationThread_started, Qt::DirectConnection);

	connect(this, &CLC7CalibrationDialog::sig_refreshCell, this, &CLC7CalibrationDialog::slot_refreshCell, Qt::BlockingQueuedConnection);
	connect(this, &CLC7CalibrationDialog::sig_refreshTableWidget, this, &CLC7CalibrationDialog::slot_refreshTableWidget, Qt::BlockingQueuedConnection);

	refreshCalibration();

	UpdateUI();
}

CLC7CalibrationDialog::~CLC7CalibrationDialog()
{
}



int CLC7CalibrationDialog::TableRowIdToRow(ILC7CalibrationTable *table, QVariant rowId)
{
	
	ID_KEY key;
	key.table = table;
	key.id = rowId;
	Q_ASSERT(m_table_row_by_rowId.contains(key));
	return m_table_row_by_rowId[key];
}

int CLC7CalibrationDialog::TableColIdToCol(ILC7CalibrationTable *table, QVariant colId)
{
	ID_KEY key;
	key.table = table;
	key.id = colId;
	Q_ASSERT(m_table_col_by_colId.contains(key));
	return m_table_col_by_colId[key];
}


void CLC7CalibrationDialog::AddEnginePresetsBox(ILC7PasswordEngine *engine, QLayout *presetslayout)
{
	int engineidx = m_engines.indexOf(engine);
	ILC7CalibrationTable *table = m_calibration_table_by_engine[engine];
	
	QGroupBox *modesbox = new QGroupBox(this);
	modesbox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
	modesbox->setTitle(QString("%1 Presets").arg(engine->GetDisplayName()));
	modesbox->setLayout(new QVBoxLayout());

	QLabel *boxlabel = new QLabel(modesbox);
	boxlabel->setText(QString("%1").arg(engine->GetDescription()));
	modesbox->layout()->addWidget(boxlabel);

	QRadioButton *radiobutton;
	QMap<QUuid, int> defaultset_by_id;
	QMap<int, QRadioButton *> defaultset_buttons;
	int defaultsetidx = 0;
	for (auto & set : m_engine_default_sets[engine])
	{
		radiobutton = new QRadioButton(modesbox);
		radiobutton->setText(set.name);
		modesbox->layout()->addWidget(radiobutton);
		defaultset_by_id[set.id] = defaultsetidx;
		defaultset_buttons[defaultsetidx] = radiobutton;
		radiobutton->setProperty("defaultsetidx", defaultsetidx);
		radiobutton->setProperty("engineidx", engineidx);
		connect(radiobutton, &QAbstractButton::clicked, this, &CLC7CalibrationDialog::slot_presetClicked);
		defaultsetidx++;
	}

	// add custom radiobutton
	radiobutton = new QRadioButton(modesbox);
	radiobutton->setText("Custom");
	modesbox->layout()->addWidget(radiobutton);
	defaultset_by_id[QUuid()] = -1;
	defaultset_buttons[-1] = radiobutton;
	radiobutton->setProperty("defaultsetidx", -1);
	radiobutton->setProperty("engineidx", engineidx);
	connect(radiobutton, &QAbstractButton::clicked, this, &CLC7CalibrationDialog::slot_presetClicked);

	// Add radio button modes box to layout
	presetslayout->addWidget(modesbox);
	
	// Check the radio button for the current default set
	QUuid dsid = table->GetDefaultSetId();
	defaultsetidx = defaultset_by_id.value(dsid, -1);
	defaultset_buttons[defaultsetidx]->setChecked(true);
}


void CLC7CalibrationDialog::AddEngineCalibrationTable(ILC7PasswordEngine *engine, QLayout *calibrationlayout)
{
	ILC7PasswordLinkage * passlink = GET_ILC7PASSWORDLINKAGE(g_pLinkage);
	ILC7ColorManager * colman = g_pLinkage->GetGUILinkage()->GetColorManager();

	QLabel *lab = new QLabel(QString("%1:").arg(engine->GetDescription()));
	calibrationlayout->addWidget(lab);

	QTableWidget *tw = new QTableWidget(calibrationlayout->widget());
	tw->setObjectName("calibrationtable");
	tw->setContextMenuPolicy(Qt::CustomContextMenu);
	tw->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	tw->viewport()->installEventFilter(this);
	calibrationlayout->addWidget(tw);
	tw->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	tw->clear();

	// Get table for engine
	ILC7CalibrationTable *table = m_calibration_table_by_engine[engine];

	// Get total number of rows
	QVector<QVariant> rowIds = table->GetAllCalibrationRowIds();
	QVector<QVariant> colIds = table->GetAllCalibrationColIds();
	int rowcount = rowIds.size();
	int colcount = colIds.size();
	tw->setRowCount(rowcount);
	tw->setColumnCount(colcount+4);
	tw->setSelectionMode(QAbstractItemView::NoSelection);
	
	// Horizontal Header
	QTableWidgetItem *twi;
	QHeaderView *hh = tw->horizontalHeader();
	hh->setSectionResizeMode(QHeaderView::Stretch);

	twi = new QTableWidgetItem("Platform");
	twi->setToolTip("The operating system these these hashes are from");
	tw->setHorizontalHeaderItem(0, twi);
	hh->setSectionResizeMode(0,QHeaderView::ResizeToContents);

	twi = new QTableWidgetItem("Hash Type");
	tw->setHorizontalHeaderItem(1, twi);
	twi->setToolTip("The hash type this algorithm operates on");
	hh->setSectionResizeMode(2, QHeaderView::ResizeToContents);

	twi = new QTableWidgetItem("Audit Type");
	twi->setToolTip("The kind of audit using this algorithm");
	tw->setHorizontalHeaderItem(2, twi);
	hh->setSectionResizeMode(1, QHeaderView::ResizeToContents);

	twi = new QTableWidgetItem("Calibrate");
	tw->setHorizontalHeaderItem(3, twi);
	twi->setToolTip("Press this button to calibrate just this single hash type");
	hh->setSectionResizeMode(3, QHeaderView::ResizeToContents);
	
	int colnum = 0;
	for (auto col : colIds)
	{
		QString error;
		ILC7PasswordEngine::CALIBRATION_COL_INFO colInfo;
		if (!engine->GetCalibrationColInfo(table, col, colInfo, error))
		{
			Q_ASSERT(0);
			continue;
		}
		QTableWidgetItem *twi = new QTableWidgetItem(colInfo.name);
		twi->setToolTip(colInfo.description);
		tw->setHorizontalHeaderItem(colnum + 4, twi);
		
		colnum++;
	}


	// No vertical header
	QHeaderView *vh = tw->verticalHeader();
	vh->setVisible(false);

	int rownum = 0;
	for (auto rowId : table->GetAllCalibrationRowIds())
	{
		ILC7CalibrationTableRow *row = table->GetOrCreateCalibrationTableRow(rowId, false);
		QString error;
		ILC7PasswordEngine::CALIBRATION_ROW_INFO rowInfo;
		if (!engine->GetCalibrationRowInfo(table, rowId, rowInfo, error))
		{
			Q_ASSERT(0);
			continue;
		}
		m_table_row_by_rowId[ID_KEY{ table, rowId }] = rownum;

		// 'headers'
		tw->setItem(rownum, 0, new QTableWidgetItem(rowInfo.platform));
		tw->item(rownum, 0)->setFlags(Qt::NoItemFlags);
		tw->setItem(rownum, 1, new QTableWidgetItem(rowInfo.hashtype));
		tw->item(rownum, 1)->setFlags(Qt::NoItemFlags);
		tw->setItem(rownum, 2, new QTableWidgetItem(rowInfo.audittype));
		tw->item(rownum, 2)->setFlags(Qt::NoItemFlags);

		// Go button
		QPushButton *gobutton = new QPushButton(tw);
		gobutton->setText("Calibrate");

		tw->setItem(rownum, 3, new QTableWidgetItem());
		tw->item(rownum, 3)->setFlags(Qt::NoItemFlags);
		tw->setCellWidget(rownum, 3, gobutton);

		connect(gobutton, &QAbstractButton::clicked, this, &CLC7CalibrationDialog::slot_singleCalibrateButton_clicked);
		gobutton->setProperty("rowId", rowId);
		gobutton->setProperty("table", (qulonglong)table);

		// no selection allowed for cells
		for (int i = 4; i < colcount + 4; i++)
		{
			QVariant colId = colIds[i - 4];

			m_table_col_by_colId[ID_KEY{ table, colId }] = i;

			tw->setItem(rownum, i, new QTableWidgetItem());
			tw->item(rownum, i)->setFlags(Qt::NoItemFlags);

			// Add cells to table map
			TABLE_KEY key;
			key.table = table;
			key.rownum = rownum;
			key.colnum = i;
			m_table_cells_by_row_col[key] = row->GetOrCreateCalibrationCell(colId, false);
		}

		rownum++;
	}

	tw->resizeRowsToContents();
	
	m_table_widgets_by_calibration_table[table] = tw;

	connect(tw, &QTableWidget::cellClicked, this, &CLC7CalibrationDialog::slot_cellClicked);
	connect(tw, &QTableWidget::cellDoubleClicked, this, &CLC7CalibrationDialog::slot_cellClicked);
	connect(tw, &QTableWidget::customContextMenuRequested, this, &CLC7CalibrationDialog::slot_customContextMenuRequested);
}


bool CLC7CalibrationDialog::tableIsCompatibleWithEngine(ILC7CalibrationTable *table, ILC7PasswordEngine *engine)
{
	ILC7CalibrationTable *emptytable = m_passlink->NewCalibrationTable();
	engine->ResetCalibrationTable(emptytable);
	bool match = table->ConfigurationMatch(emptytable);
	emptytable->Release();
	return match;
}

void CLC7CalibrationDialog::saveCalibrations()
{
	for (auto engine : m_engines)
	{
		ILC7CalibrationTable *table = m_calibration_table_by_engine[engine];
		if (!m_passlink->SaveCalibrationTable(engine->GetCalibrationKey(), table))
		{
			Q_ASSERT(0);
			g_pLinkage->GetGUILinkage()->ErrorMessage("Error", "Could not save calibration table. Please report this issue to support@l0phtcrack.com.");
		}
	}
}


void CLC7CalibrationDialog::accept()
{
	saveCalibrations();

	QDialog::accept();
}

void CLC7CalibrationDialog::reject()
{
	QDialog::reject();
}



bool CLC7CalibrationDialog::eventFilter(QObject *obj, QEvent *evt)
{
	if (evt->type() == QEvent::Wheel)
	{
		//evt->ignore();
		return false;
	}
	return QDialog::eventFilter(obj, evt);
}

void CLC7CalibrationDialog::resizeEvent(QResizeEvent *e)
{
	for (auto tw : m_table_widgets_by_calibration_table.values())
	{
		int height = (tw->rowCount() * tw->rowHeight(0) + 1) + tw->horizontalHeader()->height();
		tw->setFixedHeight(height);
	}
	QDialog::resizeEvent(e);

}

void CLC7CalibrationDialog::slot_presetClicked(bool checked)
{
	QObject *object = QObject::sender();
	QRadioButton* radiobutton = qobject_cast<QRadioButton*>(object);

	int defaultsetidx = radiobutton->property("defaultsetidx").toInt();
	int engineidx = radiobutton->property("engineidx").toInt();
	ILC7PasswordEngine * engine = m_engines[engineidx];
	ILC7CalibrationTable *table = m_calibration_table_by_engine[engine];
	const QList<ILC7PasswordEngine::CALIBRATION_DEFAULT_SET> &default_sets = m_engine_default_sets[engine];
	
	if (defaultsetidx == -1)
	{
		table->SetDefaultSetId(QUuid());
		for (auto rowId : table->GetAllCalibrationRowIds())
		{
			ILC7CalibrationTableRow *row = table->GetOrCreateCalibrationTableRow(rowId, false);
			QVariant bestid = row->GetBestColId();
			row->SetPreferredColId(bestid);
		}
	}
	else
	{
		const ILC7PasswordEngine::CALIBRATION_DEFAULT_SET &default_set = default_sets[defaultsetidx];

		table->SetDefaultSetId(default_set.id);
		for (auto rowId : table->GetAllCalibrationRowIds())
		{
			ILC7CalibrationTableRow *row = table->GetOrCreateCalibrationTableRow(rowId, false);
			row->SetPreferredColId(default_set.default_col_by_row[rowId]);
		}
	}
	
	refreshCalibration();
	UpdateUI();
}


void CLC7CalibrationDialog::slot_singleCalibrateButton_clicked(bool checked)
{
	QObject *object = QObject::sender();
	QPushButton * gobutton = qobject_cast<QPushButton *>(object);
	QVariant rowId = gobutton->property("rowId");
	ILC7CalibrationTable *table = (ILC7CalibrationTable *)gobutton->property("table").toULongLong();
		
	if (m_calibration_thread.isRunning())
	{
		m_calibration_thread.requestInterruption();
		UpdateUI();
		return;
	}
	else
	{
		// add all calibrations
		m_jobs.append(JOB{ table, rowId, QVariant() });
		
		// Start calibration
		m_calibration_type = CALIBRATION_TYPE::ROW;
		m_calibration_thread.start();
		UpdateUI();
	}
}

void CLC7CalibrationDialog::slot_calibrateButton_clicked(bool checked)
{
	if (m_calibration_thread.isRunning())
	{
		m_calibration_thread.requestInterruption();
		for (auto engine : m_engines)
		{
			engine->StopCalibration(true);
		}

		UpdateUI();
		return;
	}
	else
	{
		// add all calibrations
		for (auto engine : m_engines)
		{
			auto table = m_calibration_table_by_engine[engine];
			if (table->GetDefaultSetId().isNull())
			{
				m_jobs.append(JOB{ table, QVariant(), QVariant() });
			}
		}

		// Start calibration
		m_calibration_type = CALIBRATION_TYPE::EVERYTHING;
		m_calibration_thread.start();
		UpdateUI();
	}
}

void CLC7CalibrationDialog::slot_customContextMenuRequested(const QPoint &pos)
{
	customContextMenuRequested(pos);
}

void CLC7CalibrationDialog::customContextMenuRequested(const QPoint &pos)
{
	QObject *_sender = sender();

	//ignore if we are currently running calibration
	if (m_calibration_thread.isRunning())	
		return;
	
	// what table are we in?
	bool found = false;
	int col, row;
	QTableWidget *tw;
	for (auto engine : m_engines)
	{
		auto table = m_calibration_table_by_engine[engine];
		tw = m_table_widgets_by_calibration_table[table];

		if (tw == _sender)
		{
			col = tw->columnAt(pos.x());
			row = tw->rowAt(pos.y());

			ILC7CalibrationTableCell *cell = m_table_cells_by_row_col[TABLE_KEY{ table, row, col }];
			if (cell)
			{
				QWidget *cellwidget = tw->cellWidget(row, col);
				if (cellwidget && cellwidget->isEnabled())
				{
					m_context_table = table;
					m_context_rowId = cell->GetRow()->GetId();
					m_context_colId = cell->GetId();

					found = true;
					break;
				}
			}
		}
	}
	if (!found)
	{
		return;
	}

	QMenu contextmenu;
	QAction *act = contextmenu.addAction("Recalibrate This");
	connect(act, &QAction::triggered, this, &CLC7CalibrationDialog::slot_recalibrateCell);
	contextmenu.exec(tw->mapToGlobal(pos));
}

void CLC7CalibrationDialog::slot_recalibrateCell(void)
{
	recalibrateCell();
}

void CLC7CalibrationDialog::recalibrateCell()
{
	if (m_calibration_thread.isRunning())
	{
		Q_ASSERT(0);
		return;
	}

	// add all calibrations
	m_jobs.append(JOB{ m_context_table, m_context_rowId, m_context_colId });

	m_context_table = nullptr;
	m_context_rowId = QVariant();
	m_context_colId = QVariant();

	// Start calibration
	m_calibration_type = CALIBRATION_TYPE::CELL;
	m_calibration_thread.start();
	UpdateUI();
}

void CLC7CalibrationDialog::slot_cellClicked(int row, int col)
{
	cellClicked(row, col);
}

void CLC7CalibrationDialog::cellClicked(int row, int col)
{
	QObject *_sender = sender();

	//ignore if we are currently running calibration
	if (m_calibration_thread.isRunning())
		return;

	// what table are we in?
	QTableWidget *tw;
	for (auto engine : m_engines)
	{
		auto table = m_calibration_table_by_engine[engine];
		tw = m_table_widgets_by_calibration_table[table];

		if (tw == _sender)
		{
			if (!table->GetDefaultSetId().isNull())
			{
				g_pLinkage->GetGUILinkage()->WarningMessage("Can't change calibration defaults", "To change calibration defaults, choose the 'Custom' preset for this table.");
				return;
			}

			ILC7CalibrationTableCell *cell = m_table_cells_by_row_col[TABLE_KEY{ table, row, col }];
			if (cell)
			{
				bool is_incompatible = cell->Valid() && cell->CPS() == 0;
				QWidget *cellwidget = tw->cellWidget(row, col);
				if (cellwidget && cellwidget->isEnabled() && !is_incompatible)
				{
					cell->GetRow()->SetPreferredColId(cell->GetId());
					refreshRow(table, cell->GetRow()->GetId());
					return;
				}
			}
		}
	}
}

void CLC7CalibrationDialog::UpdateUI(void)
{
	bool is_running = m_calibration_thread.isRunning();
	bool any_calibration_available = false;
	bool all_valid = true;

	for (auto engine : m_engines)
	{
		auto table = m_calibration_table_by_engine[engine];
		auto tw = m_table_widgets_by_calibration_table[table];
		if (table->GetDefaultSetId().isNull())
		{
			any_calibration_available = true;

			// enable all calibration buttons for this table
			for (int i = 0; i < tw->rowCount(); i++)
			{
				tw->cellWidget(i, 3)->clearFocus();
				tw->cellWidget(i, 3)->setEnabled(!is_running);
			}
			
			if (!table->IsValid())
			{
				all_valid = false;
			}
		}
		else
		{
			// disable all calibration buttons for this table
			for (int i = 0; i < tw->rowCount(); i++)
			{
				tw->cellWidget(i, 3)->clearFocus();
				tw->cellWidget(i, 3)->setEnabled(false);
			}
		}
	}

	ui.okButton->setEnabled(!is_running /*&& all_valid*/);
	ui.cancelButton->setEnabled(!is_running);
	ui.calibrateButton->setEnabled(any_calibration_available && (!is_running || !m_calibration_thread.isInterruptionRequested()));
	
	QList<QWidget *> presetchildren = ui.presetsScrollArea->findChildren<QWidget*>();
	for (auto pc : presetchildren)
	{
		pc->setEnabled(!is_running);
	}


	if (is_running)
	{
		if (m_calibration_thread.isInterruptionRequested())
		{
			ui.calibrateButton->setText("Stopping...");
		}
		else
		{
			ui.calibrateButton->setText("Stop");
		}
	}
	else
	{
		// Check if all tables are valid 
		if (all_valid)
		{
			ui.calibrateButton->setText("Recalibrate");
		}
		else
		{
			ui.calibrateButton->setText("Calibrate");
		}
	}
}


void CLC7CalibrationDialog::slot_calibrationThread_started(void)
{
	runCalibration();

	QTimer::singleShot(0, this, SLOT(slot_calibrationThread_finished()));

	m_calibration_thread.quit();
}

void CLC7CalibrationDialog::slot_calibrationThread_finished()
{
	m_calibration_thread.wait();
	
	refreshCalibration();
	UpdateUI();
}


static QString FormatCPS(quint64 cps)
{
	QString mod = "";
	quint32 cpsfrac = 0;
	if (cps > 1000)
	{
		mod = "K";
		cpsfrac = cps % 1000;
		cps /= 1000;
	}
	if (cps > 1000)
	{
		mod = "M";
		cpsfrac = cps % 1000;
		cps /= 1000;
	}
	if (cps > 1000)
	{
		mod = "G";
		cpsfrac = cps % 1000;
		cps /= 1000;
	}
	if (cps > 1000)
	{
		mod = "T";
		cpsfrac = cps % 1000;
		cps /= 1000;
	}

	double dblcps = (((double)cpsfrac) / 1000) + (double)cps;

	QString cpsfmt = QString("%1 %2c/s").arg(dblcps, 0, 'f', 3).arg(mod);

	return cpsfmt;
}

void CLC7CalibrationDialog::slot_refreshCell(ILC7CalibrationTable *table, QVariant rowId, QVariant colId)
{
	refreshCell(table, rowId, colId);
}

void CLC7CalibrationDialog::refreshCell(ILC7CalibrationTable *table, QVariant rowId, QVariant colId)
{
	const bool isGuiThread = QThread::currentThread() == QCoreApplication::instance()->thread();
	if (!isGuiThread)
	{
		emit sig_refreshCell(table, rowId, colId);
		return;
	}

	int rownum = TableRowIdToRow(table, rowId);
	int colnum = TableColIdToCol(table, colId);

	ILC7PasswordEngine *engine = m_engine_by_calibration_table[table];
	QTableWidget *tw = m_table_widgets_by_calibration_table[table];
	ILC7CalibrationTableRow *row = table->GetOrCreateCalibrationTableRow(rowId, false);
	QVariant preferredColId = row->GetPreferredColId();
	bool is_default = !(table->GetDefaultSetId().isNull());
	bool is_active = preferredColId == colId;

	TABLE_KEY key;
	key.table = table;
	key.rownum = rownum;
	key.colnum = colnum;

	ILC7CalibrationTableCell *cell = m_table_cells_by_row_col.value(key, nullptr);
	
	// grey out cells that have no corresponding algorithm
	CFireWidget *firewidget = new CFireWidget(tw);
	firewidget->setActive(false);
	firewidget->setFireEnabled(false);
	firewidget->setBackgroundColor(m_colman->GetHighlightColor(), true);
	firewidget->setTextColor(QColor(m_colman->GetTextColor()), true);
	firewidget->setBackgroundColor(QColor(m_colman->GetBaseShade("CONTROL_BKGD_DISABLED")), false);
	firewidget->setTextColor(QColor(m_colman->GetTextColor()), false);// m_colman->GetBaseShade("TEXT_DISABLED")), false);
	firewidget->setHoverColor(QColor(m_colman->GetBaseShade("SELECT_BKGD_1")), true);
	firewidget->setHoverColor(QColor(m_colman->GetBaseShade("SELECT_BKGD_1")), false);
	firewidget->setBorderColor(QColor(0, 0, 0), false);
	firewidget->setBorderColor(QColor(0, 0, 0), true);

	const quint64 FIRE_CPS = 1000000000;

	if (!cell)
	{
		firewidget->setText(SMALLFONTTEXT("<i><b>Not<br>Supported</b></i>"));
		firewidget->setTextOption(QTextOption(Qt::AlignCenter));
		firewidget->setEnabled(false);
		firewidget->setFireEnabled(false);
		firewidget->setBackgroundColor(QColor(m_colman->GetBaseShade("BUTTON_COLOR_DISABLED")), false);
		firewidget->setTextColor(QColor(m_colman->GetBaseShade("TEXT_DISABLED")), false);
	}
	else
	{
		if (is_default)
		{
			if (is_active)
			{
				firewidget->setText(SMALLFONTTEXT("<i><b>Default</b></i>"));
			}
			else
			{
				firewidget->setText(SMALLFONTTEXT("<i><b>---</b></i>"));
			}
			firewidget->setEnabled(false);
			firewidget->setActive(is_active);
		}
		else
		{
			if (cell->Valid())
			{
				quint64 cps = cell->CPS();
				if (cps == 0)
				{
					firewidget->setText(SMALLFONTTEXT("<i><b>Incompatible<br>Hardware</b></i>"));
					firewidget->setTextOption(QTextOption(Qt::AlignCenter));
					firewidget->setFireEnabled(false);
					firewidget->setEnabled(true);
					firewidget->setBackgroundColor(QColor(m_colman->GetBaseShade("BUTTON_COLOR_DISABLED")), false);
					firewidget->setTextColor(QColor(m_colman->GetBaseShade("TEXT_DISABLED")), false);
				}
				else
				{
					QString cpsstr = FormatCPS(cps);

					firewidget->setText(FONTTEXT(cpsstr));
				}
				if (cps >= FIRE_CPS)
				{
					firewidget->setFireEnabled(true);
				}
			}
			else
			{
				firewidget->setText(FONTTEXT("<i><b>---</b></i>"));
				firewidget->setTextOption(QTextOption(Qt::AlignCenter));
				firewidget->setEnabled(true);
				firewidget->setFireEnabled(false);
				//firewidget->setBackgroundColor(QColor(m_colman->GetBaseShade("BUTTON_COLOR_DISABLED")), false);
				//firewidget->setTextColor(QColor(m_colman->GetBaseShade("TEXT_DISABLED")), false);
			}
			firewidget->setActive(is_active);
		}
	}

	firewidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	tw->setCellWidget(rownum, colnum, firewidget);
}

void CLC7CalibrationDialog::refreshRow(ILC7CalibrationTable *table, QVariant rowId)
{
	ILC7CalibrationTableRow *row = table->GetOrCreateCalibrationTableRow(rowId, false);
	
	QVector<QVariant> colIds = row->GetCalibrationColIds();
	for (auto colId : colIds)
	{
		refreshCell(table, rowId, colId);
	}
}

void CLC7CalibrationDialog::refreshCalibration(void)
{
	ILC7PasswordLinkage * passlink = GET_ILC7PASSWORDLINKAGE(g_pLinkage);
	ILC7ColorManager * colman = g_pLinkage->GetGUILinkage()->GetColorManager();

	// For each engine get the calibration table and refresh its rows
	for (int i = 0; i < (int)m_engines.size(); i++)
	{
		auto engine = m_engines[i];
		auto table = m_calibration_table_by_engine[engine];
		for(auto rowId: table->GetAllCalibrationRowIds())
		{
			refreshRow(table, rowId);
		}
	}
}



void CLC7CalibrationDialog::createSpinnerWidget(ILC7CalibrationTable *table, QVariant rowId, QVariant colId)
{
	QWidget *widget = new QWidget();

	WaitingSpinnerWidget* spinner = new WaitingSpinnerWidget(widget, true, false);

	spinner->setRoundness(70.0);
	spinner->setMinimumTrailOpacity(15.0);
	spinner->setTrailFadePercentage(70.0);
	spinner->setNumberOfLines(12);
	spinner->setLineLength(5 * m_colman->GetSizeRatio());
	spinner->setLineWidth(2 * m_colman->GetSizeRatio());
	spinner->setInnerRadius(5 * m_colman->GetSizeRatio());
	spinner->setRevolutionsPerSecond(1);
	spinner->setColor(m_colman->GetHighlightColor());

	widget->setLayout(new QGridLayout());
	widget->layout()->addWidget(spinner);
	widget->layout()->setAlignment(Qt::AlignCenter);
	
	int rownum = TableRowIdToRow(table, rowId);
	int colnum = TableColIdToCol(table, colId);

	QTableWidget *tw = m_table_widgets_by_calibration_table[table];
	tw->setCellWidget(rownum, colnum, widget);

	spinner->start();
}

void CLC7CalibrationDialog::removeSpinnerWidget(ILC7CalibrationTable *table, QVariant rowId, QVariant colId)
{
	QTableWidget *tw = m_table_widgets_by_calibration_table[table];
	int rownum = TableRowIdToRow(table, rowId);
	int colnum = TableColIdToCol(table, colId);

	tw->removeCellWidget(rownum, colnum);
}

void CLC7CalibrationDialog::slot_refreshTableWidget(ILC7CalibrationTable *table)
{
	QTableWidget *tw = m_table_widgets_by_calibration_table[table];
	tw->update();
}


void CLC7CalibrationDialog::calibrationCallback(const ILC7PasswordEngine::CALIBRATION_CALLBACK_ARGUMENTS &args)
{
	switch (args.activity)
	{
	case ILC7PasswordEngine::CAL_STARTED:
		break;
	case ILC7PasswordEngine::CAL_STOPPED:
		break;
	case ILC7PasswordEngine::CAL_BEGIN_CELL:
		// Add progress meter
		createSpinnerWidget(args.table, args.rowId, args.colId);
		//refreshCell(args.table, args.rowId, args.colId);
		break;
	case ILC7PasswordEngine::CAL_END_CELL:
		removeSpinnerWidget(args.table, args.rowId, args.colId);
		updatePreferredCalibration(args.table, args.rowId);
		break;
	case ILC7PasswordEngine::CAL_ERROR:
		g_pLinkage->GetGUILinkage()->GetWorkQueueWidget()->AppendToActivityLog(args.details);
		break;
	case ILC7PasswordEngine::CAL_RESULTS:
		refreshCell(args.table, args.rowId, args.colId);
		break;
	default:
		Q_ASSERT(0);
		return;
	}

	UpdateUI();
}

void CLC7CalibrationDialog::updatePreferredCalibration(ILC7CalibrationTable *table, QVariant rowId)
{
	//QTableWidget *tw = m_table_widgets_by_calibration_table[table];
	ILC7CalibrationTableRow *row = table->GetOrCreateCalibrationTableRow(rowId, false);
	QVariant bestColId = row->GetBestColId();
	row->SetPreferredColId(bestColId);
	refreshRow(table, rowId);
}

void CLC7CalibrationDialog::runCalibration(void)
{
	bool success = true;
	int jobcount = (int)m_jobs.size();
	int jobnum = 0;
	for (auto job : m_jobs)
	{
		emit sig_setProgressBarValue(jobnum);
		if (m_calibration_thread.isInterruptionRequested())
		{
			success = false;
			break;
		}
		
		ILC7PasswordEngine *engine = m_engine_by_calibration_table[job.m_table];
		
		m_current_table = job.m_table;
		m_current_rowId = job.m_rowId;
		m_current_colId = job.m_colId;

		engine->RunCalibration(job.m_table, job.m_rowId, job.m_colId, this, (ILC7PasswordEngine::CALIBRATION_CALLBACK)&CLC7CalibrationDialog::calibrationCallback);
	}
	if (success)
	{
		emit sig_setProgressBarValue(jobnum);
	}
	else
	{
		emit sig_setProgressBarValue(0);
	}

	m_current_table = nullptr;
	m_current_rowId = QVariant();
	m_current_colId = QVariant();

	m_jobs.clear();
}




///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////





CLC7CalibrateGUI::CLC7CalibrateGUI()
{
	TR;
	g_pLinkage->RegisterNotifySessionActivity(QUuid(), this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CLC7CalibrateGUI::NotifySessionActivity);
}

CLC7CalibrateGUI::~CLC7CalibrateGUI()
{
	TR;
	g_pLinkage->UnregisterNotifySessionActivity(QUuid(), this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CLC7CalibrateGUI::NotifySessionActivity);
}

ILC7Interface *CLC7CalibrateGUI::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Component")
	{
		return this;
	}
	return NULL;
}

QUuid CLC7CalibrateGUI::GetID()
{
	TR;
	return UUID_CALIBRATEGUI;
}


ILC7Component::RETURNCODE CLC7CalibrateGUI::ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl)
{
	TR;
	
	if (command == "gui")
	{
		CLC7CalibrationDialog cal(NULL);
		
		g_pLinkage->GetGUILinkage()->ShadeUI(true);
		int ret = cal.exec();
		g_pLinkage->GetGUILinkage()->ShadeUI(false);

		return SUCCESS;
	}

	return FAIL;
}


bool CLC7CalibrateGUI::ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error)
{
	TR;
	return true;
}



void CLC7CalibrateGUI::NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler)
{
	TR;
	switch (activity)
	{
	case ILC7Linkage::SESSION_OPEN_POST:
	case ILC7Linkage::SESSION_NEW_POST:
		CheckCalibration();
		break;
	case ILC7Linkage::SESSION_CLOSE_PRE:
		break;
	}
}


void CLC7CalibrateGUI::CheckCalibration(void)
{
	TR;
	const bool isGuiThread = (QThread::currentThread() == QCoreApplication::instance()->thread());
	if (isGuiThread)
	{
		slot_checkCalibration();
	}
	else
	{
		emit sig_checkCalibration();
	}
}

void CLC7CalibrateGUI::slot_checkCalibration(void)
{
	TR;
	
	/*
	// Ensure we have valid calibration data
	CLC7Calibration cal, savedcal;

	CLC7Calibration::emptyCalibration(cal);
	bool had_saved = false;
	CLC7Calibration::loadCalibration(savedcal);
	if (savedcal.isValid() && savedcal.configurationMatch(cal))
	{
		cal = savedcal;
		had_saved = true;
	}

	if (!cal.isValid())
	{
		if (g_pLinkage->GetGUILinkage()->YesNoBoxWithNeverAgain("Perform Calibration?",
			"L0phtCrack 7 can perform faster if you run calibration on your system.\n\n"
			"This is a one-time operation that measures the speed of all cracking methods for each hash type. This is only performed once, and will take a few minutes.\n\n"
			"If you do not calibrate, the default CPU-based algorithms will be chosen.\n\n"
			"Do you wish to run calibration now?",
			QString("%1:perform_calibration").arg(UUID_LC7JTRPLUGIN.toString())))
		{
			CLC7Calibrate cal(g_pLinkage->GetGUILinkage()->GetMainWindow());
			cal.slot_calibrateButton_clicked(false);
			g_pLinkage->GetGUILinkage()->ShadeUI(true);
			cal.exec();
			g_pLinkage->GetGUILinkage()->ShadeUI(false);
		}
	}
	*/
}
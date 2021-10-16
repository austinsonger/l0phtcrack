#include<stdafx.h>
#define FONT_SIZE 13
#define FONTTEXT(x) QString("<span style=\"font-size:%1px\">%2</span>").arg(m_colman->GetSizeRatio() * FONT_SIZE).arg(x)
#define SMALL_FONT_SIZE 10
#define SMALLFONTTEXT(x) QString("<span style=\"font-size:%1px\">%2</span>").arg(m_colman->GetSizeRatio() * SMALL_FONT_SIZE).arg(x)

CLC7Calibrate::CLC7Calibrate(QWidget *parent) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
{
	ui.setupUi(this);

	m_context_calibration = false;
	m_context_calibration_mask = false;

	m_passlink = GET_ILC7PASSWORDLINKAGE(g_pLinkage);
	m_colman = g_pLinkage->GetGUILinkage()->GetColorManager();

	connect(ui.calibrateButton, &QAbstractButton::clicked, this, &CLC7Calibrate::slot_calibrateButton_clicked);

	connect(this, &CLC7Calibrate::sig_setProgressBarValue, ui.calibrationProgress, &QProgressBar::setValue, Qt::BlockingQueuedConnection);
	connect(this, &CLC7Calibrate::sig_setProgressBarRange, ui.calibrationProgress, &QProgressBar::setRange, Qt::BlockingQueuedConnection);

	connect(ui.okButton, &QAbstractButton::clicked, this, &QDialog::accept);
	connect(ui.cancelButton, &QAbstractButton::clicked, this, &QDialog::reject);

	connect(this, &QDialog::accepted, this, &CLC7Calibrate::slot_accepted);

	connect(&m_calibration_thread, &QThread::started, this, &CLC7Calibrate::slot_calibrationThread_started, Qt::DirectConnection);

	connect(this, &CLC7Calibrate::sig_createSpinnerWidget, this, &CLC7Calibrate::slot_createSpinnerWidget, Qt::BlockingQueuedConnection);
	connect(this, &CLC7Calibrate::sig_removeSpinnerWidget, this, &CLC7Calibrate::slot_removeSpinnerWidget, Qt::BlockingQueuedConnection);
	connect(this, &CLC7Calibrate::sig_refreshCell, this, &CLC7Calibrate::slot_refreshCell, Qt::BlockingQueuedConnection);
	connect(this, &CLC7Calibrate::sig_refreshTableWidget, this, &CLC7Calibrate::slot_refreshTableWidget, Qt::BlockingQueuedConnection);
	
	connect(ui.calibrationNoMaskTableWidget, &QTableWidget::cellClicked, this, &CLC7Calibrate::slot_cellClicked_nomask);
	connect(ui.calibrationNoMaskTableWidget, &QTableWidget::cellDoubleClicked, this, &CLC7Calibrate::slot_cellClicked_nomask);
	connect(ui.calibrationNoMaskTableWidget, &QTableWidget::customContextMenuRequested, this, &CLC7Calibrate::slot_customContextMenuRequested_nomask);
	connect(ui.calibrationMaskTableWidget, &QTableWidget::cellClicked, this, &CLC7Calibrate::slot_cellClicked_mask);
	connect(ui.calibrationMaskTableWidget, &QTableWidget::cellDoubleClicked, this, &CLC7Calibrate::slot_cellClicked_mask);
	connect(ui.calibrationMaskTableWidget, &QTableWidget::customContextMenuRequested, this, &CLC7Calibrate::slot_customContextMenuRequested_mask);

	CLC7Calibration::emptyCalibration(m_calibration);
	
	CLC7Calibration savedcal;
	CLC7Calibration::loadCalibration(savedcal);
	if (savedcal.isValid() && savedcal.configurationMatch(m_calibration))
	{
		m_calibration = savedcal;
	}
	
	refreshCalibration();

	resize(size()*m_colman->GetSizeRatio());

	UpdateUI();
}

CLC7Calibrate::~CLC7Calibrate()
{
}




void CLC7Calibrate::reject() 
{ 
	if (m_calibration_thread.isRunning())
	{
		return;
	}

	QDialog::reject();
}



void CLC7Calibrate::slot_calibrateButton_clicked(bool checked)
{
	if (m_calibration_thread.isRunning())
	{
		m_calibration_thread.requestInterruption();
		UpdateUI();
		return;
	}
	else
	{
		m_context_calibration = false;
		m_context_calibration_mask = false;

		// Blow away kernels cache
		QDir appdatadir(g_pLinkage->GetCacheDirectory());
		appdatadir.mkdir("lc7jtr{9846c8cf-1db1-467e-83c2-c5655aa81936}");
		appdatadir.cd("lc7jtr{9846c8cf-1db1-467e-83c2-c5655aa81936}");
		appdatadir.mkdir("kernels");
		appdatadir.cd("kernels");
		appdatadir.removeRecursively();

		// Start calibration
		m_calibration_thread.start();
		UpdateUI();
	}
	
}

void CLC7Calibrate::slot_customContextMenuRequested_nomask(const QPoint &pos)
{
	customContextMenuRequested(false, pos);
}

void CLC7Calibrate::slot_customContextMenuRequested_mask(const QPoint &pos)
{
	customContextMenuRequested(true, pos);
}

void CLC7Calibrate::customContextMenuRequested(bool mask, const QPoint &pos)
{
	// ignore if we don't already have a valid calibration, must do the full thing first
	//if (!m_calibration.isValid())
	//{
	//	return;
	//}

	// ignore if we are currently running calibration
	if (m_calibration_thread.isRunning())
	{
		return;
	}

	int col, row;

	if (mask)
	{
		col = ui.calibrationMaskTableWidget->columnAt(pos.x());
		row = ui.calibrationMaskTableWidget->rowAt(pos.y());
	}
	else
	{
		col = ui.calibrationNoMaskTableWidget->columnAt(pos.x());
		row = ui.calibrationNoMaskTableWidget->rowAt(pos.y());
	}
	if (col < 0 || row < 0)
	{
		return;
	}

	m_context_cht = m_calibration.hashTypes().at(row);
	m_context_cpt = m_calibration.processorTypes().at(col);

	QString algo;
	QString jtrversion;
	if (m_context_cpt.gpuplatform() == GPU_NONE)
	{
		algo = CLC7JTR::GetCPUNodeAlgorithm(m_context_cht.hashtype());
		jtrversion = m_context_cpt.cpuinstructionset();
	}
	else if (m_context_cpt.gpuplatform() == GPU_OPENCL)
	{
		algo = CLC7JTR::GetOpenCLNodeAlgorithm(m_context_cht.hashtype());
		jtrversion = "sse2";
	}
	//else if (m_context_cpt.gpuplatform() == GPU_CUDA)
	//{
	//	algo = CLC7JTR::GetCUDANodeAlgorithm(m_context_cht.hashtype());
	//	jtrversion = "sse2";
	//}

	// ignore cells with no algorithm
	if (algo.isEmpty())
	{
		return;
	}

	QMenu contextmenu;
	QAction *act = contextmenu.addAction("Recalibrate This");
	if (mask)
	{
		connect(act, &QAction::triggered, this, &CLC7Calibrate::slot_recalibrateCell_mask);
		contextmenu.exec(ui.calibrationMaskTableWidget->mapToGlobal(pos));
	}
	else
	{
		connect(act, &QAction::triggered, this, &CLC7Calibrate::slot_recalibrateCell_nomask);
		contextmenu.exec(ui.calibrationNoMaskTableWidget->mapToGlobal(pos));
	}
}

void CLC7Calibrate::slot_recalibrateCell_nomask(void)
{
	recalibrateCell(false);
}

void CLC7Calibrate::slot_recalibrateCell_mask(void)
{
	recalibrateCell(true);
}

void CLC7Calibrate::recalibrateCell(bool mask)
{
	m_context_calibration = true;
	m_context_calibration_mask = mask;
	m_calibration_thread.start();
	UpdateUI();
}

void CLC7Calibrate::slot_cellClicked_nomask(int row, int column)
{
	cellClicked(false, row, column);
}

void CLC7Calibrate::slot_cellClicked_mask(int row, int column)
{
	cellClicked(true, row, column);
}

void CLC7Calibrate::cellClicked(bool mask, int row, int column)
{
	if (m_calibration_thread.isRunning() || !m_calibration.isValid())
	{
		return;
	}

	CLC7CalibrationHashType cht = m_calibration.hashTypes().at(row);
	CLC7CalibrationProcessorType cpt = m_calibration.processorTypes().at(column);
	
	CLC7CalibrationData value;
	if (!m_calibration.getCalibrationData(mask, cht, cpt, value) || value.CPS() == 0 || value.JTRKernel().isEmpty())
	{
		return;
	}

	CFireWidget * firewidget;
	
	CLC7CalibrationProcessorType oldcpt;
	if (m_calibration.getPreferredMethod(mask, cht, oldcpt))
	{
		int oldrow = row;
		int oldcolumn = m_calibration.processorTypes().indexOf(oldcpt);
		
		if (mask)
		{
			firewidget = (CFireWidget *)ui.calibrationMaskTableWidget->cellWidget(oldrow, oldcolumn);
		}
		else
		{
			firewidget = (CFireWidget *)ui.calibrationNoMaskTableWidget->cellWidget(oldrow, oldcolumn);
		}
		firewidget->setActive(false);
	}

	m_calibration.setPreferredMethod(mask, cht, cpt);
	
	if (mask)
	{
		CFireWidget * firewidget = (CFireWidget *)ui.calibrationMaskTableWidget->cellWidget(row, column);
		firewidget->setActive(true);
	}
	else
	{
		CFireWidget * firewidget = (CFireWidget *)ui.calibrationNoMaskTableWidget->cellWidget(row, column);
		firewidget->setActive(true);
	}

}

void CLC7Calibrate::slot_accepted(void)
{
	CLC7Calibration::saveCalibration(m_calibration);
}

void CLC7Calibrate::UpdateUI(void)
{
	if (m_calibration_thread.isRunning())
	{
		if (m_calibration_thread.isInterruptionRequested())
		{
			ui.calibrateButton->setText("Stopping Calibration...");
			ui.calibrateButton->setEnabled(false);
		}
		else
		{
			ui.calibrateButton->setText("Stop Calibration");
			ui.calibrateButton->setEnabled(true);
		}

		ui.okButton->setEnabled(false);
		ui.cancelButton->setEnabled(false);
	}
	else
	{
		if (m_calibration.isValid())
		{
			ui.calibrateButton->setText("Recalibrate");
			ui.calibrateButton->setEnabled(true);

			ui.okButton->setEnabled(true);
		}
		else
		{
			ui.calibrateButton->setText("Calibrate");
			ui.calibrateButton->setEnabled(true);

			ui.okButton->setEnabled(false);
		}

		ui.cancelButton->setEnabled(true);
	}
}


void CLC7Calibrate::slot_calibrationThread_started(void)
{
	runCalibration();

	QTimer::singleShot(0, this, SLOT(slot_calibrationThread_finished()));
	
	m_calibration_thread.quit();
}

void CLC7Calibrate::slot_calibrationThread_finished()
{
	m_calibration_thread.wait();

	if (!m_calibration.isValid())
	{
		CLC7Calibration::emptyCalibration(m_calibration);

		CLC7Calibration savedcal;
		CLC7Calibration::loadCalibration(savedcal);
		if (savedcal.isValid() && savedcal.configurationMatch(m_calibration))
		{
			m_calibration = savedcal;
		}

		refreshCalibration();
	}

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

void CLC7Calibrate::slot_refreshCell(REFRESHCELLARGS *args)
{
	refreshCell(args->mask, args->cht, args->cpt, args->row, args->col);
}

void CLC7Calibrate::refreshCell(bool mask, CLC7CalibrationHashType cht, CLC7CalibrationProcessorType cpt, int row, int col)
{
	const bool isGuiThread = QThread::currentThread() == QCoreApplication::instance()->thread();
	if (!isGuiThread)
	{
		REFRESHCELLARGS args;
		args.mask = mask;
		args.cht = cht;
		args.cpt = cpt;
		args.row = row;
		args.col = col;
		emit sig_refreshCell(&args);
		return;
	}

	QString algo;
	if (cpt.gpuplatform() == GPU_NONE)
	{
		algo = CLC7JTR::GetCPUNodeAlgorithm(cht.hashtype());
	}
	else if (cpt.gpuplatform() == GPU_OPENCL)
	{
		algo = CLC7JTR::GetOpenCLNodeAlgorithm(cht.hashtype());
	}
	//else if (cpt.gpuplatform() == GPU_CUDA)
	//{
	//	algo = CLC7JTR::GetCUDANodeAlgorithm(cht.hashtype());
	//}

	// grey out cells that have no corresponding algorithm
	CFireWidget * firewidget;
	if (mask)
	{
		firewidget = new CFireWidget(ui.calibrationMaskTableWidget);
	}
	else
	{
		firewidget = new CFireWidget(ui.calibrationNoMaskTableWidget);
	}
	firewidget->setActive(false);
	firewidget->setFireEnabled(false);
	firewidget->setBackgroundColor(m_colman->GetHighlightColor(), true);
	firewidget->setTextColor(QColor(m_colman->GetTextColor()), true);
	firewidget->setBackgroundColor(QColor(m_colman->GetBaseShade("CONTROL_BKGD_DISABLED")),false);
	firewidget->setTextColor(QColor(m_colman->GetTextColor()), false);// m_colman->GetBaseShade("TEXT_DISABLED")), false);
	firewidget->setHoverColor(QColor(m_colman->GetBaseShade("SELECT_BKGD_1")),true);
	firewidget->setHoverColor(QColor(m_colman->GetBaseShade("SELECT_BKGD_1")),false);
	firewidget->setBorderColor(QColor(0, 0, 0), false);
	firewidget->setBorderColor(QColor(0, 0, 0), true);

	const quint64 FIRE_CPS = 1000000000;
	
	if (algo.isEmpty())
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
		CLC7CalibrationData value;
		if (m_calibration.getCalibrationData(mask, cht, cpt, value))
		{
			quint64 cps = value.CPS();
			if (cps == 0)
			{
				firewidget->setText(SMALLFONTTEXT("<i><b>Incompatible<br>Hardware</b></i>"));
				firewidget->setTextOption(QTextOption(Qt::AlignCenter));
				//firewidget->setEnabled(false);
				firewidget->setFireEnabled(false);
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

			CLC7CalibrationProcessorType bestcpt;
			if (m_calibration.getPreferredMethod(mask, cht, bestcpt) && bestcpt == cpt)
			{
				firewidget->setActive(true);
			}
			else
			{
				firewidget->setActive(false);
			}
		}
		else
		{
			firewidget->setText(FONTTEXT("<i>???</i>"));
			firewidget->setTextOption(QTextOption(Qt::AlignCenter));
			firewidget->setEnabled(false);
			firewidget->setFireEnabled(false);
			firewidget->setBackgroundColor(QColor(m_colman->GetBaseShade("BUTTON_COLOR_DISABLED")), false);
			firewidget->setTextColor(QColor(m_colman->GetBaseShade("TEXT_DISABLED")), false);
		}
	}

	firewidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	if (mask)
	{
		ui.calibrationMaskTableWidget->setCellWidget(row, col, firewidget);
	}
	else
	{
		ui.calibrationNoMaskTableWidget->setCellWidget(row, col, firewidget);
	}
}

void CLC7Calibrate::refreshCalibration(void)
{
	refreshCalibrationInternal(false);
	refreshCalibrationInternal(true);
}

void CLC7Calibrate::refreshCalibrationInternal(bool mask)
{
	ILC7PasswordLinkage * passlink = GET_ILC7PASSWORDLINKAGE(g_pLinkage);
	ILC7ColorManager * colman = g_pLinkage->GetGUILinkage()->GetColorManager();

	QTableWidget *tw;
	if (mask)
	{
		tw = ui.calibrationMaskTableWidget;
	}
	else
	{
		tw = ui.calibrationNoMaskTableWidget;
	}
	tw->setRowCount(m_calibration.hashTypes().size());
	tw->setColumnCount(m_calibration.processorTypes().size());

	// Vertical Header
	QHeaderView *vh = tw->verticalHeader();
	vh->setSectionResizeMode(QHeaderView::Stretch);

	int row = 0;
	foreach(CLC7CalibrationHashType cht, m_calibration.hashTypes())
	{
		fourcc fcc = cht.hashtype();
		
		LC7HashType lc7hashtype;
		QString error;
		if (!passlink->Lookup(fcc, lc7hashtype, error))
		{
			Q_ASSERT(0);
			continue;
		}

		QTableWidgetItem *headeritem = new QTableWidgetItem(lc7hashtype.name);
		tw->setVerticalHeaderItem(row, headeritem);

		row++;
	}

	
	// Horizontal Header
	QHeaderView *hh = tw->horizontalHeader();
	hh->setSectionResizeMode(QHeaderView::Stretch);

	int col = 0;
	foreach(CLC7CalibrationProcessorType cpt, m_calibration.processorTypes())
	{
		QString title;
		QColor color;
		if (cpt.gpuplatform() == GPU_NONE)
		{
			// CPU
			title = cpt.cpuinstructionset().toUpper();
		}
		else
		{
			// GPU
			if (cpt.gpuplatform() == GPU_OPENCL)
			{
				title = "GPU/OpenCL";
				color = QColor(colman->GetBaseShade("BUTTON_BKGD_0"));
			}
			//else if (cpt.gpuplatform() == GPU_CUDA)
			//{
			//	title = "GPU/CUDA";
			//	color = QColor(colman->GetBaseShade("BUTTON_BKGD_0"));
			//}
		}

		QTableWidgetItem *headeritem = new QTableWidgetItem(title);
		if (color.isValid())
		{
			headeritem->setBackgroundColor(color);
		}
		tw->setHorizontalHeaderItem(col, headeritem);

		col++;
	}

	row = 0;
	foreach(CLC7CalibrationHashType cht, m_calibration.hashTypes())
	{
		col = 0;
		foreach(CLC7CalibrationProcessorType cpt, m_calibration.processorTypes())
		{
			refreshCell(mask, cht, cpt, row, col);
			col++;
		}
		row++;
	}


}


bool CLC7Calibrate::runCalibrationThreads(QString algo, GPUPLATFORM gpuplatform, QString jtrversion, bool mask, quint64 &cps, QMap<int, QString> &extra_opencl_kernel_args)
{
	QList<CLC7CalibrationThread *> threads;

	if (gpuplatform == GPU_NONE)
	{
		for (int i = 0; i < m_calibration.CPUThreadCount(); i++)
		{
			CLC7CalibrationThread *thread = new CLC7CalibrationThread(algo, jtrversion, -1, mask, GPU_NONE, "");

			thread->start();

			threads.append(thread);
		}
	}
	else
	{
		foreach(GPUINFO gi, m_calibration.GPUInfo())
		{
			QString platform;
			if (gi.platform == GPU_OPENCL)
			{
				platform = "OpenCL";
			}
			//else if (gi.platform == GPU_CUDA)
			//{
			//	platform = "CUDA";
			//}

			bool enabled = g_pLinkage->GetSettings()->value(UUID_LC7JTRPLUGIN.toString() + QString(":enablegpu_%1_%2").arg(platform).arg(gi.jtrindex), true).toBool();
			if (enabled && gi.platform == gpuplatform)
			{
				CLC7CalibrationThread *thread = new CLC7CalibrationThread(algo, jtrversion, gi.jtrindex, mask, gi.platform, gi.vendor);

				thread->start();

				threads.append(thread);
			}
		}
	}

	cps = 0;
	QDateTime dtstart = QDateTime::currentDateTime();

	bool any_aborted = false;
	foreach(CLC7CalibrationThread *thread, threads)
	{
		bool aborted = false;
		do
		{
			
			if (dtstart.secsTo(QDateTime::currentDateTime()) >= 30)
			{
				foreach(CLC7CalibrationThread *thread, threads)
				{
					thread->terminate();
				}
				return false;
			}
			
			if (m_calibration_thread.isInterruptionRequested())
			{
				if (!aborted)
				{
					aborted = true;
					any_aborted = true;
					thread->abort();
				}
			}
		} while (!thread->wait(250));

		if (!aborted)
		{
			cps += thread->m_cps;
			extra_opencl_kernel_args[thread->m_jtrindex] = thread->m_extra_opencl_kernel_args;
		}

		delete thread;
	}

	if (cps == 0 || any_aborted)
	{
		return false;
	}

	return true;
}


void CLC7Calibrate::slot_createSpinnerWidget(bool mask, int row, int col)
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

	if (mask)
	{
		ui.calibrationMaskTableWidget->setCellWidget(row, col, widget);
	}
	else
	{
		ui.calibrationNoMaskTableWidget->setCellWidget(row, col, widget);
	}
	
	spinner->start();
}

void CLC7Calibrate::slot_removeSpinnerWidget(bool mask, int row, int col)
{
	if (mask)
	{
		ui.calibrationMaskTableWidget->removeCellWidget(row, col);
	}
	else
	{
		ui.calibrationNoMaskTableWidget->removeCellWidget(row, col);
	}
}

void CLC7Calibrate::slot_refreshTableWidget(bool mask)
{
	if (mask)
	{
		ui.calibrationMaskTableWidget->update();
	}
	else
	{ 
		ui.calibrationNoMaskTableWidget->update();
	}
}


void CLC7Calibrate::runCalibration(void)
{
	ILC7PasswordLinkage * passlink = GET_ILC7PASSWORDLINKAGE(g_pLinkage);
	ILC7ColorManager * colman = g_pLinkage->GetGUILinkage()->GetColorManager();
	
	struct CALCELL
	{
		bool mask;
		int row;
		int col;
		CLC7CalibrationHashType cht;
		CLC7CalibrationProcessorType cpt;
		QString algo;
		QString jtrversion;
	};
	
	if (!m_context_calibration)
	{
		m_calibration.clearCalibrationData();
	}

	QList<CALCELL> calcells;

	bool mask=true;
	do
	{
		mask = !mask;

		int row = -1;
		foreach(CLC7CalibrationHashType cht, m_calibration.hashTypes())
		{
			row++;
			if (m_context_calibration && !(m_context_calibration_mask == mask && m_context_cht == cht))
			{
				continue;
			}

			int col = -1;
			foreach(CLC7CalibrationProcessorType cpt, m_calibration.processorTypes())
			{
				col++;
				if (m_context_calibration && !(m_context_calibration_mask == mask && m_context_cpt == cpt))
				{
					continue;
				}

				QString algo;
				QString jtrversion;
				if (cpt.gpuplatform() == GPU_NONE)
				{
					algo = CLC7JTR::GetCPUNodeAlgorithm(cht.hashtype());
					jtrversion = cpt.cpuinstructionset();
				}
				else if (cpt.gpuplatform() == GPU_OPENCL)
				{
					algo = CLC7JTR::GetOpenCLNodeAlgorithm(cht.hashtype());
					jtrversion = "sse2";
				}
				//else if (cpt.gpuplatform() == GPU_CUDA)
				//{
				//	algo = CLC7JTR::GetCUDANodeAlgorithm(cht.hashtype());
				//	jtrversion = "sse2";
				//}

				// grey out cells that have no corresponding algorithm
				if (!algo.isEmpty())
				{
					CALCELL calcell;
					calcell.mask = mask;
					calcell.row = row;
					calcell.col = col;
					calcell.cht = cht;
					calcell.cpt = cpt;
					calcell.algo = algo;
					calcell.jtrversion = jtrversion;
					calcells.append(calcell);
				}
			}
		}

	} while (!mask);

	emit sig_setProgressBarRange(0, calcells.size());
		
	// Process all calibrations
	int calnum = 0;
	foreach(CALCELL calcell, calcells)
	{
		emit sig_setProgressBarValue(calnum);
		if (m_calibration_thread.isInterruptionRequested())
		{
			emit sig_setProgressBarValue(0);
			return;
		}


		// Add progress meter
		emit sig_createSpinnerWidget(calcell.mask, calcell.row, calcell.col);

		// Get calibration cps
		CLC7CalibrationData value;

		quint64 cps;
		QMap<int, QString> extra_opencl_kernel_args;
		if (!runCalibrationThreads(calcell.algo, calcell.cpt.gpuplatform(), calcell.jtrversion, calcell.mask, cps, extra_opencl_kernel_args))
		{
			if (!m_calibration_thread.isInterruptionRequested())
			{
				if (!g_pLinkage->GetGUILinkage()->YesNoBox("Calibration Failed",
					calcell.cpt.gpuplatform() == GPU_NONE ?
					"Calibration for this hash type failed.<br><br>Please report this event to support@l0phtcrack.com.<br><br>Do you wish to continue calibrating the other hash types?":
					"Calibration for this hash type failed. This is a GPU hash type, and your drivers may be out of date. Please ensure you are running the latest driver version, available here:<br><br>"
					"AMD: <a href=http://support.amd.com/en-us/download>http://support.amd.com/en-us/download</a><br>"
					"NVIDIA: <a href=http://www.nvidia.com/Download/index.aspx>http://www.nvidia.com/Download/index.aspx</a><br><br>"
					"Do you wish to continue calibrating the other hash types?"))
				{
					m_calibration_thread.requestInterruption();
				}
			}
			cps = 0;
		}
		
		if (!m_calibration_thread.isInterruptionRequested())
		{
			value.setCPS(cps);
			value.setExtraOpenCLKernelArgs(extra_opencl_kernel_args);

			value.setJTRKernel(calcell.algo);

			// Save cpt to cell
			m_calibration.setCalibrationData(calcell.mask, calcell.cht, calcell.cpt, value);
		}

		// Remove progress meter
		emit sig_removeSpinnerWidget(calcell.mask, calcell.row, calcell.col);

		// Set best so far for row
		CLC7CalibrationProcessorType bestcpt;
		quint64 bestcps = 0;
		bool foundbest = false;
		foreach(CLC7CalibrationProcessorType testcpt, m_calibration.processorTypes())
		{
			CLC7CalibrationData testcd;
			if (m_calibration.getCalibrationData(calcell.mask, calcell.cht, testcpt, testcd))
			{
				if (foundbest)
				{
					if (testcd.CPS() > bestcps)
					{
						bestcpt = testcpt;
						bestcps = testcd.CPS();
					}
				}
				else
				{
					bestcpt = testcpt;
					bestcps = testcd.CPS();
					foundbest = true;
				}
			}
		}
		if (foundbest)
		{
			m_calibration.setPreferredMethod(calcell.mask, calcell.cht, bestcpt);
		}

		// Refresh all cells in row
		int updcol = 0;
		foreach(CLC7CalibrationProcessorType updcpt, m_calibration.processorTypes())
		{
			refreshCell(calcell.mask, calcell.cht, updcpt, calcell.row, updcol);
			updcol++;
		}

		calnum++;
	}

	emit sig_setProgressBarValue(calnum);


	// If calibration completed, mark it as valid
	m_calibration.setValid(true);
}



//////////////////////////////////////////////////////////////////////////////////////////

CLC7CalibrationThread::CLC7CalibrationThread(QString algo, QString jtrversion, int jtrindex, bool mask, GPUPLATFORM platform, QString vendor)
{
	m_algo = algo;
	m_jtrversion = jtrversion;
	m_jtrindex = jtrindex;
	m_cps = 0;
	m_mask = mask;
	m_exejtr = NULL;
	m_platform = platform;
	m_vendor = vendor;
}

CLC7CalibrationThread::~CLC7CalibrationThread()
{

}

void CLC7CalibrationThread::abort(void)
{
	if (m_exejtr)
	{
		m_exejtr->Abort(false);
	}
}

void CLC7CalibrationThread::terminate(void)
{
	if (m_exejtr)
	{
		m_exejtr->Terminate();
	}
}

bool CLC7CalibrationThread::selftest(QString extra_kernel_args)
{
	bool passes_self_test = true;

	m_exejtr = new CLC7ExecuteJTR(m_jtrversion);

	QStringList args;
	args << "--test=0";

	args << QString("--format=%1").arg(m_algo);
	if (m_jtrindex != -1)
	{
		args << QString("--device=%1").arg(m_jtrindex);
	}
#ifdef _DEBUG
	args << "--verbosity=5";
#endif
	m_exejtr->SetCommandLine(args, extra_kernel_args);

	QString out, err;
	int retval = m_exejtr->ExecuteWait(out, err);

	if (retval != 0)
	{
#ifdef _DEBUG
		g_pLinkage->GetGUILinkage()->AppendToActivityLog("Args:" + args.join(" ") + "\n");
		g_pLinkage->GetGUILinkage()->AppendToActivityLog("Retval: " + QString::number(retval) + "\n");
		g_pLinkage->GetGUILinkage()->AppendToActivityLog("Out:\n" + out + "\n");
		g_pLinkage->GetGUILinkage()->AppendToActivityLog("Err:\n" + err + "\n");
#endif
		passes_self_test = false;
	}

	CLC7ExecuteJTR *exejtr = m_exejtr;
	m_exejtr = NULL;
	delete exejtr;

	return passes_self_test;
}

void CLC7CalibrationThread::run(void)
{
	//setPriority(QThread::HighPriority);

	// Run self tests without benchmark
	bool passes_self_test = false;
	QString extra_opencl_kernel_args;
	if (selftest(extra_opencl_kernel_args))
	{
		passes_self_test = true;
	}
	else if (m_platform == GPU_OPENCL && 
			(m_vendor.contains("Advanced Micro") || m_vendor.contains("AMD") || m_vendor.contains("ATI")))
	{
		extra_opencl_kernel_args = "-O1";
		if (selftest(extra_opencl_kernel_args))
		{
			passes_self_test = true;
		}
	}

	// Run benchmark without self tests
	if (passes_self_test)
	{
		m_exejtr = new CLC7ExecuteJTR(m_jtrversion);

		QStringList args;
		args << "--test=1";
		if (m_mask)
		{
			args << "--mask=?b?b?b?b";
		}
		args << "--skip-self-tests";

#ifdef _DEBUG
		args << "--verbosity=5";
#endif

		args << QString("--format=%1").arg(m_algo);
		if (m_jtrindex != -1)
		{
			args << QString("--device=%1").arg(m_jtrindex);
		}
		m_exejtr->SetCommandLine(args, extra_opencl_kernel_args);


		QString out, err;
		int retval = m_exejtr->ExecuteWait(out, err);

#ifdef _DEBUG
		if (retval != 0)
		{
			g_pLinkage->GetGUILinkage()->AppendToActivityLog("Args:" + args.join(" ") + "\n");
			g_pLinkage->GetGUILinkage()->AppendToActivityLog("Retval: " + QString::number(retval) + "\n");
			g_pLinkage->GetGUILinkage()->AppendToActivityLog("Out:\n" + out + "\n");
			g_pLinkage->GetGUILinkage()->AppendToActivityLog("Err:\n" + err + "\n");
		}
#endif

		if (retval == 0)
		{
			foreach(QString line, out.split("\n"))
			{
				line = line.trimmed();

				if (line.endsWith("c/s") && (line.contains("Raw:") || line.contains("Many salts:")))
				{
					QStringList parts = line.split(":");
					QString cpspart = parts.last().trimmed();
					if (cpspart.endsWith(" c/s"))
					{
						cpspart = cpspart.left(cpspart.length() - 4);
						quint64 cps = 1;
						if (cpspart.endsWith("K"))
						{
							cps = 1000;
							cpspart = cpspart.left(cpspart.length() - 1);
						}
						else if (cpspart.endsWith("M"))
						{
							cps = 1000000;
							cpspart = cpspart.left(cpspart.length() - 1);
						}
						else if (cpspart.endsWith("G"))
						{
							cps = 1000000000;
							cpspart = cpspart.left(cpspart.length() - 1);
						}
						bool ok = true;
						quint64 cpsnum;
						cpsnum = cpspart.toULongLong(&ok);
						if (!ok)
						{
							ok = true;
							double cpsnumdbl = cpspart.toDouble(&ok);
							if (!ok)
							{
								cps = 0;
							}
							else
							{
								cps = (quint64)((double)cps)*cpsnumdbl;
							}
						}
						else
						{
							cps *= cpsnum;
						}

						m_cps = cps;
						m_extra_opencl_kernel_args = extra_opencl_kernel_args;
					}
				}
			}
		}

		CLC7ExecuteJTR *exejtr = m_exejtr;
		m_exejtr = NULL;
		delete exejtr;
	}

}

//////////////////////////////////////////////////////////////////////////////////////////


CLC7CalibrationData::CLC7CalibrationData()
{
	m_jtrkernel="";
	m_cps=0;
}

CLC7CalibrationData::CLC7CalibrationData(const CLC7CalibrationData &copy)
{
	m_jtrkernel = copy.m_jtrkernel;
	m_cps = copy.m_cps;
	m_extra_opencl_kernel_args = copy.m_extra_opencl_kernel_args;
}

quint64 CLC7CalibrationData::CPS() const
{
	return m_cps;
}

void CLC7CalibrationData::setCPS(quint64 cps)
{
	m_cps = cps;
}

QString CLC7CalibrationData::JTRKernel() const
{
	return m_jtrkernel;
}

void CLC7CalibrationData::setJTRKernel(QString jtrkernel)
{
	m_jtrkernel = jtrkernel;
}

QMap<int, QString> CLC7CalibrationData::ExtraOpenCLKernelArgs() const
{
	return m_extra_opencl_kernel_args;
}

void CLC7CalibrationData::setExtraOpenCLKernelArgs(QMap<int, QString> extra_opencl_kernel_args)
{
	m_extra_opencl_kernel_args = extra_opencl_kernel_args;
}

QDataStream &operator<<(QDataStream &out, const CLC7CalibrationData &data)
{
	out << data.m_jtrkernel;
	out << data.m_cps;
	out << data.m_extra_opencl_kernel_args;

	return out;
}

QDataStream &operator>>(QDataStream &in, CLC7CalibrationData &data)
{
	in >> data.m_jtrkernel;
	in >> data.m_cps;
	in >> data.m_extra_opencl_kernel_args;

	return in;
}

//////////////////////////////////////////////////////////////////////////////////////////

CLC7CalibrationHashType::CLC7CalibrationHashType()
{
	m_hashtype = 0;
}

CLC7CalibrationHashType::CLC7CalibrationHashType(fourcc hashtype)
{
	m_hashtype = hashtype;
}

CLC7CalibrationHashType::CLC7CalibrationHashType(const CLC7CalibrationHashType & ht)
{
	m_hashtype = ht.m_hashtype;
}

bool CLC7CalibrationHashType::operator==(const CLC7CalibrationHashType & other) const
{
	return m_hashtype == other.m_hashtype;
}

bool CLC7CalibrationHashType::operator<(const CLC7CalibrationHashType & other) const
{
	return m_hashtype < other.m_hashtype;
}

fourcc CLC7CalibrationHashType::hashtype() const
{
	return m_hashtype;
}


QDataStream &operator<<(QDataStream &out, const CLC7CalibrationHashType &ht)
{
	out << ht.m_hashtype;
	return out;
}

QDataStream &operator>>(QDataStream &in, CLC7CalibrationHashType &ht)
{
	in >> ht.m_hashtype;
	return in;
}


//////////////////////////////////////////////////////////////////////////////////////////



CLC7CalibrationProcessorType::CLC7CalibrationProcessorType()
{
	m_gpuplatform = GPU_NONE;
	m_cpuinstructionset = "";
}

CLC7CalibrationProcessorType::CLC7CalibrationProcessorType(QString cpuinstructionset)
{
	m_gpuplatform = GPU_NONE;
	m_cpuinstructionset = cpuinstructionset;
}

CLC7CalibrationProcessorType::CLC7CalibrationProcessorType(GPUPLATFORM gpuplatform)
{
	m_gpuplatform = gpuplatform;
	m_cpuinstructionset = "";
}

CLC7CalibrationProcessorType::CLC7CalibrationProcessorType(const CLC7CalibrationProcessorType & copy)
{
	m_gpuplatform = copy.m_gpuplatform;
	m_cpuinstructionset = copy.m_cpuinstructionset;
}

bool CLC7CalibrationProcessorType::operator==(const CLC7CalibrationProcessorType & other) const
{
	return (m_gpuplatform == other.m_gpuplatform) &&
		(m_cpuinstructionset == other.m_cpuinstructionset);
}

bool CLC7CalibrationProcessorType::operator<(const CLC7CalibrationProcessorType & other) const
{
	if (m_gpuplatform < other.m_gpuplatform)
	{
		return true;
	}
	if (m_gpuplatform > other.m_gpuplatform)
	{
		return false;
	}
	return m_cpuinstructionset < other.m_cpuinstructionset;
}

GPUPLATFORM CLC7CalibrationProcessorType::gpuplatform() const
{
	return m_gpuplatform;
}

QString CLC7CalibrationProcessorType::cpuinstructionset() const
{
	return m_cpuinstructionset;
}


QDataStream &operator<<(QDataStream &out, const CLC7CalibrationProcessorType &pt)
{
	out << pt.m_gpuplatform;
	out << pt.m_cpuinstructionset;
	return out;
}

QDataStream &operator>>(QDataStream &in, CLC7CalibrationProcessorType &pt)
{
	in >> (int&)pt.m_gpuplatform;
	in >> pt.m_cpuinstructionset;
	return in;
}

//////////////////////////////////////////////////////////////////////////////////////////


CLC7Calibration::CLC7CalibrationKey::CLC7CalibrationKey()
{
}

CLC7Calibration::CLC7CalibrationKey::CLC7CalibrationKey(bool mask, const CLC7CalibrationHashType &hashtype, const CLC7CalibrationProcessorType &proctype)
{
	m_mask = mask;
	m_hashtype = hashtype;
	m_proctype = proctype;
}

CLC7Calibration::CLC7CalibrationKey::CLC7CalibrationKey(const CLC7CalibrationKey & copy)
{
	m_mask = copy.m_mask;
	m_hashtype = copy.m_hashtype;
	m_proctype = copy.m_proctype;
}

bool CLC7Calibration::CLC7CalibrationKey::operator<(const CLC7Calibration::CLC7CalibrationKey & other) const
{
	if (m_mask == false && other.m_mask == true)
	{
		return true;
	}
	if (m_mask == true && other.m_mask == false)
	{
		return false;
	}
	if (m_hashtype < other.m_hashtype)
	{
		return true;
	}
	if (other.m_hashtype < m_hashtype)
	{
		return false;
	}
	return m_proctype < other.m_proctype;
}

QDataStream &operator<<(QDataStream &out, const CLC7Calibration::CLC7CalibrationKey &key)
{
	out << key.m_mask;
	out << key.m_hashtype;
	out << key.m_proctype;
	return out;
}

QDataStream &operator>>(QDataStream &in, CLC7Calibration::CLC7CalibrationKey &key)
{
	in >> key.m_mask;
	in >> key.m_hashtype;
	in >> key.m_proctype;
	return in;
}


//////////////////////////////////////////////////////////////////////////////////////////



CLC7Calibration::CLC7Calibration()
{
	m_cpu_thread_count = 0;
	m_is_valid = false;
}

bool CLC7Calibration::isValid() const
{
	return m_is_valid;
}

void CLC7Calibration::setValid(bool valid)
{
	m_is_valid = valid;
}


// Configuration
bool CLC7Calibration::configurationMatch(const CLC7Calibration & other) const
{
	// Compare everything but data, and ensure everything is valid
	return
		(m_available_cputypes == other.m_available_cputypes) &&
		(m_cpu_thread_count == other.m_cpu_thread_count) &&
		(m_gpuinfo == other.m_gpuinfo) &&
		(m_hashtypes == other.m_hashtypes) &&
		(m_proctypes == other.m_proctypes)
		;
}

QList<CLC7CalibrationHashType> CLC7Calibration::hashTypes() const
{
	return m_hashtypes;
}

QList<CLC7CalibrationProcessorType> CLC7Calibration::processorTypes() const
{
	return m_proctypes;
}

QStringList CLC7Calibration::availableCPUTypes() const
{
	return m_available_cputypes;
}

int CLC7Calibration::CPUThreadCount() const
{
	return m_cpu_thread_count;
}

QVector<GPUINFO> CLC7Calibration::GPUInfo() const
{
	return m_gpuinfo;
}

void CLC7Calibration::addHashType(const CLC7CalibrationHashType & hashtype)
{
	if (m_hashtypes.contains(hashtype))
	{
		Q_ASSERT(0);
		return;
	}
	m_hashtypes.prepend(hashtype);
}

void CLC7Calibration::addProcessorType(const CLC7CalibrationProcessorType & proctype)
{
	if (m_proctypes.contains(proctype))
	{
		Q_ASSERT(0);
		return;
	}

	m_proctypes.append(proctype);
}

void CLC7Calibration::setAvailableCPUTypes(QStringList available_cputypes)
{
	m_available_cputypes = available_cputypes;
}

void CLC7Calibration::setCPUThreadCount(int cputhreadcount)
{
	m_cpu_thread_count = cputhreadcount;
}

void CLC7Calibration::setGPUInfo(QVector<GPUINFO> gpuinfo)
{
	m_gpuinfo = gpuinfo;
}

// Calibration data
void CLC7Calibration::clearCalibrationData(void)
{
	m_is_valid = false;
	m_calibrationdata.clear();
	m_preferred_methods[0].clear();
	m_preferred_methods[1].clear();
}

void CLC7Calibration::setCalibrationData(bool mask, const CLC7CalibrationHashType & hashtype, const CLC7CalibrationProcessorType &proctype, const CLC7CalibrationData &value)
{
	CLC7CalibrationKey key(mask, hashtype, proctype);
	m_calibrationdata[key] = value;
}

bool CLC7Calibration::hasCalibrationData(bool mask, const CLC7CalibrationHashType & hashtype, const CLC7CalibrationProcessorType &proctype) const
{
	CLC7CalibrationKey key(mask, hashtype, proctype);
	return m_calibrationdata.contains(key);
}

bool CLC7Calibration::getCalibrationData(bool mask, const CLC7CalibrationHashType & hashtype, const CLC7CalibrationProcessorType &proctype, CLC7CalibrationData & value) const
{
	CLC7CalibrationKey key(mask, hashtype, proctype);
	if (!m_calibrationdata.contains(key))
	{
		return false;
	}

	value = m_calibrationdata[key];
	return true;
}

// Selected method
void CLC7Calibration::setPreferredMethod(bool mask, const CLC7CalibrationHashType & hashtype, const CLC7CalibrationProcessorType &proctype)
{
	m_preferred_methods[mask?1:0][hashtype] = proctype;
}

bool CLC7Calibration::getPreferredMethod(bool mask, const CLC7CalibrationHashType & hashtype, CLC7CalibrationProcessorType &proctype) const
{
	if (!m_preferred_methods[mask ? 1 : 0].contains(hashtype))
		return false;

	proctype = m_preferred_methods[mask ? 1 : 0][hashtype];
	return true;
}

void CLC7Calibration::saveCalibration(const CLC7Calibration &cal)
{
	// Only save valid calibrations
	if (!cal.isValid())
	{
		Q_ASSERT(0);
		return;
	}

	// See if loaded calibration meets current system configuration requirements
	ILC7Settings *settings = g_pLinkage->GetSettings();

#if PLATFORM == PLATFORM_WIN32
	QString calkey = QString("%1:calibration_win32").arg(UUID_LC7JTRPLUGIN.toString());
#elif PLATFORM == PLATFORM_WIN64
	QString calkey = QString("%1:calibration_win64").arg(UUID_LC7JTRPLUGIN.toString());
#else
#error "key plz"
#endif

	QByteArray calba;
	QDataStream calds(&calba, QIODevice::WriteOnly);;
	calds << cal;

	settings->setValue(calkey, calba);
	settings->sync();
}

void CLC7Calibration::loadCalibration(CLC7Calibration & cal)
{
	// See if loaded calibration meets current system configuration requirements
	ILC7Settings *settings = g_pLinkage->GetSettings();

#if PLATFORM == PLATFORM_WIN32
	QString calkey = QString("%1:calibration_win32").arg(UUID_LC7JTRPLUGIN.toString());
#elif PLATFORM == PLATFORM_WIN64
	QString calkey = QString("%1:calibration_win64").arg(UUID_LC7JTRPLUGIN.toString());
#else
#error "key plz"
#endif
	if (settings->contains(calkey))
	{
		QByteArray calba;
		calba = settings->value(calkey).toByteArray();

		QDataStream calds(calba);

		calds >> cal;
	}
}

void CLC7Calibration::emptyCalibration(CLC7Calibration & cal)
{
	cal = CLC7Calibration();

	ILC7CPUInformation *cpuinfo = g_pLinkage->GetCPUInformation();

	CLC7GPUInfo gpuinfo;
	gpuinfo.Detect();

	int corecount = g_pLinkage->GetSettings()->value(QString("%1:cpucount").arg(UUID_LC7JTRPLUGIN.toString()), cpuinfo->CoreCount()).toInt();
	QStringList suppins = CLC7JTR::GetSupportedInstructionSets();

	// Configuration
	cal.setGPUInfo(gpuinfo.GetGPUInfo());
	cal.setCPUThreadCount(corecount);
	cal.setAvailableCPUTypes(suppins);
	
	// Add hashes
	ILC7PasswordLinkage *passlink = GET_ILC7PASSWORDLINKAGE(g_pLinkage);
	QList<fourcc> accttypes = passlink->List();
	foreach(fourcc fcc, accttypes)
	{
		LC7HashType lc7hashtype;
		QString error;
		if (!passlink->Lookup(fcc, lc7hashtype, error))
		{
			Q_ASSERT(0);
			continue;
		}

		// Add only the ones supported by JtR that also have importers
		if (lc7hashtype.registrants["technique"].contains(UUID_LC7JTRPLUGIN) && lc7hashtype.registrants["import"].size() > 0)
		{
			cal.addHashType(fcc);
		}
	}

	// Add CPUs
	foreach(QString cputype, suppins)
	{
		cal.addProcessorType(CLC7CalibrationProcessorType(cputype));
	}

	// Add GPUs
	bool has_opencl = false;
	//bool has_cuda = false;
	foreach(GPUINFO gi, gpuinfo.GetGPUInfo())
	{
		QString platform;
		if (gi.platform == GPU_OPENCL)
		{
			platform = "OpenCL";
		}
		//else if (gi.platform == GPU_CUDA)
		//{
		//	platform = "CUDA";
		//}

		bool enabled = g_pLinkage->GetSettings()->value(UUID_LC7JTRPLUGIN.toString() + QString(":enablegpu_%1_%2").arg(platform).arg(gi.jtrindex), true).toBool();
		if (enabled)
		{
			if (gi.platform == GPU_OPENCL)
			{
				has_opencl = true;
			}

			//if (gi.platform == GPU_CUDA)
			//{
			//	has_cuda = true;
			//}
		}
	}

	if (has_opencl)
	{
		cal.addProcessorType(GPU_OPENCL);
	}
	//if (has_cuda)
	//{
	//	cal.addProcessorType(GPU_CUDA);
	//}
}


bool CLC7Calibration::GetHashTypePreferredProcessor(bool mask, fourcc hashtype, GPUPLATFORM & gpuplatform, QString & cpuinstructionset, QString &jtrkernel, QMap<int, QString> & extra_opencl_kernel_args) const
{
	if (!isValid())
	{
		return false;
	}

	CLC7CalibrationProcessorType cpt;
	if (!getPreferredMethod(mask, hashtype, cpt))
	{
		return false;
	}

	CLC7CalibrationData value;
	if (!getCalibrationData(mask, hashtype, cpt, value))
	{
		return false;
	}

	gpuplatform = cpt.gpuplatform();
	cpuinstructionset = cpt.cpuinstructionset();
	jtrkernel = value.JTRKernel();
	extra_opencl_kernel_args = value.ExtraOpenCLKernelArgs();

	return true;
}


// Serialization
QDataStream &operator<<(QDataStream &out, const CLC7Calibration &cal)
{
	quint32 version = 2;
	out << version;

	out << cal.m_is_valid;

	out << cal.m_hashtypes;
	out << cal.m_proctypes;
	out << cal.m_calibrationdata;
	out << cal.m_preferred_methods[0];
	out << cal.m_preferred_methods[1];

	out << cal.m_available_cputypes;
	out << cal.m_cpu_thread_count;
	out << cal.m_gpuinfo;

	return out;
}

QDataStream &operator>>(QDataStream &in, CLC7Calibration &cal)
{
	quint32 version;
	in >> version;
	if (version == 1)
	{
		in.setStatus(QDataStream::ReadCorruptData);
		return in;
	}
	if (version > 2)
	{
		in.setStatus(QDataStream::ReadCorruptData);
		return in;
	}

	in >> cal.m_is_valid;

	in >> cal.m_hashtypes;
	in >> cal.m_proctypes;
	in >> cal.m_calibrationdata;
	in >> cal.m_preferred_methods[0];
	in >> cal.m_preferred_methods[1];

	in >> cal.m_available_cputypes;
	in >> cal.m_cpu_thread_count;
	in >> cal.m_gpuinfo;

	return in;
}

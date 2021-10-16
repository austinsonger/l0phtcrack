#include<stdafx.h>

CLC7WorkQueueWidget::CLC7WorkQueueWidget(QWidget *parent, ILC7Controller *ctrl) :QWidget(parent)
{
	TR;
	m_cpuprog = NULL;
	m_gpuprog = NULL;
	m_thermal = NULL;

	m_ctrl = ctrl;
	m_is_new_line = true;

	ui.setupUi(this);

	CLC7App *theapp = CLC7App::getInstance();
	LC7Main *mainwin = theapp->GetMainWindow();

	setWindowFlags(windowFlags() & ~Qt::WindowCloseButtonHint);

	ui.totalCompletionProgress->setRange(0, 100);
	ui.totalCompletionProgress->setValue(0);

	ui.currentCompletionProgress->setRange(0, 100);
	ui.currentCompletionProgress->setValue(0);

	ui.statusText->setStyleSheet("* { font: 10pt \"Courier\"; }\np { margin: 0; padding: 0; }");
	ui.statusBarText->setStyleSheet("font: 10pt \"Courier\"");

	connect(ui.pauseButton, SIGNAL(clicked()), this, SLOT(slot_pauseButton()));
	connect(ui.stopButton, SIGNAL(clicked()), this, SLOT(slot_stopButton()));

	m_ctrl->RegisterNotifySessionActivity(SINGLE_WORKQUEUE_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CLC7WorkQueueWidget::NotifySessionActivity);
	m_ctrl->RegisterNotifySessionActivity(BATCH_WORKQUEUE_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CLC7WorkQueueWidget::NotifySessionActivity);

	connect(this, SIGNAL(sig_updateCurrentActivity(QString)), this, SLOT(slot_updateCurrentActivity(QString)));
	connect(this, SIGNAL(sig_scrollToBottom()), this, SLOT(slot_scrollToBottom()));
	connect(this, SIGNAL(sig_clearActivityLog()), this, SLOT(slot_clearActivityLog()));
	connect(this, SIGNAL(sig_setStatusText(QString)), this, SLOT(slot_setStatusText(QString)));
	connect(this, SIGNAL(sig_updateCurrentProgressBar(quint32)), this, SLOT(slot_updateCurrentProgressBar(quint32)));
	connect(this, SIGNAL(sig_updateTotalProgressBar(quint32)), this, SLOT(slot_updateTotalProgressBar(quint32)));

	m_waitingMovie = NULL;

	m_batch_workqueue = NULL;
	m_single_workqueue = NULL;

	ui.stopButton->setEnabled(false);
	ui.pauseButton->setEnabled(false);

	m_status_document = new QTextDocument(NULL);
	ui.statusText->setDocument(m_status_document);

	m_active_workqueue = 0;

	m_append_to_activity_log_later_timer.setInterval(1000);
	m_append_to_activity_log_later_timer.start();
	connect(&m_append_to_activity_log_later_timer, &QTimer::timeout, this, &CLC7WorkQueueWidget::slot_append_to_activity_log_later);
	m_append_to_activity_log_later = false;

	// Set up system monitor
	QTimer::singleShot(0, this, &CLC7WorkQueueWidget::slot_createSystemMonitor);

	connect(&m_sysmon_animate_thread, &QThread::started, this, &CLC7WorkQueueWidget::slot_animateSystemMonitor, Qt::DirectConnection);
}
	


CLC7WorkQueueWidget::~CLC7WorkQueueWidget()
{
	TR;
	Q_ASSERT(!m_sysmon_animate_thread.isRunning());

	delete m_status_document;
	delete m_waitingMovie;
	m_ctrl->UnregisterNotifySessionActivity(SINGLE_WORKQUEUE_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CLC7WorkQueueWidget::NotifySessionActivity);
	m_ctrl->UnregisterNotifySessionActivity(BATCH_WORKQUEUE_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CLC7WorkQueueWidget::NotifySessionActivity);
}

void CLC7WorkQueueWidget::Shutdown()
{
	m_sysmon_animate_thread.requestInterruption();
	m_sysmon_animate_thread.wait();
}

ILC7Interface *CLC7WorkQueueWidget::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7WorkQueueWidget")
	{
		return this;
	}
	return NULL;
}

void CLC7WorkQueueWidget::slot_animateSystemMonitor(void)
{
	int cnt = 0;

	while (!m_sysmon_animate_thread.isInterruptionRequested())
	{
		cnt++;
		if (cnt == 10)
		{
			UpdateSystemMonitor();
			cnt = 0;
		}

		qint64 newtime = m_sysmon_elapsed.elapsed();

		foreach(const PROGRESSCTX &ctx, m_cpu_progressbars)
		{
			qint32 newval;
			if (newtime >= ctx.newtime)
			{
				newval = ctx.newval;
			}
			else
			{
				if ((ctx.newtime - ctx.oldtime) == 0)
				{
					newval = ctx.oldval;
				}
				else
				{
					newval = ctx.oldval + (ctx.newval - ctx.oldval)*(newtime - ctx.oldtime) / (ctx.newtime - ctx.oldtime);
				}
			}

			ctx.widget->setBarValue(ctx.barid, newval);
		}

		foreach(const PROGRESSCTX &ctx, m_gpu_progressbars)
		{
			qint32 newval;
			if (newtime >= ctx.newtime)
			{
				newval = ctx.newval;
			}
			else
			{
				if ((ctx.newtime - ctx.oldtime) == 0)
				{
					newval = ctx.oldval;
				}
				else
				{
					newval = ctx.oldval + (ctx.newval - ctx.oldval)*(newtime - ctx.oldtime) / (ctx.newtime - ctx.oldtime);
				}
			}

			ctx.widget->setBarValue(ctx.barid, newval);
		}

		QThread::msleep(100);
	}

	m_sysmon_animate_thread.quit();

}

void CLC7WorkQueueWidget::slot_createSystemMonitor(void)
{
	CreateSystemMonitor();
}

#define FONT_SIZE 13
#define FONTTEXT(x) QString("<span style=\"font-size:%1px\">%2</span>").arg(m_colman->GetSizeRatio() * FONT_SIZE).arg(x)

static QColor lightnessrotate(int n, int d, QColor col)
{
	int h = col.hue();
	int s = col.hslSaturation();
	int l = col.lightness();

	l = ((l + (255 * n / d)) % 255);

	return QColor::fromHsl(h, s, l);
}

void CLC7WorkQueueWidget::CreateSystemMonitor(void)
{
	m_ctrl->GetThermalWatchdog()->RegisterNotifyThermalTransition(this, (void (QObject::*)(ILC7ThermalWatchdog::THERMAL_STATE, ILC7ThermalWatchdog::THERMAL_STATE))&CLC7WorkQueueWidget::NotifyThermalTransition);

	ILC7ColorManager *colman = m_ctrl->GetGUILinkage()->GetColorManager();
	ILC7SystemMonitor *sysmon = m_ctrl->GetSystemMonitor();
	if (colman == NULL || sysmon == NULL)
	{
		return;
	}

	QString errormessage, errortitle;
	if (sysmon->GetErrorMessage(errortitle, errormessage))
	{
		m_ctrl->GetGUILinkage()->WarningMessage(errortitle, errormessage);
	}

	QString error;

	// Add thermal label
	QLabel *templabel = new QLabel(this);
	templabel->setText("Thermal Monitor: ");
	ui.currentOperationLayout->addWidget(templabel);

	m_thermal = new QLabel(this);
	m_thermal->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
	ui.currentOperationLayout->addWidget(m_thermal);
	UpdateThermalState();
	connect(this, &CLC7WorkQueueWidget::sig_updateThermalState, this, &CLC7WorkQueueWidget::slot_updateThermalState, Qt::QueuedConnection);
	
	// Add cpu/gpu bars
	if (m_cpuprog == NULL)
	{
		QList<ILC7SystemMonitor::CPU_STATUS> cpustats;
		if (sysmon->GetAllCPUStatus(cpustats, error) && cpustats.size() > 0)
		{
			QLabel *cpulabel = new QLabel(this);
			cpulabel->setText("CPU Utilization: ");
			ui.currentOperationLayout->addWidget(cpulabel);

			m_cpuprog = new CAnimatedBarChart(this);
			//m_cpuprog->setHeaderText("CPU Utilization");
			//m_cpuprog->setHeaderTextAlignment(Qt::AlignRight | Qt::AlignBottom);
			//m_cpuprog->setHeaderTextOrientation(Qt::Horizontal);

			for (int cpu = 0; cpu < cpustats.size(); cpu++)
			{
				QString barid = QString("%1").arg(cpu);
				m_cpuprog->addBar(barid);
				m_cpuprog->setBarText(barid, barid);
				m_cpuprog->setBarTextAlignment(barid, Qt::AlignBottom | Qt::AlignHCenter);

				PROGRESSCTX ctx;
				ctx.widget = m_cpuprog;
				ctx.barid = barid;
				ctx.oldtime = ctx.newtime = m_sysmon_elapsed.elapsed();
				ctx.oldval = ctx.newval = 0;

				m_cpu_progressbars.append(ctx);
			}

			ui.currentOperationLayout->addWidget(m_cpuprog);
		}
		else
		{
			m_ctrl->GetGUILinkage()->GetWorkQueueWidget()->AppendToActivityLog("Couldn't access WMI to get CPU statistics. Enable the 'Windows Instrumentation Management' service to get CPU utilization information.");
		}
	}

	if (m_gpuprog == NULL)
	{
		QList<ILC7SystemMonitor::GPU_STATUS> gpustats;
		if (sysmon->GetAllGPUStatus(gpustats, error) && gpustats.size() > 0)
		{
			QLabel *gpulabel = new QLabel(this);
			gpulabel->setText("GPU Util/Temp/Fan: ");
			ui.currentOperationLayout->addWidget(gpulabel);

			m_gpuprog = new CAnimatedBarChart(this);

			for (int gpu = 0; gpu < gpustats.size(); gpu++)
			{
				// Utilization
				{
					QString barid = QString("%1 Util").arg(gpu);
					m_gpuprog->addBar(barid);
					m_gpuprog->setBarText(barid, barid);
					m_gpuprog->setBarTextAlignment(barid, Qt::AlignBottom | Qt::AlignHCenter);

					PROGRESSCTX ctx;
					ctx.widget = m_gpuprog;
					ctx.barid = barid;
					ctx.oldtime = ctx.newtime = m_sysmon_elapsed.elapsed();
					ctx.oldval = ctx.newval = 0;

					m_gpu_progressbars.append(ctx);
				}

				// Temperature
				{
					QString barid = QString("%1 Temp").arg(gpu);
					m_gpuprog->addBar(barid);
					m_gpuprog->setBarText(barid, barid);
					m_gpuprog->setBarTextAlignment(barid, Qt::AlignBottom | Qt::AlignHCenter);

					PROGRESSCTX ctx;
					ctx.widget = m_gpuprog;
					ctx.barid = barid;
					ctx.oldtime = ctx.newtime = m_sysmon_elapsed.elapsed();
					ctx.oldval = ctx.newval = 0;

					m_gpu_progressbars.append(ctx);
				}

				// Fanspeed
				{
					QString barid = QString("%1 RPM").arg(gpu);
					m_gpuprog->addBar(barid);
					m_gpuprog->setBarText(barid, barid);
					m_gpuprog->setBarTextAlignment(barid, Qt::AlignBottom | Qt::AlignHCenter);

					PROGRESSCTX ctx;
					ctx.widget = m_gpuprog;
					ctx.barid = barid;
					ctx.oldtime = ctx.newtime = m_sysmon_elapsed.elapsed();
					ctx.oldval = ctx.newval = 0;

					m_gpu_progressbars.append(ctx);
				}

			}
			ui.currentOperationLayout->addWidget(m_gpuprog);
		}
	}

	RecolorCallback();

	m_sysmon_elapsed.start();
	m_sysmon_animate_thread.start();

}

void CLC7WorkQueueWidget::slot_updateThermalState(void)
{
	UpdateThermalState();
}

void CLC7WorkQueueWidget::UpdateThermalState(void)
{
	ILC7ThermalWatchdog::THERMAL_STATE thermal_state = m_ctrl->GetThermalWatchdog()->thermalState();
	if (thermal_state == ILC7ThermalWatchdog::COOL)
	{
		m_thermal->setText("COOL");
		m_thermal->setStyleSheet("border: 1px solid black; background-color: #2b9bf1; color: black;");
	}
	else if (thermal_state == ILC7ThermalWatchdog::WARM)
	{
		m_thermal->setText("WARM");
		m_thermal->setStyleSheet("border: 1px solid black; background-color: #deb927; color: black;");
	}
	else if (thermal_state == ILC7ThermalWatchdog::HOT)
	{
		m_thermal->setText("HOT");
		m_thermal->setStyleSheet("border: 1px solid black; background-color: #d02525; color: black;");
	}
}

void CLC7WorkQueueWidget::UpdateSystemMonitor(void)
{
	ILC7ColorManager *colman = m_ctrl->GetGUILinkage()->GetColorManager();
	ILC7SystemMonitor *sysmon = m_ctrl->GetSystemMonitor();
	if (sysmon == NULL || colman == NULL)
	{
		return;
	}

	QString error;
	
	if (m_cpuprog != NULL)
	{
		QList<ILC7SystemMonitor::CPU_STATUS> cpustats;
		if (sysmon->GetAllCPUStatus(cpustats, error) && cpustats.size() > 0)
		{
			for (int cpu = 0; cpu < cpustats.size(); cpu++)
			{
				m_cpu_progressbars[cpu].oldtime = m_cpu_progressbars[cpu].newtime;
				m_cpu_progressbars[cpu].oldval = m_cpu_progressbars[cpu].newval;
				m_cpu_progressbars[cpu].newtime = m_sysmon_elapsed.elapsed() + 1000;
				m_cpu_progressbars[cpu].newval = cpustats[cpu].utilization;

				QString barid = m_cpu_progressbars[cpu].barid;
				if (cpustats[cpu].current_mhz == cpustats[cpu].max_mhz)
				{
					m_cpuprog->setBarToolTip(barid, QString("CPU #%1, %2MHz, %3%").arg(cpu).arg(cpustats[cpu].max_mhz).arg(cpustats[cpu].utilization));
				}
				else
				{
					m_cpuprog->setBarToolTip(barid, QString("CPU #%1, %2MHz/%3MHz, %4%").arg(cpu).arg(cpustats[cpu].current_mhz).arg(cpustats[cpu].max_mhz).arg(cpustats[cpu].utilization));
				}
			}
		}
	}

	if (m_gpuprog != NULL)
	{
		QList<ILC7SystemMonitor::GPU_STATUS> gpustats;
		if (sysmon->GetAllGPUStatus(gpustats, error) && gpustats.size() > 0)
		{
			for (int gpu = 0; gpu < gpustats.size(); gpu++)
			{
				for (int bar = 0; bar < 3; bar++)
				{
					int gpubar = (gpu * 3 + bar);

					m_gpu_progressbars[gpubar].oldtime = m_gpu_progressbars[gpubar].newtime;
					m_gpu_progressbars[gpubar].oldval = m_gpu_progressbars[gpubar].newval;
					m_gpu_progressbars[gpubar].newtime = m_sysmon_elapsed.elapsed() + 1000;

					QString barid = m_gpu_progressbars[gpubar].barid;
					if (bar == 0)
					{
						m_gpu_progressbars[gpubar].newval = gpustats[gpu].utilization;
						m_gpuprog->setBarToolTip(barid, QString("%1 GPU #%2 (%3) %4%")
							.arg(gpustats[gpu].gpu_type)
							.arg(gpu)
							.arg(gpustats[gpu].gpu_name)
							.arg(gpustats[gpu].utilization));
					}
					else if (bar == 1)
					{
						m_gpu_progressbars[gpubar].newval = gpustats[gpu].temperature;
						m_gpuprog->setBarToolTip(barid, QString("%1 GPU #%2 (%3) %4C")
							.arg(gpustats[gpu].gpu_type)
							.arg(gpu)
							.arg(gpustats[gpu].gpu_name)
							.arg(gpustats[gpu].temperature));
					}
					else if (bar == 2)
					{
						m_gpu_progressbars[gpubar].newval = gpustats[gpu].fanspeed;
						m_gpuprog->setBarToolTip(barid, QString("%1 GPU #%2 (%3) %4RPM")
						.arg(gpustats[gpu].gpu_type)
						.arg(gpu)
						.arg(gpustats[gpu].gpu_name)
						.arg(gpustats[gpu].fanspeed_rpm));
					}
				}
			}
		}
	}
}


void CLC7WorkQueueWidget::Decorate(void)
{
	TR;
	ILC7ColorManager *colman = CLC7App::getInstance()->GetMainWindow()->GetColorManager();
	colman->RegisterRecolorCallback(this, (void(QObject::*)(void))&CLC7WorkQueueWidget::RecolorCallback);
	RecolorCallback();
}

void CLC7WorkQueueWidget::RecolorCallback(void)
{
	TR;
	ILC7ColorManager *colman = CLC7App::getInstance()->GetMainWindow()->GetColorManager();
	if (colman == NULL)
	{
		return;
	}

	QString disabled = colman->GetBaseShade("TEXT_DISABLED");
	QString highlight = colman->GetHighlightShade("HIGHLIGHT_BKGD_0");
	QString normal = colman->GetTextColor();

	if (m_waitingMovie)
	{
		delete m_waitingMovie;
	}
	if (colman->GetSizeRatio() != 1)
	{
		//		m_waitingMovie = new QMovie(":/lc7/queueimages/in_progress.mng", QByteArray("mng"), this);
		m_waitingMovie = new QMovie(":/lc7/queueimages/in_progress2.mng", QByteArray("mng"), this);
		//		m_waitingMovie->setScaledSize(QSize(64, 64));
	}
	else
	{
		m_waitingMovie = new QMovie(":/lc7/queueimages/in_progress.mng", QByteArray("mng"), this);
		//		m_waitingMovie->setScaledSize(QSize(32, 32));
	}

	if (m_cpuprog)
	{
		m_cpuprog->setDPIScale(colman->GetSizeRatio());
		m_cpuprog->setBorderColor(QColor(colman->GetBaseShade("BORDER_COLOR")), 0);
		m_cpuprog->setBorderColor(QColor(colman->GetBaseShade("BORDER_COLOR")), 1);
		m_cpuprog->setBackgroundColor(QColor(colman->GetBaseShade("CONTROL_BKGD_DISABLED")), 0);
		m_cpuprog->setBackgroundColor(QColor(colman->GetBaseShade("CONTROL_BKGD")), 1);
		m_cpuprog->setHeaderTextColor(QColor(colman->GetBaseShade("TEXT_DISABLED")), 0);
		m_cpuprog->setHeaderTextColor(QColor(colman->GetBaseShade("TEXT_COLOR")), 1);
		m_cpuprog->setBorderWidth(1);
		m_cpuprog->setBorderGap(1);
		m_cpuprog->setBarWidth(QString(), 16);
		m_cpuprog->setBarGap(QString(), 2);
		m_cpuprog->setBarColor(QString(), colman->GetBaseColor(), 0);
		m_cpuprog->setBarColor(QString(), colman->GetHighlightColor(), 1);
		m_cpuprog->setBarTextColor(QString(), QColor(colman->GetBaseShade("TEXT_DISABLED")), 0);
		m_cpuprog->setBarTextColor(QString(), QColor(colman->GetBaseShade("TEXT_COLOR")), 1);
	}

	if (m_gpuprog)
	{
		m_gpuprog->setDPIScale(colman->GetSizeRatio());
		m_gpuprog->setBorderColor(QColor(colman->GetBaseShade("BORDER_COLOR")), 0);
		m_gpuprog->setBorderColor(QColor(colman->GetBaseShade("BORDER_COLOR")), 1);
		m_gpuprog->setBackgroundColor(QColor(colman->GetBaseShade("CONTROL_BKGD_DISABLED")), 0);
		m_gpuprog->setBackgroundColor(QColor(colman->GetBaseShade("CONTROL_BKGD")), 1);
		m_gpuprog->setHeaderTextColor(QColor(colman->GetBaseShade("TEXT_DISABLED")), 0);
		m_gpuprog->setHeaderTextColor(QColor(colman->GetBaseShade("TEXT_COLOR")), 1);
		m_gpuprog->setBorderWidth(1);
		m_gpuprog->setBorderGap(1);
		m_gpuprog->setBarWidth(QString(), 12);
		m_gpuprog->setBarGap(QString(), 2);
		m_gpuprog->setBarColor(QString(), colman->GetBaseColor(), 0);
		m_gpuprog->setBarColor(QString(), colman->GetHighlightColor(), 1);
		m_gpuprog->setBarTextColor(QString(), QColor(colman->GetBaseShade("TEXT_DISABLED")), 0);
		m_gpuprog->setBarTextColor(QString(), QColor(colman->GetBaseShade("TEXT_COLOR")), 1);

		int bartype = 0;
		foreach(PROGRESSCTX ctx, m_gpu_progressbars)
		{
			if (bartype == 0)
			{
				m_gpuprog->setBarColor(ctx.barid, colman->GetBaseColor(), 0);
				m_gpuprog->setBarColor(ctx.barid, colman->GetHighlightColor(), 1);
			}
			else if (bartype == 1)
			{
				m_gpuprog->setBarColor(ctx.barid, lightnessrotate(1, 3, colman->GetBaseColor()), 0);
				m_gpuprog->setBarColor(ctx.barid, lightnessrotate(1, 3, colman->GetHighlightColor()), 1);
			}
			else if (bartype == 2)
			{
				m_gpuprog->setBarColor(ctx.barid, lightnessrotate(2, 3, colman->GetBaseColor()), 0);
				m_gpuprog->setBarColor(ctx.barid, lightnessrotate(2, 3, colman->GetHighlightColor()), 1);
			}
			bartype = (bartype + 1) % 3;
		}
	}


	UpdateUI();
}


void CLC7WorkQueueWidget::NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler)
{
	TR;
	switch (activity)
	{
	case ILC7Linkage::SESSION_OPEN_POST:
	case ILC7Linkage::SESSION_NEW_POST:
		if (handler && handler->GetId() == BATCH_WORKQUEUE_HANDLER_ID)
		{
			m_batch_workqueue = (ILC7WorkQueue *)handler;
			m_batch_workqueue->AddQueueChangedListener(this, (void (QObject::*)(void))&CLC7WorkQueueWidget::onBatchWorkQueueChanged);
			if (m_batch_workqueue->GetWorkQueueState() != ILC7WorkQueue::UNVALIDATED)
			{
				onBatchWorkQueueChanged();
			}
		}
		if (handler && handler->GetId() == SINGLE_WORKQUEUE_HANDLER_ID)
		{
			m_single_workqueue = (ILC7WorkQueue *)handler;
			m_single_workqueue->AddQueueChangedListener(this, (void (QObject::*)(void))&CLC7WorkQueueWidget::onSingleWorkQueueChanged);
			if (m_single_workqueue->GetWorkQueueState() != ILC7WorkQueue::UNVALIDATED)
			{
				onSingleWorkQueueChanged();
			}
		}
		break;
	case ILC7Linkage::SESSION_CLOSE_PRE:
		if (handler && handler->GetId() == BATCH_WORKQUEUE_HANDLER_ID)
		{
			m_active_workqueue = 0;
			m_batch_workqueue = NULL;
		}
		if (handler && handler->GetId() == SINGLE_WORKQUEUE_HANDLER_ID)
		{
			m_active_workqueue = 0;
			m_single_workqueue = NULL;
		}
		break;
	}

	UpdateUI();
}

void CLC7WorkQueueWidget::onSingleWorkQueueChanged(void)
{
	TR;
	m_active_workqueue = 1;
	UpdateUI();
}

void CLC7WorkQueueWidget::onBatchWorkQueueChanged(void)
{
	TR;
	m_active_workqueue = 2;
	UpdateUI();
}

void CLC7WorkQueueWidget::UpdateQueueIcon(ILC7WorkQueue *workqueue)
{
	TR;
	ILC7ColorManager *colman = CLC7App::getInstance()->GetMainWindow()->GetColorManager();
	if (!colman)
	{
		return;
	}
	QString suffix;
	QSize iconsize;
	if (colman->GetSizeRatio() == 1)
	{
		suffix = ".png";
		iconsize = QSize(32, 32);
	}
	else
	{
		suffix = ".png";
		iconsize = QSize(64, 64);
	}


	if (m_single_workqueue == NULL && m_batch_workqueue == NULL)
	{
		ui.waiting->setMovie(NULL);
		ui.waiting->setPixmap(QPixmap(QString(":/lc7/queueimages/null%1").arg(suffix)));
	}
	else
	{

		switch (workqueue ? workqueue->GetWorkQueueState() : ILC7WorkQueue::UNVALIDATED)
		{
		case ILC7WorkQueue::UNVALIDATED:
			m_waitingMovie->stop();
			ui.waiting->setMovie(NULL);
			ui.waiting->setPixmap(QPixmap(QString(":/lc7/queueimages/unvalidated%1").arg(suffix)));
			break;
		case ILC7WorkQueue::VALIDATED:
			m_waitingMovie->stop();
			ui.waiting->setMovie(NULL);
			ui.waiting->setPixmap(QPixmap(QString(":/lc7/queueimages/validated%1").arg(suffix)));
			break;
		case ILC7WorkQueue::INVALID:
			m_waitingMovie->stop();
			ui.waiting->setMovie(NULL);
			ui.waiting->setPixmap(QPixmap(QString(":/lc7/queueimages/invalid%1").arg(suffix)));
			break;
		case ILC7WorkQueue::IN_PROGRESS:
			ui.waiting->setPixmap(QPixmap());
			m_waitingMovie->start();
			ui.waiting->setMovie(m_waitingMovie);
			break;
		case ILC7WorkQueue::STOPPED:
			m_waitingMovie->stop();
			ui.waiting->setMovie(NULL);
			ui.waiting->setPixmap(QPixmap(QString(":/lc7/queueimages/stopped%1").arg(suffix)));
			break;
		case ILC7WorkQueue::PAUSED:
			m_waitingMovie->stop();
			ui.waiting->setMovie(NULL);
			ui.waiting->setPixmap(QPixmap(QString(":/lc7/queueimages/paused%1").arg(suffix)));
			break;
		case ILC7WorkQueue::COMPLETE:
			m_waitingMovie->stop();
			ui.waiting->setMovie(NULL);
			ui.waiting->setPixmap(QPixmap(QString(":/lc7/queueimages/complete%1").arg(suffix)));
			break;
		case ILC7WorkQueue::FAIL:
			m_waitingMovie->stop();
			ui.waiting->setMovie(NULL);
			ui.waiting->setPixmap(QPixmap(QString(":/lc7/queueimages/fail%1").arg(suffix)));
			break;
		}
	}
	ui.waiting->setFixedSize(iconsize);
}

void CLC7WorkQueueWidget::UpdateCurrentActivity(QString text)
{
	emit sig_updateCurrentActivity(text);
}

void CLC7WorkQueueWidget::ClearActivityLog()
{
	emit sig_clearActivityLog();
}

void CLC7WorkQueueWidget::ScrollToBottom()
{
	emit sig_scrollToBottom();
}

void CLC7WorkQueueWidget::appendLine(QString strdate, QString strtime, QString line)
{
	QMessageLogger(__FILE__, __LINE__, Q_FUNC_INFO).debug("%s", (const char *)line.toUtf8());

	if (m_is_new_line)
	{
		if (strdate != m_last_strdate)
		{
			m_appendString.append(QString("<font color=#808080>%1</font><br>").arg(strdate.toHtmlEscaped()));
			m_last_strdate = strdate;
		}

		m_appendString.append(QString("<font color=#808080>%1</font> ").arg(strtime.toHtmlEscaped()));
	}

	m_appendString.append(line.toHtmlEscaped());

	if (m_is_new_line)
	{
		m_appendString.append("<br>");
	}
}

void CLC7WorkQueueWidget::AppendToActivityLog(QString text)
{
	QMutexLocker lock(&m_appendStringLock);

	// split on newlines
	QString strdate = QDate::currentDate().toString(Qt::SystemLocaleShortDate);
	QString strtime = QTime::currentTime().toString("hh:mm:ss");

	QStringList lines = text.split("\n");
	if (lines.isEmpty())
	{
		return;
	}

	QString lastline;
	lastline = lines.takeLast();

	foreach(QString line, lines)
	{
		m_is_new_line = true;
		appendLine(strdate, strtime, line);
	}

	if (!lastline.isEmpty())
	{
		m_is_new_line = text.endsWith('\n');
		appendLine(strdate, strtime, lastline);
	}

	m_append_to_activity_log_later = true;
}

void CLC7WorkQueueWidget::SetStatusText(QString text)
{
	emit sig_setStatusText(text);
}

void CLC7WorkQueueWidget::UpdateCurrentProgressBar(quint32 cur)
{
	emit sig_updateCurrentProgressBar(cur);
}

void CLC7WorkQueueWidget::UpdateTotalProgressBar(quint32 cur)
{
	emit sig_updateTotalProgressBar(cur);
}

void CLC7WorkQueueWidget::slot_stopButton()
{
	TR;
	ILC7WorkQueue::STATE batchwqs = m_batch_workqueue->GetWorkQueueState();
	if (batchwqs == ILC7WorkQueue::IN_PROGRESS ||
		batchwqs == ILC7WorkQueue::PAUSED)
	{
		if (batchwqs == ILC7WorkQueue::PAUSED)
		{
			if (!m_ctrl->GetGUILinkage()->YesNoBox("Are you sure?", "Stopping the queue while paused clears the state. You will not be able to continue the current queue."))
			{
				return;
			}
		}
		m_batch_workqueue->StopRequest();
	}

	ILC7WorkQueue::STATE singlewqs = m_single_workqueue->GetWorkQueueState();
	if (singlewqs == ILC7WorkQueue::IN_PROGRESS ||
		singlewqs == ILC7WorkQueue::PAUSED)
	{
		if (singlewqs == ILC7WorkQueue::PAUSED)
		{
			if (!m_ctrl->GetGUILinkage()->YesNoBox("Are you sure?", "Stopping the operation while paused clears the state. You will not be able to continue the current operation."))
			{
				return;
			}
		}
		m_single_workqueue->StopRequest();
	}
}


void CLC7WorkQueueWidget::slot_pauseButton()
{
	TR;
	ILC7WorkQueue::STATE batchwqs = m_batch_workqueue->GetWorkQueueState();
	if (batchwqs == ILC7WorkQueue::IN_PROGRESS)
	{
		m_batch_workqueue->PauseRequest();
	}
	else if (batchwqs == ILC7WorkQueue::PAUSED)
	{
		m_batch_workqueue->StartRequest();
	}

	ILC7WorkQueue::STATE singlewqs = m_single_workqueue->GetWorkQueueState();
	if (singlewqs == ILC7WorkQueue::IN_PROGRESS)
	{
		m_single_workqueue->PauseRequest();
	}
	else if (singlewqs == ILC7WorkQueue::PAUSED)
	{
		m_single_workqueue->StartRequest();
	}
}

void CLC7WorkQueueWidget::slot_updateCurrentActivity(QString text)
{
	ui.currentOperation->setText(text);
}

void CLC7WorkQueueWidget::slot_clearActivityLog()
{
	ui.statusText->clear();
}

void CLC7WorkQueueWidget::slot_scrollToBottom()
{
	ui.statusText->verticalScrollBar()->setSliderPosition(ui.statusText->verticalScrollBar()->maximum());
}


void CLC7WorkQueueWidget::slot_append_to_activity_log_later(void)
{
	TR;
	if (!m_append_to_activity_log_later)
	{
		return;
	}
	QMutexLocker lock(&m_appendStringLock);

	bool at_bottom = false;
	if (ui.statusText->verticalScrollBar()->sliderPosition() == ui.statusText->verticalScrollBar()->maximum())
	{
		at_bottom = true;
	}

	ui.statusText->appendHtml(m_appendString);

	if (at_bottom)
	{
		ui.statusText->verticalScrollBar()->setSliderPosition(ui.statusText->verticalScrollBar()->maximum());
	}

	m_appendString.clear();
	m_append_to_activity_log_later = false;
}

void CLC7WorkQueueWidget::slot_setStatusText(QString status)
{
	ui.statusBarText->setPlainText(status);
}

void CLC7WorkQueueWidget::slot_updateCurrentProgressBar(quint32 cur)
{
	ui.currentCompletionProgress->setValue(cur);
}

void CLC7WorkQueueWidget::slot_updateTotalProgressBar(quint32 cur)
{
	ui.totalCompletionProgress->setValue(cur);
}

bool CLC7WorkQueueWidget::Save(QDataStream & ds)
{
	TR;
	quint32 version = 1;
	ds << version;

	ds << ui.statusBarText->toHtml();
	ds << m_status_document->toHtml();
	ds << ui.currentCompletionProgress->value();
	ds << ui.totalCompletionProgress->value();
	ds << ui.currentOperation->text();

	return true;
}

void CLC7WorkQueueWidget::UpdateUI(void)
{
	TR;
	if (m_active_workqueue == 0)
	{
		UpdateQueueIcon(NULL);
		ui.pauseButton->setEnabled(false);
		ui.stopButton->setEnabled(false);
	}
	else if (m_active_workqueue == 1 && m_single_workqueue)
	{
		UpdateQueueIcon(m_single_workqueue);

		if (m_single_workqueue->IsThermalShutdown())
		{
			ui.pauseButton->setText("Thermal Shutdown");
			ui.pauseButton->setEnabled(false);
		}
		else if (m_single_workqueue->IsPauseEnabled() && !m_single_workqueue->IsPauseRequested() && !m_single_workqueue->IsStopRequested())
		{
			ui.pauseButton->setEnabled(true);

			if (m_single_workqueue->GetWorkQueueState() == ILC7WorkQueue::PAUSED)
			{
				ui.pauseButton->setText("Resume");
			}
			else
			{
				ui.pauseButton->setText("Pause");
			}
		}
		else
		{
			ui.pauseButton->setEnabled(false);
		}

		if (m_single_workqueue->IsStopEnabled() && !m_single_workqueue->IsPauseRequested() && !m_single_workqueue->IsStopRequested())
		{
			ui.stopButton->setEnabled(true);
		}
		else
		{
			ui.stopButton->setEnabled(false);
		}
	}
	else if (m_active_workqueue == 2 && m_batch_workqueue)
	{
		UpdateQueueIcon(m_batch_workqueue);

		if (m_batch_workqueue->IsThermalShutdown())
		{
			ui.pauseButton->setText("Thermal Shutdown");
			ui.pauseButton->setEnabled(false);
		}
		else if (m_batch_workqueue->IsPauseEnabled())
		{
			ui.pauseButton->setEnabled(true);
			if (m_batch_workqueue->GetWorkQueueState() == ILC7WorkQueue::PAUSED)
			{
				ui.pauseButton->setText("Resume");
			}
			else
			{
				ui.pauseButton->setText("Pause");
			}
		}
		else
		{
			ui.pauseButton->setEnabled(false);
		}

		if (m_batch_workqueue->IsStopEnabled())
		{
			ui.stopButton->setEnabled(true);
		}
		else
		{
			ui.stopButton->setEnabled(false);
		}
	}
}

bool CLC7WorkQueueWidget::Load(QDataStream & ds)
{
	TR;
	quint32 version;
	ds >> version;
	if (version != 1)
	{
		return false;
	}

	QString s;

	ds >> s;
	ui.statusBarText->setHtml(s);

	ds >> s;
	m_status_document->setHtml(s);

	int v;
	ds >> v;
	ui.currentCompletionProgress->setValue(v);

	ds >> v;
	ui.totalCompletionProgress->setValue(v);

	ds >> s;
	ui.currentOperation->setText(s);

	m_last_strdate = "";

	return true;
}

void CLC7WorkQueueWidget::Reset(void)
{
	TR;
	ui.statusBarText->setHtml("");
	m_status_document->setHtml("");
	ui.currentCompletionProgress->setValue(0);
	ui.totalCompletionProgress->setValue(0);
	ui.currentOperation->setText("");

	m_last_strdate = "";

	// Print banner
	QString appname = " L0phtCrack 7 - v" VERSION_STRING " ";
	QString underline = QString("-").repeated(appname.length());
	AppendToActivityLog(appname + "\n" + underline + "\n\n");
}

void CLC7WorkQueueWidget::NotifyThermalTransition(ILC7ThermalWatchdog::THERMAL_STATE oldstate, ILC7ThermalWatchdog::THERMAL_STATE newstate)
{
	emit sig_updateThermalState();
}





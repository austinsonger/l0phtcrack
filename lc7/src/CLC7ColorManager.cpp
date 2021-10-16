#include"stdafx.h"

#define QT_QTPROPERTYBROWSER_IMPORT 
#include"../../external/qtpropertybrowser/src/QtPropertyManager.h"

#undef TR
#define TR


CLC7ColorManager::CLC7ColorManager()
{
	TR;
	m_currentScreen = qApp->primaryScreen();

	ILC7Settings *settings = CLC7App::getInstance()->GetController()->GetSettings();
	settings->AddValueChangedListener(this, (void (QObject::*)(QString, const QVariant &))&CLC7ColorManager::slot_settingsValueChanged);

	m_default_shades["BASE"] = 0;
	m_default_shades["BORDER_COLOR"] = 1;
	m_default_shades["BUTTON_BKGD_0"] = 1;
	m_default_shades["BUTTON_BKGD_1"] = 3;
	m_default_shades["BUTTON_BKGD_PRESSED_0"] = -3;
	m_default_shades["BUTTON_BKGD_PRESSED_1"] = 0;
	m_default_shades["BUTTON_COLOR_DISABLED"] = 1;
	m_default_shades["CHECKBOX_COLOR"] = 4;
	m_default_shades["COMBO_SELECTION_BKGD"] = 1;
	m_default_shades["CONTROL_BKGD"] = -1;
	m_default_shades["CONTROL_BKGD_DISABLED"] = -2;
	m_default_shades["DOCKWIDGET_BORDER"] = 2;
	m_default_shades["EXTENDED_TAB_WIDGET_CHECKED_0"] = 2;
	m_default_shades["EXTENDED_TAB_WIDGET_CHECKED_1"] = 0;
	m_default_shades["EXTENDED_TAB_WIDGET_UNCHECKED_0"] = -3;
	m_default_shades["EXTENDED_TAB_WIDGET_UNCHECKED_1"] = -1;
	m_default_shades["HEADER_BKGD"] = 3;
	m_default_shades["HEADER_BORDER"] = 4;
	m_default_shades["HIGHLIGHT_BKGD_0"] = -3;
	m_default_shades["HIGHLIGHT_BKGD_1"] = 3;
	m_default_shades["HIGHLIGHT_CONTROL_BKGD_0"] = -5;
	m_default_shades["HIGHLIGHT_CONTROL_BKGD_1"] = -4;
	m_default_shades["HIGHLIGHT_CONTROL_BORDER"] = -3;
	m_default_shades["HIGHLIGHT_CONTROL_TEXT"] = 4;
	m_default_shades["PROGRESS_BKGD_0"] = 3;
	m_default_shades["PROGRESS_BKGD_1"] = -3;
	m_default_shades["SCROLL_H_BKGD_0"] = -2;
	m_default_shades["SCROLL_H_BKGD_1"] = 2;
	m_default_shades["SCROLL_HANDLE_H_BKGD_0"] = 2;
	m_default_shades["SCROLL_HANDLE_H_BKGD_1"] = 3;
	m_default_shades["SCROLL_HANDLE_V_BKGD_0"] = 2;
	m_default_shades["SCROLL_HANDLE_V_BKGD_1"] = 3;
	m_default_shades["SCROLL_V_BKGD_0"] = -2;
	m_default_shades["SCROLL_V_BKGD_1"] = 2;
	m_default_shades["SELECT_BKGD_0"] = 4;
	m_default_shades["SELECT_BKGD_1"] = 2;
	m_default_shades["SEP_COLOR_1"] = 3;
	m_default_shades["SHADOW"] = -4;
	m_default_shades["SLIDER_COLOR_0"] = 5;
	m_default_shades["SLIDER_COLOR_1"] = 4;
	m_default_shades["TEXT_DISABLED"] = 2;
	m_default_shades["TOOLTIP_BKGD"] = 3;
	m_default_shades["WIDGET_BKGD"] = -2;
	m_default_shades["WIDGET_DISABLED"] = 1;

	m_temporary_dir = CLC7App::getInstance()->GetController()->NewTemporaryDir();
	
	slot_reloadSettings();

	m_bNeedsReload = false;
}

CLC7ColorManager::~CLC7ColorManager()
{
	TR;
	if (!m_temporary_dir.isEmpty())
	{
		QDir dir(m_temporary_dir);
		if (dir.exists())
		{
			dir.removeRecursively();
		}
		m_temporary_dir = "";
	}
}


ILC7Interface *CLC7ColorManager::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7ColorManager")
	{
		return this;
	}
	return NULL;
}

QIcon CLC7ColorManager::GetMonoColorIcon(QString resource, QColor normal, QColor active, QColor disabled)
{
	TR;
	if (!normal.isValid())
	{
		normal = QColor(GetBaseShade("HEADER_BORDER"));
	}
	if (!active.isValid())
	{
		active = QColor(GetHighlightShade("BASE"));
	}
	if (!disabled.isValid())
	{
		disabled = QColor(GetBaseShade("BORDER_COLOR"));
	}

	QPixmap pixmap_normal(GetMonoColorPixmapResource(resource, normal));
	QPixmap pixmap_active(GetMonoColorPixmapResource(resource, active));
	QPixmap pixmap_disabled(GetMonoColorPixmapResource(resource, disabled));

	QIcon icon;
	icon.addPixmap(pixmap_normal, QIcon::Normal, QIcon::Off);
	icon.addPixmap(pixmap_active, QIcon::Active, QIcon::Off);
	icon.addPixmap(pixmap_active, QIcon::Selected, QIcon::Off);
	icon.addPixmap(pixmap_disabled, QIcon::Disabled, QIcon::Off);
	icon.addPixmap(pixmap_normal, QIcon::Normal, QIcon::On);
	icon.addPixmap(pixmap_active, QIcon::Active, QIcon::On);
	icon.addPixmap(pixmap_active, QIcon::Selected, QIcon::On);
	icon.addPixmap(pixmap_disabled, QIcon::Disabled, QIcon::On);

	return icon;
}

QMap<QString, int> CLC7ColorManager::GetDefaultShades()
{
	TR;
	return m_default_shades;
}

QColor CLC7ColorManager::GetDefaultBaseColor()
{
	TR;
	return QColor("#505050");
}

QColor CLC7ColorManager::GetDefaultHighlightColor()
{
	TR;
	return QColor("#F27800");
}


void CLC7ColorManager::slot_settingsValueChanged(QString key, const QVariant &val)
{
	TR;

	if (key.startsWith("_theme_:"))
	{
		ReloadSettings();
	}
}



void CLC7ColorManager::ReloadSettings(void)
{
	TR;
	if (!m_bNeedsReload)
	{
		m_bNeedsReload = true;
		const bool isGuiThread = (QThread::currentThread() == QCoreApplication::instance()->thread());
		if (isGuiThread)
		{
			slot_reloadSettings();
		}
		else
		{
			QTimer::singleShot(0, this, &CLC7ColorManager::slot_reloadSettings);
		}
	}
}

void CLC7ColorManager::slot_reloadSettings(void)
{
	TR;

	if (!m_temporary_dir.isEmpty())
	{
		QDir dir(m_temporary_dir);
		if (dir.exists())
		{
			dir.removeRecursively();
		}
		m_temporary_dir = "";
	}
	m_temporary_dir = CLC7App::getInstance()->GetController()->NewTemporaryDir();

	ILC7Settings *settings = CLC7App::getInstance()->GetController()->GetSettings();

	m_basecolor = settings->value("_theme_:basecolor", GetDefaultBaseColor()).value<QColor>();
	m_highlightcolor = settings->value("_theme_:highlightcolor", GetDefaultHighlightColor()).value<QColor>();
	m_shades.clear();

#ifdef _DEBUG
	foreach(QString key, m_default_shades.keys())
	{
		m_shades[key] = settings->value(QString("_theme_:shade_") + key, m_default_shades[key]).toInt();
	}
#else
	m_shades=m_default_shades;
#endif

	RecalculateColors();

	m_bNeedsReload = false;
}

void CLC7ColorManager::SetBaseColor(QColor color)
{
	TR;
	m_basecolor = color;
	RecalculateColors();
}

QColor CLC7ColorManager::GetBaseColor()
{
	TR;
	return m_basecolor;
}

void CLC7ColorManager::SetHighlightColor(QColor color)
{
	TR;
	m_highlightcolor = color;
	RecalculateColors();
}

QColor CLC7ColorManager::GetHighlightColor()
{
	TR;
	return m_highlightcolor;
}

QString CLC7ColorManager::GetTextColor()
{
	TR;
	return m_textcolor.name(QColor::HexRgb);
}

QString CLC7ColorManager::GetInverseTextColor()
{
	TR;
	int h, s, l;
	m_textcolor.getHsl(&h, &s, &l);

	l = 255 - l;

	return QColor::fromHsl(h, s, l).name(QColor::HexRgb);
}

static void applyShade(int shade, int &h, int &s, int &l)
{
	if (l >= 128)
	{
		shade = -shade;
	}

	if (shade > 0)
	{
		l += ((shade * (255 - l)) / 8);
	}
	else
	{
		l += ((shade * l) / 8);
	}
}

QString CLC7ColorManager::GetHighlightShade(QString name)
{
	TR;
	int h, s, l;
	m_highlightcolor.getHsl(&h, &s, &l);

	int shade = m_shades[name];

	applyShade(shade, h, s, l);

	return QColor::fromHsl(h, s, l).name(QColor::HexRgb);
}

QString CLC7ColorManager::GetBaseShade(QString name)
{
	TR;
	int h, s, l;
	m_basecolor.getHsl(&h, &s, &l);

	int shade = m_shades[name];

	applyShade(shade, h, s, l);

	return QColor::fromHsl(h, s, l).name(QColor::HexRgb);
}



QString CLC7ColorManager::GetStyleSheet(void)
{
	TR;
	return m_stylesheet;
}


#ifdef Q_OS_WIN

const float DEFAULT_DPI = 96.0;

#endif //Q_OS_WIN

void CLC7ColorManager::RecalculateColors()
{
	TR;
	// determine if screen should be hidpi or not
	qreal logicaldpi = m_currentScreen->logicalDotsPerInch();
	qreal logicalpixelratio = logicaldpi / DEFAULT_DPI;
	if (logicalpixelratio >= 1.5)
	{
		m_size_ratio = 2;
	}
	else
	{
		m_size_ratio = 1;
	}

	int h, s, l;
	m_basecolor.getHsl(&h, &s, &l);
	if (l < 128)
	{
		m_textcolor = QColor("#FFFFFF");
	}
	else
	{
		m_textcolor = QColor("#000000");
	}

	// Load stylesheet
	QString strStyleSheet;
#ifdef _DEBUG
	if (QFile::exists("../../../lc7/resources/darkstyle.qss"))
	{
		QFile res("../../../lc7/resources/darkstyle.qss");
		res.open(QIODevice::ReadOnly);
		strStyleSheet = QString::fromLatin1(res.readAll());
	}
	else
	{
#endif
		QFile res(":/qdarkstyle/darkstyle.qss");
		res.open(QIODevice::ReadOnly);
		strStyleSheet = QString::fromLatin1(res.readAll());
#ifdef _DEBUG
	}
#endif

	// Grab all URLS
	QRegExp re_url("url\\(:([^\\)]*)\\)");

	int offset = 0;
	QStringList urls;
	while ((offset = re_url.indexIn(strStyleSheet, offset)) != -1)
	{
		QString url = re_url.cap(1);
		if (!urls.contains(url))
		{
			urls.append(url);
		}
		offset += re_url.matchedLength();
	}

	// Extract resources that match URL to disk, recolor, write out
	int pngnum = 0;
	foreach(QString url, urls)
	{
		QPixmap newpixmap = GetHueColorPixmapResource(QString(":") + url);

		// Write out pixmap and rewrite url to new pixmap
		QString outname = QDir(m_temporary_dir).filePath(QString("%1.png").arg(pngnum));
		QFile out(outname);
		out.open(QIODevice::WriteOnly);
		newpixmap.save(&out, "PNG");

		outname = outname.replace("$","\\$");
		
		strStyleSheet = strStyleSheet.replace(QString("url(:%1)").arg(url), QString("url(%1)").arg(outname));

		pngnum++;
	}

	// Text
	strStyleSheet = strStyleSheet.replace("##TEXT_COLOR##", GetTextColor());
	strStyleSheet = strStyleSheet.replace("##INVERSE_TEXT_COLOR##", GetInverseTextColor());
	strStyleSheet = strStyleSheet.replace("##TEXT_DISABLED##", GetBaseShade("TEXT_DISABLED"));

	// Widget
	strStyleSheet = strStyleSheet.replace("##WIDGET_DISABLED##", GetBaseShade("WIDGET_DISABLED"));
	strStyleSheet = strStyleSheet.replace("##WIDGET_BKGD##", GetBaseShade("WIDGET_BKGD"));

	// Border
	strStyleSheet = strStyleSheet.replace("##BORDER_COLOR##", GetBaseShade("BORDER_COLOR"));
	strStyleSheet = strStyleSheet.replace("##DOCKWIDGET_BORDER##", GetBaseShade("DOCKWIDGET_BORDER"));

	// Separator
	strStyleSheet = strStyleSheet.replace("##SEP_COLOR_1##", GetBaseShade("SEP_COLOR_1"));
	strStyleSheet = strStyleSheet.replace("##SEP_COLOR_2##", GetTextColor());

	// Control
	strStyleSheet = strStyleSheet.replace("##CONTROL_BKGD##", GetBaseShade("CONTROL_BKGD"));
	strStyleSheet = strStyleSheet.replace("##CONTROL_BKGD_DISABLED##", GetBaseShade("CONTROL_BKGD_DISABLED"));

	// Highlight
	strStyleSheet = strStyleSheet.replace("##HIGHLIGHT_COLOR##", GetHighlightShade("BASE"));
	strStyleSheet = strStyleSheet.replace("##HIGHLIGHT_CONTROL_TEXT##", GetHighlightShade("HIGHLIGHT_CONTROL_TEXT"));
	strStyleSheet = strStyleSheet.replace("##HIGHLIGHT_CONTROL_BORDER##", GetHighlightShade("HIGHLIGHT_CONTROL_BORDER"));
	strStyleSheet = strStyleSheet.replace("##HIGHLIGHT_CONTROL_BKGD##", "QLinearGradient( x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 " +
		GetHighlightShade("HIGHLIGHT_CONTROL_BKGD_0") + " stop: 1 " + GetHighlightShade("HIGHLIGHT_CONTROL_BKGD_1") + ")");

	// Selection
	strStyleSheet = strStyleSheet.replace("##HIGHLIGHT_BKGD##", "QLinearGradient( x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 " +
		GetHighlightShade("HIGHLIGHT_BKGD_0") + " stop: 1 " + GetHighlightShade("HIGHLIGHT_BKGD_1") + ")");

	strStyleSheet = strStyleSheet.replace("##SELECT_BKGD##", "QLinearGradient( x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 " +
		GetBaseShade("SELECT_BKGD_0") + " stop: 1 " + GetBaseShade("SELECT_BKGD_1") + ")");
	strStyleSheet = strStyleSheet.replace("##SELECT_BKGD_1##", GetBaseShade("SELECT_BKGD_0"));
	strStyleSheet = strStyleSheet.replace("##SELECT_BKGD_2##", GetBaseShade("SELECT_BKGD_1"));
	strStyleSheet = strStyleSheet.replace("##COMBO_SELECTION_BKGD##", GetBaseShade("COMBO_SELECTION_BKGD"));

	// Button
	strStyleSheet = strStyleSheet.replace("##BUTTON_BKGD##", "QLinearGradient( x1: 0, y1: 1, x2: 0, y2: 0, stop: 0 " +
		GetBaseShade("BUTTON_BKGD_0") + " stop: 1 " + GetBaseShade("BUTTON_BKGD_1") + ")");
	strStyleSheet = strStyleSheet.replace("##BUTTON_BKGD_1##", GetBaseShade("BUTTON_BKGD_0"));
	strStyleSheet = strStyleSheet.replace("##BUTTON_BKGD_2##", GetBaseShade("BUTTON_BKGD_1"));
	strStyleSheet = strStyleSheet.replace("##BUTTON_COLOR_DISABLED##", GetBaseShade("BUTTON_COLOR_DISABLED"));
	strStyleSheet = strStyleSheet.replace("##BUTTON_BKGD_PRESSED##", "QLinearGradient( x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 " +
		GetBaseShade("BUTTON_BKGD_PRESSED_0") + " stop: 1 " + GetBaseShade("BUTTON_BKGD_PRESSED_1") + ")");
	// Checkbox
	strStyleSheet = strStyleSheet.replace("##CHECKBOX_COLOR##", GetBaseShade("CHECKBOX_COLOR"));

	// Slider
	strStyleSheet = strStyleSheet.replace("##SLIDER_COLOR_1##", GetBaseShade("SLIDER_COLOR_0"));
	strStyleSheet = strStyleSheet.replace("##SLIDER_COLOR_2##", GetBaseShade("SLIDER_COLOR_1"));

	// Tooltip
	strStyleSheet = strStyleSheet.replace("##TOOLTIP_BKGD##", GetBaseShade("TOOLTIP_BKGD"));

	// Progress
	strStyleSheet = strStyleSheet.replace("##PROGRESS_BKGD##", "QLinearGradient(x1:1, y1:0, x2:0, y2:0, stop:0 " +
		GetHighlightShade("PROGRESS_BKGD_0") + " stop:1 " + GetHighlightShade("PROGRESS_BKGD_1") + ")");

	// Extended Tab Widget
	strStyleSheet = strStyleSheet.replace("##EXTENDED_TAB_WIDGET_CHECKED##", "QLinearGradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 " +
		GetBaseShade("EXTENDED_TAB_WIDGET_CHECKED_0") + " stop:1 " + GetBaseShade("EXTENDED_TAB_WIDGET_CHECKED_1") + ")");
	strStyleSheet = strStyleSheet.replace("##EXTENDED_TAB_WIDGET_UNCHECKED##", "QLinearGradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 " +
		GetBaseShade("EXTENDED_TAB_WIDGET_UNCHECKED_0") + " stop:1 " + GetBaseShade("EXTENDED_TAB_WIDGET_UNCHECKED_1") + ")");

	// Scrollbar
	strStyleSheet = strStyleSheet.replace("##SCROLL_H_BKGD##", "QLinearGradient( x1: 0, y1: 1, x2: 0, y2: 0, stop: 0 " +
		GetBaseShade("SCROLL_H_BKGD_0") + " stop: 1 " + GetBaseShade("SCROLL_H_BKGD_1") + ")");
	strStyleSheet = strStyleSheet.replace("##SCROLL_V_BKGD##", "QLinearGradient( x1: 1, y1: 0, x2: 0, y2: 0, stop: 0 " +
		GetBaseShade("SCROLL_V_BKGD_0") + " stop: 1 " + GetBaseShade("SCROLL_V_BKGD_1") + ")");
	strStyleSheet = strStyleSheet.replace("##SCROLL_HANDLE_H_BKGD##", "QLinearGradient( x1: 0, y1: 1, x2: 0, y2: 0, stop: 0 " +
		GetBaseShade("SCROLL_HANDLE_H_BKGD_0") + " stop: 1 " + GetBaseShade("SCROLL_HANDLE_H_BKGD_1") + ")");
	strStyleSheet = strStyleSheet.replace("##SCROLL_HANDLE_V_BKGD##", "QLinearGradient( x1: 1, y1: 0, x2: 0, y2: 0, stop: 0 " +
		GetBaseShade("SCROLL_HANDLE_V_BKGD_0") + " stop: 1 " + GetBaseShade("SCROLL_HANDLE_V_BKGD_1") + ")");

	// Headers
	strStyleSheet = strStyleSheet.replace("##HEADER_BKGD##", GetBaseShade("HEADER_BKGD"));
	strStyleSheet = strStyleSheet.replace("##HEADER_BORDER##", GetBaseShade("HEADER_BORDER"));

	// Messages
	strStyleSheet = strStyleSheet.replace("##ERROR_BKGD##", "QLinearGradient( x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #ff825b, stop: 1 #f13900)");
	strStyleSheet = strStyleSheet.replace("##WARNING_BKGD##", "QLinearGradient( x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #ffd65b, stop: 1 #fffd35)");
	strStyleSheet = strStyleSheet.replace("##SUCCESS_BKGD##", "QLinearGradient( x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #79ff6f, stop: 1 #36ff00)");

	// Shadow
	strStyleSheet = strStyleSheet.replace("##SHADOW##", GetBaseShade("SHADOW"));

	m_stylesheet = strStyleSheet;

	CLC7App::getInstance()->setStyleSheet(strStyleSheet);

	QPalette p;
	p.setColor(QPalette::Link, GetHighlightShade("HIGHLIGHT_BKGD_1"));
	p.setColor(QPalette::LinkVisited, GetHighlightShade("HIGHLIGHT_BKGD_1"));
	CLC7App::getInstance()->setPalette(p);

	SetCheckboxPixmap(
		GetHueColorPixmapResource(":/QtPropertyManager/checkbox_true.png"),
		GetHueColorPixmapResource(":/QtPropertyManager/checkbox_false.png")
		);

	foreach(QCommandLinkButton *clbutton, m_clbuttons)
	{
		DoCommandLinkButtonStyling(clbutton);
	}

	emit sig_RecolorCallback();
}

QPixmap CLC7ColorManager::HueRecolorPixmap(const QPixmap *pixmap, QColor huecolor)
{
	TR;
	if (!huecolor.isValid())
	{
		huecolor = GetHighlightColor();
	}

	QColor defaulthighlightcolor = GetDefaultHighlightColor();
	int ldiff = huecolor.lightness() - defaulthighlightcolor.lightness();
	int sdiff = huecolor.hslSaturation() - defaulthighlightcolor.hslSaturation();

	QPixmap newpixmap;

	QImage img = pixmap->toImage();

	QImage newImage(img.width(), img.height(), QImage::Format_ARGB32);

	QColor oldColor;
	QColor newColor;

	for (int x = 0; x < newImage.width(); x++){
		for (int y = 0; y < newImage.height(); y++){

			QColor color = QColor::fromRgba(img.pixel(x, y));
			
			int s = color.hslSaturation();
			int l = color.lightness();

			if ((s + sdiff) > 255)
				s = 255;
			else if ((s + sdiff) < 0)
				s = 0;
			else
				s = s + sdiff;

			if ((l + ldiff) > 255)
				l = 255;
			else if ((l + ldiff) < 0)
				l = 0;
			else
				l = l + ldiff;

			color.setHsl(huecolor.hslHue(), s, l, color.alpha());
			newImage.setPixel(x, y, color.rgba());

		}
	}
	return QPixmap::fromImage(newImage);
}

QPixmap CLC7ColorManager::MonoRecolorPixmap(const QPixmap *pixmap, QColor basecolor)
{
	TR;
	if (!basecolor.isValid())
	{
		basecolor = GetBaseColor();
	}

	QPixmap newpixmap;

	QImage img = pixmap->toImage();

	QImage newImage(img.width(), img.height(), QImage::Format_ARGB32);

	QColor oldColor;
	QColor newColor;

	for (int x = 0; x < newImage.width(); x++){
		for (int y = 0; y < newImage.height(); y++){

			QColor color = QColor::fromRgba(img.pixel(x, y));
			color.setHsl(basecolor.hue(), basecolor.hslSaturation(), basecolor.lightness(), color.alpha());
			newImage.setPixel(x, y, color.rgba());

		}
	}
	return QPixmap::fromImage(newImage);
}


void CLC7ColorManager::reload()
{
	TR;
	ReloadSettings();
}

QPixmap CLC7ColorManager::GetHueColorPixmap(QPixmap pixmap, QColor huecolor)
{
	TR;
	return HueRecolorPixmap(&pixmap, huecolor);
}

QPixmap CLC7ColorManager::GetHueColorPixmapResource(QString resource_path, QColor huecolor)
{
	TR;
	// Use larger pixmap if we have one and need one
	if (m_size_ratio == 2)
	{
		QString rpath(resource_path.left(resource_path.lastIndexOf(".")));
		QString rext(resource_path.mid(resource_path.lastIndexOf(".") + 1));
		QString res2x = rpath + "@2x." + rext;
		QResource res(res2x);
		if (res.isValid())
		{
			resource_path = res2x;
		}
	}

	QPixmap pixmap(resource_path);
	return HueRecolorPixmap(&pixmap, huecolor);
}

QPixmap CLC7ColorManager::GetMonoColorPixmap(QPixmap pixmap, QColor basecolor)
{
	TR;
	return MonoRecolorPixmap(&pixmap, basecolor);
}

QPixmap CLC7ColorManager::GetMonoColorPixmapResource(QString resource_path, QColor basecolor)
{
	TR;
	// Use larger pixmap if we have one and need one
	if (m_size_ratio == 2)
	{
		QString rpath(resource_path.left(resource_path.lastIndexOf(".")));
		QString rext(resource_path.mid(resource_path.lastIndexOf(".") + 1));
		QString res2x = rpath + "@2x." + rext;
		QResource res(res2x);
		if (res.isValid())
		{
			resource_path = res2x;
		}
	}

	QPixmap pixmap(resource_path);
	return MonoRecolorPixmap(&pixmap, basecolor);
}


void CLC7ColorManager::RegisterRecolorCallback(QObject *slot_obj, void (QObject::*slot_method)(void))
{
	TR;
	connect(this, &CLC7ColorManager::sig_RecolorCallback, slot_obj, slot_method);
}

QGraphicsEffect *CLC7ColorManager::CreateShadowEffect(void)
{
	TR;
	QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect;
	effect->setBlurRadius(6);
	effect->setXOffset(3);
	effect->setYOffset(3);
	effect->setColor(GetBaseShade("SHADOW"));

	return effect;
}


void CLC7ColorManager::slot_destroyedCLButton(QObject *object)
{
	TR;
	m_clbuttons.removeAll((QCommandLinkButton *)object);
}

void CLC7ColorManager::DoCommandLinkButtonStyling(QCommandLinkButton *commandlinkbutton)
{
	TR;
	QPixmap clpixmap(GetHueColorPixmapResource(":/qss_icons/rc/commandlink.png"));
	commandlinkbutton->setIcon(QIcon(clpixmap));
	commandlinkbutton->setIconSize(clpixmap.size());
	commandlinkbutton->setGraphicsEffect(CreateShadowEffect());
	commandlinkbutton->setSizePolicy(commandlinkbutton->sizePolicy().horizontalPolicy(), QSizePolicy::Fixed);
	commandlinkbutton->setFixedHeight(20 + (24 * m_size_ratio));
}

void CLC7ColorManager::StyleCommandLinkButton(QCommandLinkButton *commandlinkbutton)
{
	TR;
	if (!m_clbuttons.contains(commandlinkbutton))
	{
		m_clbuttons.append(commandlinkbutton);

		connect(commandlinkbutton, &QObject::destroyed, this, &CLC7ColorManager::slot_destroyedCLButton);
		connect(commandlinkbutton, &QAbstractButton::pressed, this, &CLC7ColorManager::onCommandLinkButtonPressed);
		connect(commandlinkbutton, &QAbstractButton::released, this, &CLC7ColorManager::onCommandLinkButtonReleased);
	}
	DoCommandLinkButtonStyling(commandlinkbutton);
}

void CLC7ColorManager::onCommandLinkButtonPressed()
{
	TR;
	QCommandLinkButton *commandlinkbutton = (QCommandLinkButton *)QObject::sender();
	commandlinkbutton->graphicsEffect()->setEnabled(false);
	commandlinkbutton->move(commandlinkbutton->pos() + QPoint(3, 3));
}

void CLC7ColorManager::onCommandLinkButtonReleased()
{
	TR;
	QCommandLinkButton *commandlinkbutton = (QCommandLinkButton *)QObject::sender();
	commandlinkbutton->graphicsEffect()->setEnabled(true);
	commandlinkbutton->move(commandlinkbutton->pos() + QPoint(-3, -3));
}

void CLC7ColorManager::slot_screenChanged(QScreen *screen)
{
	TR;
	m_currentScreen = screen;
	RecalculateColors();
}

int CLC7ColorManager::GetSizeRatio(void)
{
	TR;
	return m_size_ratio;
}

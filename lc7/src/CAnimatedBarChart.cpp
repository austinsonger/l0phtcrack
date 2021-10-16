#include <stdafx.h>

#include <time.h>
#include <iostream>
#include <cmath>
#include <random>
#include <algorithm>

#define GETRANDOM( min, max ) (((rand() % ((max) - (min) + 1))) + min)
#define ENABLED_OR_DEFAULT(bi, f) ((bi.enabled_mask & BARINFO::ENABLED_##f)?bi.f:m_default_bi.f)

template<typename F>
QColor interpolate(QColor a, QColor b, float t, F interpolator)
{
	QColor final;

	final = QColor::fromHsl(
		interpolator(a.hue(), b.hue(), t),
		interpolator(a.hslSaturation(), b.hslSaturation(), t),
		interpolator(a.lightness(), b.lightness(), t)
		);

	return final.toRgb();
}

int linear(int a, int b, float t)
{
	return a * (1 - t) + b * t;
}


CAnimatedBarChart::CAnimatedBarChart(QWidget *parent) : QWidget(parent)
{
	m_pPaintBuffers[0] = NULL;
	m_pPaintBuffers[1] = NULL;
	
	reset();

	m_refresh.setInterval(100);
	connect(&m_refresh, &QTimer::timeout, this, &CAnimatedBarChart::slot_refresh);

	m_refresh.start();
}

CAnimatedBarChart::~CAnimatedBarChart()
{
	reset();
}

void CAnimatedBarChart::reset()
{
	if (m_pPaintBuffers[0] != NULL)
		delete[] m_pPaintBuffers[0];
	if (m_pPaintBuffers[1] != NULL)
		delete[] m_pPaintBuffers[1];
	m_pPaintBuffers[0] = NULL;
	m_pPaintBuffers[1] = NULL;
	m_iCurPaintBuffer = 0;

	m_iWidth = 0;
	m_iHeight = 0;

	m_dirty = 0xFFFFFFFF;
	m_hover = false;
	m_active = true;

	m_border_width = 1;
	m_border_gap = 1;
	m_dpi_scale = 1;

	m_background_color[0] = QColor(64, 64, 64);
	m_background_color[1] = QColor(0, 0, 0);
	//m_hover_color[0] = QColor(64, 64, 64);
	//m_hover_color[1] = QColor(32, 32, 32);
	m_border_color[0] = QColor(128, 128, 128);
	m_border_color[1] = QColor(192, 192, 192);

	m_header_alignment = Qt::AlignTop | Qt::AlignHCenter;
	m_header_text_color[0] = QColor(128, 128, 128);
	m_header_text_color[1] = QColor(255, 255, 255);
	m_st_header.setTextFormat(Qt::TextFormat::AutoText);

	m_bars_orientation = Qt::Vertical;

	m_default_bi.text = "";
	m_default_bi.value = 0.0;
	m_default_bi.width = 32;
	m_default_bi.gap = 1;
	m_default_bi.text_alignment = Qt::AlignCenter;
	m_default_bi.font = QFont();
	m_default_bi.bar_color_inactive = QColor(192, 192, 192);
	m_default_bi.bar_color_active = QColor(255, 255, 255);
	m_default_bi.text_color_inactive = QColor(64, 64, 64);
	m_default_bi.text_color_active = QColor(0, 0, 0);
	m_default_bi.text_orientation = Qt::Horizontal;
	m_default_bi.m_st_bar.setText("");
	m_default_bi.m_st_bar.setTextFormat(Qt::TextFormat::AutoText);
		
	clear();
}

void CAnimatedBarChart::clear()
{
	m_header_text = "";
	m_bar_index.clear();
	m_bar_info.clear();
}

void CAnimatedBarChart::slot_refresh(void)
{
	if (m_pPaintBuffers[0] == NULL || m_sizePaintBuffer != size())
	{
		initialize();
	}

	drawFrame();
}

void CAnimatedBarChart::enterEvent(QEvent *e)
{
	setMouseTracking(true);
	m_hover = true;
	update();

	QWidget::enterEvent(e);
}

void CAnimatedBarChart::mouseMoveEvent(QMouseEvent *e)
{
	QPoint pm = e->pos();

	for (int n = 0; n < m_bar_info.size(); n++)
	{
		if ((m_bars_orientation == Qt::Vertical && (m_bar_info[n].last_rect.left() <= pm.x() && m_bar_info[n].last_rect.right() >= pm.x())) ||
			(m_bars_orientation == Qt::Horizontal && (m_bar_info[n].last_rect.top() <= pm.y() && m_bar_info[n].last_rect.bottom() >= pm.y())))
		{
			m_bar_info[n].hover = true;
			//setToolTip(m_bar_info[n].tiptext);
			QToolTip::showText(mapToGlobal(pm), m_bar_info[n].tiptext);
		}
		else
		{
			m_bar_info[n].hover = false;
			//setToolTip(m_header_text);
		}
	}
		
	QWidget::mouseMoveEvent(e);
}

void CAnimatedBarChart::leaveEvent(QEvent * e)
{
	setMouseTracking(false);
	m_hover = false;
	update();

	QWidget::leaveEvent(e);
}

void CAnimatedBarChart::resizeEvent(QResizeEvent *evt)
{
	QSize sz = size();

	// recalculate static text
	if (m_header_text_orientation == Qt::Horizontal)
	{
		m_st_header.setTextWidth(sz.width() - ((m_border_width + m_border_gap) * 2 * m_dpi_scale));
	}
	else
	{
		m_st_header.setTextWidth(sz.height() - ((m_border_width + m_border_gap) * 2 * m_dpi_scale));
	}

	QWidget::resizeEvent(evt);
}

void CAnimatedBarChart::paintEvent(QPaintEvent*)
{
	if (m_pPaintBuffers[0] == NULL || m_sizePaintBuffer != size())
	{
		initialize();
		drawFrame();
	}

	QPainter painter(this);

	QImage *frontbuf = FrontBuffer();
	if (frontbuf)
	{
		painter.drawImage(QPoint(0, 0), *frontbuf);
	}
	else
	{
		painter.fillRect(rect(), m_background_color[ m_active ? 1 : 0 ]);
	}
}

void CAnimatedBarChart::setActive(bool active)
{
	m_active = active;
	m_dirty = 0xFFFFFFFF;
	update();
}

bool CAnimatedBarChart::active(void) const
{
	return m_active;
}

void CAnimatedBarChart::setDPIScale(int scale)
{
	m_dpi_scale = scale;
	m_dirty = 0xFFFFFFFF;
	update();
}

int CAnimatedBarChart::dpiIScale() const
{
	return m_dpi_scale;
}

////////////////////////////////////////////////////////////////////////////////////

void CAnimatedBarChart::recalculateSize(void)
{
	QSize size=QSize(0,0), minsize=QSize(0,0);

	int w = 0, minw = 0;
	int h = 0, minh = 0;

	for (int i = 0; i < m_bar_info.size(); i++)
	{
		const BARINFO &bi = m_bar_info[i];

		int bw = ENABLED_OR_DEFAULT(bi, width);
		int bg = ENABLED_OR_DEFAULT(bi, gap);

		w += (bw + bg) * m_dpi_scale;
		minw += m_dpi_scale; // Bar must remain visible, gap can be zero if necessary
	}

	// Minimum bar height
	h += m_dpi_scale;
	minh += m_dpi_scale;

	// Add border size
	w += m_border_width * 2 * m_dpi_scale + m_border_gap * 2 * m_dpi_scale;
	h += m_border_width * 2 * m_dpi_scale + m_border_gap * 2 * m_dpi_scale;
	
	// Orient
	if (m_bars_orientation == Qt::Vertical)
	{
		size = QSize(w, h);
		minsize = QSize(minw, minh);
	}
	else
	{
		size = QSize(h, w);
		minsize = QSize(minh, minw);
	}

	// Apply
	m_sizeHint = size;
	m_minimumSizeHint = minsize;

//	if (m_bars_orientation == Qt::Vertical)
//	{
//		setMinimumWidth(w);
//	}
//	else
//	{
//		setMinimumHeight(h);
//	}

	update();
}

QSize CAnimatedBarChart::sizeHint() const
{
	return m_sizeHint;
}

QSize CAnimatedBarChart::minimumSizeHint() const
{
	return m_sizeHint;// m_minimumSizeHint;
}

void CAnimatedBarChart::initialize()
{
	m_iHeight = size().height();
	m_iWidth = size().width();

	initPaintBuffers();
}


void CAnimatedBarChart::initPaintBuffers()
{
	if (m_pPaintBuffers[0] != NULL)
		delete[] m_pPaintBuffers[0];
	if (m_pPaintBuffers[1] != NULL)
		delete[] m_pPaintBuffers[1];
	m_pPaintBuffers[0] = NULL;
	m_pPaintBuffers[1] = NULL;
	m_iCurPaintBuffer = 0;

	m_sizePaintBuffer = QSize(m_iWidth, m_iHeight);

	m_pPaintBuffers[0] = new QImage(m_sizePaintBuffer, QImage::Format::Format_RGB32);
	m_pPaintBuffers[1] = new QImage(m_sizePaintBuffer, QImage::Format::Format_RGB32);
}

static QColor dim(QColor c)
{
	return c.darker(120);
}
static QColor bright(QColor c)
{
	return c.lighter(120);
}

void CAnimatedBarChart::drawBar(QPainter & painter, int &barpos, BARINFO &barinfo)
{
	painter.save();

	// Calculate bar
	QRect barrect;

	int barsize = ENABLED_OR_DEFAULT(barinfo, width);
	int bargap = ENABLED_OR_DEFAULT(barinfo, gap);

	if (m_bars_orientation == Qt::Vertical)
	{
		int baseline = height() - ((m_border_width + m_border_gap) * m_dpi_scale);
		int maxbarheight = height() - ((m_border_width + m_border_gap) * 2 * m_dpi_scale);
		int barheight = (int) ((barinfo.value * (double)maxbarheight) / 100.0) ;

		barrect = QRect(barpos, baseline - barheight, barsize*m_dpi_scale, barheight);
	}
	else
	{
		int baseline = (m_border_width + m_border_gap) * m_dpi_scale;
		int maxbarwidth = width() - ((m_border_width + m_border_gap) * 2 * m_dpi_scale);
		int barwidth = (int)((barinfo.value * (double)maxbarwidth) / 100.0);

		barrect = QRect(baseline, barpos, barwidth, barsize*m_dpi_scale);
	}

	// Draw bar
	if (barinfo.hover)
	{
		painter.setPen(QPen(m_active ? ENABLED_OR_DEFAULT(barinfo, bar_color_active) : ENABLED_OR_DEFAULT(barinfo, bar_color_inactive), m_dpi_scale));
		painter.setBrush(bright(m_active ? ENABLED_OR_DEFAULT(barinfo, bar_color_active) : ENABLED_OR_DEFAULT(barinfo, bar_color_inactive)));
	}
	else
	{
		painter.setPen(QPen(dim(m_active ? ENABLED_OR_DEFAULT(barinfo, bar_color_active) : ENABLED_OR_DEFAULT(barinfo, bar_color_inactive)), m_dpi_scale));
		painter.setBrush(m_active ? ENABLED_OR_DEFAULT(barinfo, bar_color_active) : ENABLED_OR_DEFAULT(barinfo, bar_color_inactive));
	}
	painter.drawRect(barrect);

	// Save bar rectangle
	barinfo.last_rect = barrect;

	// Draw bar text
	


	// Move to next bar
	barpos += ((barsize + bargap) * m_dpi_scale);

	painter.restore();
}

static double getRotation(Qt::Orientation o, Qt::Alignment a)
{
	double rot = 0.0;
	
	if (o == Qt::Vertical)
	{	
		if (a & Qt::AlignRight)
		{
			rot = 90;
		}
		else
		{
			rot = -90;
		}
	}

	return rot;
}

static QPoint getTextAnchor(QRect tr, Qt::Orientation o, Qt::Alignment a)
{
	QPoint anchor = tr.topLeft();
	
	if (o == Qt::Vertical)
	{
		if (a & Qt::AlignRight)
		{
			anchor = tr.topRight();
		}
		else
		{
			anchor = tr.bottomLeft();
		}
	}

	return anchor;
}


static QRect rectAlign(QRect bounds, QRect r, Qt::Alignment a)
{
	int x, y;
	int w, h;

	w = r.width();
	h = r.height();

	if ((a & Qt::AlignHorizontal_Mask) == Qt::AlignLeft)
	{
		x = bounds.left();
	}
	else if ((a & Qt::AlignHorizontal_Mask) == Qt::AlignRight)
	{
		x = bounds.right() - w;
	}
	else if ((a & Qt::AlignHorizontal_Mask) == Qt::AlignHCenter)
	{
		x = bounds.center().x() - (r.width() / 2);
	}
	
	if ((a & Qt::AlignVertical_Mask) == Qt::AlignTop)
	{
		y = bounds.top();
	}
	else if ((a & Qt::AlignVertical_Mask) == Qt::AlignBottom)
	{
		y = bounds.bottom() - h;
	}
	else if ((a & Qt::AlignVertical_Mask) == Qt::AlignVCenter)
	{
		y = bounds.center().y() - (r.height() / 2);
	}

	QRect tr = QRect(x, y, w, h);

	//tr = bounds.intersected(tr);

	return tr;
}

void CAnimatedBarChart::drawHeader(QPainter & painter)
{
	if (m_header_text.isEmpty())
	{
		return;
	}
		
	// Get borderless area rectangle
	QRect r(0, 0, width(), height());
	r.adjust((m_border_width + m_border_gap) * m_dpi_scale, 
		(m_border_width + m_border_gap) * m_dpi_scale, 
		-(m_border_width + m_border_gap) * m_dpi_scale,
		-(m_border_width + m_border_gap) * m_dpi_scale);

	// Get and rotate text rectangle
	QSize s = m_st_header.size().toSize();
	QRect tr;
	if (m_header_text_orientation == Qt::Horizontal)
	{
		tr = QRect(0, 0, s.width(), s.height());
	}
	else
	{
		tr = QRect(0, 0, s.height(), s.width());
	}
	
	// Align text rectangle in borderless area rectangle
	QRect aligned_tr = rectAlign(r, tr, m_header_alignment);
	
	// Get text rotation and anchor
	double rot = getRotation(m_header_text_orientation, m_header_alignment);
	QPoint anchor = getTextAnchor(aligned_tr, m_header_text_orientation, m_header_alignment);
	
	// Draw rotated
	painter.save();
	painter.translate(anchor);
	painter.rotate(rot);
	painter.setPen(m_header_text_color[m_active?1:0]);
	painter.drawStaticText(QPoint(0, 0), m_st_header);
	painter.restore();
}

void CAnimatedBarChart::drawFrame()
{
	// Draw to back buffer
	QImage *buffer = BackBuffer();
	QPainter painter(buffer);

	// Draw background and border
	painter.save();
	painter.setPen(QPen(m_border_color[m_active ? 1 : 0], m_dpi_scale));
	painter.setBrush(QBrush(m_background_color[m_active ? 1 : 0]));
	//painter.drawRoundedRect(0, 0, buffer->width() - m_dpi_scale, buffer->height() - m_dpi_scale, 2 * m_dpi_scale, 2 * m_dpi_scale);
	painter.drawRect(0, 0, buffer->width(), buffer->height());

	// Draw lines on background
	painter.setPen(QPen(bright(m_background_color[m_active ? 1 : 0]), m_dpi_scale));
	if (m_bars_orientation == Qt::Vertical)
	{
		for (int i = ((m_border_width + m_border_gap) * m_dpi_scale); i < (buffer->height() - ((m_border_width + m_border_gap) * m_dpi_scale)); i += (6 * m_dpi_scale))
		{
			painter.drawLine(QPoint((m_border_width + m_border_gap) * m_dpi_scale, i),
				QPoint(width() - (m_border_width + m_border_gap + 1) * m_dpi_scale, i));
		}
	}
	else
	{
		for (int i = ((m_border_width + m_border_gap) * m_dpi_scale); i < (buffer->width() - ((m_border_width + m_border_gap) * m_dpi_scale)); i += (6 * m_dpi_scale))
		{
			painter.drawLine(QPoint(i, (m_border_width + m_border_gap) * m_dpi_scale),
				QPoint(i, height() - (m_border_width + m_border_gap + 1) * m_dpi_scale));

		}
	}

	painter.restore();

	// Draw all bars
	int barpos = 0;
	barpos += (m_border_width + m_border_gap) * m_dpi_scale;
	for (int n = 0; n < m_bar_info.size(); n++)
	{
		drawBar(painter, barpos, m_bar_info[n]);
	}

	// Draw header
	drawHeader(painter);
	
	// Now swap to front buffer for rendering
	SwapBuffers();

	m_dirty = 0;

	update();
}

void CAnimatedBarChart::setBorderWidth(int width)
{
	m_border_width = width;
	m_dirty = 0xFFFFFFFF;
	recalculateSize();
}

int CAnimatedBarChart::borderWidth(void) const
{
	return m_border_width;
}

void CAnimatedBarChart::setBorderGap(int gap)
{
	m_border_gap = gap;
	m_dirty = 0xFFFFFFFF;
	recalculateSize();
}

int CAnimatedBarChart::borderGap(void) const
{
	return m_border_gap;
}

void CAnimatedBarChart::setBarsOrientation(Qt::Orientation bars_orientation)
{
	m_bars_orientation = bars_orientation;
	m_dirty_bars = 1;
	recalculateSize();
}

Qt::Orientation CAnimatedBarChart::barsOrientation() const
{
	return m_bars_orientation;
}

void CAnimatedBarChart::setBackgroundColor(QColor backgroundcolor, bool active)
{
	m_background_color[active ? 1 : 0] = backgroundcolor;
	m_dirty = 0xFFFFFFFF;
}

QColor CAnimatedBarChart::backgroundColor(bool active) const
{
	return m_background_color[active ? 1 : 0];
}

void CAnimatedBarChart::setBorderColor(QColor bordercolor, bool active)
{
	m_border_color[active ? 1 : 0] = bordercolor;
	m_dirty = 0xFFFFFFFF;
}

QColor CAnimatedBarChart::borderColor(bool active) const
{
	return m_border_color[active ? 1 : 0];
}

void CAnimatedBarChart::setHeaderTextColor(QColor headertextcolor, bool active)
{
	m_header_text_color[active ? 1 : 0] = headertextcolor;
	m_dirty_header = 1;;
}

QColor CAnimatedBarChart::HeaderTextColor(bool active) const
{
	return m_header_text_color[active ? 1 : 0];
}

void CAnimatedBarChart::setHeaderText(const QString & text)
{
	m_header_text = text;
	m_dirty_header = 1;
	m_st_header.setText(text);
}

QString CAnimatedBarChart::headerText() const
{
	return m_header_text;
}

void CAnimatedBarChart::setHeaderTextOrientation(Qt::Orientation orientation)
{
	m_header_text_orientation = orientation;
	m_dirty_header = 1;
	recalculateSize();
}

Qt::Orientation CAnimatedBarChart::headerTextOrientation() const
{
	return m_header_text_orientation;
}

void CAnimatedBarChart::setHeaderTextAlignment(Qt::Alignment align)
{
	m_header_alignment = align;
	m_dirty_header = 1;

	QTextOption to = m_st_header.textOption();
	to.setAlignment(m_header_alignment);
	m_st_header.setTextOption(to);
}

Qt::Alignment CAnimatedBarChart::headerTextAlignment() const
{
	return m_header_alignment;
}

void CAnimatedBarChart::setHeaderTextFont(QFont font)
{
	m_header_font = font;
	m_dirty_header = 1;
	recalculateSize();
}

QFont CAnimatedBarChart::headerTextFont() const
{
	return m_header_font;
}

void CAnimatedBarChart::addBar(QString id)
{
	BARINFO bi = m_default_bi;
	bi.id = id;

	m_bar_index[id] = m_bar_info.size();
	m_bar_info.append(bi);

	m_dirty_bars = 1;
	m_dirty_bars_text = 1;

	recalculateSize();
}

void CAnimatedBarChart::insertBar(int position, QString id)
{
	if (position<0 || position > m_bar_info.size())
	{
		Q_ASSERT(0);
		return;
	}

	BARINFO bi = m_default_bi;
	bi.id = id;

	foreach(QString bid, m_bar_index.keys())
	{
		if (m_bar_index[bid] >= position)
			m_bar_index[bid]++;
	}

	m_bar_index[id] = position;
	m_bar_info.insert(position, bi);

	m_dirty_bars = 1;
	m_dirty_bars_text = 1;

	recalculateSize();
}

int CAnimatedBarChart::barPosition(QString id)
{
	return m_bar_index.value(id, -1);
}

QString CAnimatedBarChart::barId(int position)
{
	if (position < 0 || position >= m_bar_info.size())
	{
		Q_ASSERT(0);
		return QString();
	}
	return m_bar_info[position].id;
}

void CAnimatedBarChart::removeAt(int position)
{
	if (position < 0 || position >= m_bar_info.size())
	{
		Q_ASSERT(0);
		return;
	}

	if (!m_bar_info[position].id.isEmpty())
	{
		m_bar_index.remove(m_bar_info[position].id);
	}
	m_bar_info.removeAt(position);

	m_dirty_bars = 1;
	m_dirty_bars_text = 1;

	recalculateSize();
}

int CAnimatedBarChart::count() const
{
	return m_bar_info.size();
}

void CAnimatedBarChart::setBarValue(QString id, double value)
{
	if (id.isEmpty())
	{
		m_default_bi.value = value;
	}
	else
	{
		if (!m_bar_index.contains(id))
		{
			Q_ASSERT(0);
			return;
		}

		m_bar_info[m_bar_index[id]].value = value;
		m_bar_info[m_bar_index[id]].enabled_mask |= BARINFO::ENABLED_value;
	}
	m_dirty_bars = 1;
}

double CAnimatedBarChart::barValue(QString id) const
{
	if (id.isEmpty())
	{
		return m_default_bi.value;
	}
	
	if (!m_bar_index.contains(id))
	{
		Q_ASSERT(0);
		return 0.0;
	}
	return ENABLED_OR_DEFAULT(m_bar_info[m_bar_index[id]], value);
}

void CAnimatedBarChart::setBarText(QString id, QString text)
{
	if (id.isEmpty())
	{
		m_default_bi.text = text;
	}
	else
	{
		if (!m_bar_index.contains(id))
		{
			Q_ASSERT(0);
			return;
		}
		m_bar_info[m_bar_index[id]].text = text;
		m_bar_info[m_bar_index[id]].enabled_mask |= BARINFO::ENABLED_text;
	}

	m_dirty_bars_text = 1;
	recalculateSize();
}

QString CAnimatedBarChart::barText(QString id) const
{
	if (id.isEmpty())
	{
		return m_default_bi.text;
	}

	if (!m_bar_index.contains(id))
	{
		Q_ASSERT(0);
		return QString();
	}
	return ENABLED_OR_DEFAULT(m_bar_info[m_bar_index[id]], text);
}

void CAnimatedBarChart::setBarToolTip(QString id, QString tiptext)
{
	if (id.isEmpty())
	{
		m_default_bi.tiptext = tiptext;
	}
	else
	{
		if (!m_bar_index.contains(id))
		{
			Q_ASSERT(0);
			return;
		}
		m_bar_info[m_bar_index[id]].tiptext = tiptext;
		m_bar_info[m_bar_index[id]].enabled_mask |= BARINFO::ENABLED_tiptext;
	}
}

QString CAnimatedBarChart::barToolTip(QString id) const
{
	if (id.isEmpty())
	{
		return m_default_bi.tiptext;
	}

	if (!m_bar_index.contains(id))
	{
		Q_ASSERT(0);
		return QString();
	}
	return ENABLED_OR_DEFAULT(m_bar_info[m_bar_index[id]], tiptext);
}


void CAnimatedBarChart::setBarWidth(QString id, int width)
{
	if (id.isEmpty())
	{
		m_default_bi.width = width;
	}
	else
	{
		if (!m_bar_index.contains(id))
		{
			Q_ASSERT(0);
			return;
		}
		m_bar_info[m_bar_index[id]].width = width;
		m_bar_info[m_bar_index[id]].enabled_mask |= BARINFO::ENABLED_width;
	}

	m_dirty = 0xFFFFFFFF;
	recalculateSize();
}

int CAnimatedBarChart::barWidth(QString id) const
{
	if (id.isEmpty())
	{
		return m_default_bi.width;
	}
	if (!m_bar_index.contains(id))
	{
		Q_ASSERT(0);
		return 0;
	}
	return ENABLED_OR_DEFAULT(m_bar_info[m_bar_index[id]], width);
}

void CAnimatedBarChart::setBarGap(QString id, int gap)
{
	if (id.isEmpty())
	{
		m_default_bi.gap = gap;
	}
	else
	{
		if (!m_bar_index.contains(id))
		{
			Q_ASSERT(0);
			return;
		}
		m_bar_info[m_bar_index[id]].gap = gap;
		m_bar_info[m_bar_index[id]].enabled_mask |= BARINFO::ENABLED_gap;
	}

	m_dirty = 0xFFFFFFFF;
	recalculateSize();
}

int CAnimatedBarChart::barGap(QString id) const
{
	if (id.isEmpty())
	{
		return m_default_bi.gap;
	}
	if (!m_bar_index.contains(id))
	{
		Q_ASSERT(0);
		return 0;
	}
	return ENABLED_OR_DEFAULT(m_bar_info[m_bar_index[id]],gap);
}


void CAnimatedBarChart::setBarTextFont(QString id, QFont font)
{
	if (id.isEmpty())
	{
		m_default_bi.font = font;
	}
	else
	{
		if (!m_bar_index.contains(id))
		{
			Q_ASSERT(0);
			return;
		}
		m_bar_info[m_bar_index[id]].font = font;
		m_bar_info[m_bar_index[id]].enabled_mask |= BARINFO::ENABLED_font;
	}
	m_dirty_bars_text = 1;
	recalculateSize();
}

QFont CAnimatedBarChart::barTextFont(QString id) const
{
	if (id.isEmpty())
	{
		return m_default_bi.font;
	}
	if (!m_bar_index.contains(id))
	{
		Q_ASSERT(0);
		return QFont();
	}
	return ENABLED_OR_DEFAULT(m_bar_info[m_bar_index[id]], font);
}

void CAnimatedBarChart::setBarColor(QString id, QColor clr, bool active)
{
	if (id.isEmpty())
	{
		if (active)
		{
			m_default_bi.bar_color_active = clr;
		}
		else
		{
			m_default_bi.bar_color_inactive = clr;
		}		
	}
	else
	{
		if (!m_bar_index.contains(id))
		{
			Q_ASSERT(0);
			return;
		}
		
		if (active)
		{
			m_bar_info[m_bar_index[id]].bar_color_active = clr;
			m_bar_info[m_bar_index[id]].enabled_mask |= BARINFO::ENABLED_bar_color_active;
		}
		else
		{
			m_bar_info[m_bar_index[id]].bar_color_inactive = clr;
			m_bar_info[m_bar_index[id]].enabled_mask |= BARINFO::ENABLED_bar_color_inactive;
		}

	}
	m_dirty_bars = 1;
}

QColor CAnimatedBarChart::barColor(QString id, bool active) const
{
	if (id.isEmpty())
	{
		return active ? ENABLED_OR_DEFAULT(m_default_bi, bar_color_active) : ENABLED_OR_DEFAULT(m_default_bi, bar_color_inactive);
	}

	if (!m_bar_index.contains(id))
	{
		Q_ASSERT(0);
		return QColor();
	}
	return active ? ENABLED_OR_DEFAULT(m_bar_info[m_bar_index[id]], bar_color_active) : ENABLED_OR_DEFAULT(m_bar_info[m_bar_index[id]], bar_color_inactive);
}

void CAnimatedBarChart::setBarTextColor(QString id, QColor clr, bool active)
{
	if (id.isEmpty())
	{
		if (active)
		{
			m_default_bi.text_color_active = clr;
		}
		else
		{
			m_default_bi.text_color_inactive = clr;
		}
	}
	else
	{
		if (!m_bar_index.contains(id))
		{
			Q_ASSERT(0);
			return;
		}

		if (active)
		{
			m_bar_info[m_bar_index[id]].text_color_active  = clr;
			m_bar_info[m_bar_index[id]].enabled_mask |= BARINFO::ENABLED_text_color_active;
		}
		else
		{
			m_bar_info[m_bar_index[id]].text_color_inactive = clr;
			m_bar_info[m_bar_index[id]].enabled_mask |= BARINFO::ENABLED_text_color_inactive;
		}

	}
	m_dirty_bars = 1;
}

QColor CAnimatedBarChart::barTextColor(QString id, bool active) const
{
	if (id.isEmpty())
	{
		return active ? ENABLED_OR_DEFAULT(m_default_bi, text_color_active) : ENABLED_OR_DEFAULT(m_default_bi, text_color_inactive);
	}

	if (!m_bar_index.contains(id))
	{
		Q_ASSERT(0);
		return QColor();
	}
	return active ? ENABLED_OR_DEFAULT(m_bar_info[m_bar_index[id]], text_color_active) : ENABLED_OR_DEFAULT(m_bar_info[m_bar_index[id]], text_color_inactive);
}

void CAnimatedBarChart::setBarTextOrientation(QString id, Qt::Orientation orientation)
{
	if (id.isEmpty())
	{
		m_default_bi.text_orientation = orientation;
	}
	else
	{
		if (!m_bar_index.contains(id))
		{
			Q_ASSERT(0);
			return;
		}

		m_bar_info[m_bar_index[id]].text_orientation = orientation;
		m_bar_info[m_bar_index[id]].enabled_mask |= BARINFO::ENABLED_text_orientation;
	}
	m_dirty_bars_text = 1;
	recalculateSize();
}

Qt::Orientation CAnimatedBarChart::barTextOrientation(QString id) const
{
	if (id.isEmpty())
	{
		return ENABLED_OR_DEFAULT(m_default_bi, text_orientation);
	}

	if (!m_bar_index.contains(id))
	{
		Q_ASSERT(0);
		return Qt::Horizontal;
	}
	return ENABLED_OR_DEFAULT(m_bar_info[m_bar_index[id]], text_orientation);
}

void CAnimatedBarChart::setBarTextAlignment(QString id, Qt::Alignment align)
{
	if (id.isEmpty())
	{
		m_default_bi.text_alignment = align;
	}
	else
	{
		if (!m_bar_index.contains(id))
		{
			Q_ASSERT(0);
			return;
		}
		m_bar_info[m_bar_index[id]].text_alignment = align;
		m_bar_info[m_bar_index[id]].enabled_mask |= BARINFO::ENABLED_text_alignment;
	}

	m_dirty_bars_text = 1;
	recalculateSize();
}

Qt::Alignment CAnimatedBarChart::barTextAlignment(QString id) const
{
	if (id.isEmpty())
	{
		return ENABLED_OR_DEFAULT(m_default_bi, text_alignment);
	}
	else
	{
		if (!m_bar_index.contains(id))
		{
			Q_ASSERT(0);
			return Qt::AlignCenter;
		}
	}
	return ENABLED_OR_DEFAULT(m_bar_info[m_bar_index[id]], text_alignment);
}


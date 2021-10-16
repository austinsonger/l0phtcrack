#include <stdafx.h>
#include <time.h>
#include <iostream>
#include <cmath>
#include <random>
#include <algorithm>

#define GETRANDOM( min, max ) (((rand() % ((max) - (min) + 1))) + min)
#define FLAME_HIDE 4

template<typename F>
QColor interpolate(QColor a, QColor b, float t, F interpolator)
{
	QColor final;

	final = QColor::fromHsv(
		interpolator(a.hue(), b.hue(), t),
		interpolator(a.saturation(), b.saturation(), t),
		interpolator(a.value(), b.value(), t)
		);

	return final.toRgb();
}

int linear(int a, int b, float t)
{
	return a * (1 - t) + b * t;
}


CFireWidget::CFireWidget(QWidget *parent) : QWidget(parent), m_noise(rand())
{
	m_iWidth = 0;
	m_iHeight = 0;
	m_pFireBuffers[0] = NULL;
	m_pFireBuffers[1] = NULL;
	m_iCurFireBuffer = 0;
	m_pCoolBuffer = NULL;
	m_iFireSource = 0;
	m_iFireChance = 10;
	m_iAvgFlameWidth = 35;
	m_iAlpha = 255;
	
	m_hover = false;
	m_fireEnabled = true;
	m_fireAmount = 1.0;
	m_text = "<b>lorem<br>ipsum</b>";
	m_statictext.setTextFormat(Qt::TextFormat::AutoText);
	m_statictext.setText(m_text);
	m_background = QColor(0,0,0);
	m_textcolor = QColor(255,255,255);
	m_activebackground = QColor(255, 255, 255);
	m_activetextcolor = QColor(0, 0, 0);
	m_activehovercolor = QColor(128,128,128);
	m_hovercolor = QColor(128, 128, 128);
	m_activebordercolor = QColor(0, 0, 0);
	m_bordercolor = QColor(0, 0, 0);

	m_active = false;

	m_screenbuf = NULL;

	InitPalette();

	m_refresh.setInterval(40);
	connect(&m_refresh, &QTimer::timeout, this, &CFireWidget::slot_refresh);

	m_refresh.start();
}

CFireWidget::~CFireWidget()
{
	if (m_pFireBuffers[0] != NULL)
		delete[] m_pFireBuffers[0];
	if (m_pFireBuffers[1] != NULL)
		delete[] m_pFireBuffers[1];
	m_pFireBuffers[0] = NULL;
	m_pFireBuffers[1] = NULL;
	m_iCurFireBuffer = 0;

	if (m_screenbuf != NULL)
		delete[] m_screenbuf;

}

void CFireWidget::slot_refresh(void)
{
	if (m_screenbuf == NULL || m_screenbuf_size != size())
	{
		Initialize();
	}

	Render(m_screenbuf);

	repaint();
}

void CFireWidget::enterEvent(QEvent *)
{
	m_hover = true;
	update();
}

void CFireWidget::leaveEvent(QEvent *)
{
	m_hover = false;
	update();
}

void CFireWidget::paintEvent(QPaintEvent*)
{
	QPainter painter(this);

	if (m_fireEnabled && m_screenbuf)
	{
		painter.drawImage(QPoint(0, 0), QImage((uchar *)m_screenbuf, m_screenbuf_size.width(), m_screenbuf_size.height(), QImage::Format::Format_RGB32));
		
		int tx = width() / 2 - (m_statictext.size().width() / 2);
		int ty = height() / 2 - (m_statictext.size().height() / 2);
		painter.setPen(QColor(0,0,0,160));
		painter.drawStaticText(QPoint(tx-1,ty), m_statictext);
		painter.drawStaticText(QPoint(tx+1, ty), m_statictext);
		painter.drawStaticText(QPoint(tx, ty-1), m_statictext);
		painter.drawStaticText(QPoint(tx, ty+1), m_statictext);
		painter.setPen(m_active ? m_activetextcolor : m_textcolor);
		painter.drawStaticText(QPoint(tx, ty), m_statictext);
	}
	else
	{
		painter.fillRect(rect(), m_active ? m_activebackground : m_background);
		
		int tx = width() / 2 - (m_statictext.size().width() / 2);
		int ty = height() / 2 - (m_statictext.size().height() / 2);
		painter.setPen(QColor(0, 0, 0, 80));
		painter.drawStaticText(QPoint(tx - 1, ty), m_statictext);
		painter.drawStaticText(QPoint(tx + 1, ty), m_statictext);
		painter.drawStaticText(QPoint(tx, ty - 1), m_statictext);
		painter.drawStaticText(QPoint(tx, ty + 1), m_statictext);
		painter.setPen(m_active ? m_activetextcolor : m_textcolor);
		painter.drawStaticText(QPoint(tx, ty), m_statictext);
	}

	QPen borderpen(m_active ? m_activebordercolor : m_bordercolor);
	borderpen.setWidth(1);
	painter.setPen(borderpen);
	painter.drawRect(0,0, qMax(width()-1,0),qMax(height()-1,0));

	if (m_hover && isEnabled())
	{
		QPen pen(m_active ? m_activehovercolor : m_hovercolor);
		pen.setWidth(1);
		painter.setPen(pen);
		painter.drawRect(1, 1, qMax(width() - 3, 0), qMax(height() - 3,0));
	}
	
}

void CFireWidget::setActive(bool active)
{
	m_active = active;
	repaint();
}

bool CFireWidget::active(void) const
{
	return m_active;
}


void CFireWidget::setFirePalette(QRgb palette[256])
{
	memcpy(m_pPaletteBuffer, palette, sizeof(QRgb) * 256);
}

const QRgb *CFireWidget::firePalette() const
{
	return m_pPaletteBuffer;
}

void CFireWidget::setBackgroundColor(QColor background, bool active)
{
	if (active)
		m_activebackground = background;
	else
		m_background = background;
	repaint();
}

QColor CFireWidget::backgroundColor(bool active) const
{
	return active ? m_activebackground : m_background;
}

void CFireWidget::setTextColor(QColor textcolor, bool active)
{
	if (active)
		m_activetextcolor = textcolor;
	else
		m_textcolor = textcolor;
	repaint();
}

QColor CFireWidget::textColor(bool active) const
{
	return active?m_activetextcolor:m_textcolor;
}

void CFireWidget::setHoverColor(QColor hovercolor, bool active)
{
	if (active)
		m_activehovercolor = hovercolor;
	else
		m_hovercolor = hovercolor;
	repaint();
}

QColor CFireWidget::hoverColor(bool active) const
{
	return active ? m_activehovercolor : m_hovercolor;
}

void CFireWidget::setBorderColor(QColor bordercolor, bool active)
{
	if (active)
		m_activebordercolor = bordercolor;
	else
		m_bordercolor = bordercolor;
	repaint();
}

QColor CFireWidget::borderColor(bool active) const
{
	return active ? m_activebordercolor : m_bordercolor;
}

void CFireWidget::setFireEnabled(bool enable)
{
	m_fireEnabled = enable;
	if (!m_fireEnabled)
	{
		m_refresh.stop();
		repaint();
	}
	else
	{
		m_refresh.start();
	}
}

bool CFireWidget::fireEnabled(void) const
{
	return m_fireEnabled;
}

void CFireWidget::setFireAmount(double amount)
{
	m_fireAmount = amount;
}

double CFireWidget::fireAmount() const
{
	return m_fireAmount;
}

void CFireWidget::setText(const QString & text)
{
	m_text = text;
	m_statictext.setText(text);
	repaint();
}

QString CFireWidget::text() const
{
	return m_text;
}

void CFireWidget::setTextOption(const QTextOption &textOption)
{
	m_statictext.setTextOption(textOption);
	repaint();
}

void CFireWidget::setTextFormat(Qt::TextFormat textFormat)
{
	m_statictext.setTextFormat(textFormat);
	repaint();
}

////////////////////////////////////////////////////////////////////////////////////

void CFireWidget::Initialize()
{
	m_iHeight = size().height();
	m_iWidth = size().width();
	m_iAvgFlameWidth = m_iWidth / 10;

	InitScreenBuf();
	InitFire();
	InitCool();
}

void CFireWidget::InitScreenBuf()
{
	if (m_screenbuf)
	{
		delete m_screenbuf;
	}
	m_screenbuf = new QRgb[m_iHeight * m_iWidth];
	memset(m_screenbuf, 0, m_iHeight * m_iWidth * sizeof(QRgb));
	m_screenbuf_size = QSize(m_iWidth, m_iHeight);
}

void CFireWidget::InitFire()
{
	if (m_pFireBuffers[0] != NULL)
		delete[] m_pFireBuffers[0];
	if (m_pFireBuffers[1] != NULL)
		delete[] m_pFireBuffers[1];
	m_pFireBuffers[0] = NULL;
	m_pFireBuffers[1] = NULL;
	m_iCurFireBuffer = 0;

	m_sizeFireBuffer = QSize(m_iWidth, m_iHeight + FLAME_HIDE);

	m_pFireBuffers[0] = new quint8[m_sizeFireBuffer.width() * m_sizeFireBuffer.height()];
	m_pFireBuffers[1] = new quint8[m_sizeFireBuffer.width() * m_sizeFireBuffer.height()];

	memset(m_pFireBuffers[0], 0, m_sizeFireBuffer.width() * m_sizeFireBuffer.height());
	memset(m_pFireBuffers[1], 0, m_sizeFireBuffer.width() * m_sizeFireBuffer.height());
}

void CFireWidget::InitCool()
{
	if (m_pCoolBuffer != NULL)
		delete[] m_pCoolBuffer;
	m_pCoolBuffer = NULL;

	m_sizeCoolBuffer = m_sizeFireBuffer;
	int w = m_sizeCoolBuffer.width();
	int h = m_sizeCoolBuffer.height();
	
	m_pCoolBuffer = new quint8[w*h];
	memset(m_pCoolBuffer, 0, w*h);
	m_cool_z = 0;

	AnimateCoolBuffer();
}

void CFireWidget::AnimateCoolBuffer()
{
	int w = m_sizeCoolBuffer.width();
	int h = m_sizeCoolBuffer.height();

	double nx, ny;
	if (w > h)
	{
		nx = w / 4;
		ny = nx * h / w;
	}
	else
	{
		ny = h / 4;
		nx = ny * h / w;
	}

	// Make cool buffer pattern
	int p = 0;
	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			double dx = (double)x / (double)w;
			double dy = (double)y / (double)h;

			double n = m_noise.noise(nx * dx, ny * dy, m_cool_z);

			m_pCoolBuffer[p] = floor(255 * n);
			p++;
		}
	}

	m_cool_z++;
}


void CFireWidget::ClrHotSpots()
{
	// clear the fire line
	memset(BackBuffer() + (m_iFireSource*m_iWidth), 0, m_iWidth);
}

void CFireWidget::InitPalette()
{
	int FireColorStops[4];
	QColor FireColors[4];

	FireColorStops[0] = 0;
	FireColors[0] = qRgb(0, 0, 0); // Black

	FireColorStops[1] = 85;
	FireColors[1] = qRgb(163, 44, 0); // Red

	FireColorStops[2] = 170;
	FireColors[2] = qRgb(248, 126, 27); // Orange

	FireColorStops[3] = 255;
	FireColors[3] = qRgb(253, 255, 205); // White

	// Create a gradient between all the colors we have for our fire...
	for (int iColor = 1; iColor < _countof(FireColors); iColor++)
	{
		QColor acol = FireColors[iColor - 1];
		int astop = FireColorStops[iColor - 1];

		QColor bcol = FireColors[iColor];
		int bstop = FireColorStops[iColor];

		for (int i = astop; i <= bstop; i++)
		{
			m_pPaletteBuffer[i] = interpolate(acol, bcol, ((float)(i - astop)) / (float)(bstop - astop), &linear).rgb();
		}
	}
}


void CFireWidget::SetHotSpots()
{
	ClrHotSpots();

	long lPosition = 0;
	while (lPosition < m_iWidth)
	{
		// see if we should do a flame
		if (GETRANDOM(0, 100) < m_iFireChance)
		{
			// get a flame width
			long lFlameWidth = GETRANDOM(1, m_iAvgFlameWidth);
			for (int i = 0; i < lFlameWidth; i++)
			{
				if (lPosition < m_iWidth)
				{
					BackBuffer()[m_iFireSource*m_iWidth + lPosition] = 254;// set a hot spot! 
					lPosition++;
				}
			}
		}
		lPosition++;
	}
}

void CFireWidget::MakeLines()
{
	int x, y;

	int w = m_sizeFireBuffer.width();
	int h = m_sizeFireBuffer.height();

	int bufoff = m_iFireSource * m_iWidth;

	quint8 *frontbuf = FrontBuffer() + bufoff + w;
	quint8 *backbuf = BackBuffer() + bufoff;
	quint8 *coolbuf = CoolBuffer() + bufoff + w;

	int fa = (int)(m_fireAmount * 256.0);

	for (y = m_iFireSource; y < (h - 1); y++)
	{
		for (x = 0; x < w; x++)
		{
			int c = Average(backbuf, x, y);
			int cool = *coolbuf;
			int coolgradient = cool * y / (2 * h);
			int coolfactor = (cool / (h * fa / 256));
			c -= coolfactor;
			c = qMax(c, 0);
			
			*frontbuf = (quint8)c;

			frontbuf++;
			backbuf++;
			coolbuf++;
		}
	}
}

quint8 CFireWidget::Average(quint8 *src, int x, int y)
{
	quint32 ave = 0;

	const int width = m_sizeFireBuffer.width();
	const int height = m_sizeFireBuffer.height();

	//	ave =  (quint32) *(src);
	ave += (quint32)*(src + 1 - ((x >= width - 1) ? width : 0));
	//	ave += (quint32) *(src + 2 - ((x >= width - 2) ? width : 0));
	ave += (quint32)*(src - 1 + ((x < 1) ? width : 0));
	//	ave += (quint32) *(src - 2 + ((x < 2) ? width : 0));
	ave += (quint32)*(src - width + ((y < 1) ? (width * 2) : 0));
	ave += (quint32)*(src + width - ((y >= height - 1) ? (width * 2) : 0));

	ave >>= 2;

	return (quint8)ave;
}

void CFireWidget::Render(QRgb* pVideoMemory)
{
//	AnimateCoolBuffer();
	SetHotSpots();  // generate random hotspots  
	MakeLines();	// do all the math and screen updating  
	SwapBuffers();
	SetHotSpots();  // generate random hotspots  
	MakeLines();	// do all the math and screen updating  

	quint8 *src, *dst;
	QRgb rgb;

	dst = (quint8*)pVideoMemory;
	src = FrontBuffer() + (m_sizeFireBuffer.height() * m_sizeFireBuffer.width()) - 1;

	for (int y = 0; y < m_iHeight; y++)
	{
		for (int x = 0; x < m_iWidth; x++)
		{	
			rgb = m_pPaletteBuffer[*src];
			if (!m_active)
			{
				int gray = qGray(rgb);
				rgb = qRgb(gray, gray, gray);
			}

			dst[2] = (quint8)(((qRed(rgb) - dst[2])*m_iAlpha + (dst[2] << 8)) >> 8);
			dst[1] = (quint8)(((qGreen(rgb) - dst[1])*m_iAlpha + (dst[1] << 8)) >> 8);
			dst[0] = (quint8)(((qBlue(rgb) - dst[0])*m_iAlpha + (dst[0] << 8)) >> 8);

			//dst[2] = qRed(rgb);
			//dst[1] = qGreen(rgb);
			//dst[0] = qBlue(rgb);

			//dst[0] = dst[1] = dst[2] = CoolBuffer()[(y * m_iWidth) + x];

			dst += 4;
			src--;
		}
	}

	SwapBuffers();
}

////////////////////////////////////////////////////////////////////

PerlinNoise::PerlinNoise() 
{
	p = {
		151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142,
		8, 99, 37, 240, 21, 10, 23, 190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117,
		35, 11, 32, 57, 177, 33, 88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71,
		134, 139, 48, 27, 166, 77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41,
		55, 46, 245, 40, 244, 102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89,
		18, 169, 200, 196, 135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226,
		250, 124, 123, 5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182,
		189, 28, 42, 223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167,
		43, 172, 9, 129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246,
		97, 228, 251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239,
		107, 49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254,
		138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180 };

	// Duplicate the permutation vector
	p.insert(p.end(), p.begin(), p.end());
}

// Generate a new permutation vector based on the value of seed
PerlinNoise::PerlinNoise(unsigned int seed) 
{
	p.resize(256);

	// Fill p with values from 0 to 255
	std::iota(p.begin(), p.end(), 0);

	// Initialize a random engine with seed
	std::default_random_engine engine(seed);

	// Suffle  using the above random engine
	std::shuffle(p.begin(), p.end(), engine);

	// Duplicate the permutation vector
	p.insert(p.end(), p.begin(), p.end());
}

double PerlinNoise::noise(double x, double y, double z)
{
	// Find the unit cube that contains the point
	int X = (int)floor(x) & 255;
	int Y = (int)floor(y) & 255;
	int Z = (int)floor(z) & 255;

	// Find relative x, y,z of point in cube
	x -= floor(x);
	y -= floor(y);
	z -= floor(z);

	// Compute fade curves for each of x, y, z
	double u = fade(x);
	double v = fade(y);
	double w = fade(z);

	// Hash coordinates of the 8 cube corners
	int A = p[X] + Y;
	int AA = p[A] + Z;
	int AB = p[A + 1] + Z;
	int B = p[X + 1] + Y;
	int BA = p[B] + Z;
	int BB = p[B + 1] + Z;

	// Add blended results from 8 corners of cube
	double res = lerp(w, lerp(v, lerp(u, grad(p[AA], x, y, z), grad(p[BA], x - 1, y, z)), lerp(u, grad(p[AB], x, y - 1, z), grad(p[BB], x - 1, y - 1, z))), lerp(v, lerp(u, grad(p[AA + 1], x, y, z - 1), grad(p[BA + 1], x - 1, y, z - 1)), lerp(u, grad(p[AB + 1], x, y - 1, z - 1), grad(p[BB + 1], x - 1, y - 1, z - 1))));
	return (res + 1.0) / 2.0;
}

double PerlinNoise::fade(double t) 
{
	return t * t * t * (t * (t * 6 - 15) + 10);
}

double PerlinNoise::lerp(double t, double a, double b)
{
	return a + t * (b - a);
}

double PerlinNoise::grad(int hash, double x, double y, double z) 
{
	int h = hash & 15;
	// Convert lower 4 bits of hash inot 12 gradient directions
	double u = h < 8 ? x : y,
		v = h < 4 ? y : h == 12 || h == 14 ? x : z;
	return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}
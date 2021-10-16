#ifndef __INC_CFIREWIDGET_H
#define __INC_CFIREWIDGET_H

#include <vector>

class PerlinNoise {
	// The permutation vector
	std::vector<int> p;
public:
	// Initialize with the reference values for the permutation vector
	PerlinNoise();
	// Generate a new permutation vector based on the value of seed
	PerlinNoise(unsigned int seed);
	// Get a noise value, for 2D images z can have any value
	double noise(double x, double y, double z);
private:
	double fade(double t);
	double lerp(double t, double a, double b);
	double grad(int hash, double x, double y, double z);
};

class CFireWidget:public QWidget
{
	Q_OBJECT

private:

	QTimer m_refresh;

	// props
	int	m_iFlameHeight;
	int	m_iWidth;
	int	m_iHeight;
	int	m_iFireSource;	//The y position for the lit spots
	int	m_iFireChance;
	int	m_iAvgFlameWidth;
	int	m_iAlpha;

	int m_iCurFireBuffer;
	quint8 *m_pFireBuffers[2];
	QSize m_sizeFireBuffer;

	quint8 *m_pCoolBuffer;
	QSize m_sizeCoolBuffer;
	double m_cool_z;

	PerlinNoise m_noise;

	
	inline quint8 *FrontBuffer() { return m_pFireBuffers[m_iCurFireBuffer]; }
	inline quint8 *BackBuffer() { return m_pFireBuffers[1-m_iCurFireBuffer]; }
	inline void SwapBuffers() { m_iCurFireBuffer = 1 - m_iCurFireBuffer; }
	inline quint8 *CoolBuffer() { return m_pCoolBuffer; }

	QRgb m_pPaletteBuffer[256];
	int *m_pYIndexes;

	QRgb *m_screenbuf;
	QSize m_screenbuf_size;

	QStaticText m_statictext;

	bool m_hover;
	bool m_active;
	bool m_fireEnabled;
	double m_fireAmount;
	QString m_text;
	QColor m_activebackground;
	QColor m_background;
	QColor m_activetextcolor;
	QColor m_textcolor;
	QColor m_activehovercolor;
	QColor m_hovercolor;
	QColor m_activebordercolor;
	QColor m_bordercolor;

public:
	CFireWidget(QWidget *parent);
	virtual ~CFireWidget();

	void setActive(bool active);
	bool active(void) const;
	void setBackgroundColor(QColor backgroundcolor, bool active);
	QColor backgroundColor(bool active) const;
	void setTextColor(QColor textcolor, bool active);
	QColor textColor(bool active) const;
	void setHoverColor(QColor hovercolor, bool active);
	QColor hoverColor(bool active) const;
	void setBorderColor(QColor bordercolor, bool active);
	QColor borderColor(bool active) const;
	void setText(const QString & text);
	QString text() const;

	void setTextOption(const QTextOption &textOption);
	void setTextFormat(Qt::TextFormat textFormat);

	void setFireEnabled(bool enable);
	bool fireEnabled(void) const;
	void setFirePalette(QRgb palette[256]);
	const QRgb *firePalette() const;
	void setFireAmount(double amount);
	double fireAmount() const;
	
	
protected:
	void Initialize();
	void InitScreenBuf();
	void InitFire();
	void InitCool();
	void InitPalette();
	void ClrHotSpots();
	void SetHotSpots();
	void MakeLines();
	void AnimateCoolBuffer();
	
	void Render(QRgb * pVideoMemory);
	quint8 Average(quint8 *src, int x, int y);
	
	virtual void paintEvent(QPaintEvent*);
	virtual void enterEvent(QEvent *);
	virtual void leaveEvent(QEvent *);
	

private slots:
	void slot_refresh(void);

};

#endif
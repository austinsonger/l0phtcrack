#include<stdafx.h>

CLabelRadioButton::CLabelRadioButton(QString text, QWidget *parent) :QWidget(parent)
{
	TR;
	m_radiobutton = new QRadioButton(parent);
	m_radiobutton->setText("");

	m_label = new CLabelClickable(parent);
	m_label->setText(text);
	m_label->setWordWrap(true);
	m_label->setBuddy(m_radiobutton);
	m_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	
	connect(m_label, &CLabelClickable::clicked, this, &CLabelRadioButton::slot_labelClicked);

	QHBoxLayout *layout = new QHBoxLayout(this);
	layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	layout->setMargin(4);
//	layout->setContentsMargins(0, 0, 0, 0);

	layout->addWidget(m_radiobutton);
	layout->addWidget(m_label);

	setLayout(layout);

	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

CLabelRadioButton::~CLabelRadioButton()
{
}

void CLabelRadioButton::slot_labelClicked(void)
{
	m_radiobutton->click();
}

QRadioButton *CLabelRadioButton::radioButton()
{
	return m_radiobutton;
}

QLabel *CLabelRadioButton::label()
{
	return m_label;
}


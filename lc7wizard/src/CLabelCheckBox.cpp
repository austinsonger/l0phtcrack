#include<stdafx.h>

CLabelCheckBox::CLabelCheckBox(QString text, QWidget *parent):QWidget(parent)
{
	TR;
	m_checkbox = new QCheckBox(parent);
	m_checkbox->setText("");

	m_label = new CLabelClickable(parent);
	m_label->setText(text);
	m_label->setWordWrap(true);
	m_label->setBuddy(m_checkbox);
	m_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	connect(m_label, &CLabelClickable::clicked, this, &CLabelCheckBox::slot_labelClicked);

	QHBoxLayout *layout = new QHBoxLayout(this);
	layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);

	layout->addWidget(m_checkbox);
	layout->addWidget(m_label);

	setLayout(layout);

	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

CLabelCheckBox::~CLabelCheckBox()
{
}


void CLabelCheckBox::slot_labelClicked(void)
{
	m_checkbox->click();
}

QCheckBox *CLabelCheckBox::checkBox()
{
	return m_checkbox;
}

QLabel *CLabelCheckBox::label()
{
	return m_label;
}


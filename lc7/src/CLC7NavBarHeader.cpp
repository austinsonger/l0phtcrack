#include"stdafx.h"

CLC7NavBarHeader::CLC7NavBarHeader(CLC7NavBarGroup *group):QLabel(NULL)
{
	m_group=group;
	
	setAlignment(Qt::AlignCenter);
	//setFixedHeight(24);
	setMargin(4);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

CLC7NavBarHeader::~CLC7NavBarHeader()
{TR;
	
}

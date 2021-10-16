#include"stdafx.h"

CLC7JohnConfSection::CLC7JohnConfSection(QString name)
{
	m_name=name;
}

QString CLC7JohnConfSection::getName()
{
	return m_name;
}

QMap<QString,QString> CLC7JohnConfSection::getValues()
{
	return m_values;
}

QVector<QString> CLC7JohnConfSection::getLines()
{
	return m_lines;
}

////////////////////////////////////////////////////////////////////////////////////

CLC7JohnConf::CLC7JohnConf()
{
	m_valid=true;

	QDir lc7jtrdir(g_pLinkage->GetPluginsDirectory());
	lc7jtrdir.cd("lc7jtr{9846c8cf-1db1-467e-83c2-c5655aa81936}");
	
	if(!parse(lc7jtrdir.filePath("john.conf"),lc7jtrdir.absolutePath()))
	{
		m_valid=false;
	}
//	if(!parse(lc7jtrdir.filePath("lc7.conf"),lc7jtrdir.absolutePath()))
//	{
//		m_valid=false;
//	}
}

CLC7JohnConf::~CLC7JohnConf()
{
	foreach(CLC7JohnConfSection *section,m_section_map)
	{
		delete section;
	}
}

bool CLC7JohnConf::isValid()
{
	return m_valid;
}

QMap<QString,CLC7JohnConfSection *> CLC7JohnConf::getSectionMap()
{
	return m_section_map;
}

bool CLC7JohnConf::parse(QString filename, QString $john_value)
{
	QList<QTextStream *> instack;

	filename.replace("$JOHN", $john_value);

	QFile *file=new QFile(filename);
	if(!file->open(QIODevice::ReadOnly))
	{
		delete file;
		return false;
	}
	instack.push_back(new QTextStream(file->readAll()));
	delete file;

	QString line;
	CLC7JohnConfSection *cursection=NULL;
	bool cur_is_kv;
	while(instack.size()>0)
	{
		QTextStream *pts=instack.back();
		instack.pop_back();
		
		while(!pts->atEnd())
		{
			line=pts->readLine();
			if(line.startsWith("#"))
			{
				// Comment
			}
			else if(line.startsWith(".include"))
			{
				QString incpath=line.mid(8).trimmed();
				if(incpath.startsWith("["))
				{
					QRegExp re("\\[(.*)\\]");
					int pos=re.indexIn(line);
					if(pos!=-1)
					{
						QString section_name=re.cap(1);
						// Look up section and append
						if(m_section_map.contains(section_name))
						{
							CLC7JohnConfSection *incsection=m_section_map[section_name];
							int beforesize=cursection->m_lines.size();
							int tocopy=incsection->m_lines.size();
							cursection->m_lines.resize(beforesize+tocopy);
							qCopy(incsection->m_lines.begin(),incsection->m_lines.end(),cursection->m_lines.begin()+beforesize);
							cursection->m_values.unite(incsection->m_values);
						}
					}
				}
				else
				{
					if(incpath.startsWith("<") && incpath.endsWith(">") && incpath.size()>=2)
					{
						incpath=$john_value+"/"+incpath.mid(1,incpath.size()-2);
					}
					else if(incpath.startsWith("\"") && incpath.endsWith("\"") && incpath.size()>=2)
					{
						incpath=incpath.mid(1,incpath.size()-2).replace("$JOHN", $john_value);
					}
						
					// Load new file
					QFile *incfile=new QFile(incpath);
					if(!incfile->open(QIODevice::ReadOnly))
					{
						delete incfile;
		
						if(!incpath.endsWith("/john.local.conf"))
						{
							Q_FOREACH(QTextStream *t,instack) delete t;
							return false;
						}
					}
					else
					{
						QTextStream *incts=new QTextStream(incfile->readAll());
						delete incfile;
						instack.push_back(pts);
						pts=incts;
					}
				}
			}
			else if(line.startsWith("["))
			{
				// New Section
				QRegExp re("\\[(.*)\\]");
				int pos=re.indexIn(line);
				if(pos!=-1)
				{
					QString section_name=re.cap(1);
					cur_is_kv=!section_name.startsWith("List.");
				
					QMap<QString,CLC7JohnConfSection *>::iterator iter=m_section_map.find(section_name);
					if(iter==m_section_map.end())
					{
						cursection=new CLC7JohnConfSection(section_name);
						m_section_map.insert(section_name,cursection);
					}
					else
					{
						cursection=*iter;
					}
				}
			}
			else if(cursection)
			{
				if(cur_is_kv)
				{
					// Key/Value
					int pos=line.indexOf('=');
					if(pos!=-1)
					{
						QString key;
						QString value;
						key=line.left(pos).trimmed();
						value=line.mid(pos+1).trimmed();
						cursection->m_values.insert(key,value);
					}
				}
				else
				{
					// Just lines
					cursection->m_lines.append(line);
				}
			}
		}
		
		delete pts;
	}
	return true;
}

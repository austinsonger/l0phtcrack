#ifndef __INC_CLC7JOHNCONF_H
#define __INC_CLC7JOHNCONF_H

#include<QtCore>

class CLC7JohnConfSection
{
private:
	friend class CLC7JohnConf;

//	QVector m_keys;
	QString m_name;
	QMap<QString,QString> m_values;
	QVector<QString> m_lines;

	bool load(QDataStream & in);
	bool save(QDataStream & out);

public:
	CLC7JohnConfSection(QString name);

	QString getName();
	QMap<QString,QString> getValues();
	QVector<QString> getLines();
};

class CLC7JohnConf
{
private:
	bool m_valid;
//	QVector m_section_keys;
	QMap<QString,CLC7JohnConfSection *> m_section_map;

public:

	CLC7JohnConf();
	~CLC7JohnConf();
	bool isValid();

	QMap<QString,CLC7JohnConfSection *> getSectionMap();

protected:
	bool load_cache(QString filename, QString $john_value);
	bool save_cache(QString filename, QString $john_value);
	bool parse(QString filename, QString $john_value);
};

#endif

#ifndef __INC_LC7REMEDIATION_H
#define __INC_LC7REMEDIATION_H

struct LC7Remediation
{
	QUuid component;
	QString command;
	QString description;
	QMap<QString, QVariant> config;

	inline LC7Remediation() {}
	inline LC7Remediation(QUuid _component, QString _command, QString _description, const QMap<QString, QVariant> & _config):
		component(_component), command(_command), description(_description), config(_config) {}
	inline LC7Remediation(const LC7Remediation &copy):
		component(copy.component), command(copy.command), description(copy.description), config(copy.config) {}
};

inline QDataStream &operator<<(QDataStream & ds, const LC7Remediation &remediation)
{
	ds << remediation.component;
	ds << remediation.command;
	ds << remediation.description;
	ds << remediation.config;
	return ds;
}

inline QDataStream &operator>>(QDataStream & ds, LC7Remediation &remediation)
{
	ds >> remediation.component;
	ds >> remediation.command;
	ds >> remediation.description;
	ds >> remediation.config;
	return ds;
}

typedef QList<LC7Remediation> LC7Remediations;

#endif
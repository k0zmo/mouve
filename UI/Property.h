#pragma once

#include "Prerequisites.h"
#include "Logic/NodeType.h"

#include <QStringList>
#include <QVector>
#include <QVariant>
#include <QStyleOption>

class QStyleOptionViewItem;
class QPainter;

class Property
{
public:
	Property(const QString& name,
		const QVariant& value,
		EPropertyType type,
		Property* parent = nullptr);

	virtual ~Property();

	/// Dodaje na koniec listy nowego potomka
	void appendChild(Property* child);

	/// Zwraca potomka o wskazanym indeksie
	Property* child(int row);

	/// Zwraca liczbe potomkow
	int childCount() const;

	/// Zwraca indeks jaki Wlasciwosc zajmuje u przodka
	int childNumber() const;

	Property* parent() const;
	bool isRoot() const;

	QString name() const;
	void setName(const QString& name);

	virtual QVariant value(int role = Qt::UserRole) const;
	virtual void setValue(const QVariant& value,
		int role = Qt::UserRole);

	// Type is immutable
	EPropertyType type() const;

	virtual QWidget* createEditor(QWidget* parent,
		const QStyleOptionViewItem& option);

	virtual bool setEditorData(QWidget* editor,
		const QVariant& data);

	virtual QVariant editorData(QWidget* editor);

	virtual bool paint(QPainter* painter,
		const QStyleOptionViewItem& option,
		const QVariant& value);

protected:
	QVector<Property*> _children;
	Property* _parent;
	QString _name;
	QVariant _value;
	EPropertyType _type;
};

class DoubleProperty : public Property
{
public:
	DoubleProperty(const QString& name, double value);

	QWidget* createEditor(QWidget* parent,
		const QStyleOptionViewItem& option) override;
	bool setEditorData(QWidget* editor,
		const QVariant& data) override;
	QVariant editorData(QWidget* editor) override;

	///
	/// min, max, step, decimals(int)
	///
};

class EnumProperty : public Property
{
public:
	EnumProperty(const QString& name, 
		const QStringList& valueList,
		int currentIndex = 0);

	QVariant value(int role = Qt::UserRole) const override;
	void setValue(const QVariant& value, 
		int role = Qt::UserRole) override;

	QWidget* createEditor(QWidget* parent,
		const QStyleOptionViewItem& option) override;
	bool setEditorData(QWidget* editor,
		const QVariant& data) override;
	QVariant editorData(QWidget* editor) override;

private:
	QStringList _valueList;
};

class IntegerProperty : public Property
{
public:
	IntegerProperty(const QString& name, int value);

	QWidget* createEditor(QWidget* parent,
		const QStyleOptionViewItem& option) override;
	bool setEditorData(QWidget* editor,
		const QVariant& data) override;
	QVariant editorData(QWidget* editor) override;
};

class StringProperty : public Property
{
public:
	StringProperty(const QString& name, const QString& value);

	// We use default factory here for string (not need for widget customization for now)
};

class BooleanProperty : public Property
{
public:
	BooleanProperty(const QString& name, bool value);

	QWidget* createEditor(QWidget* parent,
		const QStyleOptionViewItem& option) override;
	bool setEditorData(QWidget* editor,
		const QVariant& data) override;
	QVariant editorData(QWidget* editor) override;

	bool paint(QPainter* painter, 
		const QStyleOptionViewItem& option,
		const QVariant& value) override;
};

class FilePathProperty : public Property
{
public:
	FilePathProperty(const QString& name, 
					 const QString& initialPath = QString());

	QWidget* createEditor(QWidget* parent,
		const QStyleOptionViewItem& option) override;
	bool setEditorData(QWidget* editor,
		const QVariant& data) override;
	QVariant editorData(QWidget* editor) override;
};

//
// WIP
//
/*
struct vec3
{
	double x, y, z;
};

class Vector3Property : public Property
{
public:
	Vector3Property(const QString& name, vec3 value)
		: Property(name, QVariant(0), EPropertyType::Vector3, nullptr)
		, _x(new DoubleProperty("X", value.x))
		, _y(new DoubleProperty("Y", value.y))
		, _z(new DoubleProperty("Z", value.z))
	{
		appendChild(_x);
		appendChild(_y);
		appendChild(_z);
	}

	QVariant value(int role = Qt::UserRole) const override
	{
		Q_UNUSED(role);

		QString str = QString("[ %1 %2 %3 ]")
			.arg(_x->value().toDouble())
			.arg(_y->value().toDouble())
			.arg(_z->value().toDouble());

		return str;
	}

private:
	DoubleProperty* _x;
	DoubleProperty* _y;
	DoubleProperty* _z;
};
*/

inline Property* Property::child(int row)
{ return _children.value(row); }

inline int Property::childCount() const
{ return _children.count(); }

inline Property* Property::parent() const
{ return _parent; }

inline bool Property::isRoot() const
{ return childCount() > 0; }

inline QString Property::name() const
{ return _name; }

inline void Property::setName(const QString& name)
{ _name = name; }

inline QVariant Property::value(int role) const
{ Q_UNUSED(role);
  return _value; }

inline void Property::setValue(const QVariant& value, int role)
{ Q_UNUSED(role);
  _value = value; /* emit() ??*/ }

// Type is immutable
inline EPropertyType Property::type() const
{ return _type; }

inline QWidget* Property::createEditor(QWidget* parent,
									   const QStyleOptionViewItem& option)
{ Q_UNUSED(parent); Q_UNUSED(option);
  return nullptr; }

inline bool Property::setEditorData(QWidget* editor,
									const QVariant& data)
{ Q_UNUSED(editor); Q_UNUSED(data);
  return false; }

inline QVariant Property::editorData(QWidget* editor)
{ Q_UNUSED(editor);
  return QVariant(); }

inline bool Property::paint(QPainter* painter, 
							const QStyleOptionViewItem& option, 
							const QVariant& value)
{ Q_UNUSED(painter); Q_UNUSED(option); Q_UNUSED(value);
  return false; }

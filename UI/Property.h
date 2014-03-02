#pragma once

#include "Prerequisites.h"
#include "Logic/NodeProperty.h"

#include <QStringList>
#include <QVector>
#include <QVariant>
#include <QStyleOption>

#include <QFileInfo>

class QStyleOptionViewItem;
class QPainter;

Q_DECLARE_METATYPE(Enum)
Q_DECLARE_METATYPE(Matrix3x3)
Q_DECLARE_METATYPE(Filepath)

typedef QList<QPair<QString, QString>> PropertyHintList;

class Property
{
public:
	explicit Property(const QString& name,
		const QVariant& value,
		EPropertyType type,
		Property* parent = nullptr);

	virtual ~Property();

	PropertyID propertyID() const;
	void setPropertyID(PropertyID propID);

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
	virtual bool setValue(const QVariant& value,
		int role = Qt::UserRole);

	// Type is immutable
	EPropertyType type() const;

	virtual QWidget* createEditor(QWidget* parent);

	virtual bool setEditorData(QWidget* editor,
		const QVariant& data);

	virtual QVariant editorData(QWidget* editor);

	virtual bool paint(QPainter* painter,
		const QStyleOptionViewItem& option,
		const QVariant& value);

	virtual void setUiHints(const PropertyHintList& list) { Q_UNUSED(list); }

protected:
	QVector<Property*> _children;
	Property* _parent;
	QString _name;
	QVariant _value;
	EPropertyType _type;
	PropertyID _propID;
};

class DoubleProperty : public Property
{
public:
	explicit DoubleProperty(const QString& name, double value);

	QWidget* createEditor(QWidget* parent) override;
	bool setEditorData(QWidget* editor,
		const QVariant& data) override;
	QVariant editorData(QWidget* editor) override;

	void setUiHints(const PropertyHintList& list) override;

private:
	double _min;
	double _max;
	double _step;
	int _decimals;
};

class EnumProperty : public Property
{
public:
	explicit EnumProperty(const QString& name, Enum value);

	QVariant value(int role = Qt::UserRole) const override;
	bool setValue(const QVariant& value, 
		int role = Qt::UserRole) override;

	QWidget* createEditor(QWidget* parent) override;
	bool setEditorData(QWidget* editor,
		const QVariant& data) override;
	QVariant editorData(QWidget* editor) override;

	void setUiHints(const PropertyHintList& list) override;

private:
	QStringList _valueList;
};

class IntegerProperty : public Property
{
public:
	explicit IntegerProperty(const QString& name, int value);

	QWidget* createEditor(QWidget* parent) override;
	bool setEditorData(QWidget* editor,
		const QVariant& data) override;
	QVariant editorData(QWidget* editor) override;

	void setUiHints(const PropertyHintList& list) override;

private:
	int _min;
	int _max;
	int _step;
	bool _wrap;
};

class StringProperty : public Property
{
public:
	explicit StringProperty(const QString& name, const QString& value);

	// We use default factory here for string (not need for widget customization for now)
};

class BooleanProperty : public Property
{
public:
	explicit BooleanProperty(const QString& name, bool value);

	QWidget* createEditor(QWidget* parent) override;
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
	explicit FilePathProperty(const QString& name, 
		const Filepath& initialPath = Filepath());

	QVariant value(int role = Qt::UserRole) const override;
	bool setValue(const QVariant& value, 
		int role = Qt::UserRole) override;

	QWidget* createEditor(QWidget* parent) override;
	bool setEditorData(QWidget* editor,
		const QVariant& data) override;
	QVariant editorData(QWidget* editor) override;

	void setUiHints(const PropertyHintList& list) override;

private:
	QFileInfo _fileInfo;
	QString _filter;
	bool _save;
};

// For now - its simple 3x3 matrix
class MatrixProperty : public Property
{
public:
	explicit MatrixProperty(const QString& name,
		const Matrix3x3& initial = Matrix3x3(1));

	QVariant value(int role = Qt::UserRole) const override;

	QWidget* createEditor(QWidget* parent) override;
	bool setEditorData(QWidget* editor,
		const QVariant& data) override;
	QVariant editorData(QWidget* editor) override;
};

inline PropertyID Property::propertyID() const
{ return _propID; }

inline void Property::setPropertyID(PropertyID propID)
{ _propID = propID; }

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
{ Q_UNUSED(role); return _value; }

inline bool Property::setValue(const QVariant& value, int role)
{ Q_UNUSED(role);
if(_value != value) { _value = value; return true; } return false; }

// Type is immutable
inline EPropertyType Property::type() const
{ return _type; }

inline QWidget* Property::createEditor(QWidget* parent)
{ Q_UNUSED(parent); return nullptr; }

inline bool Property::setEditorData(QWidget* editor,
									const QVariant& data)
{ Q_UNUSED(editor); Q_UNUSED(data);
  return false; }

inline QVariant Property::editorData(QWidget* editor)
{ Q_UNUSED(editor); return QVariant(); }

inline bool Property::paint(QPainter* painter, 
							const QStyleOptionViewItem& option, 
							const QVariant& value)
{ Q_UNUSED(painter); Q_UNUSED(option); Q_UNUSED(value); return false; }

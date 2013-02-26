#include "Property.h"

#include <QDoubleSpinBox>
#include <QComboBox>
#include <QApplication>

#include "PropertyWidgets.h"

template <typename T>
bool tryConvert(const QString& str, T& value) 
{ static_assert(false, "conversion doesn't exist"); return false; }


template <> 
bool tryConvert<int>(const QString& str, int& value)
{
	bool ok;
	int v = str.toInt(&ok);
	if(ok)
		value = v;
	return ok;
}

template <> 
bool tryConvert<bool>(const QString& str, bool& value)
{
	QString lowerCase = str.toLower();
	if(lowerCase == QStringLiteral("true")
		|| lowerCase == QStringLiteral("1"))
	{
		value = true;
		return true;
	}
	else if(lowerCase == QStringLiteral("false")
		|| lowerCase == QStringLiteral("0"))
	{
		value = false;
		return true;
	}
	return false;
}

template <>
bool tryConvert<double>(const QString& str, double& value)
{
	bool ok;
	double v = str.toDouble(&ok);
	if(ok)
		value = v;
	return ok;
}

Property::Property(const QString& name,
				   const QVariant& value,
				   EPropertyType type,
				   Property* parent)
	: _parent(parent)
	, _name(name)
	, _value(value)
	, _type(type)
	, _propID(InvalidPropertyID)
{
	// Check if type matches
	switch(type)
	{
	case EPropertyType::Unknown:
		break;

	case EPropertyType::Boolean:
		Q_ASSERT(QMetaType::Type(value.type()) == QMetaType::Bool);
		break;

	case EPropertyType::Integer:
		Q_ASSERT(QMetaType::Type(value.type()) == QMetaType::Int);
		break;

	case EPropertyType::Double:
		Q_ASSERT(QMetaType::Type(value.type()) == QMetaType::Double);
		break;

	case EPropertyType::Enum:
		Q_ASSERT(QMetaType::Type(value.type()) == QMetaType::Int);
		break;

	case EPropertyType::Filepath:
		Q_ASSERT(QMetaType::Type(value.type()) == QMetaType::QString);
		break;

	case EPropertyType::String:
		Q_ASSERT(QMetaType::Type(value.type()) == QMetaType::QString);
		break;
/*
	case EPropertyType::Vector3:
		break;
*/
	}
}

Property::~Property()
{
	qDeleteAll(_children);
}

void Property::appendChild(Property* child)
{
	if(!child->parent())
	{
		_children.append(child);
		child->_parent = this;
	}
}

int Property::childNumber() const
{
	if(!_parent)
		return 0;

	Property* mutableThis = const_cast<Property*>(this);
	return _parent->_children.indexOf(mutableThis);
}

DoubleProperty::DoubleProperty(const QString& name, double value)
	: Property(name, QVariant(value), EPropertyType::Double, nullptr)
	, _min(-std::numeric_limits<double>::max())
	, _max(+std::numeric_limits<double>::max())
	, _step(1.0)
	, _decimals(2)
{
}

QWidget* DoubleProperty::createEditor(QWidget* parent,
									  const QStyleOptionViewItem& option)
{
	Q_UNUSED(option);

	PropertyDoubleSpinBox* editor = new PropertyDoubleSpinBox(parent);

	editor->setMinimum(_min);
	editor->setMaximum(_max);
	editor->setSingleStep(_step);
	editor->setDecimals(_decimals);

	return editor;
}

bool DoubleProperty::setEditorData(QWidget* editor, const QVariant& data)
{
	PropertyDoubleSpinBox* editor_ = qobject_cast<PropertyDoubleSpinBox*>(editor);

	if(editor_ != nullptr)
	{
		if(QMetaType::Type(data.type()) == QMetaType::Double)
		{
			editor_->blockSignals(true);
			editor_->setValue(data.toDouble());
			editor_->blockSignals(false);

			return true;
		}
	}

	return false;
}

QVariant DoubleProperty::editorData(QWidget* editor)
{
	PropertyDoubleSpinBox* editor_ = qobject_cast<PropertyDoubleSpinBox*>(editor);

	if(editor_ != nullptr)
	{
		return editor_->value();
	}

	return QVariant();
}

void DoubleProperty::setUiHints(const AttributeMap& map)
{
	AttributeMap::const_iterator it = map.begin();
	while(it != map.end())
	{
		QString hintName = it.key();
		if(hintName == QStringLiteral("min"))
			tryConvert(it.value(), _min);
		else if(hintName == QStringLiteral("max"))
			tryConvert(it.value(), _max);
		else if(hintName == QStringLiteral("step"))
			tryConvert(it.value(), _step);
		else if(hintName == QStringLiteral("decimals"))
			tryConvert(it.value(), _decimals);

		++it;
	}
}

EnumProperty::EnumProperty(const QString& name, 
						   const QStringList& valueList)
	: Property(name, QVariant(0), EPropertyType::Enum, nullptr)
	, _valueList(valueList)
{
}

QVariant EnumProperty::value(int role) const
{
	if(role == Qt::UserRole)
	{
		return Property::value(role).toInt();
	}
	else
	{
		return _valueList[Property::value(role).toInt()];
	}
}

bool EnumProperty::setValue(const QVariant& value, int role)
{
	if(role == Qt::EditRole)
	{
		if(QMetaType::Type(value.type()) == QMetaType::Int)
		{
			int index = value.toInt();
			
			if(index != _value)
			{
				Q_ASSERT(index >= 0);
				Q_ASSERT(index < _valueList.count());

				_value = index;

				return true;
			}
		}
	}

	return false;
}

QWidget* EnumProperty::createEditor(QWidget* parent, 
									const QStyleOptionViewItem& option)
{
	Q_UNUSED(option);

	PropertyComboBox* editor = new PropertyComboBox(parent);
	editor->addItems(_valueList);
	return editor;
}

bool EnumProperty::setEditorData(QWidget* editor, const QVariant& data)
{
	PropertyComboBox* editor_ = qobject_cast<PropertyComboBox*>(editor);

	if(editor_ != nullptr)
	{
		if(QMetaType::Type(data.type()) == QMetaType::QString)
		{
			editor_->blockSignals(true);
			editor_->setCurrentIndex(_value.toInt());
			editor_->blockSignals(false);

			return true;
		}
	}

	return false;
}

QVariant EnumProperty::editorData(QWidget* editor)
{
	PropertyComboBox* editor_ = qobject_cast<PropertyComboBox*>(editor);

	if(editor_ != nullptr)
	{
		return editor_->currentIndex();
	}

	return QVariant();
}

void EnumProperty::setUiHints(const AttributeMap& map)
{
	AttributeMap::const_iterator it = map.begin();
	while(it != map.end())
	{
		QString hintName = it.key();
		if(hintName == QStringLiteral("index"))
		{
			int index = 0;
			if(tryConvert(it.value(), index))
				_value = index;
		}
		++it;
	}
}

IntegerProperty::IntegerProperty(const QString& name, int value)
	: Property(name, QVariant(value), EPropertyType::Integer, nullptr)
	, _min(-std::numeric_limits<int>::max())
	, _max(+std::numeric_limits<int>::max())
	, _step(1)
	, _wrap(false)
{
}

QWidget* IntegerProperty::createEditor(QWidget* parent,
									   const QStyleOptionViewItem& option)
{
	Q_UNUSED(option);

	PropertySpinBox* editor = new PropertySpinBox(parent);

	editor->setMinimum(_min);
	editor->setMaximum(_max);
	editor->setSingleStep(_step);
	editor->setWrapping(_wrap);

	return editor;
}

bool IntegerProperty::setEditorData(QWidget* editor, const QVariant& data)
{
	PropertySpinBox* editor_ = qobject_cast<PropertySpinBox*>(editor);

	if(editor_ != nullptr)
	{
		if(QMetaType::Type(data.type()) == QMetaType::Int)
		{
			editor_->blockSignals(true);
			editor_->setValue(data.toInt());
			editor_->blockSignals(false);

			return true;
		}
	}

	return false;
}

QVariant IntegerProperty::editorData(QWidget* editor)
{
	QSpinBox* editor_ = qobject_cast<QSpinBox*>(editor);

	if(editor_ != nullptr)
	{
		return editor_->value();
	}

	return QVariant();
}

void IntegerProperty::setUiHints(const AttributeMap& map)
{
	AttributeMap::const_iterator it = map.begin();
	while(it != map.end())
	{
		QString hintName = it.key();
		if(hintName == QStringLiteral("min"))
			tryConvert(it.value(), _min);
		else if(hintName == QStringLiteral("max"))
			tryConvert(it.value(), _max);
		else if(hintName == QStringLiteral("step"))
			tryConvert(it.value(), _step);
		else if(hintName == QStringLiteral("wrap"))
			tryConvert(it.value(), _wrap);

		++it;
	}
}

StringProperty::StringProperty(const QString& name, const QString& value)
	: Property(name, QVariant(value), EPropertyType::String, nullptr)
{
}

BooleanProperty::BooleanProperty(const QString& name, bool value)
	: Property(name, QVariant(value), EPropertyType::Boolean, nullptr)
{
}

QWidget* BooleanProperty::createEditor(QWidget* parent,
									   const QStyleOptionViewItem& option)
{
	return new PropertyCheckBox(parent);
}

bool BooleanProperty::setEditorData(QWidget* editor,
									const QVariant& data)
{
	PropertyCheckBox* editor_ = qobject_cast<PropertyCheckBox*>(editor);

	if(editor_ != nullptr)
	{
		if(QMetaType::Type(data.type()) == QMetaType::Bool)
		{
			editor_->blockCheckBoxSignals(true);
			editor_->setChecked(data.toBool());
			editor_->blockCheckBoxSignals(false);

			return true;
		}
	}

	return false;
}

QVariant BooleanProperty::editorData(QWidget* editor)
{
	PropertyCheckBox* editor_ = qobject_cast<PropertyCheckBox*>(editor);

	if(editor_ != nullptr)
	{
		return editor_->isChecked();
	}

	return QVariant();
}

bool BooleanProperty::paint(QPainter* painter, 
							const QStyleOptionViewItem& option,
							const QVariant& value)
{
	QStyleOptionButton opt;
	opt.state = QStyle::State_Enabled;
	opt.state |= value.toBool() ? QStyle::State_On: QStyle::State_Off;
	opt.rect = option.rect.translated(4,0);
	opt.fontMetrics = option.fontMetrics;
	opt.palette = option.palette;

	const QWidget* widget = option.widget;
	QStyle *style = widget ? widget->style() : QApplication::style();

	// draw the background
	style->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, widget);
	// draw the control
	style->drawControl(QStyle::CE_CheckBox, &opt, painter);

	return true;
}

FilePathProperty::FilePathProperty(const QString& name,
								   const QString& initialPath)
	: Property(name, QVariant(QFileInfo(initialPath).absoluteFilePath()), 
		EPropertyType::Filepath, nullptr)
	, _fileInfo(initialPath)
	, _filter()
{
}

QVariant FilePathProperty::value(int role) const
{
	if(role == Qt::UserRole || role == Qt::EditRole)
	{
		// Returns full path to a file
		return Property::value(role).toString();
	}
	else
	{
		// For display role - returns just a file name
		return _fileInfo.fileName();
	}
}

bool FilePathProperty::setValue(const QVariant& value, int role)
{
	if(role == Qt::EditRole)
	{
		//qDebug() << Q_FUNC_INFO << value;

		if(QMetaType::Type(value.type()) == QMetaType::QString)
		{
			QFileInfo tmpInfo = QFileInfo(value.toString());
			if(tmpInfo != _fileInfo 
				&& tmpInfo.exists())
			{
				_fileInfo = tmpInfo;
				_value = _fileInfo.filePath();
				return true;
			}
		}
	}

	return false;
}

QWidget* FilePathProperty::createEditor(QWidget* parent,
										const QStyleOptionViewItem& option)
{
	FileRequester* editor = new FileRequester(parent);

	/// TODO:
	/// editor->setFilter(_filter);

	editor->setFilter(QObject::tr(
		"Popular image formats (*.bmp *.jpeg *.jpg *.png *.tiff);;"
		"Windows bitmaps (*.bmp *.dib);;"
		"JPEG files (*.jpeg *.jpg *.jpe);;"
		"JPEG 2000 files (*.jp2);;"
		"Portable Network Graphics (*.png);;"
		"Portable image format (*.pbm *.pgm *.ppm);;"
		"Sun rasters (*.sr *.ras);;"
		"TIFF files (*.tiff *.tif);;"
		"All files (*.*)"));

	return editor;
}

bool FilePathProperty::setEditorData(QWidget* editor,
									 const QVariant& data)
{
	FileRequester* editor_ = qobject_cast<FileRequester*>(editor);

	if(editor_ != nullptr)
	{
		if(QMetaType::Type(data.type()) == QMetaType::QString)
		{
			//qDebug() << Q_FUNC_INFO << data.toString();
			//editor_->setFilePath(data.toString());

			editor_->blockLineEditSignals(true);
			editor_->setText(data.toString());
			editor_->blockLineEditSignals(false);

			return true;
		}
	}

	return false;
}

QVariant FilePathProperty::editorData(QWidget* editor)
{
	FileRequester* editor_ = qobject_cast<FileRequester*>(editor);

	if(editor_ != nullptr)
	{
		//qDebug() << Q_FUNC_INFO << editor_->text();
		return editor_->text();
	}

	return QVariant();
}

void FilePathProperty::setUiHints(const AttributeMap& map)
{
	AttributeMap::const_iterator it = map.begin();
	while(it != map.end())
	{
		QString hintName = it.key();
		if(hintName == QStringLiteral("filter"))
			_filter = it.value();
		++it;
	}
}

MatrixProperty::MatrixProperty(const QString& name,
	const Matrix3x3& initial)
	: Property(name, QVariant::fromValue<Matrix3x3>(initial), EPropertyType::Matrix, nullptr)
{
}

QVariant MatrixProperty::value(int role) const
{
	if(role != Qt::DisplayRole)
		return Property::value(role);

	auto m = _value.value<Matrix3x3>();

	QString str = "[";
	int column = 0;
	for(auto v : m.v)
	{
		str += QString(" %1").arg(v);
		++column;
		if(!(column % 3))
			str += ";";
	}
	str += " ]";

	return str;
}

//bool setValue(const QVariant& value, 
//	int role = Qt::UserRole) override;

#include <QPushButton>

QWidget* MatrixProperty::createEditor(QWidget* parent,
	const QStyleOptionViewItem& option)
{
	return new PropertyMatrixButton(parent);
}

bool MatrixProperty::setEditorData(QWidget* editor,
	const QVariant& data)
{
	PropertyMatrixButton* ed = qobject_cast<PropertyMatrixButton*>(editor);

	if(ed)
	{
		if(data.userType() == QMetaTypeId<Matrix3x3>::qt_metatype_id())
		{
			ed->setMatrix(data.value<Matrix3x3>());
			return true;
		}
	}

	return false;
}

QVariant MatrixProperty::editorData(QWidget* editor)
{
	PropertyMatrixButton* ed = qobject_cast<PropertyMatrixButton*>(editor);

	if(ed)
		return QVariant::fromValue<Matrix3x3>(ed->matrix());

	return QVariant();
}

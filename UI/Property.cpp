#include "Property.h"

#include <QDoubleSpinBox>
#include <QComboBox>
#include <QApplication>

#include "PropertyWidgets.h"

Property::Property(const QString& name,
				   const QVariant& value,
				   EPropertyType type,
				   Property* parent)
	: _parent(parent)
	, _name(name)
	, _value(value)
	, _type(type)
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
{
}

QWidget* DoubleProperty::createEditor(QWidget* parent,
									  const QStyleOptionViewItem& option)
{
	Q_UNUSED(option);

	PropertyDoubleSpinBox* editor = new PropertyDoubleSpinBox(parent);

	editor->setFrame(false);
	editor->setDecimals(3);
	editor->setMinimum(-100.0);
	editor->setMaximum(100.0);
	editor->setSingleStep(0.1);

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

EnumProperty::EnumProperty(const QString& name, 
						   const QStringList& valueList,
						   int currentIndex)
	: Property(name, QVariant(currentIndex), EPropertyType::Enum, nullptr)
	, _valueList(valueList)
{
}

QVariant EnumProperty::value(int role) const
{
	Q_UNUSED(role);

	return _valueList[Property::value(role).toInt()];
}

void EnumProperty::setValue(const QVariant& value, int role)
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

				// Teraz sygnal jest wyzwalany w momencie out of focus'a
				qDebug() << "[TODO] New value:" << index << _valueList[index];

				_value = index;
			}
		}
	}
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

IntegerProperty::IntegerProperty(const QString& name, int value)
	: Property(name, QVariant(value), EPropertyType::Integer, nullptr)
{
}

QWidget* IntegerProperty::createEditor(QWidget* parent,
									   const QStyleOptionViewItem& option)
{
	Q_UNUSED(option);

	PropertySpinBox* editor = new PropertySpinBox(parent);

	editor->setFrame(false);
	editor->setMinimum(-100);
	editor->setMaximum(100);
	editor->setSingleStep(1);

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
	Q_UNUSED(option);

	PropertyCheckBox* editor = new PropertyCheckBox(parent);

	return editor;
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
	: Property(name, QVariant(initialPath), EPropertyType::Filepath, nullptr)
{
}

QWidget* FilePathProperty::createEditor(QWidget* parent,
										const QStyleOptionViewItem& option)
{
	//auto editor = new FileRequester(parent);
	//auto editor = new ShortenTextLineEdit(parent);
	//return editor;

	//return nullptr;
	return new QLineEdit(parent);
}

bool FilePathProperty::setEditorData(QWidget* editor,
									 const QVariant& data)
{
	//FileRequester* editor_ = qobject_cast<FileRequester*>(editor);

	//if(editor_ != nullptr)
	//{
	//    if(QMetaType::Type(data.type()) == QMetaType::QString)
	//    {
	//        // editor_->blockSignals(true);
	//        editor_->setFilePath(data.toString());
	//        // editor_->blockSignals(false);

	//        return true;
	//    }
	//}

	return false;
}

QVariant FilePathProperty::editorData(QWidget* editor)
{
	//FileRequester* editor_ = qobject_cast<FileRequester*>(editor);

	//if(editor_ != nullptr)
	//{
	//    qDebug() << editor_->filePath();
	//    return editor_->filePath();
	//}

	return QVariant();
}
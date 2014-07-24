/*
 * Copyright (c) 2013-2014 Kajetan Swierk <k0zmo@outlook.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "Property.h"

#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QApplication>

#include "PropertyWidgets.h"

template <typename T>
bool tryConvert(const QString& str, T& value) 
{
    // Expression must be T-dependent to be evaluated at the moment of instantiation
    static_assert(sizeof(T) == 0, "Conversion doesn't exist");
}

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
        Q_ASSERT(false);
        break;

    case EPropertyType::Boolean:
        Q_ASSERT(value.type() == QVariant::Bool);
        break;

    case EPropertyType::Integer:
        Q_ASSERT(value.type() == QVariant::Int);
        break;

    case EPropertyType::Double:
        Q_ASSERT(value.type() == QVariant::Double);
        break;

    case EPropertyType::String:
        Q_ASSERT(value.type() == QVariant::String);
        break;

    case EPropertyType::Enum:
        Q_ASSERT(value.userType() == qMetaTypeId<Enum>());
        break;

    case EPropertyType::Filepath:
        Q_ASSERT(value.userType() == qMetaTypeId<Filepath>());
        break;

    case EPropertyType::Matrix:
        Q_ASSERT(value.userType() == qMetaTypeId<Matrix3x3>());
        break;
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

QWidget* DoubleProperty::createEditor(QWidget* parent)
{
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
        if(data.type() == qMetaTypeId<double>())
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
        return editor_->value();
    return QVariant();
}

void DoubleProperty::setUiHints(const PropertyHintList& list)
{
    PropertyHintList::const_iterator it = list.begin();
    while(it != list.end())
    {
        QString hintName = it->first;
        if(hintName == QStringLiteral("min"))
            tryConvert(it->second, _min);
        else if(hintName == QStringLiteral("max"))
            tryConvert(it->second, _max);
        else if(hintName == QStringLiteral("step"))
            tryConvert(it->second, _step);
        else if(hintName == QStringLiteral("decimals"))
            tryConvert(it->second, _decimals);
        ++it;
    }
}

EnumProperty::EnumProperty(const QString& name, Enum index)
    : Property(name, QVariant::fromValue<Enum>(index), EPropertyType::Enum, nullptr)
{
}

QVariant EnumProperty::value(int role) const
{
    if(role == Qt::UserRole)
    {
        // Returns enum value
        return Property::value(role);
    }
    else
    {
        // Returns human-readable enum name
        int index = Property::value(role).value<Enum>().data();
        if(index < _valueList.count())
            return _valueList[index];
        return QVariant();
    }
}

bool EnumProperty::setValue(const QVariant& value, int role)
{
    if(role == Qt::EditRole)
    {
        if(value.userType() == qMetaTypeId<Enum>())
        {
            Enum index = value.value<Enum>();
            
            if(index.data() >= 0 
                && index.data() < _valueList.count())
            {
                _value = QVariant::fromValue<Enum>(Enum(index));
                return true;
            }
        }
    }

    return false;
}

QWidget* EnumProperty::createEditor(QWidget* parent)
{
    PropertyComboBox* editor = new PropertyComboBox(parent);
    editor->addItems(_valueList);
    return editor;
}

bool EnumProperty::setEditorData(QWidget* editor, const QVariant& data)
{
    PropertyComboBox* editor_ = qobject_cast<PropertyComboBox*>(editor);

    if(editor_ != nullptr)
    {
        // For Display role we return QString and that's what is send to us back
        if(data.type() == qMetaTypeId<QString>())
        {
            editor_->blockSignals(true);
            editor_->setCurrentIndex(_value.value<Enum>().data());
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
        return QVariant::fromValue<Enum>(Enum(editor_->currentIndex()));
    return QVariant();
}

void EnumProperty::setUiHints(const PropertyHintList& list)
{
    PropertyHintList::const_iterator it = list.begin();
    while(it != list.end())
    {
        if(it->first == QStringLiteral("item"))
            _valueList.append(it->second);

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

QWidget* IntegerProperty::createEditor(QWidget* parent)
{
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
        if(data.type() == qMetaTypeId<int>())
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
        return editor_->value();
    return QVariant();
}

void IntegerProperty::setUiHints(const PropertyHintList& list)
{
    PropertyHintList::const_iterator it = list.begin();
    while(it != list.end())
    {
        QString hintName = it->first;
        if(hintName == QStringLiteral("min"))
            tryConvert(it->second, _min);
        else if(hintName == QStringLiteral("max"))
            tryConvert(it->second, _max);
        else if(hintName == QStringLiteral("step"))
            tryConvert(it->second, _step);
        else if(hintName == QStringLiteral("wrap"))
            tryConvert(it->second, _wrap);

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

QWidget* BooleanProperty::createEditor(QWidget* parent)
{
    return new PropertyCheckBox(parent);
}

bool BooleanProperty::setEditorData(QWidget* editor,
                                    const QVariant& data)
{
    PropertyCheckBox* editor_ = qobject_cast<PropertyCheckBox*>(editor);

    if(editor_ != nullptr)
    {
        if(data.type() == qMetaTypeId<bool>())
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
        return editor_->isChecked();
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

QString filepathToString(const Filepath& filepath)
{
    return QString::fromStdString(filepath.data());
}

QVariant stringToFilepathVariant(const QString& filepath)
{
    return QVariant::fromValue<Filepath>(Filepath(filepath.toStdString()));
}

FilePathProperty::FilePathProperty(const QString& name,
                                   const Filepath& initialPath)
    : Property(name, stringToFilepathVariant(QFileInfo(filepathToString(initialPath)).absoluteFilePath()),
    EPropertyType::Filepath, nullptr)
    , _fileInfo(QString::fromStdString(initialPath.data()))
    , _filter()
    , _save(false)
{
}

QVariant FilePathProperty::value(int role) const
{
    if(role == Qt::UserRole || role == Qt::EditRole)
    {
        // Returns full path to a file
        return Property::value(role);
    }
    else
    {
        if(!_save)
        {
            // For display role - returns just a file name
            QString ret = _fileInfo.fileName();
            return ret.isEmpty() ? filepathToString(_value.value<Filepath>()) : ret;
        }
        else
        {
            return filepathToString(_value.value<Filepath>());
        }
    }
}

bool FilePathProperty::setValue(const QVariant& value, int role)
{
    if(role == Qt::EditRole)
    {
        if(value.type() == qMetaTypeId<QString>())
        {
            if(!_save)
            {
                QFileInfo tmpInfo = QFileInfo(value.toString());
                if(tmpInfo != _fileInfo 
                    && tmpInfo.exists())
                {
                    _fileInfo = tmpInfo;
                    _value = stringToFilepathVariant(_fileInfo.filePath());
                    return true;
                }

                QRegExp re("^(https?|ftp|file|rtsp)://.+$");
                if(re.exactMatch(value.toString()))
                {
                    _fileInfo = QFileInfo();
                    _value = stringToFilepathVariant(value.toString());
                    return true;
                }
            }
            else
            {
                _fileInfo = QFileInfo();
                _value = stringToFilepathVariant(value.toString());
                return true;
            }
        }
    }

    return false;
}

QWidget* FilePathProperty::createEditor(QWidget* parent)
{
    FileRequester* editor = new FileRequester(parent);
    editor->setFilter(_filter);
    editor->setMode(_save);
    return editor;
}

bool FilePathProperty::setEditorData(QWidget* editor,
                                     const QVariant& data)
{
    FileRequester* editor_ = qobject_cast<FileRequester*>(editor);

    if(editor_ != nullptr)
    {
        if(data.userType() == qMetaTypeId<Filepath>())
        {
            editor_->blockLineEditSignals(true);
            editor_->setText(filepathToString(data.value<Filepath>()));
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
        return editor_->text();
    return QVariant();
}

void FilePathProperty::setUiHints(const PropertyHintList& list)
{
    PropertyHintList::const_iterator it = list.begin();
    while(it != list.end())
    {
        QString hintName = it->first;
        if(hintName == QStringLiteral("filter"))
            _filter = it->second;
        if(hintName == QStringLiteral("save"))
            tryConvert(it->second, _save);
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

QWidget* MatrixProperty::createEditor(QWidget* parent)
{
    return new PropertyMatrixButton(parent);
}

bool MatrixProperty::setEditorData(QWidget* editor,
    const QVariant& data)
{
    PropertyMatrixButton* ed = qobject_cast<PropertyMatrixButton*>(editor);

    if(ed)
    {
        if(data.userType() == qMetaTypeId<Matrix3x3>())
        {
            bool prev = ed->blockSignals(true);
            ed->setMatrix(data.value<Matrix3x3>());
            ed->blockSignals(prev);
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

#include "PropertyModel.h"

#include <QRegExp>

PropertyModel::PropertyModel(NodeID nodeID, QObject* parent)
	: QAbstractItemModel(parent)
	, _nodeID(nodeID)
	, _root(new Property("Name", "Value", EPropertyType::String, nullptr))
	, _currentGroup(_root.data())
{
}

PropertyModel::~PropertyModel()
{
}

void PropertyModel::addPropertyGroup(const QString& groupName)
{
	_currentGroup = new Property(groupName, QString(), 
		EPropertyType::String, nullptr);
	_root->appendChild(_currentGroup);
}

void PropertyModel::addProperty(PropertyID propID, EPropertyType propType,
	const QString& propName, const QVariant& value, const QString& uiHint)
{
	if(!_propIdMap.contains(propID))
	{
		Property* prop;

		switch(propType)
		{
		case EPropertyType::Boolean:
			prop = new BooleanProperty(propName, value.toBool());
			break;
		case EPropertyType::Integer:
			prop = new IntegerProperty(propName, value.toInt());
			break;
		case EPropertyType::Double:
			prop = new DoubleProperty(propName, value.toDouble());
			break;
		case EPropertyType::Enum:
			prop = new EnumProperty(propName, value.toStringList());
			break;
		case EPropertyType::Matrix:
			prop = new MatrixProperty(propName, value.value<Matrix3x3>());
			break;
		case EPropertyType::Filepath:
			prop = new FilePathProperty(propName, value.toString());
			break;
		case EPropertyType::String:
			prop = new StringProperty(propName, value.toString());
			break;
		default:
			/// TODO: throw ??
			return;
		}

		if(!uiHint.isEmpty())
		{
			AttributeMap uiHintMap;
			QRegExp re(QString("([^= ]*):{1}([^,]*),?"));
			re.setMinimal(false);
			int pos = 0;

			while((pos = re.indexIn(uiHint, pos)) != -1)
			{
				uiHintMap.insert(re.cap(1), re.cap(2).trimmed());
				pos += re.matchedLength();
			}

			if(!uiHintMap.isEmpty())
				prop->setUiHints(uiHintMap);
		}

		prop->setPropertyID(propID);
		_propIdMap.insert(propID, prop);
		_currentGroup->appendChild(prop);
	}
	else
	{
		/// TODO: throw ??
	}
}

int PropertyModel::columnCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent);
	return 2;
}

int PropertyModel::rowCount(const QModelIndex& parent) const
{
	return property(parent)->childCount();
}

QModelIndex PropertyModel::index(int row, int column, 
								 const QModelIndex& parent) const
{

	if(!hasIndex(row, column, parent))
		return QModelIndex();

	Property* parent_ = property(parent);
	Property* item = parent_->child(row);

	if(!item)
		return QModelIndex();

	return createIndex(row, column, item);
}

QModelIndex PropertyModel::parent(const QModelIndex& index) const
{
	if(!index.isValid())
		return QModelIndex();

	Property* item = property(index);
	Property* parent_ = item->parent();

	if(!parent_ || parent_ == _root.data())
		return QModelIndex();

	return createIndex(parent_->childNumber(), 0, parent_);
}

QVariant PropertyModel::data(const QModelIndex& index, int role) const
{
	if(!index.isValid())
		return QVariant();

	Property* item = property(index);

	switch(role)
	{
	case Qt::DisplayRole:
	case Qt::EditRole:
		if(index.column() == 0)
			return item->name();
		if(index.column() == 1)
			return item->value(role);

	//case Qt::SizeHintRole:
		//return QSize(QFontMetrics(QFont()).width(
		//	data(index, Qt::DisplayRole).toString()) + 30, 22);
	//case Qt::DecorationRole:
	//case Qt::ToolTipRole:
		//break;
	//case Qt::BackgroundRole:
	//	if(item->isRoot())
	//		return QColor(Qt::lightGray);
	//	break;
	}

	return QVariant();
}

bool PropertyModel::setData(const QModelIndex& index, 
							const QVariant& value,
							int role)
{
	if(index.isValid() && role == Qt::EditRole)
	{
		Property* item = property(index);
		QVariant oldValue = item->value(role);
		
		if(item->setValue(value, role))
		{
			bool ok = true;
			emit propertyChanged(_nodeID, item->propertyID(), item->value(), &ok);

			if(ok)
			{
				emit dataChanged(index, index);
				return true;
			}
			else
			{
				item->setValue(oldValue, role);
				return false;
			}			
		}
	}

	return false;
}

Qt::ItemFlags PropertyModel::flags(const QModelIndex& index) const
{
	if(!index.isValid())
		return Qt::ItemIsEnabled;

	Property* item = property(index);

	if(item->isRoot())
		return Qt::ItemIsEnabled;

	Qt::ItemFlags defaultFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

	// if(!item->isReadonly())
		defaultFlags |= Qt::ItemIsEditable;

	return defaultFlags;
}

QVariant PropertyModel::headerData(int section,
								   Qt::Orientation orientation,
								   int role) const
{
	if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
	{
		switch(section)
		{
		case 0:
			return _root->name();

		case 1:
			return _root->value();
		}
	}

	return QVariant();
}

QModelIndex PropertyModel::buddy(const QModelIndex& index) const
{
	if(index.isValid())
	{
		if(index.column() == 0)
		{
			return createIndex(index.row(), 1, index.internalPointer());
		}
	}

	return index;
}

Property* PropertyModel::property(const QModelIndex& index) const
{
	if(index.isValid())
	{
		 Property* item = static_cast<Property*>(index.internalPointer());
		 if(item != nullptr)
			 return item;
	}
	return _root.data();
}
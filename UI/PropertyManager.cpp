#include "PropertyManager.h"
#include "PropertyModel.h"
#include "Property.h"

PropertyManager::PropertyManager(QObject* parent)
    : QObject(parent)
{
}

PropertyManager::~PropertyManager()
{
}

void PropertyManager::newProperty(NodeID nodeID, 
								  PropertyID propID,
								  EPropertyType propType,
								  const QString& propName,
								  const QVariant& value,
								  const QString& uiHint)
{
	auto propModel = _idPropertyModelHash.value(nodeID);
	if(propModel == nullptr)
	{
		propModel = new PropertyModel(nodeID, this);
		_idPropertyModelHash.insert(nodeID, propModel);
	}

	auto prop = propModel->property(propID);

	if(prop == nullptr)
	{
		switch(propType)
		{
		case EPropertyType::Boolean:
			propModel->addProperty(propID, new BooleanProperty(propName, value.toBool()));
			break;
		case EPropertyType::Integer:
			propModel->addProperty(propID, new IntegerProperty(propName, value.toInt()));
			break;
		case EPropertyType::Double:
			propModel->addProperty(propID, new DoubleProperty(propName, value.toDouble()));
			break;
		case EPropertyType::Enum:
			/// TODO: How to pass default index?
			propModel->addProperty(propID, new EnumProperty(propName, value.toStringList()));
			break;
		case EPropertyType::Filepath:
			propModel->addProperty(propID, new FilePathProperty(propName, value.toString()));
			break;
		case EPropertyType::String:
			propModel->addProperty(propID, new StringProperty(propName, value.toString()));
			break;
		}
	}
}

PropertyModel* PropertyManager::propertyModel(NodeID nodeID)
{
	return _idPropertyModelHash.value(nodeID);
}
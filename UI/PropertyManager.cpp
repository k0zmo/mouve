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
	PropertyModel* propModel = propertyModelCreate(nodeID);
	propModel->addProperty(propID, propType, propName, value, uiHint);
}

void PropertyManager::newPropertyGroup(NodeID nodeID, const QString& groupName)
{
	PropertyModel* propModel = propertyModelCreate(nodeID);
	propModel->addPropertyGroup(groupName);
}

PropertyModel* PropertyManager::propertyModelCreate(NodeID nodeID)
{
	auto propModel = _idPropertyModelHash.value(nodeID);

	if(propModel == nullptr)
	{
		// Create new model - this is a new node
		propModel = new PropertyModel(nodeID, this);
		_idPropertyModelHash.insert(nodeID, propModel);
	}

	return propModel;
}

PropertyModel* PropertyManager::propertyModel(NodeID nodeID)
{
	return _idPropertyModelHash.value(nodeID);
}
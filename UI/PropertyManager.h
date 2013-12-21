#pragma once

#include "Prerequisites.h"
#include "Logic/NodeType.h"

#include <QObject>

class PropertyManager : public QObject
{
	Q_OBJECT
public:
	explicit PropertyManager(QObject* parent = nullptr);
	~PropertyManager() override;

	void clear();

	void newProperty(NodeID nodeID, PropertyID propID, EPropertyType propType,
		const QString& propName, const QVariant& value, const QString& uiHint);
	void newPropertyGroup(NodeID nodeID, const QString& groupName);

	void deletePropertyModel(NodeID nodeID);

	PropertyModel* propertyModel(NodeID nodeID);

	static QVariant nodePropertyToVariant(const NodeProperty& propValue);
	static NodeProperty variantToNodeProperty(const QVariant& value);

private:
	QHash<NodeID, PropertyModel*> _idPropertyModelHash;

private:
	PropertyModel* propertyModelCreate(NodeID nodeID);
};


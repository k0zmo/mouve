#pragma once

#include "Prerequisites.h"
#include "Logic/NodeType.h"

#include <QObject>

class PropertyManager : public QObject
{
	Q_OBJECT
public:
	PropertyManager(QObject* parent = nullptr);
	~PropertyManager() override;

	void newProperty(NodeID nodeID, PropertyID propID, EPropertyType propType,
		const QString& propName, const QVariant& value, const QString& uiHint);

	PropertyModel* propertyModel(NodeID nodeID);

private:
	QHash<NodeID, PropertyModel*> _idPropertyModelHash;
	QHash<PropertyModel*, NodeID> _propertyModelHash;
};


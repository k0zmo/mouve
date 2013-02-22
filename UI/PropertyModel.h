#pragma once

#include "Prerequisites.h"

#include <QAbstractItemModel>

class Property;

class PropertyModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	PropertyModel(NodeID nodeID, QObject* parent = nullptr);
	~PropertyModel() override;

	/// TODO: make insertRow
	void addProperty(PropertyID propID, Property* prop);
	Property* property(PropertyID propID);

	int columnCount(const QModelIndex& parent = QModelIndex()) const override;
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;

	QModelIndex index(int row, int column, 
		const QModelIndex& parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex& index) const override;

	QVariant data(const QModelIndex& index,
		int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex& index,
		const QVariant& value,
		int role = Qt::EditRole) override;

	Qt::ItemFlags flags(const QModelIndex& index) const override;

	QVariant headerData(int section, 
		Qt::Orientation orientation, 
		int role = Qt::DisplayRole) const override;

	QModelIndex buddy(const QModelIndex& index) const override;

	NodeID nodeID() const;

signals:
	void propertyChanged(NodeID nodeID, PropertyID propID, const QVariant& newValue);

private:
	Property* property(const QModelIndex& index) const;

private:
	QScopedPointer<Property> _root;
	NodeID _nodeID;
	QMap<PropertyID, Property*> _propIdMap;
	/// TODO: This or let Property hold its ID
	QMap<Property*, PropertyID> _propertyMap;
};

inline NodeID PropertyModel::nodeID() const
{ return _nodeID; }
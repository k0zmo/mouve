#pragma once

#include "Prerequisites.h"
#include "Property.h"

#include <QAbstractItemModel>

class Property;

class PropertyModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	explicit PropertyModel(NodeID nodeID, QObject* parent = nullptr);
	~PropertyModel() override;

	/// TODO: insertRow
	void addPropertyGroup(const QString& groupName);
	void addProperty(PropertyID propID, EPropertyType propType,
		const QString& propName, const QVariant& value, const QString& uiHint);

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
	void propertyChanged(NodeID nodeID,
		PropertyID propID, const QVariant& newValue);

private:
	Property* property(const QModelIndex& index) const;

private:
	QScopedPointer<Property> _root;
	QMap<PropertyID, Property*> _propIdMap;
	NodeID _nodeID;
	Property* _currentGroup;
};

inline NodeID PropertyModel::nodeID() const
{ return _nodeID; }
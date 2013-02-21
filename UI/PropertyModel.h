#pragma once

#include "Prerequisites.h"

#include <QAbstractItemModel>

class Property;

class PropertyModel : public QAbstractItemModel
{
public:
	PropertyModel(QObject* parent = nullptr);
	~PropertyModel() override;

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

private:
	Property* property(const QModelIndex& index) const;

private:
	QScopedPointer<Property> _root;
};

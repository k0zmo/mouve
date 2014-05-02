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
        PropertyID propID, const QVariant& newValue,
        bool* ok);

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
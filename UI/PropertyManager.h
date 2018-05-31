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
#include "Logic/NodeType.h"

#include <QObject>
#include <QHash>

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


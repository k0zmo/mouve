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

void PropertyManager::clear()
{
    qDeleteAll(_idPropertyModelHash);
    _idPropertyModelHash.clear();
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

void PropertyManager::deletePropertyModel(NodeID nodeID)
{
    PropertyModel* model = propertyModel(nodeID);
    if(model != nullptr)
    {
        int res = _idPropertyModelHash.remove(nodeID);
        Q_ASSERT(res == 1); (void) res;
        model->deleteLater();
    }
}

PropertyModel* PropertyManager::propertyModel(NodeID nodeID)
{
    return _idPropertyModelHash.value(nodeID);
}

QVariant PropertyManager::nodePropertyToVariant(const NodeProperty& propValue)
{
    switch(propValue.type())
    {
    case EPropertyType::Unknown:
        return QVariant();
    case EPropertyType::Boolean:
        return propValue.toBool();
    case EPropertyType::Integer:
        return propValue.toInt();
    case EPropertyType::Double:
        return propValue.toDouble();
    case EPropertyType::Enum:
        return QVariant::fromValue<Enum>(propValue.toEnum());
    case EPropertyType::Matrix:
        return QVariant::fromValue<Matrix3x3>(propValue.toMatrix3x3());
    case EPropertyType::Filepath:
        return QVariant::fromValue<Filepath>(propValue.toFilepath());
    case EPropertyType::String:
        return QString::fromStdString(propValue.toString());
    default:
        return QVariant();
    }
}

NodeProperty PropertyManager::variantToNodeProperty(const QVariant& value)
{
    switch(value.type())
    {
    case QVariant::Bool:
        return NodeProperty(value.toBool());
    case QVariant::Int:
        return NodeProperty(value.toInt());
    case QVariant::Double:
        return NodeProperty(value.toDouble());
    case QVariant::String:
        return NodeProperty(value.toString().toStdString());
    default:
        break; // Just to shut down g++
    }

    if(value.userType() == qMetaTypeId<Matrix3x3>())
        return NodeProperty(value.value<Matrix3x3>());
    else if(value.userType() == qMetaTypeId<Enum>())
        return NodeProperty(value.value<Enum>());
    else if(value.userType() == qMetaTypeId<Filepath>())
        return NodeProperty(value.value<Filepath>());

    return NodeProperty();
}

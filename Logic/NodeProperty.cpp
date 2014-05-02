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

#include "NodeProperty.h"

NodeProperty::NodeProperty()
    : _type(EPropertyType::Unknown)
    , _data()
{
}

NodeProperty::NodeProperty(bool value)
    : _type(EPropertyType::Boolean)
    , _data(value)
{
}

NodeProperty::NodeProperty(int value)
    : _type(EPropertyType::Integer)
    , _data(value)
{
}

NodeProperty::NodeProperty(Enum value)
    : _type(EPropertyType::Enum)
    , _data(value)
{
}

NodeProperty::NodeProperty(double value)
    : _type(EPropertyType::Double)
    , _data(value)
{
}

NodeProperty::NodeProperty(float value)
    : _type(EPropertyType::Double)
    , _data(value)
{
}

NodeProperty::NodeProperty(const Matrix3x3& value)
    : _type(EPropertyType::Matrix)
    , _data(value)
{
}

NodeProperty::NodeProperty(const Filepath& value)
    : _type(EPropertyType::Filepath)
    , _data(value)
{
}

NodeProperty::NodeProperty(const std::string& value)
    : _type(EPropertyType::String)
    , _data(value)
{
}

NodeProperty::~NodeProperty()
{
}

bool NodeProperty::toBool() const
{
    if(_type != EPropertyType::Boolean)
        throw boost::bad_get();

    return boost::get<bool>(_data);
}

int NodeProperty::toInt() const
{
    if(_type != EPropertyType::Integer)
        throw boost::bad_get();

    return boost::get<int>(_data);
}

Enum NodeProperty::toEnum() const
{
    if(_type != EPropertyType::Enum)
        throw boost::bad_get();

    return boost::get<Enum>(_data);
}

double NodeProperty::toDouble() const
{
    if(_type != EPropertyType::Double)
        throw boost::bad_get();

    return boost::get<double>(_data);
}

float NodeProperty::toFloat() const
{
    if(_type != EPropertyType::Double)
        throw boost::bad_get();

    return static_cast<float>(boost::get<double>(_data));
}

Matrix3x3 NodeProperty::toMatrix3x3() const
{
    if(_type != EPropertyType::Matrix)
        throw boost::bad_get();

    return boost::get<Matrix3x3>(_data);
}

Filepath NodeProperty::toFilepath() const
{
    if(_type != EPropertyType::Filepath)
        throw boost::bad_get();

    return boost::get<Filepath>(_data);
}

std::string NodeProperty::toString() const
{
    if(_type != EPropertyType::String)
        throw boost::bad_get();

    return boost::get<std::string>(_data);
}

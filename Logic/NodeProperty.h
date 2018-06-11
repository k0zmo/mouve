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

#include <type_traits>

#ifndef Q_MOC_RUN
#include <boost/variant.hpp>
#endif

// Allowed types of node property
enum class EPropertyType : int
{
    Unknown,
    Boolean,
    // Available UI hints:
    // * min: minimum value that property can hold
    // * max: maximum value that property can hold
    // * step: step value used when spin box value is incremented/decremented
    // * wrap: is spin box circular (stepping up from max takes min and vice versa)
    Integer,
    // Available UI hints:
    // * min: minimum value that property can hold
    // * max: maximum value that property can hold
    // * step: step value used when spin box value is incremented/decremented
    // * decimals: precision of the spin box in decimals
    Double,
    // Available UI hints:
    // * item: Human readable enum value name. Should occur n times (n - number of values that enum can take).
    Enum,
    Matrix,
    // Available UI hints:
    // * filter: Filter used when displaying files in file dialog
    // * save: if true file dialog will ask for new file, not existing
    Filepath,
    String,
};

namespace std
{
    MOUVE_EXPORT string to_string(EPropertyType);
}

// Simple class representing 3x3 matrix
struct Matrix3x3
{
    double v[9];

    Matrix3x3()
    {
        for(int i = 0; i < 9; ++i)
            v[i] = 0.0;
    }

    Matrix3x3(double center)
    {
        for(int i = 0; i < 9; ++i)
            v[i] = 0.0;
        v[4] = center;
    }

    Matrix3x3(double m11, double m12, double m13,
        double m21, double m22, double m23,
        double m31, double m32, double m33)
    {
        v[0] = m11; v[1] = m12; v[2] = m13;
        v[3] = m21; v[4] = m22; v[5] = m23;
        v[6] = m31; v[7] = m32; v[8] = m33;
    }
};

// Simple wrapper for Enum types (typedef doesn't create a new type)
class Enum
{
public:
    typedef std::int32_t enum_data;

    Enum()
        : _v(-1)
    {
    }

    explicit Enum(enum_data v)
        : _v(v)
    {
    }

    template <typename EnumType>
    Enum(EnumType v, typename std::enable_if<std::is_enum<EnumType>::value>::type* = 0)
        : _v(static_cast<enum_data>(v))
    {
    }

    template <typename EnumType>
    typename std::enable_if<std::is_enum<EnumType>::value, EnumType>::type cast() const
    {
        return EnumType(_v);
    }

    // no explicit operators in VS2012
    enum_data data() const
    {
        return _v;
    }

private:
    enum_data _v;
};

class Filepath
{
public:
    typedef std::string filepath_data;

    Filepath()
        : _v()
    {
    }

    explicit Filepath(filepath_data v)
        : _v(v)
    {
    }

    // no explicit operators in VS2012
    filepath_data data() const
    {
        return _v;
    }

private:
    filepath_data _v;
};

class MOUVE_EXPORT NodeProperty
{
    typedef boost::variant<
        bool,
        int,
        Enum,
        double,
        Matrix3x3,
        Filepath,
        std::string
    > property_data;

public:
    NodeProperty();
    NodeProperty(bool value);
    NodeProperty(Enum value);
    NodeProperty(int value);
    NodeProperty(double value);
    NodeProperty(float value);
    NodeProperty(const Matrix3x3& value);
    NodeProperty(const Filepath& value);
    NodeProperty(const std::string& value);

    template <typename EnumType,
        class = typename std::enable_if<std::is_enum<EnumType>::value>::type>
    NodeProperty(EnumType v)
        : NodeProperty{Enum{v}}
    {
    }

    ~NodeProperty();

    bool toBool() const;
    int toInt() const;
    Enum toEnum() const;
    double toDouble() const;
    float toFloat() const;
    Matrix3x3 toMatrix3x3() const;
    Filepath toFilepath() const;
    std::string toString() const;

    // T must be exactly the same as in property_data
    template <class T> inline T& cast();
    template <class T> inline const T& cast() const;
    // T must be constructible from current held data
    template <class T> inline T cast_value() const;

    bool isValid() const;
    EPropertyType type() const;

private:
    EPropertyType _type;
    property_data _data;
};

inline bool NodeProperty::isValid() const
{ return _type != EPropertyType::Unknown; }

inline EPropertyType NodeProperty::type() const
{ return _type; }

namespace
{
    template <class T>
    EPropertyType propertyType_(typename std::enable_if<std::is_constructible<NodeProperty, T>::value>::type* = 0)
    {
        static NodeProperty np{ T() };
        return np.type();
    }

    template <class T>
    EPropertyType propertyType_(typename std::enable_if<!std::is_constructible<NodeProperty, T>::value>::type* = 0)
    {
        return EPropertyType::Unknown;
    }

    // Visitor for variant value casting - returns value types
    template <class T>
    class convert_visitor : public boost::static_visitor<T>
    {
    private:
        // Used to specialize template member function (for bool type) of template class
        template <class C> struct type_wrap {};

    public:
        template <class U>
        typename std::enable_if<std::is_convertible<U, T>::value, T>::type
            operator()(U& i) const
        {
            return convert_impl(i, type_wrap<T>());
        }

        template <class U>
        typename std::enable_if<!std::is_convertible<U, T>::value, T>::type
            operator()(U&) const
        {
            throw boost::bad_get();
        }

    private:
        template <class U, class C>
        T convert_impl(U& i, type_wrap<C>) const
        {
            return static_cast<T>(i);
        }

        // convert_impl specialization for bool return type
        template <class U>
        T convert_impl(U& i, type_wrap<bool>) const
        {
            // Shuts up MSVC whining something about performance warning
            return static_cast<T>(!!(i));
        }
    };
}

template <class T>
EPropertyType propertyType()
{
    return propertyType_<T>();
}

template <class T>
T& NodeProperty::cast()
{
    return const_cast<T&>(const_cast<const NodeProperty*>(this)->cast<T>());
}

template <class T>
const T& NodeProperty::cast() const
{
    if(_type != EPropertyType::Unknown && _type == propertyType<T>())
        return boost::get<T>(_data);
    throw boost::bad_get();
}

template <class T>
T NodeProperty::cast_value() const
{
    if(_type != EPropertyType::Unknown && _type == propertyType<T>())
        return boost::apply_visitor(convert_visitor<T>(), _data);
    throw boost::bad_get();
}

template <class T>
class TypedNodeProperty : public NodeProperty
{
    static_assert(std::is_constructible<NodeProperty, T>::value ||
                  std::is_convertible<T, NodeProperty>::value,
                  "NodeProperty is not constructible from T");
public:
    typedef T type;

    explicit TypedNodeProperty(T value)
        : NodeProperty(value)
    {
    }

    operator T() const { return cast_value<T>(); }
};

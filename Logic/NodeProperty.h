#pragma once

#include "Prerequisites.h"

#include <type_traits>
#include <boost/variant.hpp>

/// Allowed types of node property
enum class EPropertyType : int
{
	Unknown,
	Boolean,
	Integer,
	Double,
	Enum,
	Matrix,
	Filepath,
	String,
}; 

/// Simple class representing 3x3 matrix
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

/// Simple wrapper for Enum types (typedef doesn't create a new type)
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
	typename std::enable_if<std::is_enum<EnumType>::value, EnumType>::type cast()
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

class LOGIC_EXPORT NodeProperty
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

#if K_COMPILER != K_COMPILER_MSVC
	// MSVC2012 allows to make chained user-defined conversion
	template <typename EnumType>
	NodeProperty(EnumType v, typename std::enable_if<std::is_enum<EnumType>::value>::type* = 0)
		: NodeProperty{v}
	{
	}
#endif

	~NodeProperty();

	bool toBool() const;
	int toInt() const;
	Enum toEnum() const;
	double toDouble() const;
	float toFloat() const;
	Matrix3x3 toMatrix3x3() const;
	Filepath toFilepath() const;
	std::string toString() const;

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
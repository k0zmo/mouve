#pragma once

#include <type_traits>

template<bool cond, typename T, typename U>
struct if_
{
	typedef U type;
};

template<typename T, typename U>
struct if_<true, T, U>
{
	typedef T type;
};

template<typename T>
struct not_
	: public std::integral_constant<bool,
		!T::value
	>
{
};

template<bool B>
struct not_bool
	: public std::integral_constant<bool,
		!B
	>
{
};

template<typename T>
struct is_scoped_enum
	: public std::integral_constant<bool,
		std::is_enum<T>::value && !std::is_convertible<T, int>::value
	>
{
};

// Works also with enum types (not like std version)
template<typename T>
struct is_unsigned_
	: public std::integral_constant<bool,
		(typename std::remove_cv<T>::type)(0) < (typename std::remove_cv<T>::type)(-1)
	>
{
};

template<typename T>
struct is_signed_
	: public std::integral_constant<bool,
		not_<is_unsigned_<T>>::value
	>
{
};

template<typename T>
struct is_enum_unsigned
	: public std::integral_constant<bool,
		is_unsigned_<T>::value &&
		std::is_enum<T>::value
	>
{
};

template<typename T>
struct is_enum_signed
	: public std::integral_constant<bool,
		is_signed_<T>::value &&
		std::is_enum<T>::value
	>
{
};
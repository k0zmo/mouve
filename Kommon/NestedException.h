// Compatbility file for MSVC2013/2015.
// Taken from libc++: include/exception file, dual licensed under the MIT and
// the University of Illinois Open Source Licenses

#pragma once

#include "konfig.h"

// Only for MSVC
#if defined(_MSC_VER) && _MSC_VER < 1910

#include <exception>
#include <type_traits>
#include <utility>

namespace std
{
class nested_exception
{
public:
    nested_exception() _NOEXCEPT : __ptr_{current_exception()} {}
    //nested_exception(const nested_exception&) noexcept = default;
    //nested_exception& operator=(const nested_exception&) noexcept = default;
    virtual ~nested_exception() _NOEXCEPT {}

    // access functions
    __declspec(noreturn) void rethrow_nested() const
    {
        if (__ptr_ == nullptr)
            terminate();
        std::rethrow_exception(__ptr_);
    }

    exception_ptr nested_ptr() const { return __ptr_; }

private:
    exception_ptr __ptr_;
};

template <class _Tp>
struct __nested : public _Tp, public nested_exception
{
    explicit __nested(_Tp&& __tp) : _Tp{forward<_Tp>(__tp)} {}
};

template <class _Tp>
__declspec(noreturn) void throw_with_nested(
    _Tp&& __t,
    typename enable_if<
        is_class<typename remove_reference<_Tp>::type>::value &&
        !is_base_of<nested_exception,
                    typename remove_reference<_Tp>::type>::value>::type* = 0)
{
    throw __nested<typename remove_reference<_Tp>::type>(forward<_Tp>(__t));
}

template <class _Tp>
__declspec(noreturn) void throw_with_nested(
    _Tp&& __t,
    typename enable_if<
        !is_class<typename remove_reference<_Tp>::type>::value ||
        is_base_of<nested_exception,
                   typename remove_reference<_Tp>::type>::value>::type* = 0)
{
    throw forward<_Tp>(__t);
}

template <class _Ep>
void
    rethrow_if_nested(const _Ep& __e,
                      typename enable_if<is_polymorphic<_Ep>::value>::type* = 0)
{
    const nested_exception* __ptr = dynamic_cast<const nested_exception*>(&__e);
    if (__ptr)
        __ptr->rethrow_nested();
}

template <class _Ep>
void rethrow_if_nested(
    const _Ep& __e, typename enable_if<!is_polymorphic<_Ep>::value>::type* = 0)
{
}
}

#else
#include <exception>
#endif

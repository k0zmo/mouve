#pragma once

#include "konfig.h"
#include <cassert>

template <class Type>
class Singleton
{
public:
	Singleton();
	virtual ~Singleton();

	static Type& instance();
	static Type* instancePtr();

protected:
	static Type* _singleton;

#if K_COMPILER == K_COMPILER_MSVC
private:
	Singleton(const Singleton<Type>&);
	Singleton& operator=(const Singleton<Type>&);
#elif K_COMPILER == K_COMPILER_GCC
public:
	Singleton(const Singleton<Type>&) = delete;
	Singleton& operator=(const Singleton<Type>&) = delete;
#endif
};

template <class Type>
inline Singleton<Type>::Singleton()
{ assert(!_singleton); _singleton = static_cast<Type*>(this); }

template <class Type>
inline Singleton<Type>::~Singleton()
{ assert(_singleton); _singleton = nullptr; }

template <class Type>
inline Type& Singleton<Type>::instance()
{ assert(_singleton); return *_singleton; }

template <class Type>
inline Type* Singleton<Type>::instancePtr()
{ return _singleton; }


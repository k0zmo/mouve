#pragma once

#include <qglobal.h>

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

#if defined(Q_CC_MSVC)
private:
	Singleton(const Singleton<Type>&);
	Singleton& operator=(const Singleton<Type>&);
#elif defined(Q_CC_GNU)
public:
	Singleton(const Singleton<Type>&) = delete;
	Singleton& operator=(const Singleton<Type>&) = delete;
#endif
};

template <class Type>
inline Singleton<Type>::Singleton()
{ Q_ASSERT(!_singleton); _singleton = static_cast<Type*>(this); }

template <class Type>
inline Singleton<Type>::~Singleton()
{ Q_ASSERT(_singleton); _singleton = nullptr; }

template <class Type>
inline Type& Singleton<Type>::instance()
{ Q_ASSERT(_singleton); return *_singleton; }

template <class Type>
inline Type* Singleton<Type>::instancePtr()
{ return _singleton; }


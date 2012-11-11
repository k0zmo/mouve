#pragma once

#include <cassert>

template <class Type>
class Singleton
{
public:
	Singleton()
	{
		singleton = static_cast<Type*>(this);
	}

	~Singleton()
	{
		assert(singleton);
		singleton = nullptr;
	}

	static Type& instance()
	{
		assert(singleton);
		return *singleton;
	}

	static Type* instancePtr()
	{
		return singleton;
	}

protected:
	static Type* singleton;

private:
	Singleton(const Singleton<Type>&);
	Singleton& operator=(const Singleton<Type>&);
};

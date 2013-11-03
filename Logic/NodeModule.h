#pragma once

#include "Prerequisites.h"

class LOGIC_EXPORT NodeModule
{
public:
	virtual bool initialize() = 0;
	virtual bool isInitialized() = 0;
	virtual std::string moduleName() const = 0;

	bool ensureInitialized()
	{
		if(!isInitialized())
			return initialize();
		return true;
	}
};
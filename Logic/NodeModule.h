#pragma once

#include "Prerequisites.h"

class MOUVE_LOGIC_EXPORT NodeModule
{
public:
	virtual ~NodeModule() {}

	virtual bool initialize() = 0;
	virtual bool ensureInitialized() = 0;

	virtual std::string moduleName() const = 0;
};
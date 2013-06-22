#pragma once

#include "NodeType.h"
#include "NodeFactory.h"

#include "Kommon/SharedLibrary.h"

class LOGIC_EXPORT NodePlugin
{
	K_DISABLE_COPY(NodePlugin);
public:
	NodePlugin(const std::string& fileName);
	virtual ~NodePlugin();

	int logicVersion() const;
	int pluginVersion() const;
	void registerPlugin(NodeSystem* nodeSystem);	

private:
	typedef int LogicVersionFunc();
	typedef int PluginVersionFunc();
	typedef void RegisterPluginFunc(NodeSystem*);

	LibraryHandle _sharedLibraryHandle;
	LogicVersionFunc* _logicVersionFunc;
	PluginVersionFunc* _pluginVersionFunc;
	RegisterPluginFunc* _registerPluginFunc;
};

inline int NodePlugin::logicVersion() const
{ return _logicVersionFunc(); }

inline int NodePlugin::pluginVersion() const
{ return _pluginVersionFunc(); }

inline void NodePlugin::registerPlugin(NodeSystem* nodeSystem)
{ return _registerPluginFunc(nodeSystem); }
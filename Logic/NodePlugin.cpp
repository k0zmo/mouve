#include "NodePlugin.h"

NodePlugin::NodePlugin(const std::string& fileName)
	: _sharedLibraryHandle(nullptr)
	, _logicVersionFunc(nullptr)
	, _pluginVersionFunc(nullptr)
	, _registerPluginFunc(nullptr)
{
	try
	{
		_sharedLibraryHandle = SharedLibrary::loadLibrary(fileName.c_str());

		_logicVersionFunc = SharedLibrary::getFunctionAddress<LogicVersionFunc>
			(_sharedLibraryHandle, "logicVersion");
		_pluginVersionFunc = SharedLibrary::getFunctionAddress<PluginVersionFunc>
			(_sharedLibraryHandle, "pluginVersion");
		_registerPluginFunc = SharedLibrary::getFunctionAddress<RegisterPluginFunc>
			(_sharedLibraryHandle, "registerPlugin");
	}
	catch (std::exception&)
	{
		if(_sharedLibraryHandle)
			SharedLibrary::unloadLibrary(_sharedLibraryHandle);
		throw;
	}
}

NodePlugin::~NodePlugin()
{
	if(_sharedLibraryHandle)
		SharedLibrary::unloadLibrary(_sharedLibraryHandle);
}
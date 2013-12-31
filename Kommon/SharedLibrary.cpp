#include "SharedLibrary.h"

namespace SharedLibrary {

#if K_SYSTEM == K_SYSTEM_WINDOWS

LibraryHandle loadLibrary(const char* path)
{
	HMODULE moduleHandle = ::LoadLibraryA(path);
	if(!moduleHandle)
		throw std::runtime_error(std::string("Could not load given module: ") + path);
	return moduleHandle;
}

void unloadLibrary(LibraryHandle sharedLibraryHandle)
{
	BOOL result = ::FreeLibrary(sharedLibraryHandle);
	if(result == FALSE)
		throw std::runtime_error("Could not unload given module");
}

#elif K_SYSTEM == K_SYSTEM_LINUX

LibraryHandle loadLibrary(const char* path)
{
	void* sharedObject = ::dlopen(path, RTLD_NOW);
	if(!sharedObject)
		throw std::runtime_error(std::string("Could not load given shared object: ") + path);
	return sharedObject;
}

void unloadLibrary(LibraryHandle sharedLibraryHandle)
{
	int result = ::dlclose(sharedLibraryHandle);
	if(result != 0)
		throw std::runtime_error("Could not unload given module");
}

#endif

}
#pragma once

#include "konfig.h"
#include <stdexcept>

#if K_SYSTEM == K_SYSTEM_WINDOWS
#  define WIN32_LEAN_AND_MEAN
#  define VC_EXTRALEAN
#  include <windows.h>
typedef HMODULE LibraryHandle;
#elif K_SYSTEM == K_SYSTEM_LINUX
#  include <dlfcn.h>
typedef void* LibraryHandle;
#else
#  error "OS not supported"
#endif

namespace SharedLibrary {

LibraryHandle loadLibrary(const char* path);
void unloadLibrary(LibraryHandle sharedLibraryHandle);

template<class Signature>
	Signature* getFunctionAddress(LibraryHandle sharedLibraryHandle, 
		const char* functionName)
	{
#if K_SYSTEM == K_SYSTEM_WINDOWS
		FARPROC functionAddress = ::GetProcAddress(sharedLibraryHandle, functionName);
		if(!functionAddress)
			throw std::runtime_error("Could not find exported function");
		return reinterpret_cast<Signature*>(functionAddress);
#elif K_SYSTEM == K_SYSTEM_LINUX
		::dlerror(); // clear error value

		void* functionAddress = ::dlsym(sharedLibraryHandle, functionName);
		const char* error = ::dlerror(); // check for error
		if(!error)
			throw std::runtime_error("Could not find exported function");
		return reinterpret_cast<Signature*>(functionAddress);
#endif
	}
}

/*
 * Copyright (c) 2013-2014 Kajetan Swierk <k0zmo@outlook.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#pragma once

#include "konfig.h"
#include <stdexcept>
#include <string>

#if K_SYSTEM == K_SYSTEM_WINDOWS
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

template<class SignaturePtr>
    SignaturePtr getFunctionAddress(LibraryHandle sharedLibraryHandle, 
        const char* functionName)
    {
#if K_SYSTEM == K_SYSTEM_WINDOWS
        FARPROC functionAddress = ::GetProcAddress(sharedLibraryHandle, functionName);
        if(!functionAddress)
            throw std::runtime_error(std::string("Could not find exported function: ") + functionName);
        return reinterpret_cast<SignaturePtr>(functionAddress);
#elif K_SYSTEM == K_SYSTEM_LINUX
        ::dlerror(); // clear error value

        void* functionAddress = ::dlsym(sharedLibraryHandle, functionName);
        const char* error = ::dlerror(); // check for error
        if(error)
            throw std::runtime_error(std::string("Could not find exported function: ") + functionName);
        return reinterpret_cast<SignaturePtr>(functionAddress);
#endif
    }
}

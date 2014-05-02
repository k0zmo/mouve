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
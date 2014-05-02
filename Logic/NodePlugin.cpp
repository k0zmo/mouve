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
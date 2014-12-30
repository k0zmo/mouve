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

#include "konfig.h"
#include "ModulePath.h"
#include "Utils.h"

#if K_SYSTEM == K_SYSTEM_WINDOWS
#  define WIN32_LEAN_AND_MEAN
#  define VC_EXTRALEAN
#  include <windows.h>

namespace priv {

static HMODULE moduleHandle()
{
    static int dummy{0};
    MEMORY_BASIC_INFORMATION mbi;
    if (!::VirtualQuery(&dummy, &mbi, sizeof(mbi)))
        return nullptr;
    return static_cast<HMODULE>(mbi.AllocationBase);
}

static std::string modulePath(HMODULE module)
{
    // try using stack only first
    wchar_t stackBuffer[MAX_PATH];
    const wchar_t* tmpPtr = stackBuffer; // default to stackBuffer
    std::wstring tmpBuf; 

    DWORD size = GetModuleFileNameW(module, stackBuffer, countof(stackBuffer));
    std::string out; // defined here so copy elision can work its magic (NRVO)
    if (size == 0) // something went really wrong
        return out;

    if (size == countof(stackBuffer))
    {
        tmpBuf.resize(countof(stackBuffer) * 2);

        while (true)
        {
            size = GetModuleFileNameW(
                module, const_cast<wchar_t*>(tmpBuf.data()), tmpBuf.size());

            if (size < tmpBuf.size())
                break;

            tmpBuf.resize(2 * tmpBuf.size()); // grow twice (no upper bound)
        }

        tmpPtr = tmpBuf.data();
    }

    DWORD length = WideCharToMultiByte(CP_UTF8, 0, tmpPtr, -1, nullptr, 0,
                                       nullptr, nullptr);
    out.resize(length);
    WideCharToMultiByte(CP_UTF8, 0, tmpPtr, -1, const_cast<char*>(out.data()),
                        static_cast<DWORD>(out.size()), nullptr, nullptr);
    return out;
}

}

std::string executablePath()
{
    return priv::modulePath(nullptr);
}

std::string getModulePath()
{
    return priv::modulePath(priv::moduleHandle());
}

#elif K_SYSTEM == K_SYSTEM_LINUX

#include <fstream>
#include <climits>
#include <cstdlib>
#include <inttypes.h>
#include <string>

std::string executablePath()
{
    std::string out;
    out.resize(PATH_MAX);
    char* resolved = realpath("/proc/self/exe", 
                              const_cast<char*>(out.data()));
    if (!resolved)
        out.clear();
    return out;
}

std::string modulePath()
{
    std::ifstream maps{"/proc/self/maps", std::ios::in};
    std::string out;
    if (!maps.is_open())
        return out;

    std::string buffer;
    while (std::getline(maps, buffer))
    {
        // ex:
        // 35b1800000-35b1820000     r-xp 00000000 08:02 135522  /usr/lib64/ld-2.15.so
        // 7fffb2d48000-7fffb2d49000 r-xp 00000000 00:00      0  [vdso]
        uint64_t low, high;
        char perms[5];
        uint64_t offset;
        uint32_t devMajor, devMinor;
        uint32_t inode;
        char path[PATH_MAX];

        if (sscanf(buffer.c_str(),
                   "%" PRIx64 "-%" PRIx64 " %s %" PRIx64 " %x:%x %u %s\n", &low,
                   &high, perms, &offset, &devMajor, &devMinor, &inode,
                   path) == 8)
        {
            uint64_t myAddr = (uint64_t)(intptr_t)(modulePath);
            if (low <= myAddr &&
                myAddr <= high) // myAddr is between low and high
            {
                out.resize(PATH_MAX);
                char* resolved = realpath(path, const_cast<char*>(out.data()));
                if (!resolved)
                    out.clear(); // something wrong happened
                break;
            }
        }
    }

    return out;
}

#endif
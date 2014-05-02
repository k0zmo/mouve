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

#if defined(HAVE_OPENCL)

#include "../Prerequisites.h"

#include <clw/clw.h>

using std::string;
using std::vector;
using std::unordered_multimap;

typedef uint32_t KernelID;
static const KernelID InvalidKernelID = KernelID(0);
struct GpuRegisteredProgram;

class KernelLibrary
{
public:
    KernelLibrary();
    KernelLibrary(const clw::Context& context,
        const string& programsDirectory = ""
        /*const vector<clw::Device>& devices*/);
    ~KernelLibrary();

    void create(const clw::Context& context,
        const string& programsDirectory = "");
    bool isCreated() const { return _context.isCreated(); }

    /// TODO: 
    ///void purgeLibrary();
    /// TODO: Przejedz po wszystkich programach
    ///       i usun te ktorych zaden kernel nie uzywa
    ///void purgeGarbage();

    KernelID registerKernel(const string& kernelName, 
        const string& programName, const string& buildOptions = "");
    clw::Kernel acquireKernel(KernelID kernelId);

    KernelID updateKernel(KernelID kernelId, const string& buildOptions);
    void rebuildProgram(const string& programName);

    vector<GpuRegisteredProgram> populateListOfRegisteredPrograms() const;

private:
    struct KernelEntry
    {
        KernelEntry() {}
        KernelEntry(const string& kernelName, const string& programName, 
            const string& buildOptions)
            : kernelName(kernelName)
            , programName(programName)
            , buildOptions(buildOptions)
        {}

        //KernelID kernelId;
        string kernelName;
        string programName;
        string buildOptions;
        clw::Kernel kernel;
    };

    struct ProgramEntry
    {
        ProgramEntry() {}
        ProgramEntry(const clw::Program& program, const string& buildOptions)
            : program(program) , buildOptions(buildOptions) {}

        clw::Program program;
        string buildOptions;
    };

    clw::Context _context;
    vector<KernelEntry> _kernels;
    unordered_multimap<string, ProgramEntry> _programs;
    string _programsDirectory;

private:
    clw::Program findProgram(const string& programName, const string& buildOptions);
    bool isProgramRegistered(const string& programName, const string& buildOptions);
    clw::Program buildProgram(const string& programName, const string& buildOptions);
    ProgramEntry* findProgramEntry(const string& programName, const string& buildOptions);
    void updateKernelEntry(const clw::Program& program, KernelEntry& entry,
        const string& buildOptions);
};

#endif

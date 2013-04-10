#if defined(HAVE_OPENCL)

#include "GpuKernelLibrary.h"
#include "GpuException.h"

#include <sstream>

KernelLibrary::KernelLibrary()
{
}

KernelLibrary::KernelLibrary(const clw::Context& context,
	const string& programsDirectory
	/*const vector<clw::Device>& devices*/)
	: _context(context)
	, _programsDirectory(programsDirectory)
{
	// Add invalid kernel to a list
	_kernels.emplace_back();
}

KernelLibrary::~KernelLibrary()
{
	_kernels.clear();
	_programs.clear();
}

void KernelLibrary::create(const clw::Context& context,
						   const string& programsDirectory)
{
	_kernels.clear();
	_programs.clear();

	_context = context;
	_programsDirectory = programsDirectory;

	// Add invalid kernel to a list
	_kernels.emplace_back();
}

KernelID KernelLibrary::registerKernel(const string& kernelName, 
	const string& programName, const string& buildOptions)
{
	// Was this kernel registered before
	for(auto iter = _kernels.begin(), end = _kernels.end(); iter != end; ++iter)
	{
		if(iter->kernelName == kernelName
			&& iter->programName == programName
			&& iter->buildOptions == buildOptions)
		{
			return KernelID(iter - _kernels.begin());
		}
	}

	// Create new kernel entry
	KernelID kernelId = KernelID(_kernels.size());
	
	// Add it to a library
	_kernels.emplace_back(kernelName, programName, buildOptions);

	// Add program to a library only if it wasn't done before
	if(!isProgramRegistered(programName, buildOptions))
		// Normally, we should be able to do perfect-forwarding
		// with .emplace(programName, clw::Program(), buildOptions) 
		// but MSVC2012 doesn't allow for this (yet) - neither does gcc 4.7.2 it seems
		_programs.emplace(programName, ProgramEntry(clw::Program(), buildOptions));
		
	return kernelId;
}
	
clw::Kernel KernelLibrary::acquireKernel(KernelID kernelId)
{
	if(kernelId >= _kernels.size())
		return clw::Kernel();
	KernelEntry& entry = _kernels[kernelId];

	// Need to create the kernel first before returning it
	if(entry.kernel.isNull() && kernelId != InvalidKernelID)
	{
		// Check first if the program with the same buildOptions hasn't been built before
		ProgramEntry* pentry = findProgramEntry(entry.programName, entry.buildOptions);
			
		// For now just throw, this shouldn't happen ever
		if(!pentry)
			throw std::logic_error("Something went wrong");
		clw::Program& program = pentry->program;

		// Has the program already been built
		if(!program.isBuilt())
		{
			program = buildProgram(entry.programName, entry.buildOptions);
			// Second time's a charm
			if(!program.isBuilt())
				throw GpuBuildException(program.log());
		}

		// Update kernel
		updateKernelEntry(pentry->program, entry, entry.buildOptions);
		return entry.kernel;
	}
	return entry.kernel;
}

KernelID KernelLibrary::updateKernel(KernelID kernelId, const string& buildOptions)
{
	if(kernelId >= _kernels.size() || kernelId == InvalidKernelID)
		return InvalidKernelID;
	KernelEntry& entry = _kernels[kernelId];

	if(entry.buildOptions == buildOptions)
		return kernelId;

	// Maybe there's already a program with the same build options?
	ProgramEntry* pentry = findProgramEntry(entry.programName, buildOptions);
			
	if(!pentry)
	{
		// Nope, we got ourselves whole new program-build options tuple
		clw::Program program = buildProgram(entry.programName, buildOptions);
		if(!program.isBuilt())
			throw GpuBuildException(program.log());

		// Add new program entry to our library
		_programs.emplace(entry.programName, ProgramEntry(program, buildOptions));

		updateKernelEntry(program, entry, buildOptions);	
		return kernelId;
	}
	else
	{
		// Tuple Program-buildOptions has been already added, we gonna use it
		clw::Program& program = pentry->program;

		// Has the program already been built
		if(!program.isBuilt())
		{
			program = buildProgram(entry.programName, buildOptions);
			// Second time's a charm
			if(!program.isBuilt())
				throw GpuBuildException(program.log());
		}

		// Program has been built, update kernel entry
		updateKernelEntry(program, entry, buildOptions);
		return kernelId;
	}
}

void KernelLibrary::rebuildProgram(const string& programName)
{
	auto range = _programs.equal_range(programName);
	for(auto iter = range.first; iter != range.second; ++iter)
	{
		// Rebuild every program that has been cached
		clw::Program& prog = iter->second.program;
		prog = buildProgram(iter->first, iter->second.buildOptions);
		if(!prog.isBuilt())
			throw GpuBuildException(prog.log());

		// Now - cover kernels
		for(auto& entry : _kernels)
		{
			if(entry.programName == programName
				&& entry.buildOptions == iter->second.buildOptions)
			{
				updateKernelEntry(prog, entry, entry.buildOptions);
			}
		}
	}
}

vector<RegisteredProgram> KernelLibrary::populateListOfRegisteredPrograms() const
{
	vector<RegisteredProgram> list;

	// Iterate over every program name
	for(auto iter = _programs.begin(), end = _programs.end(); iter != end;
		iter = _programs.upper_bound(iter->first))
	{
		RegisteredProgram entry;
		entry.programName = iter->first;

		// Iterate over every build options of current program name
		auto range = _programs.equal_range(entry.programName);
		for(auto ri = range.first, re = range.second; ri != re; ++ri)
		{
			// find how many kernels uses this program-build options configuration
			int used = 0;
			for(auto& kentry : _kernels)
			{
				if(kentry.programName == entry.programName
					&& kentry.buildOptions == ri->second.buildOptions)
				{
					++used;
				}
			}

			entry.builds.emplace_back(used, ri->second.buildOptions, ri->second.program.isBuilt());
		}

		list.push_back(std::move(entry));
	}

	return list;
}

clw::Program KernelLibrary::findProgram(const string& programName,
										const string& buildOptions)
{
	auto range = _programs.equal_range(programName);
	for(auto iter = range.first; iter != range.second; ++iter)
	{
		if(iter->second.buildOptions == buildOptions)
		{
			return iter->second.program;
		}
	}
	return clw::Program();
}

bool KernelLibrary::isProgramRegistered(const string& programName,
										const string& buildOptions)
{
	auto range = _programs.equal_range(programName);
	for(auto iter = range.first; iter != range.second; ++iter)
	{
		if(iter->second.buildOptions == buildOptions)
		{
			return true;
		}
	}
	return false;
}

clw::Program KernelLibrary::buildProgram(const string& programName, 
										 const string& buildOptions)
{
	// Program hasn't been built, need to it right now
	string programPath = _programsDirectory + programName;
	clw::Program program = _context.createProgramFromSourceFile(programPath);
	if(program.isNull())
		return clw::Program();
	program.build(buildOptions);
	return program;
}

KernelLibrary::ProgramEntry* KernelLibrary::findProgramEntry(const string& programName, 
															 const string& buildOptions)
{
	auto range = _programs.equal_range(programName);
	for(auto iter = range.first; iter != range.second; ++iter)
	{
		if(iter->second.buildOptions == buildOptions)
		{
			return &(iter->second);
		}
	}
	return nullptr;
}

void KernelLibrary::updateKernelEntry(const clw::Program& program,
									  KernelEntry& entry,
									  const string& buildOptions)
{
	/// TODO: reuse kernels if possible? (we got referencing counting after all)
	clw::Kernel kernel = program.createKernel(entry.kernelName.c_str());
	if(kernel.isNull())
	{
		std::ostringstream strm;
		strm << "Kernel " << entry.kernelName << " doesn't exist in program "
			<< entry.programName << ".\n\nList of kernels: \n";
		auto kernels = program.createKernels();
		string kernelNames;
		for(auto& k : kernels)
			strm << "\t" << k.name() << "\n";
		throw GpuBuildException(strm.str());
	}

	entry.buildOptions = buildOptions;
	entry.kernel = kernel;	
}

#endif
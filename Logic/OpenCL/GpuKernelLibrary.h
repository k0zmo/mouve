#pragma once

#if defined(HAVE_OPENCL)

#include "Prerequisites.h"

#include <clw/clw.h>

using std::string;
using std::vector;
using std::unordered_multimap;

typedef uint32_t KernelID;
static const KernelID InvalidKernelID = KernelID(0);
struct RegisteredProgram;

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

	vector<RegisteredProgram> populateListOfRegisteredPrograms() const;

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

struct RegisteredProgram
{
	struct Build
	{
		Build() {}
		Build(int kernelUsing, const string& options, bool built)
			: kernelUsing(kernelUsing)
			, options(options)
			, built(built)
		{}

		int kernelUsing;
		string options;
		bool built;
	};

	string programName;
	vector<Build> builds;
};

#endif

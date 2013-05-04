#if defined(HAVE_OPENCL)

#include "GpuNodeModule.h"
#include "GpuException.h"

namespace {
string kernelsDirectory();
}

GpuNodeModule::GpuNodeModule(bool interactiveInit)
	: _maxConstantMemory(0)
	, _maxLocalMemory(0)
	, _interactiveInit(interactiveInit)
{
}

bool GpuNodeModule::initialize()
{
	bool res;
	if(_interactiveInit)
		res = createInteractive();
	else
		res = createDefault();

	if(res)
	{
		static string allKernelsDirectory = kernelsDirectory() + "/";
		_library.create(_context, allKernelsDirectory);
	}

	return res;
}

bool GpuNodeModule::isInitialized()
{
	return _context.isCreated();
}

std::string GpuNodeModule::moduleName() const
{
	return "opencl";
}

//#define INTEL_DEBUGGING

bool GpuNodeModule::createDefault()
{
#if defined(INTEL_DEBUGGING)
	auto devs = clw::deviceFiltered(
		clw::Filter::PlatformVendor(clw::Vendor_Intel) 
		&& clw::Filter::DeviceType(clw::Cpu));
	if(!_context.create(devs) || _context.numDevices() == 0)
		return false;
#else
	if(!_context.create(clw::Default) || _context.numDevices() == 0)
		return false;
#endif
		
	_device = _context.devices()[0];
	_queue = _context.createCommandQueue(_device, clw::Property_ProfilingEnabled);
	_dataQueue = _context.createCommandQueue(_device, clw::Property_ProfilingEnabled);

	clw::installErrorHandler([=](cl_int error_id, const std::string& message)
	{
		if(error_id != CL_BUILD_PROGRAM_FAILURE)
			throw GpuNodeException(error_id, message);
	});

	if(!_device.isNull())
	{
		_maxConstantMemory = _device.maximumConstantBufferSize();
		_maxLocalMemory = _device.localMemorySize();
	}

	return !_device.isNull() && !_queue.isNull();
}

bool GpuNodeModule::createInteractive()
{
	return false;
}

KernelID GpuNodeModule::registerKernel(const string& kernelName,
									   const string& programName,
									   const string& buildOptions)
{
	string opts = additionalBuildOptions(programName);
	return _library.registerKernel(kernelName, programName, buildOptions + opts);
}

clw::Kernel GpuNodeModule::acquireKernel(KernelID kernelId)
{
	return _library.acquireKernel(kernelId);
}

KernelID GpuNodeModule::updateKernel(KernelID kernelId,
									 const string& buildOptions)
{
	//string opts = additionalBuildOptions(programName);
	return _library.updateKernel(kernelId, buildOptions);
}

vector<RegisteredProgram> GpuNodeModule::populateListOfRegisteredPrograms() const
{
	return _library.populateListOfRegisteredPrograms();
}

void GpuNodeModule::rebuildProgram(const string& programName)
{
	_library.rebuildProgram(programName);
}

#pragma region Kernels Directory

#if K_SYSTEM == K_SYSTEM_WINDOWS

#include <Windows.h>
#include <QFileInfo>
#include <QDir>

namespace {

HMODULE moduleHandle()
{
	static int s_somevar = 0;
	MEMORY_BASIC_INFORMATION mbi;
	if(!::VirtualQuery(&s_somevar, &mbi, sizeof(mbi)))
	{
		return NULL;
	}
	return static_cast<HMODULE>(mbi.AllocationBase);
}

string kernelsDirectory()
{
	HMODULE hModule = moduleHandle();
	char buffer[MAX_PATH];
	DWORD dwSize = GetModuleFileNameA(hModule, buffer, MAX_PATH);
	//dwSize = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
	QFileInfo fi(QString::fromLatin1(buffer));
	QDir dllDir = fi.absoluteDir();
	if(!dllDir.cd("kernels"))
		// fallback to dll directory
		return dllDir.absolutePath().toStdString();
	return dllDir.absolutePath().toStdString();
}

}

#elif K_SYSTEM == K_SYSTEM_LINUX

namespace {
string kernelsDirectory()
{
	return QStringLiteral("./kernels").toStdString();
}
}

#endif

#pragma endregion

string GpuNodeModule::additionalBuildOptions(const std::string& programName) const
{
	string opts;
#if defined(K_DEBUG) && K_COMPILER == K_COMPILER_MSVC
	// Enable kernel debugging if device is CPU and it's Intel platform
	if(_device.platform().vendorEnum() == clw::Vendor_Intel
		&& _device.deviceType() == clw::Cpu)
	{
		QFileInfo thisFile(K_FILE);
		QDir thisDirectory = thisFile.dir();
		QString s = thisDirectory.dirName();
		if(thisDirectory.cd("kernels"))
		{
			QString fullKernelsPath = thisDirectory.absoluteFilePath(QString::fromStdString(programName));
			opts += " -g -s " + fullKernelsPath.toStdString();
		}
	}
#endif
	return opts;
}

#endif

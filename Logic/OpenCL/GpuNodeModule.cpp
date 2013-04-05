#include "GpuNodeModule.h"
#include "GpuException.h"

GpuNodeModule::GpuNodeModule(bool interactiveInit)
	: _interactiveInit(interactiveInit)
{
}

bool GpuNodeModule::initialize()
{
	if(_interactiveInit)
		return createInteractive();
	else
		return createDefault();
	return false;
}

bool GpuNodeModule::ensureInitialized()
{
	if(!_context.isCreated())
		return initialize();
	return true;
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

	clw::installErrorHandler([=](cl_int error_id, const std::string& message)
	{
		if(error_id != CL_BUILD_PROGRAM_FAILURE)
			throw gpu_node_exception(error_id, message);
	});

	return !_device.isNull() && !_queue.isNull();
}

bool GpuNodeModule::createInteractive()
{
	return false;
}

#pragma region Kernels Directory

#if K_SYSTEM == K_SYSTEM_WINDOWS

#include <Windows.h>
#include <QFileInfo>
#include <QDir>

HMODULE GetMyModuleHandle()
{
	static int s_somevar = 0;
	MEMORY_BASIC_INFORMATION mbi;
	if(!::VirtualQuery(&s_somevar, &mbi, sizeof(mbi)))
	{
		return NULL;
	}
	return static_cast<HMODULE>(mbi.AllocationBase);
}

QString GetKernelDirectory()
{
	HMODULE hModule = GetMyModuleHandle();
	char buffer[MAX_PATH];
	DWORD dwSize = GetModuleFileNameA(hModule, buffer, MAX_PATH);
	//dwSize = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
	QFileInfo fi(QString::fromLatin1(buffer));
	QDir dllDir = fi.absoluteDir();
	if(!dllDir.cd("kernels"))
		// fallback to dll directory
		return dllDir.absolutePath();
	return dllDir.absolutePath();
}

#elif K_SYSTEM == K_SYSTEM_LINUX

QString GetKernelDirectory()
{
	return QStringLiteral("./");
}

#endif

#pragma endregion

bool GpuNodeModule::buildProgram(const std::string& programName)
{
	static QString allProgramsPath = GetKernelDirectory();
	QString programPath = QDir(allProgramsPath).absoluteFilePath(QString::fromStdString(programName));

	clw::Program program = _context.createProgramFromSourceFile(programPath.toStdString());
	if(program.isNull())
		return false;

	std::string opts;

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

	if(!program.build(opts))
		throw gpu_build_exception(program.log());
	//std::cout << program.log() << std::endl;

	_programs[programName] = program;
	return true;
}

clw::Kernel GpuNodeModule::acquireKernel(const std::string& programName, 
		const std::string& kernelName)
{
	auto it = _programs.find(programName);
	if(it == _programs.end())
	{
		if(!buildProgram(programName))
			return clw::Kernel();
		it = _programs.find(programName);
	}

	return it->second.createKernel(kernelName);
}
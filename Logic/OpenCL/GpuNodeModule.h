#pragma once

#if defined(HAVE_OPENCL)

#include "Logic/NodeModule.h"
#include "GpuKernelLibrary.h"

enum class EDeviceType
{
	None     = 0,
	Gpu      = clw::Gpu,
	Cpu      = clw::Cpu,
	Default  = clw::Default,
	Specific = 0xFFFF,
};

using std::vector;
using std::string;

struct GpuPlatform
{
	string name;
	vector<string> devices;
};

struct GpuInteractiveResult
{
	EDeviceType type;
	int platform;
	int device;
};

class LOGIC_EXPORT GpuNodeModule : public NodeModule
{
public:
	explicit GpuNodeModule(bool interactiveInit);

	void setInteractiveInit(bool interactiveInit);
	bool isInteractiveInit() const;

	bool initialize() override;
	bool isInitialized() override;
	std::string moduleName() const override;

	bool createDefault();
	bool createInteractive();

	vector<GpuPlatform> availablePlatforms() const;

	typedef std::function<GpuInteractiveResult(const vector<GpuPlatform>& gpuPlatforms)> OnCreateInteractive;
	OnCreateInteractive onCreateInteractive;

	KernelID registerKernel(const string& kernelName, 
		const string& programName, const string& buildOptions = "");
	clw::Kernel acquireKernel(KernelID kernelId);
	KernelID updateKernel(KernelID kernelId, const string& buildOptions);

	vector<RegisteredProgram> populateListOfRegisteredPrograms() const;
	void rebuildProgram(const string& programName);

	bool isConstantMemorySufficient(uint64_t memSize) const;
	bool isLocalMemorySufficient(uint64_t memSize) const;

	clw::Context& context();
	const clw::Context& context() const;
	clw::Device& device();
	const clw::Device& device() const;
	clw::CommandQueue& queue();
	const clw::CommandQueue& queue() const;
	clw::CommandQueue& dataQueue();
	const clw::CommandQueue& dataQueue() const;

private:
	std::string additionalBuildOptions(const std::string& programName) const;
	bool createAfterContext();

private:
	clw::Context _context;
	clw::Device _device;
	clw::CommandQueue _queue;
	clw::CommandQueue _dataQueue;

	uint64_t _maxConstantMemory;
	uint64_t _maxLocalMemory;

	bool _interactiveInit;

	KernelLibrary _library;
};

inline void GpuNodeModule::setInteractiveInit(bool interactiveInit)
{ _interactiveInit = interactiveInit; }
inline bool GpuNodeModule::isInteractiveInit() const
{ return _interactiveInit; }
inline bool GpuNodeModule::isConstantMemorySufficient(uint64_t memSize) const
{ return memSize <= _maxConstantMemory; }
inline bool GpuNodeModule::isLocalMemorySufficient(uint64_t memSize) const
{ return memSize <= _maxLocalMemory; }
inline clw::Context& GpuNodeModule::context()
{ return _context; }
inline const clw::Context& GpuNodeModule::context() const
{ return _context; }
inline clw::Device& GpuNodeModule::device()
{ return _device; }
inline const clw::Device& GpuNodeModule::device() const
{ return _device; }
inline clw::CommandQueue& GpuNodeModule::queue()
{ return _queue; }
inline const clw::CommandQueue& GpuNodeModule::queue() const
{ return _queue; }
inline clw::CommandQueue& GpuNodeModule::dataQueue()
{ return _dataQueue; }
inline const clw::CommandQueue& GpuNodeModule::dataQueue() const
{ return _dataQueue; }

#endif
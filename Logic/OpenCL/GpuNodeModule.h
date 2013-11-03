#pragma once

#if defined(HAVE_OPENCL)

#include "IGpuNodeModule.h"
#include "GpuKernelLibrary.h"

using std::vector;
using std::string;

class LOGIC_EXPORT GpuNodeModule : public IGpuNodeModule
{
public:
	explicit GpuNodeModule(bool interactiveInit);
	virtual ~GpuNodeModule();

	void setInteractiveInit(bool interactiveInit) override;
	bool isInteractiveInit() const override;

	bool initialize() override;
	bool isInitialized() override;
	std::string moduleName() const override;

	bool createDefault() override;
	bool createInteractive() override;

	vector<GpuPlatform> availablePlatforms() const override;

	KernelID registerKernel(const string& kernelName, 
		const string& programName, const string& buildOptions = "");
	clw::Kernel acquireKernel(KernelID kernelId);
	KernelID updateKernel(KernelID kernelId, const string& buildOptions);

	vector<GpuRegisteredProgram> populateListOfRegisteredPrograms() const override;
	void rebuildProgram(const string& programName) override;

	bool isConstantMemorySufficient(uint64_t memSize) const;
	bool isLocalMemorySufficient(uint64_t memSize) const;

	void beginPerfMarker(const char* markerName, const char* groupName = nullptr);
	void endPerfMarker();

	clw::Context& context();
	const clw::Context& context() const;
	clw::Device& device();
	const clw::Device& device() const;
	clw::CommandQueue& queue();
	const clw::CommandQueue& queue() const;
	clw::CommandQueue& dataQueue();
	const clw::CommandQueue& dataQueue() const;

	bool performanceMarkersInitialized() const;

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

	KernelLibrary _library;

	bool _interactiveInit;
	bool _perfMarkersInitialized;
};

// RAII styled perfMarker
class GpuPerformanceMarker
{
public:
	GpuPerformanceMarker(
		bool perfMarkersInitialized,
		const char* markerName,
		const char* groupName = nullptr);

	~GpuPerformanceMarker();
private:
	bool _ok;
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
inline bool GpuNodeModule::performanceMarkersInitialized() const
{ return _perfMarkersInitialized; }

#endif
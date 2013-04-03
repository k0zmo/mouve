#pragma once

#include "../NodeModule.h"

#include <clw/clw.h>

#include <functional>

class LOGIC_EXPORT GpuNodeModule : public NodeModule
{
public:
	explicit GpuNodeModule(bool interactiveInit);

	void setInteractiveInit(bool interactiveInit);
	bool isInteractiveInit() const;

	bool initialize() override;
	bool ensureInitialized() override;
	std::string moduleName() const override;

	bool createDefault();
	bool createInteractive();

	/// Very temporary solution
	bool buildProgram(const std::string& programName);
	clw::Kernel acquireKernel(const std::string& programName, 
			const std::string& kernelName);

	clw::Context& context();
	const clw::Context& context() const;
	clw::Device& device();
	const clw::Device& device() const;
	clw::CommandQueue& queue();
	const clw::CommandQueue& queue() const;

private:
	clw::Context _context;
	clw::Device _device;
	clw::CommandQueue _queue;

	bool _interactiveInit;
	/// Very temporary solution
	std::unordered_map<std::string, clw::Program> _programs;
};

inline void GpuNodeModule::setInteractiveInit(bool interactiveInit)
{ _interactiveInit = interactiveInit; }
inline bool GpuNodeModule::isInteractiveInit() const
{ return _interactiveInit; }
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
#include "GpuNodeModule.h"

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

bool GpuNodeModule::createDefault()
{
	if(!_context.create(clw::Default) || _context.numDevices() == 0)
		return false;
	_device = _context.devices()[0];
	_queue = _context.createCommandQueue(_device, clw::Property_ProfilingEnabled);

	/*clw::installErrorHandler([=](cl_int error_id, const std::string& message)
	{
		std::cout << "OpenCL error (" << error_id << "): " << message << std::endl;
	});*/

	return !_device.isNull() && !_queue.isNull();
}

bool GpuNodeModule::createInteractive()
{
	return false;
}
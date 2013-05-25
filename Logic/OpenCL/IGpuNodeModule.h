#pragma once

#include "Logic/NodeModule.h"

enum class EDeviceType
{
	None     = 0,
	Default  = K_BIT(0),
	Cpu      = K_BIT(1),
	Gpu      = K_BIT(2),
	Specific = 0xFFFF,
};

struct GpuPlatform;
struct GpuInteractiveResult;
struct GpuRegisteredProgram;

typedef std::function<
	GpuInteractiveResult(const std::vector<GpuPlatform>& gpuPlatforms)
> OnCreateInteractive;

class LOGIC_EXPORT IGpuNodeModule : public NodeModule
{
public:
	virtual void setInteractiveInit(bool interactiveInit) = 0;
	virtual bool isInteractiveInit() const = 0;

	virtual bool createDefault() = 0;
	virtual bool createInteractive() = 0;
	OnCreateInteractive onCreateInteractive;

	virtual std::vector<GpuPlatform> availablePlatforms() const = 0;

	virtual std::vector<GpuRegisteredProgram> populateListOfRegisteredPrograms() const = 0;
	virtual void rebuildProgram(const std::string& programName) = 0;
};

struct GpuPlatform
{
	std::string name;
	std::vector<std::string> devices;
};

struct GpuInteractiveResult
{
	EDeviceType type;
	int platform;
	int device;
};

struct GpuRegisteredProgram
{
	struct Build
	{
		Build() {}
		Build(const std::vector<std::string>& kernels, const std::string& options, bool built)
			: kernels(kernels)
			, options(options)
			, built(built)
		{}

		std::vector<std::string> kernels;
		std::string options;
		bool built;
	};

	std::string programName;
	std::vector<Build> builds;
};

LOGIC_EXPORT std::unique_ptr<IGpuNodeModule> createGpuModule();
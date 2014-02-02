#include "Logic/NodeSystem.h"

extern "C" K_DECL_EXPORT int logicVersion() { return LOGIC_VERSION; }
extern "C" K_DECL_EXPORT int pluginVersion() { return 1; }

extern void registerKuwaharaFilter(NodeSystem* nodeSystem);
extern void registerGpuKuwaharaFilter(NodeSystem* nodeSystem);

extern "C" K_DECL_EXPORT void registerPlugin(NodeSystem* nodeSystem)
{
    registerKuwaharaFilter(nodeSystem);
    registerGpuKuwaharaFilter(nodeSystem);
}
#include "Logic/NodeSystem.h"

extern "C" K_DECL_EXPORT int logicVersion() { return LOGIC_VERSION; }
extern "C" K_DECL_EXPORT int pluginVersion() { return 1; }

extern void registerAmpMorphologyNodes(NodeSystem* nodeSystem);
extern void registerAmpArithmeticNodes(NodeSystem* nodeSystem);

extern "C" K_DECL_EXPORT void registerPlugin(NodeSystem* nodeSystem)
{
    registerAmpArithmeticNodes(nodeSystem);
    registerAmpMorphologyNodes(nodeSystem);
}
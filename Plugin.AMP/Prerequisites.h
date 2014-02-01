#pragma once

#include "Kommon/konfig.h"

// Only when compiler is MSVC2012+
#if K_COMPILER == K_COMPILER_MSVC
#  if _MSC_VER >= 1700 
#    define CPP_AMP_SUPPORTED
#    include <amp.h>
#    include <amp_graphics.h>
#    include <amp_math.h>
#    include <amp_short_vectors.h>
#  endif
#endif

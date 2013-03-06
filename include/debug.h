#ifndef _slsimlib_debug_declare_
#define _slsimlib_debug_declare_

#ifndef NDEBUG
#include <cstdio>
#define SLSIMLIB_DEBUG(x) std::fprintf(stderr, "%s:%d: %s: %s\n", __FILE__, __LINE__, __func__, x)
#else
#define SLSIMLIB_DEBUG(x) ((void)0)
#endif

#endif

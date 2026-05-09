#pragma once

#include <cstdio>

#ifndef NDEBUG
#define DEBUG_PRINT(...) std::fprintf(stderr, __VA_ARGS__)
#else
#define DEBUG_PRINT(...) ((void)0)
#endif

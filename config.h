#pragma once
#include <string.h>
// these are required by network plugin
#define HAVE_STRVERSCMP 1

#ifndef HAVE_STRLCPY
size_t  strlcpy(char *dst, const char *src, size_t size/*of dest buffer*/);
#endif


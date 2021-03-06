/**
 * @file  coretools.h
 *
 * @brief Declaration file for Coretools.cpp
 */
#pragma once

#include "UnicodeString.h"
#include <strsafe.h>

void replace_char(TCHAR *s, int target, int repl);

size_t linelen(const char *string, size_t maxlen);

template <typename T, size_t N>
T *_tcscpy_safe(T(&dst)[N], const T *src)
{
	StringCchCopy(reinterpret_cast<T *>(&dst), N, src);
	return reinterpret_cast<T *>(&dst);
}

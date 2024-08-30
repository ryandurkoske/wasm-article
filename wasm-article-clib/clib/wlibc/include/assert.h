#pragma once

#define static_assert _Static_assert

extern __attribute__((import_module("wlibc"),import_name("assert")))
	void _assert(const char* msg, const char* file, unsigned int line);

#ifndef NDEBUG
#define assert(condition) (void)((condition) || (_assert (#condition, __FILE__, __LINE__),0))
#else
#define assert(condition)
#endif
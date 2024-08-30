#pragma once

#ifndef NULL
#define NULL ((void*)0)
#endif

__attribute__((import_module("wlibc"),import_name("abort"))) void abort();

inline int abs(int x) { return __builtin_abs(x); }
inline long labs(long x) { return __builtin_labs(x); }
inline long long llabs(long long x) { return __builtin_llabs(x); }
//TODO: could implement rand


typedef __SIZE_TYPE__ size_t;
void* malloc(size_t);
void free(void*);
//TODO: calloc / realloc
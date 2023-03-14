#pragma once

typedef __SIZE_TYPE__ size_t;

__attribute__((visibility("default"))) void* malloc(size_t);
__attribute__((visibility("default"))) void free(void*);
#pragma once

#ifdef TARGET_WASM

#define EXPORT __attribute__((visibility("default")))
#define IMPORT extern

#endif
#ifdef TARGET_NATIVE

#define EXPORT 
#define IMPORT

#endif
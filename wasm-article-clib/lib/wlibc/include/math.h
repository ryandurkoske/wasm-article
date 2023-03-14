#pragma once

#define _HUGE_ENUF  1e+300
#define INFINITY   (_HUGE_ENUF * _HUGE_ENUF)
#define HUGE_VAL ((double)INFINITY)
#define HUGE_VALF ((float)INFINITY)
#define HUGE_VALL ((long double)INFINTY)

inline double acos(double x) { return __builtin_acos(x); }
inline float acosf(float x){ return __builtin_acosf(x); }

inline double  asin(double x){ return __builtin_asin(x); }
inline float asinf(float x){ return __builtin_asinf(x); }

inline double  atan(double x){ return __builtin_atan(x); }
inline float atanf(float x){ return __builtin_atanf(x); }

inline double  atan2(double y,double x){ return __builtin_atan2(y,x); }
inline float atan2f(float y,float x){ return __builtin_atan2f(y,x); }

inline double  cos(double x){ return __builtin_cos(x); }
inline float cosf(float x){ return __builtin_cosf(x); }

inline double  cosh(double x){ return __builtin_cosh(x); }
inline float coshf(float x){ return __builtin_coshf(x); }

inline double  sin(double x){ return __builtin_sin(x); }
inline float sinf(float x){ return __builtin_sinf(x); }

inline double  sinh(double x){ return __builtin_sinh(x); }
inline float sinhf(float x){ return __builtin_sinhf(x); }

inline double  tan(double x){ return __builtin_tan(x); }
inline float tanf(float x){ return __builtin_tanf(x); }

inline double  tanh(double x){ return __builtin_tanh(x); }
inline float tanhf(float x){ return __builtin_tanhf(x); }

inline double  exp(double x){ return __builtin_exp(x); }
inline float expf(float x){ return __builtin_expf(x); }

inline double  frexp(double x, int* exponent){ return __builtin_frexp(x, exponent); }
inline float frexpf(float x, int* exponent){ return __builtin_frexpf(x, exponent); }

/**
 * Built-in functions
 */
//inline double  ldexp(double x, int* exponent){ return __builtin_ldexp(x, exponent); }
//inline float ldexpf(float x, int* exponent){ return __builtin_ldexpf(x, exponent); }

inline double  log(double x){ return __builtin_log(x); }
inline float logf(float x){ return __builtin_logf(x); }

inline double  log10(double x){ return __builtin_log10(x); }
inline float log10f(float x){ return __builtin_log10f(x); }

inline double  modf(double x, double* integer){ return __builtin_modf(x, integer); }
inline float modff(float x, float* integer){ return __builtin_modff(x, integer); }

inline double  pow(double x, double y){ return __builtin_pow(x,y); }
inline float powf(float x, float y){ return __builtin_powf(x,y); }

inline double sqrt(double x){ return __builtin_sqrt(x); }
inline float sqrtf(float x){ return __builtin_sqrtf(x); }

inline double ceil(double x){ return __builtin_ceil(x); }
inline float ceilf(float x){ return __builtin_ceilf(x); }

inline int abs(int x){ return __builtin_abs(x); }
inline double fabs(double x){ return __builtin_fabs(x); }
inline float fabsf(float x){ return __builtin_fabsf(x); }

inline double floor(double x){ return __builtin_floor(x); }
inline float floorf(float x){ return __builtin_floorf(x); }

inline double fmod(double x, double y){ return __builtin_fmod(x,y); }
inline float fmodf(float x, float y){ return __builtin_fmodf(x,y); }

#pragma once

//types
#define HUGE_VALF ((float)0x7f800000)
#define HUGE_VAL ((double)0x7ff0000000000000)
#define HUGE_VALL ((long double)0x7ff0000000000000)
#define NAN ((float)0x7fc00000)

//floating-point weirdness handlers
#define FP_NAN       0
#define FP_INFINITE  1
#define FP_ZERO      2
#define FP_SUBNORMAL 3
#define FP_NORMAL    4

#define isinf(x)             (__builtin_isinf(x))
#define isnan(x)             (__builtin_isnan(x))
#define isnormal(x)          (__builtin_isnormal(x))
#define isfinite(x)          (__builtin_isfinite(x))
#define signbit(x)           (__builtin_signbit(x))
#define isunordered(x, y)    (__builtin_isunordered(x, y))
#define fpclassify(x)        (__builtin_fpclassify(FP_NAN, FP_INFINITE, \
                                                   FP_NORMAL, FP_SUBNORMAL, \
                                                   FP_ZERO, x))

//Comparisons (Here for completeness. Equivalent to a regular comparison.)
// NOTE: if any two args are NAN some hardware would 
// throw an exception.  But in wasms case all NAN comparisons are falsey
#define isless(x, y)         (__builtin_isless(x, y))
#define islessequal(x, y)    (__builtin_islessequal(x, y))
#define islessgreater(x, y)  (__builtin_islessgreater(x, y))
#define isgreater(x, y)      (__builtin_isgreater(x, y))
#define isgreaterequal(x, y) (__builtin_isgreaterequal(x, y))

//trig
static inline double cos(double x) { return __builtin_cos(x); }
static inline float cosf(float x ) { return __builtin_cosf(x); }

static inline double sin(double x) { return __builtin_sin(x); }
static inline float sinf(float x ) { return __builtin_sinf(x); }

static inline double tan(double x) { return __builtin_tan(x); }
static inline float tanf(float x ) { return __builtin_tanf(x); }

static inline double acos(double x) { return __builtin_acos(x); }
static inline float acosf(float x ) { return __builtin_acosf(x); }

static inline double asin(double x) { return __builtin_asin(x); }
static inline float asinf(float x ) { return __builtin_asinf(x); }

static inline double atan(double x) { return __builtin_atan(x); }
static inline float atanf(float x ) { return __builtin_atanf(x); }

static inline double atan2(double y, double x) { return __builtin_atan2(y, x); }
static inline float atan2f(float y, float x ) { return __builtin_atan2f(y, x); }

static inline double cosh(double x) { return __builtin_cosh(x); }
static inline float coshf(float x ) { return __builtin_coshf(x); }

static inline double sinh(double x) { return __builtin_sinh(x); }
static inline float sinhf(float x ) { return __builtin_sinhf(x); }

static inline double tanh(double x) { return __builtin_tanh(x); }
static inline float tanhf(float x ) { return __builtin_tanhf(x); }

//exp
static inline double exp(double x) { return __builtin_exp(x); }
static inline float expf(float x ) { return __builtin_expf(x); }

static inline double frexp(double x, int *exponent) { return __builtin_frexp(x, exponent); }
static inline float frexpf(float x , int *exponent) { return __builtin_frexpf(x, exponent); }

//inline double ldexp(double x, int *exponent) { return __builtin_ldexp(x, exponent); }
//inline float ldexpf(float x, int *exponent) { return __builtin_ldexpf(x, exponent); }

static inline double log(double x) { return __builtin_log(x); }
static inline float logf(float x) { return __builtin_logf(x); }

static inline double log10(double x) { return __builtin_log10(x); }
static inline float log10f(float x) { return __builtin_log10f(x); }

static inline double modf(double x, double *y) { return __builtin_modf(x, y); }
static inline float modff(float x, float *y) { return __builtin_modff(x, y); }

static inline double fmod(double x, double y) { return __builtin_fmod(x, y); }
static inline float fmodf(float x, float y) { return __builtin_fmodf(x, y); }

static inline double pow(double x, double y) { return __builtin_pow(x, y); }
static inline float powf(float x, float y) { return __builtin_powf(x, y); }

//geometric
static inline double sqrt(double x) { return __builtin_sqrt(x); }
static inline float sqrtf(float x) { return __builtin_sqrtf(x); }

//sign
static inline double fabs(double x) { return __builtin_fabs(x); }
static inline float fabsf(float x) { return __builtin_fabsf(x); }

//integer
static inline double ceil(double x) { return __builtin_ceil(x); }
static inline float ceilf(float x) { return __builtin_ceilf(x); }

static inline double floor(double x) { return __builtin_floor(x); }
static inline float floorf(float x) { return __builtin_floorf(x); }

static inline double round(double x) { return __builtin_floor(x + 0.5); }
static inline float roundf(float x) { return __builtin_floorf(x + 0.5f); }

#include "stdio.h"

#include <stdarg.h> // for va_arg(), va_list()
#include <stddef.h> // size_t, ptrdiff_t
#include <stdint.h>

extern __attribute__((import_module("wlibc"),import_name("perror")))
	void _perror(const char* err_msg);
	
extern __attribute__((import_module("wlibc"),import_name("puts")))
	int _puts(const char* str);

void perror(const char* err_msg){
	_perror(err_msg);
}
int puts(const char* str){
	return _puts(str);
}


// How many characters per callback fn.
// (feel free to change this to accomodate your stack size)
// NOTE: printf family can successfully return a write length > WLIBC_STDIO_STACKBUF_SIZE.
// 		Before printf exceeds the stack buffer size it will flush to the JS.
//		The wlibc equivalent of setbuf for stdout exists on the JS side 
#ifndef WLIBC_STDIO_STACKBUF_SIZE
#define WLIBC_STDIO_STACKBUF_SIZE 512
#endif

typedef unsigned long ulong;//the standard wants length args to be unsigned long instead of int, but we want to support wasm64 with standards compliance
typedef __UINTPTR_TYPE__ uintptr;//we're only targeting wasm

extern __attribute__((import_module("wlibc"),import_name("console_buf")))
	int wlibc_console_buf(char const *const start, unsigned long len);

#ifndef WLIBC_STDIO_NOFLOAT
// internal float utility functions
static int32_t real_to_str(char const **start, uint32_t *len, char *out, int32_t *decimal_pos, double value, uint32_t frac_digits);
static int32_t real_to_parts(int64_t *bits, int32_t *expo, double value);
#define STBSP__SPECIAL 0x7000
#endif

static struct
{
   short temp; // force next field to be 2-byte aligned
   char pair[201];
} digitpair =
{
  0,
   "00010203040506070809101112131415161718192021222324"
   "25262728293031323334353637383940414243444546474849"
   "50515253545556575859606162636465666768697071727374"
   "75767778798081828384858687888990919293949596979899"
};

#define FLAG_LEFTJUST 1
#define FLAG_LEADINGPLUS 2
#define FLAG_LEADINGSPACE 4
#define STBSP__LEADING_0X 8
#define FLAG_LEADINGZERO 16
#define FLAG_INTMAX 32
#define FLAG_TRIPLET_COMMA 64
#define FLAG_NEGATIVE 128
#define FLAG_METRIC_SUFFIX 256
#define FLAG_HALFWIDTH 512
#define FLAG_METRIC_NOSPACE 1024
#define FLAG_METRIC_1024 2048
#define FLAG_METRIC_JEDEC 4096

static void lead_sign(uint32_t fl, char *sign)
{
   sign[0] = 0;
   if (fl & FLAG_NEGATIVE) {
      sign[0] = 1;
      sign[1] = '-';
   } else if (fl & FLAG_LEADINGSPACE) {
      sign[0] = 1;
      sign[1] = ' ';
   } else if (fl & FLAG_LEADINGPLUS) {
      sign[0] = 1;
      sign[1] = '+';
   }
}

static ADDR_SAN uint32_t strlen_limited(char const *s, uint32_t limit)
{
   char const * sn = s;

   // get up to 4-byte alignment
   for (;;) {
      if (((uintptr)sn & 3) == 0)
         break;

      if (!limit || *sn == 0)
         return (uint32_t)(sn - s);

      ++sn;
      --limit;
   }

   // scan over 4 bytes at a time to find terminating 0
   // this will intentionally scan up to 3 bytes past the end of buffers,
   // but becase it works 4B aligned, it will never cross page boundaries
   // (hence the ADDR_SAN markup; the over-read here is intentional
   // and harmless)
   while (limit >= 4) {
      uint32_t v = *(uint32_t *)sn;
      // bit hack to find if there's a 0 byte in there
      if ((v - 0x01010101) & (~v) & 0x80808080UL)
         break;

      sn += 4;
      limit -= 4;
   }

   // handle the last few characters to find actual size
   while (limit && *sn) {
      ++sn;
      --limit;
   }

   return (uint32_t)(sn - s);
}


typedef char * FormatCallback(const char *buf, void *user, ulong len);
static int _vsprintf(FormatCallback *callback, void *user, char *buf, char const *fmt, va_list va);

ADDR_SAN int _vsprintf(FormatCallback *callback, void *user, char *buf, char const *fmt, va_list va)
{
   static char hex[] = "0123456789abcdefxp";
   static char hexu[] = "0123456789ABCDEFXP";
   char *bf;
   char const *f;
   ulong tlen = 0;

   bf = buf;
   f = fmt;
   for (;;) {
      int32_t fw, pr, tz;
      uint32_t fl;

      // macros for the callback buffer stuff
      #define chk_cb_bufL(bytes)                        \
         {                                                     \
            ulong len = (ulong)(bf - buf);                       \
            if ((len + (bytes)) >= WLIBC_STDIO_STACKBUF_SIZE) {          \
               tlen += len;                                    \
               if (0 == (bf = buf = callback(buf, user, len))) \
                  goto done;                                   \
            }                                                  \
         }
      #define chk_cb_buf(bytes)    \
         {                                \
            if (callback) {               \
               chk_cb_bufL(bytes); \
            }                             \
         }
      #define flush_cb()                      \
         {                                           \
            chk_cb_bufL(WLIBC_STDIO_STACKBUF_SIZE - 1); \
         } // flush if there is even one byte in the buffer
      #define cb_buf_clamp(cl, v)                	\
         cl = v;                                        	\
         if (callback) {                                	\
            ulong lg = WLIBC_STDIO_STACKBUF_SIZE - (ulong)(bf - buf); \
            if (cl > lg)                                	\
               cl = lg;                                 	\
         }

      // fast copy everything up to the next % (or end of string)
      for (;;) {
         while (((uintptr)f) & 3) {
         schk1:
            if (f[0] == '%')
               goto scandd;
         schk2:
            if (f[0] == 0)
               goto endfmt;
            chk_cb_buf(1);
            *bf++ = f[0];
            ++f;
         }
         for (;;) {
            // Check if the next 4 bytes contain %(0x25) or end of string.
            // Using the 'hasless' trick:
            // https://graphics.stanford.edu/~seander/bithacks.html#HasLessInWord
            uint32_t v, c;
            v = *(uint32_t *)f;
            c = (~v) & 0x80808080;
            if (((v ^ 0x25252525) - 0x01010101) & c)
               goto schk1;
            if ((v - 0x01010101) & c)
               goto schk2;
            if (callback)
               if ((WLIBC_STDIO_STACKBUF_SIZE - (ulong)(bf - buf)) < 4)
                  goto schk1;
                if(((uintptr)bf) & 3) {
                    bf[0] = f[0];
                    bf[1] = f[1];
                    bf[2] = f[2];
                    bf[3] = f[3];
                } else
            {
                *(uint32_t *)bf = v;
            }
            bf += 4;
            f += 4;
         }
      }
   scandd:

      ++f;

      // ok, we have a percent, read the modifiers first
      fw = 0;
      pr = -1;
      fl = 0;
      tz = 0;

      // flags
      for (;;) {
         switch (f[0]) {
         // if we have left justify
         case '-':
            fl |= FLAG_LEFTJUST;
            ++f;
            continue;
         // if we have leading plus
         case '+':
            fl |= FLAG_LEADINGPLUS;
            ++f;
            continue;
         // if we have leading space
         case ' ':
            fl |= FLAG_LEADINGSPACE;
            ++f;
            continue;
         // if we have leading 0x
         case '#':
            fl |= STBSP__LEADING_0X;
            ++f;
            continue;
         // if we have thousand commas
         case '\'':
            fl |= FLAG_TRIPLET_COMMA;
            ++f;
            continue;
         // if we have kilo marker (none->kilo->kibi->jedec)
         case '$':
            if (fl & FLAG_METRIC_SUFFIX) {
               if (fl & FLAG_METRIC_1024) {
                  fl |= FLAG_METRIC_JEDEC;
               } else {
                  fl |= FLAG_METRIC_1024;
               }
            } else {
               fl |= FLAG_METRIC_SUFFIX;
            }
            ++f;
            continue;
         // if we don't want space between metric suffix and number
         case '_':
            fl |= FLAG_METRIC_NOSPACE;
            ++f;
            continue;
         // if we have leading zero
         case '0':
            fl |= FLAG_LEADINGZERO;
            ++f;
            goto flags_done;
         default: goto flags_done;
         }
      }
   flags_done:

      // get the field width
      if (f[0] == '*') {
         fw = va_arg(va, uint32_t);
         ++f;
      } else {
         while ((f[0] >= '0') && (f[0] <= '9')) {
            fw = fw * 10 + f[0] - '0';
            f++;
         }
      }
      // get the precision
      if (f[0] == '.') {
         ++f;
         if (f[0] == '*') {
            pr = va_arg(va, uint32_t);
            ++f;
         } else {
            pr = 0;
            while ((f[0] >= '0') && (f[0] <= '9')) {
               pr = pr * 10 + f[0] - '0';
               f++;
            }
         }
      }

      // handle integer size overrides
      switch (f[0]) {
      // are we halfwidth?
      case 'h':
         fl |= FLAG_HALFWIDTH;
         ++f;
         if (f[0] == 'h')
            ++f;  // QUARTERWIDTH
         break;
      // are we 64-bit (unix style)
      case 'l':
         fl |= ((sizeof(long) == 8) ? FLAG_INTMAX : 0);
         ++f;
         if (f[0] == 'l') {
            fl |= FLAG_INTMAX;
            ++f;
         }
         break;
      // are we 64-bit on intmax? (c99)
      case 'j':
         fl |= (sizeof(size_t) == 8) ? FLAG_INTMAX : 0;
         ++f;
         break;
      // are we 64-bit on size_t or ptrdiff_t? (c99)
      case 'z':
         fl |= (sizeof(ptrdiff_t) == 8) ? FLAG_INTMAX : 0;
         ++f;
         break;
      case 't':
         fl |= (sizeof(ptrdiff_t) == 8) ? FLAG_INTMAX : 0;
         ++f;
         break;
      // are we 64-bit (msft style)
      case 'I':
         if ((f[1] == '6') && (f[2] == '4')) {
            fl |= FLAG_INTMAX;
            f += 3;
         } else if ((f[1] == '3') && (f[2] == '2')) {
            f += 3;
         } else {
            fl |= ((sizeof(void *) == 8) ? FLAG_INTMAX : 0);
            ++f;
         }
         break;
      default: break;
      }

      // handle each replacement
      switch (f[0]) {
         #define STBSP__NUMSZ 512 // big enough for e308 (with commas) or e-307
         char num[STBSP__NUMSZ];
         char lead[8];
         char tail[8];
         char *s;
         char const *h;
         uint32_t l, n, cs;
         uint64_t n64;
#ifndef WLIBC_STDIO_NOFLOAT
         double fv;
#endif
         int32_t dp;
         char const *sn;

      case 's':
         // get the string
         s = va_arg(va, char *);
         if (s == 0)
            s = (char *)"null";
         // get the length, limited to desired precision
         // always limit to ~0u chars since our counts are 32b
         l = strlen_limited(s, (pr >= 0) ? pr : ~0u);
         lead[0] = 0;
         tail[0] = 0;
         pr = 0;
         dp = 0;
         cs = 0;
         // copy the string in
         goto scopy;

      case 'c': // char
         // get the character
         s = num + STBSP__NUMSZ - 1;
         *s = (char)va_arg(va, int);
         l = 1;
         lead[0] = 0;
         tail[0] = 0;
         pr = 0;
         dp = 0;
         cs = 0;
         goto scopy;

      case 'n': // weird write-bytes specifier
      {
         ulong *d = va_arg(va, ulong *);
         *d = tlen + (ulong)(bf - buf);
      } break;

#ifdef WLIBC_STDIO_NOFLOAT
      case 'A':              // float
      case 'a':              // hex float
      case 'G':              // float
      case 'g':              // float
      case 'E':              // float
      case 'e':              // float
      case 'f':              // float
         va_arg(va, double); // eat it
         s = (char *)"No float";
         l = 8;
         lead[0] = 0;
         tail[0] = 0;
         pr = 0;
         cs = 0;
         (void)sizeof(dp);
         goto scopy;
#else
      case 'A': // hex float
      case 'a': // hex float
         h = (f[0] == 'A') ? hexu : hex;
         fv = va_arg(va, double);
         if (pr == -1)
            pr = 6; // default is 6
         // read the double into a string
         if (real_to_parts((int64_t *)&n64, &dp, fv))
            fl |= FLAG_NEGATIVE;

         s = num + 64;

         lead_sign(fl, lead);

         if (dp == -1023)
            dp = (n64) ? -1022 : 0;
         else
            n64 |= (((uint64_t)1) << 52);
         n64 <<= (64 - 56);
         if (pr < 15)
            n64 += ((((uint64_t)8) << 56) >> (pr * 4));

		// add leading chars
         lead[1 + lead[0]] = '0';
         lead[2 + lead[0]] = 'x';
         lead[0] += 2;

         *s++ = h[(n64 >> 60) & 15];
         n64 <<= 4;
         if (pr)
            *s++ = WLIBC_DECIMAL_POINT;
         sn = s;

         // print the bits
         n = pr;
         if (n > 13)
            n = 13;
         if (pr > (int32_t)n)
            tz = pr - n;
         pr = 0;
         while (n--) {
            *s++ = h[(n64 >> 60) & 15];
            n64 <<= 4;
         }

         // print the expo
         tail[1] = h[17];
         if (dp < 0) {
            tail[2] = '-';
            dp = -dp;
         } else
            tail[2] = '+';
         n = (dp >= 1000) ? 6 : ((dp >= 100) ? 5 : ((dp >= 10) ? 4 : 3));
         tail[0] = (char)n;
         for (;;) {
            tail[n] = '0' + dp % 10;
            if (n <= 3)
               break;
            --n;
            dp /= 10;
         }

         dp = (int)(s - sn);
         l = (int)(s - (num + 64));
         s = num + 64;
         cs = 1 + (3 << 24);
         goto scopy;

      case 'G': // float
      case 'g': // float
         h = (f[0] == 'G') ? hexu : hex;
         fv = va_arg(va, double);
         if (pr == -1)
            pr = 6;
         else if (pr == 0)
            pr = 1; // default is 6
         // read the double into a string
         if (real_to_str(&sn, &l, num, &dp, fv, (pr - 1) | 0x80000000))
            fl |= FLAG_NEGATIVE;

         // clamp the precision and delete extra zeros after clamp
         n = pr;
         if (l > (uint32_t)pr)
            l = pr;
         while ((l > 1) && (pr) && (sn[l - 1] == '0')) {
            --pr;
            --l;
         }

         // should we use %e
         if ((dp <= -4) || (dp > (int32_t)n)) {
            if (pr > (int32_t)l)
               pr = l - 1;
            else if (pr)
               --pr; // when using %e, there is one digit before the decimal
            goto doexpfromg;
         }
         // this is the insane action to get the pr to match %g semantics for %f
         if (dp > 0) {
            pr = (dp < (int32_t)l) ? l - dp : 0;
         } else {
            pr = -dp + ((pr > (int32_t)l) ? (int32_t) l : pr);
         }
         goto dofloatfromg;

      case 'E': // float
      case 'e': // float
         h = (f[0] == 'E') ? hexu : hex;
         fv = va_arg(va, double);
         if (pr == -1)
            pr = 6; // default is 6
         // read the double into a string
         if (real_to_str(&sn, &l, num, &dp, fv, pr | 0x80000000))
            fl |= FLAG_NEGATIVE;
      doexpfromg:
         tail[0] = 0;
         lead_sign(fl, lead);
         if (dp == STBSP__SPECIAL) {
            s = (char *)sn;
            cs = 0;
            pr = 0;
            goto scopy;
         }
         s = num + 64;
         // handle leading chars
         *s++ = sn[0];

         if (pr)
            *s++ = WLIBC_DECIMAL_POINT;

         // handle after decimal
         if ((l - 1) > (uint32_t)pr)
            l = pr + 1;
         for (n = 1; n < l; n++)
            *s++ = sn[n];
         // trailing zeros
         tz = pr - (l - 1);
         pr = 0;
         // dump expo
         tail[1] = h[0xe];
         dp -= 1;
         if (dp < 0) {
            tail[2] = '-';
            dp = -dp;
         } else
            tail[2] = '+';

         n = (dp >= 100) ? 5 : 4;

         tail[0] = (char)n;
         for (;;) {
            tail[n] = '0' + dp % 10;
            if (n <= 3)
               break;
            --n;
            dp /= 10;
         }
         cs = 1 + (3 << 24); // how many tens
         goto flt_lead;

      case 'f': // float
         fv = va_arg(va, double);
      doafloat:
         // do kilos
         if (fl & FLAG_METRIC_SUFFIX) {
            double divisor;
            divisor = 1000.0f;
            if (fl & FLAG_METRIC_1024)
               divisor = 1024.0;
            while (fl < 0x4000000) {
               if ((fv < divisor) && (fv > -divisor))
                  break;
               fv /= divisor;
               fl += 0x1000000;
            }
         }
         if (pr == -1)
            pr = 6; // default is 6
         // read the double into a string
         if (real_to_str(&sn, &l, num, &dp, fv, pr))
            fl |= FLAG_NEGATIVE;
      dofloatfromg:
         tail[0] = 0;
         lead_sign(fl, lead);
         if (dp == STBSP__SPECIAL) {
            s = (char *)sn;
            cs = 0;
            pr = 0;
            goto scopy;
         }
         s = num + 64;

         // handle the three decimal varieties
         if (dp <= 0) {
            int32_t i;
            // handle 0.000*000xxxx
            *s++ = '0';
            if (pr)
               *s++ = WLIBC_DECIMAL_POINT;
            n = -dp;
            if ((int32_t)n > pr)
               n = pr;
            i = n;
            while (i) {
               if ((((uintptr)s) & 3) == 0)
                  break;
               *s++ = '0';
               --i;
            }
            while (i >= 4) {
               *(uint32_t *)s = 0x30303030;
               s += 4;
               i -= 4;
            }
            while (i) {
               *s++ = '0';
               --i;
            }
            if ((int32_t)(l + n) > pr)
               l = pr - n;
            i = l;
            while (i) {
               *s++ = *sn++;
               --i;
            }
            tz = pr - (n + l);
            cs = 1 + (3 << 24); // how many tens did we write (for commas below)
         } else {
            cs = (fl & FLAG_TRIPLET_COMMA) ? ((600 - (uint32_t)dp) % 3) : 0;
            if ((uint32_t)dp >= l) {
               // handle xxxx000*000.0
               n = 0;
               for (;;) {
                  if ((fl & FLAG_TRIPLET_COMMA) && (++cs == 4)) {
                     cs = 0;
                     *s++ = WLIBC_THOUSANDS_SEP;
                  } else {
                     *s++ = sn[n];
                     ++n;
                     if (n >= l)
                        break;
                  }
               }
               if (n < (uint32_t)dp) {
                  n = dp - n;
                  if ((fl & FLAG_TRIPLET_COMMA) == 0) {
                     while (n) {
                        if ((((uintptr)s) & 3) == 0)
                           break;
                        *s++ = '0';
                        --n;
                     }
                     while (n >= 4) {
                        *(uint32_t *)s = 0x30303030;
                        s += 4;
                        n -= 4;
                     }
                  }
                  while (n) {
                     if ((fl & FLAG_TRIPLET_COMMA) && (++cs == 4)) {
                        cs = 0;
                        *s++ = WLIBC_THOUSANDS_SEP;
                     } else {
                        *s++ = '0';
                        --n;
                     }
                  }
               }
               cs = (int)(s - (num + 64)) + (3 << 24); // cs is how many tens
               if (pr) {
                  *s++ = WLIBC_DECIMAL_POINT;
                  tz = pr;
               }
            } else {
               // handle xxxxx.xxxx000*000
               n = 0;
               for (;;) {
                  if ((fl & FLAG_TRIPLET_COMMA) && (++cs == 4)) {
                     cs = 0;
                     *s++ = WLIBC_THOUSANDS_SEP;
                  } else {
                     *s++ = sn[n];
                     ++n;
                     if (n >= (uint32_t)dp)
                        break;
                  }
               }
               cs = (int)(s - (num + 64)) + (3 << 24); // cs is how many tens
               if (pr)
                  *s++ = WLIBC_DECIMAL_POINT;
               if ((l - dp) > (uint32_t)pr)
                  l = pr + dp;
               while (n < l) {
                  *s++ = sn[n];
                  ++n;
               }
               tz = pr - (l - dp);
            }
         }
         pr = 0;

         // handle k,m,g,t
         if (fl & FLAG_METRIC_SUFFIX) {
            char idx;
            idx = 1;
            if (fl & FLAG_METRIC_NOSPACE)
               idx = 0;
            tail[0] = idx;
            tail[1] = ' ';
            {
               if (fl >> 24) { // SI kilo is 'k', JEDEC and SI kibits are 'K'.
                  if (fl & FLAG_METRIC_1024)
                     tail[idx + 1] = "_KMGT"[fl >> 24];
                  else
                     tail[idx + 1] = "_kMGT"[fl >> 24];
                  idx++;
                  // If printing kibits and not in jedec, add the 'i'.
                  if (fl & FLAG_METRIC_1024 && !(fl & FLAG_METRIC_JEDEC)) {
                     tail[idx + 1] = 'i';
                     idx++;
                  }
                  tail[0] = idx;
               }
            }
         };

      flt_lead:
         // get the length that we copied
         l = (uint32_t)(s - (num + 64));
         s = num + 64;
         goto scopy;
#endif

      case 'B': // upper binary
      case 'b': // lower binary
         h = (f[0] == 'B') ? hexu : hex;
         lead[0] = 0;
         if (fl & STBSP__LEADING_0X) {
            lead[0] = 2;
            lead[1] = '0';
            lead[2] = h[0xb];
         }
         l = (8 << 4) | (1 << 8);
         goto radixnum;

      case 'o': // octal
         h = hexu;
         lead[0] = 0;
         if (fl & STBSP__LEADING_0X) {
            lead[0] = 1;
            lead[1] = '0';
         }
         l = (3 << 4) | (3 << 8);
         goto radixnum;

      case 'p': // pointer
         fl |= (sizeof(void *) == 8) ? FLAG_INTMAX : 0;
         pr = sizeof(void *) * 2;
         fl &= ~FLAG_LEADINGZERO; // 'p' only prints the pointer with zeros
                                    // fall through - to X

      case 'X': // upper hex
      case 'x': // lower hex
         h = (f[0] == 'X') ? hexu : hex;
         l = (4 << 4) | (4 << 8);
         lead[0] = 0;
         if (fl & STBSP__LEADING_0X) {
            lead[0] = 2;
            lead[1] = '0';
            lead[2] = h[16];
         }
      radixnum:
         // get the number
         if (fl & FLAG_INTMAX)
            n64 = va_arg(va, uint64_t);
         else
            n64 = va_arg(va, uint32_t);

         s = num + STBSP__NUMSZ;
         dp = 0;
         // clear tail, and clear leading if value is zero
         tail[0] = 0;
         if (n64 == 0) {
            lead[0] = 0;
            if (pr == 0) {
               l = 0;
               cs = 0;
               goto scopy;
            }
         }
         // convert to string
         for (;;) {
            *--s = h[n64 & ((1 << (l >> 8)) - 1)];
            n64 >>= (l >> 8);
            if (!((n64) || ((int32_t)((num + STBSP__NUMSZ) - s) < pr)))
               break;
            if (fl & FLAG_TRIPLET_COMMA) {
               ++l;
               if ((l & 15) == ((l >> 4) & 15)) {
                  l &= ~15;
                  *--s = WLIBC_THOUSANDS_SEP;
               }
            }
         };
         // get the tens and the comma pos
         cs = (uint32_t)((num + STBSP__NUMSZ) - s) + ((((l >> 4) & 15)) << 24);
         // get the length that we copied
         l = (uint32_t)((num + STBSP__NUMSZ) - s);
         // copy it
         goto scopy;

      case 'u': // unsigned
      case 'i':
      case 'd': // integer
         // get the integer and abs it
         if (fl & FLAG_INTMAX) {
            int64_t i64 = va_arg(va, int64_t);
            n64 = (uint64_t)i64;
            if ((f[0] != 'u') && (i64 < 0)) {
               n64 = (uint64_t)-i64;
               fl |= FLAG_NEGATIVE;
            }
         } else {
            int32_t i = va_arg(va, int32_t);
            n64 = (uint32_t)i;
            if ((f[0] != 'u') && (i < 0)) {
               n64 = (uint32_t)-i;
               fl |= FLAG_NEGATIVE;
            }
         }

#ifndef WLIBC_STDIO_NOFLOAT
         if (fl & FLAG_METRIC_SUFFIX) {
            if (n64 < 1024)
               pr = 0;
            else if (pr == -1)
               pr = 1;
            fv = (double)(int64_t)n64;
            goto doafloat;
         }
#endif

         // convert to string
         s = num + STBSP__NUMSZ;
         l = 0;

         for (;;) {
            // do in 32-bit soft_chunks (avoid lots of 64-bit divides even with constant denominators)
            char *o = s - 8;
            if (n64 >= 100000000) {
               n = (uint32_t)(n64 % 100000000);
               n64 /= 100000000;
            } else {
               n = (uint32_t)n64;
               n64 = 0;
            }
            if ((fl & FLAG_TRIPLET_COMMA) == 0) {
               do {
                  s -= 2;
                  *(uint16_t *)s = *(uint16_t *)&digitpair.pair[(n % 100) * 2];
                  n /= 100;
               } while (n);
            }
            while (n) {
               if ((fl & FLAG_TRIPLET_COMMA) && (l++ == 3)) {
                  l = 0;
                  *--s = WLIBC_THOUSANDS_SEP;
                  --o;
               } else {
                  *--s = (char)(n % 10) + '0';
                  n /= 10;
               }
            }
            if (n64 == 0) {
               if ((s[0] == '0') && (s != (num + STBSP__NUMSZ)))
                  ++s;
               break;
            }
            while (s != o)
               if ((fl & FLAG_TRIPLET_COMMA) && (l++ == 3)) {
                  l = 0;
                  *--s = WLIBC_THOUSANDS_SEP;
                  --o;
               } else {
                  *--s = '0';
               }
         }

         tail[0] = 0;
         lead_sign(fl, lead);

         // get the length that we copied
         l = (uint32_t)((num + STBSP__NUMSZ) - s);
         if (l == 0) {
            *--s = '0';
            l = 1;
         }
         cs = l + (3 << 24);
         if (pr < 0)
            pr = 0;

      scopy:
         // get fw=leading/trailing space, pr=leading zeros
         if (pr < (int32_t)l)
            pr = l;
         n = pr + lead[0] + tail[0] + tz;
         if (fw < (int32_t)n)
            fw = n;
         fw -= n;
         pr -= l;

         // handle right justify and leading zeros
         if ((fl & FLAG_LEFTJUST) == 0) {
            if (fl & FLAG_LEADINGZERO) // if leading zeros, everything is in pr
            {
               pr = (fw > pr) ? fw : pr;
               fw = 0;
            } else {
               fl &= ~FLAG_TRIPLET_COMMA; // if no leading zeros, then no commas
            }
         }

         // copy the spaces and/or zeros
         if (fw + pr) {
            int32_t i;
            uint32_t c;

            // copy leading spaces (or when doing %8.4d stuff)
            if ((fl & FLAG_LEFTJUST) == 0)
               while (fw > 0) {
                  cb_buf_clamp(i, fw);
                  fw -= i;
                  while (i) {
                     if ((((uintptr)bf) & 3) == 0)
                        break;
                     *bf++ = ' ';
                     --i;
                  }
                  while (i >= 4) {
                     *(uint32_t *)bf = 0x20202020;
                     bf += 4;
                     i -= 4;
                  }
                  while (i) {
                     *bf++ = ' ';
                     --i;
                  }
                  chk_cb_buf(1);
               }

            // copy leader
            sn = lead + 1;
            while (lead[0]) {
               cb_buf_clamp(i, lead[0]);
               lead[0] -= (char)i;
               while (i) {
                  *bf++ = *sn++;
                  --i;
               }
               chk_cb_buf(1);
            }

            // copy leading zeros
            c = cs >> 24;
            cs &= 0xffffff;
            cs = (fl & FLAG_TRIPLET_COMMA) ? ((uint32_t)(c - ((pr + cs) % (c + 1)))) : 0;
            while (pr > 0) {
               cb_buf_clamp(i, pr);
               pr -= i;
               if ((fl & FLAG_TRIPLET_COMMA) == 0) {
                  while (i) {
                     if ((((uintptr)bf) & 3) == 0)
                        break;
                     *bf++ = '0';
                     --i;
                  }
                  while (i >= 4) {
                     *(uint32_t *)bf = 0x30303030;
                     bf += 4;
                     i -= 4;
                  }
               }
               while (i) {
                  if ((fl & FLAG_TRIPLET_COMMA) && (cs++ == c)) {
                     cs = 0;
                     *bf++ = WLIBC_THOUSANDS_SEP;
                  } else
                     *bf++ = '0';
                  --i;
               }
               chk_cb_buf(1);
            }
         }

         // copy leader if there is still one
         sn = lead + 1;
         while (lead[0]) {
            int32_t i;
            cb_buf_clamp(i, lead[0]);
            lead[0] -= (char)i;
            while (i) {
               *bf++ = *sn++;
               --i;
            }
            chk_cb_buf(1);
         }

         // copy the string
         n = l;
         while (n) {
            int32_t i;
            cb_buf_clamp(i, n);
            n -= i;
            while (i) {
               *bf++ = *s++;
               --i;
            }
            chk_cb_buf(1);
         }

         // copy trailing zeros
         while (tz) {
            int32_t i;
            cb_buf_clamp(i, tz);
            tz -= i;
            while (i) {
               if ((((uintptr)bf) & 3) == 0)
                  break;
               *bf++ = '0';
               --i;
            }
            while (i >= 4) {
               *(uint32_t *)bf = 0x30303030;
               bf += 4;
               i -= 4;
            }
            while (i) {
               *bf++ = '0';
               --i;
            }
            chk_cb_buf(1);
         }

         // copy tail if there is one
         sn = tail + 1;
         while (tail[0]) {
            int32_t i;
            cb_buf_clamp(i, tail[0]);
            tail[0] -= (char)i;
            while (i) {
               *bf++ = *sn++;
               --i;
            }
            chk_cb_buf(1);
         }

         // handle the left justify
         if (fl & FLAG_LEFTJUST)
            if (fw > 0) {
               while (fw) {
                  int32_t i;
                  cb_buf_clamp(i, fw);
                  fw -= i;
                  while (i) {
                     if ((((uintptr)bf) & 3) == 0)
                        break;
                     *bf++ = ' ';
                     --i;
                  }
                  while (i >= 4) {
                     *(uint32_t *)bf = 0x20202020;
                     bf += 4;
                     i -= 4;
                  }
                  while (i--)
                     *bf++ = ' ';
                  chk_cb_buf(1);
               }
            }
         break;

      default: // unknown, just copy code
         s = num + STBSP__NUMSZ - 1;
         *s = f[0];
         l = 1;
         fw = fl = 0;
         lead[0] = 0;
         tail[0] = 0;
         pr = 0;
         dp = 0;
         cs = 0;
         goto scopy;
      }
      ++f;
   }
endfmt:

   if (!callback)
      *bf = 0;
   else
      flush_cb();

done:
   return tlen + (ulong)(bf - buf);
}

// ============================================================================
//   wrapper functions

typedef struct {
   char *buf;
   ulong count;
   ulong length;
   char tmp[WLIBC_STDIO_STACKBUF_SIZE];
} sprintf_context_t;

static char *clamp_callback(char const *buf, void *user, ulong len)
{
   sprintf_context_t *c = (sprintf_context_t *)user;
   c->length += len;

   if (len > c->count)
      len = c->count;

   if (len) {
      if (buf != c->buf) {
         char const *s, *se;
         char *d;
         d = c->buf;
         s = buf;
         se = buf + len;
         do {
            *d++ = *s++;
         } while (s < se);
      }
      c->buf += len;
      c->count -= len;
   }

   if (c->count <= 0)
      return c->tmp;
   return (c->count >= WLIBC_STDIO_STACKBUF_SIZE) ? c->buf : c->tmp; // go direct into buffer if you can
}
static char * count_clamp_callback(char const *buf, void *user, ulong len )
{
   sprintf_context_t * c = (sprintf_context_t*)user;
   (void) sizeof(buf);

   c->length += len;
   return c->tmp; // go direct into buffer if you can
}
int snprintf(char *buf, ulong count, char const *fmt, ...)
{
   int result;
   va_list va;
   va_start(va, fmt);

   result = vsnprintf(buf, count, fmt, va);
   va_end(va);

   return result;
}
int vsnprintf(char * buf, ulong count, char const *fmt, va_list va )
{
   sprintf_context_t c;

   if ( (count == 0) && !buf )
   {
      c.length = 0;

      _vsprintf( count_clamp_callback, &c, c.tmp, fmt, va );
   }
   else
   {
      int l;

      c.buf = buf;
      c.count = count;
      c.length = 0;

      _vsprintf( clamp_callback, &c, clamp_callback(0,&c,0), fmt, va );

      // zero-terminate
      l = (ulong)( c.buf - buf );
      if ( l >= count ) // should never be greater, only equal (or less) than count
         l = count - 1;
      buf[l] = 0;
   }

   return c.length;
}

int sprintf(char *buf, char const *fmt, ...)
{
   int result;
   va_list va;
   va_start(va, fmt);
   result = _vsprintf(0, 0, buf, fmt, va);
   va_end(va);
   return result;
}
int vsprintf(char *buf, char const *fmt, va_list va)
{
   return _vsprintf(0, 0, buf, fmt, va);
}

typedef struct {
   ulong length;
   char tmp[WLIBC_STDIO_STACKBUF_SIZE];
} printf_context_t;
static char * printf_callback(char const *buf, void *user, ulong len )
{
   printf_context_t * c = (printf_context_t*)user;

	wlibc_console_buf(&c->tmp[0], len);

   c->length += len;
   return &c->tmp[0]; // go direct into buffer if you can
}
int printf(char const *fmt, ...)
{
	int result;
	va_list va;
	va_start(va, fmt);

	result = vprintf(fmt, va);
	va_end(va);

	return result;
}
int vprintf(char const *fmt, va_list va){

	//buf is a fixed size stack allocation.
	//When buf reaches max it will add it to the JS buffer
	printf_context_t c;
	c.length = 0;

	_vsprintf(printf_callback, &c, &c.tmp[0], fmt, va);

	return c.length;	
}



// =======================================================================
//   low level float utility functions

#ifndef WLIBC_STDIO_NOFLOAT

// copies d to bits w/ strict aliasing (this compiles to nothing on /Ox)
#define STBSP__COPYFP(dest, src)                   \
   {                                               \
      int cn;                                      \
      for (cn = 0; cn < 8; cn++)                   \
         ((char *)&dest)[cn] = ((char *)&src)[cn]; \
   }

// get float info
static int32_t real_to_parts(int64_t *bits, int32_t *expo, double value)
{
   double d;
   int64_t b = 0;

   // load value and round at the frac_digits
   d = value;

   STBSP__COPYFP(b, d);

   *bits = b & ((((uint64_t)1) << 52) - 1);
   *expo = (int32_t)(((b >> 52) & 2047) - 1023);

   return (int32_t)((uint64_t) b >> 63);
}

static double const bot[23] = {
   1e+000, 1e+001, 1e+002, 1e+003, 1e+004, 1e+005, 1e+006, 1e+007, 1e+008, 1e+009, 1e+010, 1e+011,
   1e+012, 1e+013, 1e+014, 1e+015, 1e+016, 1e+017, 1e+018, 1e+019, 1e+020, 1e+021, 1e+022
};
static double const negbot[22] = {
   1e-001, 1e-002, 1e-003, 1e-004, 1e-005, 1e-006, 1e-007, 1e-008, 1e-009, 1e-010, 1e-011,
   1e-012, 1e-013, 1e-014, 1e-015, 1e-016, 1e-017, 1e-018, 1e-019, 1e-020, 1e-021, 1e-022
};
static double const negboterr[22] = {
   -5.551115123125783e-018,  -2.0816681711721684e-019, -2.0816681711721686e-020, -4.7921736023859299e-021, -8.1803053914031305e-022, 4.5251888174113741e-023,
   4.5251888174113739e-024,  -2.0922560830128471e-025, -6.2281591457779853e-026, -3.6432197315497743e-027, 6.0503030718060191e-028,  2.0113352370744385e-029,
   -3.0373745563400371e-030, 1.1806906454401013e-032,  -7.7705399876661076e-032, 2.0902213275965398e-033,  -7.1542424054621921e-034, -7.1542424054621926e-035,
   2.4754073164739869e-036,  5.4846728545790429e-037,  9.2462547772103625e-038,  -4.8596774326570872e-039
};
static double const top[13] = {
   1e+023, 1e+046, 1e+069, 1e+092, 1e+115, 1e+138, 1e+161, 1e+184, 1e+207, 1e+230, 1e+253, 1e+276, 1e+299
};
static double const negtop[13] = {
   1e-023, 1e-046, 1e-069, 1e-092, 1e-115, 1e-138, 1e-161, 1e-184, 1e-207, 1e-230, 1e-253, 1e-276, 1e-299
};
static double const toperr[13] = {
   8388608,
   6.8601809640529717e+028,
   -7.253143638152921e+052,
   -4.3377296974619174e+075,
   -1.5559416129466825e+098,
   -3.2841562489204913e+121,
   -3.7745893248228135e+144,
   -1.7356668416969134e+167,
   -3.8893577551088374e+190,
   -9.9566444326005119e+213,
   6.3641293062232429e+236,
   -5.2069140800249813e+259,
   -5.2504760255204387e+282
};
static double const negtoperr[13] = {
   3.9565301985100693e-040,  -2.299904345391321e-063,  3.6506201437945798e-086,  1.1875228833981544e-109,
   -5.0644902316928607e-132, -6.7156837247865426e-155, -2.812077463003139e-178,  -5.7778912386589953e-201,
   7.4997100559334532e-224,  -4.6439668915134491e-247, -6.3691100762962136e-270, -9.436808465446358e-293,
   8.0970921678014997e-317
};

static uint64_t const powten[20] = {
   1,
   10,
   100,
   1000,
   10000,
   100000,
   1000000,
   10000000,
   100000000,
   1000000000,
   10000000000ULL,
   100000000000ULL,
   1000000000000ULL,
   10000000000000ULL,
   100000000000000ULL,
   1000000000000000ULL,
   10000000000000000ULL,
   100000000000000000ULL,
   1000000000000000000ULL,
   10000000000000000000ULL
};
#define tento19th (1000000000000000000ULL)

#define ddmulthi(oh, ol, xh, yh)                            \
   {                                                               \
      double ahi = 0, alo, bhi = 0, blo;                           \
      int64_t bt;                                             \
      oh = xh * yh;                                                \
      STBSP__COPYFP(bt, xh);                                       \
      bt &= ((~(uint64_t)0) << 27);                           \
      STBSP__COPYFP(ahi, bt);                                      \
      alo = xh - ahi;                                              \
      STBSP__COPYFP(bt, yh);                                       \
      bt &= ((~(uint64_t)0) << 27);                           \
      STBSP__COPYFP(bhi, bt);                                      \
      blo = yh - bhi;                                              \
      ol = ((ahi * bhi - oh) + ahi * blo + alo * bhi) + alo * blo; \
   }

#define ddtoS64(ob, xh, xl)          \
   {                                        \
      double ahi = 0, alo, vh, t;           \
      ob = (int64_t)xh;                \
      vh = (double)ob;                      \
      ahi = (xh - vh);                      \
      t = (ahi - xh);                       \
      alo = (xh - (ahi - t)) - (vh + t);    \
      ob += (int64_t)(ahi + alo + xl); \
   }

#define ddrenorm(oh, ol) \
   {                            \
      double s;                 \
      s = oh + ol;              \
      ol = ol - (s - oh);       \
      oh = s;                   \
   }

#define ddmultlo(oh, ol, xh, xl, yh, yl) ol = ol + (xh * yl + xl * yh);

#define ddmultlos(oh, ol, xh, yl) ol = ol + (xh * yl);

static void raise_to_power10(double *ohi, double *olo, double d, int32_t power) // power can be -323 to +350
{
   double ph, pl;
   if ((power >= 0) && (power <= 22)) {
      ddmulthi(ph, pl, d, bot[power]);
   } else {
      int32_t e, et, eb;
      double p2h, p2l;

      e = power;
      if (power < 0)
         e = -e;
      et = (e * 0x2c9) >> 14; /* %23 */
      if (et > 13)
         et = 13;
      eb = e - (et * 23);

      ph = d;
      pl = 0.0;
      if (power < 0) {
         if (eb) {
            --eb;
            ddmulthi(ph, pl, d, negbot[eb]);
            ddmultlos(ph, pl, d, negboterr[eb]);
         }
         if (et) {
            ddrenorm(ph, pl);
            --et;
            ddmulthi(p2h, p2l, ph, negtop[et]);
            ddmultlo(p2h, p2l, ph, pl, negtop[et], negtoperr[et]);
            ph = p2h;
            pl = p2l;
         }
      } else {
         if (eb) {
            e = eb;
            if (eb > 22)
               eb = 22;
            e -= eb;
            ddmulthi(ph, pl, d, bot[eb]);
            if (e) {
               ddrenorm(ph, pl);
               ddmulthi(p2h, p2l, ph, bot[e]);
               ddmultlos(p2h, p2l, bot[e], pl);
               ph = p2h;
               pl = p2l;
            }
         }
         if (et) {
            ddrenorm(ph, pl);
            --et;
            ddmulthi(p2h, p2l, ph, top[et]);
            ddmultlo(p2h, p2l, ph, pl, top[et], toperr[et]);
            ph = p2h;
            pl = p2l;
         }
      }
   }
   ddrenorm(ph, pl);
   *ohi = ph;
   *olo = pl;
}

// given a float value, returns the significant bits in bits, and the position of the
//   decimal point in decimal_pos.  +/-INF and NAN are specified by special values
//   returned in the decimal_pos parameter.
// frac_digits is absolute normally, but if you want from first significant digits (got %g and %e), or in 0x80000000
static int32_t real_to_str(char const **start, uint32_t *len, char *out, int32_t *decimal_pos, double value, uint32_t frac_digits)
{
   double d;
   int64_t bits = 0;
   int32_t expo, e, ng, tens;

   d = value;
   STBSP__COPYFP(bits, d);
   expo = (int32_t)((bits >> 52) & 2047);
   ng = (int32_t)((uint64_t) bits >> 63);
   if (ng)
      d = -d;

   if (expo == 2047) // is nan or inf?
   {
      *start = (bits & ((((uint64_t)1) << 52) - 1)) ? "NaN" : "Inf";
      *decimal_pos = STBSP__SPECIAL;
      *len = 3;
      return ng;
   }

   if (expo == 0) // is zero or denormal
   {
      if (((uint64_t) bits << 1) == 0) // do zero
      {
         *decimal_pos = 1;
         *start = out;
         out[0] = '0';
         *len = 1;
         return ng;
      }
      // find the right expo for denormals
      {
         int64_t v = ((uint64_t)1) << 51;
         while ((bits & v) == 0) {
            --expo;
            v >>= 1;
         }
      }
   }

   // find the decimal exponent as well as the decimal bits of the value
   {
      double ph, pl;

      // log10 estimate - very specifically tweaked to hit or undershoot by no more than 1 of log10 of all expos 1..2046
      tens = expo - 1023;
      tens = (tens < 0) ? ((tens * 617) / 2048) : (((tens * 1233) / 4096) + 1);

      // move the significant bits into position and stick them into an int
      raise_to_power10(&ph, &pl, d, 18 - tens);

      // get full as much precision from double-double as possible
      ddtoS64(bits, ph, pl);

      // check if we undershot
      if (((uint64_t)bits) >= tento19th)
         ++tens;
   }

   // now do the rounding in integer land
   frac_digits = (frac_digits & 0x80000000) ? ((frac_digits & 0x7ffffff) + 1) : (tens + frac_digits);
   if ((frac_digits < 24)) {
      uint32_t dg = 1;
      if ((uint64_t)bits >= powten[9])
         dg = 10;
      while ((uint64_t)bits >= powten[dg]) {
         ++dg;
         if (dg == 20)
            goto noround;
      }
      if (frac_digits < dg) {
         uint64_t r;
         // add 0.5 at the right position and round
         e = dg - frac_digits;
         if ((uint32_t)e >= 24)
            goto noround;
         r = powten[e];
         bits = bits + (r / 2);
         if ((uint64_t)bits >= powten[dg])
            ++tens;
         bits /= r;
      }
   noround:;
   }

   // kill long trailing runs of zeros
   if (bits) {
      uint32_t n;
      for (;;) {
         if (bits <= 0xffffffff)
            break;
         if (bits % 1000)
            goto donez;
         bits /= 1000;
      }
      n = (uint32_t)bits;
      while ((n % 1000) == 0)
         n /= 1000;
      bits = n;
   donez:;
   }

   // convert to string
   out += 64;
   e = 0;
   for (;;) {
      uint32_t n;
      char *o = out - 8;
      // do the conversion in soft_chunks of U32s (avoid most 64-bit divides, worth it, constant denomiators be damned)
      if (bits >= 100000000) {
         n = (uint32_t)(bits % 100000000);
         bits /= 100000000;
      } else {
         n = (uint32_t)bits;
         bits = 0;
      }
      while (n) {
         out -= 2;
         *(uint16_t *)out = *(uint16_t *)&digitpair.pair[(n % 100) * 2];
         n /= 100;
         e += 2;
      }
      if (bits == 0) {
         if ((e) && (out[0] == '0')) {
            ++out;
            --e;
         }
         break;
      }
      while (out != o) {
         *--out = '0';
         ++e;
      }
   }

   *decimal_pos = tens;
   *start = out;
   *len = e;
   return ng;
}

#endif // WLIBC_STDIO_NOFLOAT

/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2017 Sean Barrett
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/
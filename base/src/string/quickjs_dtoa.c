/*
 * QuickJS Javascript Engine
 *
 * Copyright (c) 2017-2021 Fabrice Bellard
 * Copyright (c) 2017-2021 Charlie Gordon
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <assert.h>
#include <fenv.h>
#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__APPLE__)
#include <malloc/malloc.h>
#elif defined(__linux__)
#include <malloc.h>
#elif defined(__FreeBSD__)
#include <malloc_np.h>
#endif

#define BOOL int
#define MAX_SAFE_INTEGER (((int64_t)1 << 53) - 1)

/* 2 <= base <= 36 */
static char* i64toa(char* buf_end, int64_t n, unsigned int base) {
  char* q = buf_end;
  int digit, is_neg;

  is_neg = 0;
  if (n < 0) {
    is_neg = 1;
    n = -n;
  }
  *--q = '\0';
  do {
    digit = (uint64_t)n % base;
    n = (uint64_t)n / base;
    if (digit < 10)
      digit += '0';
    else
      digit += 'a' - 10;
    *--q = digit;
  } while (n != 0);
  if (is_neg) *--q = '-';
  return q;
}

/* buf1 contains the printf result */
static void js_ecvt1(double d, int n_digits, int* decpt, int* sign, char* buf,
                     int rounding_mode, char* buf1, int buf1_size) {
  if (rounding_mode != FE_TONEAREST) fesetround(rounding_mode);
  snprintf(buf1, buf1_size, "%+.*e", n_digits - 1, d);
  if (rounding_mode != FE_TONEAREST) fesetround(FE_TONEAREST);
  *sign = (buf1[0] == '-');
  /* mantissa */
  buf[0] = buf1[1];
  if (n_digits > 1) memcpy(buf + 1, buf1 + 3, n_digits - 1);
  buf[n_digits] = '\0';
  /* exponent */
  *decpt = atoi(buf1 + n_digits + 2 + (n_digits > 1)) + 1;
}

/* maximum buffer size for js_dtoa */
#define JS_DTOA_BUF_SIZE 128

/* needed because ecvt usually limits the number of digits to
   17. Return the number of digits. */
static int js_ecvt(double d, int n_digits, int* decpt, int* sign, char* buf,
                   BOOL is_fixed) {
  int rounding_mode;
  char buf_tmp[JS_DTOA_BUF_SIZE];

  if (!is_fixed) {
    unsigned int n_digits_min, n_digits_max;
    /* find the minimum amount of digits (XXX: inefficient but simple) */
    n_digits_min = 1;
    n_digits_max = 17;
    while (n_digits_min < n_digits_max) {
      n_digits = (n_digits_min + n_digits_max) / 2;
      js_ecvt1(d, n_digits, decpt, sign, buf, FE_TONEAREST, buf_tmp,
               sizeof(buf_tmp));
      if (strtod(buf_tmp, NULL) == d) {
        /* no need to keep the trailing zeros */
        while (n_digits >= 2 && buf[n_digits - 1] == '0') n_digits--;
        n_digits_max = n_digits;
      } else {
        n_digits_min = n_digits + 1;
      }
    }
    n_digits = n_digits_max;
    rounding_mode = FE_TONEAREST;
  } else {
    rounding_mode = FE_TONEAREST;
  }
  js_ecvt1(d, n_digits, decpt, sign, buf, rounding_mode, buf_tmp,
           sizeof(buf_tmp));
  return n_digits;
}

/* radix != 10 is only supported with flags = JS_DTOA_VAR_FORMAT */
/* use as many digits as necessary */
#define JS_DTOA_VAR_FORMAT (0 << 0)

static void js_dtoa1(char* buf, double d, int radix, int n_digits, int flags) {
  char* q;

  if (!isfinite(d)) {
    if (isnan(d)) {
      strcpy(buf, "NaN");
    } else {
      q = buf;
      if (d < 0) *q++ = '-';
      strcpy(q, "Infinity");
    }
  } else if (flags == JS_DTOA_VAR_FORMAT) {
    int64_t i64;
    char buf1[70], *ptr;
    i64 = (int64_t)d;
    if (d != i64 || i64 > MAX_SAFE_INTEGER || i64 < -MAX_SAFE_INTEGER)
      goto generic_conv;
    /* fast path for integers */
    ptr = i64toa(buf1 + sizeof(buf1), i64, radix);
    strcpy(buf, ptr);
  } else {
    if (d == 0.0) d = 0.0; /* convert -0 to 0 */
    {
      char buf1[JS_DTOA_BUF_SIZE];
      int sign, decpt, k, n, i, p, n_max;
      BOOL is_fixed;
    generic_conv:
      is_fixed = 0;
      if (is_fixed) {
        n_max = n_digits;
      } else {
        n_max = 21;
      }
      /* the number has k digits (k >= 1) */
      k = js_ecvt(d, n_digits, &decpt, &sign, buf1, is_fixed);
      n = decpt; /* d=10^(n-k)*(buf1) i.e. d= < x.yyyy 10^(n-1) */
      q = buf;
      if (sign) *q++ = '-';
      if (n >= 1 && n <= n_max) {
        if (k <= n) {
          memcpy(q, buf1, k);
          q += k;
          for (i = 0; i < (n - k); i++) *q++ = '0';
          *q = '\0';
        } else {
          /* k > n */
          memcpy(q, buf1, n);
          q += n;
          *q++ = '.';
          for (i = 0; i < (k - n); i++) *q++ = buf1[n + i];
          *q = '\0';
        }
      } else if (n >= -5 && n <= 0) {
        *q++ = '0';
        *q++ = '.';
        for (i = 0; i < -n; i++) *q++ = '0';
        memcpy(q, buf1, k);
        q += k;
        *q = '\0';
      } else {
        /* exponential notation */
        *q++ = buf1[0];
        if (k > 1) {
          *q++ = '.';
          for (i = 1; i < k; i++) *q++ = buf1[i];
        }
        *q++ = 'e';
        p = n - 1;
        if (p >= 0) *q++ = '+';
        snprintf(q, sizeof(q), "%d", p);
      }
    }
  }
}

extern void js_dtoa(char* buf, double val) {
  js_dtoa1(buf, val, 10, 0, JS_DTOA_VAR_FORMAT);
}

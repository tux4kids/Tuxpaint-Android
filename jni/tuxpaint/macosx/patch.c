/*
 *  patch.c
 *  TuxPaint
 *
 *  Created by Eric (EP) on 13-09-13.
 *  Copyright 2013 __MyCompanyName__. All rights reserved.
 *
 *      Patch to fix code (incompatible, missing...)    //EP
 *
 */

#include <stdlib.h>
#include <errno.h>
#include <mm_malloc.h>
#include <png.h>
#include "patch.h"


// missing from wchar.h on Mac
wchar_t* wcsdup(const wchar_t* ws)
{
        wchar_t *ret;
        size_t len;
                
        len = wcslen (ws);
        ret = malloc ((len + 1) * sizeof (wchar_t));
        if (ret == 0)
                return ret;
        return (wcscpy (ret, ws));
}


// missing, needed by __nl_find_msg in libintl.a(dcigettext.o)
// http://forums.macrumors.com/showthread.php?t=1284479
#undef iconv_t
typedef void* iconv_t;
extern size_t libiconv(iconv_t cd,  char* * inbuf, size_t *inbytesleft, char* * outbuf, size_t *outbytesleft);
extern iconv_t libiconv_open(const char* tocode, const char* fromcode);

size_t iconv(iconv_t cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
{
        return libiconv(cd, inbuf, inbytesleft, outbuf, outbytesleft);
}

iconv_t iconv_open(const char *tocode, const char *fromcode)
{
        return libiconv_open(tocode, fromcode);
}


// to fix ineffective setlocale() in i18n.c or force language to Inuktitut
// must be called prior to setup_i18n()
patch_i18n(const char* locale)
{
        setenv("LANG", locale, 1);                      // takes language passed as an argument
//      setenv("LANG", "iu_CA.UTF-8", 1);       // forces language to Inuktitut
}


#ifdef PATCH_10_5
// missing, needed by __udivmodti4
// http://www.publicsource.apple.com/source/clang/clang-137/src/projects/compiler-rt/lib/int_lib.h
#include <limits.h>
typedef unsigned su_int;
typedef          long long di_int;
typedef unsigned long long du_int;
#ifdef __i386__
typedef int      ti_int __attribute__ ((mode (DI)));
typedef unsigned tu_int __attribute__ ((mode (DI)));
#else
typedef int      ti_int __attribute__ ((mode (TI)));
typedef unsigned tu_int __attribute__ ((mode (TI)));
#endif
typedef union
{
    tu_int all;
    struct
    {
#if _YUGA_LITTLE_ENDIAN
        du_int low;
        du_int high;
#else
        du_int high;
        du_int low;
#endif /* _YUGA_LITTLE_ENDIAN */
    }s;
} utwords;

// missing, needed by __umodti3 and __udivti3
// http://www.publicsource.apple.com/source/clang/clang-137/src/projects/compiler-rt/lib/udivmodti4.c
tu_int __udivmodti4(tu_int a, tu_int b, tu_int* rem)
{
    const unsigned n_udword_bits = sizeof(du_int) * CHAR_BIT;
    const unsigned n_utword_bits = sizeof(tu_int) * CHAR_BIT;
    utwords n;
    n.all = a;
    utwords d;
    d.all = b;
    utwords q;
    utwords r;
    unsigned sr;
    /* special cases, X is unknown, K != 0 */
    if (n.s.high == 0)
    {
        if (d.s.high == 0)
        {
            /* 0 X
             * ---
             * 0 X
             */
            if (rem)
                *rem = n.s.low % d.s.low;
            return n.s.low / d.s.low;
        }
        /* 0 X
         * ---
         * K X
         */
        if (rem)
            *rem = n.s.low;
        return 0;
    }
    /* n.s.high != 0 */
    if (d.s.low == 0)
    {
        if (d.s.high == 0)
        {
            /* K X
             * ---
             * 0 0
             */
            if (rem)
                *rem = n.s.high % d.s.low;
            return n.s.high / d.s.low;
        }
        /* d.s.high != 0 */
        if (n.s.low == 0)
        {
            /* K 0
             * ---
             * K 0
             */
            if (rem)
            {
                r.s.high = n.s.high % d.s.high;
                r.s.low = 0;
                *rem = r.all;
            }
            return n.s.high / d.s.high;
        }
        /* K K
         * ---
         * K 0
         */
        if ((d.s.high & (d.s.high - 1)) == 0)     /* if d is a power of 2 */
        {
            if (rem)
            {
                r.s.low = n.s.low;
                r.s.high = n.s.high & (d.s.high - 1);
                *rem = r.all;
            }
            return n.s.high >> __builtin_ctzll(d.s.high);
        }
        /* K K
         * ---
         * K 0
         */
        sr = __builtin_clzll(d.s.high) - __builtin_clzll(n.s.high);
        /* 0 <= sr <= n_udword_bits - 2 or sr large */
        if (sr > n_udword_bits - 2)
        {
                        if (rem)
                *rem = n.all;
            return 0;
        }
        ++sr;
        /* 1 <= sr <= n_udword_bits - 1 */
        /* q.all = n.all << (n_utword_bits - sr); */
        q.s.low = 0;
        q.s.high = n.s.low << (n_udword_bits - sr);
        /* r.all = n.all >> sr; */
        r.s.high = n.s.high >> sr;
        r.s.low = (n.s.high << (n_udword_bits - sr)) | (n.s.low >> sr);
    }
    else  /* d.s.low != 0 */
    {
        if (d.s.high == 0)
        {
            /* K X
             * ---
             * 0 K
             */
            if ((d.s.low & (d.s.low - 1)) == 0)     /* if d is a power of 2 */
            {
                if (rem)
                    *rem = n.s.low & (d.s.low - 1);
                if (d.s.low == 1)
                    return n.all;
                unsigned sr = __builtin_ctzll(d.s.low);
                q.s.high = n.s.high >> sr;
                q.s.low = (n.s.high << (n_udword_bits - sr)) | (n.s.low >> sr);
                return q.all;
            }
            /* K X
             * ---
             * 0 K
             */
            sr = 1 + n_udword_bits + __builtin_clzll(d.s.low)
                        - __builtin_clzll(n.s.high);
            /* 2 <= sr <= n_utword_bits - 1
             * q.all = n.all << (n_utword_bits - sr);
             * r.all = n.all >> sr;
             * if (sr == n_udword_bits)
             * {
             *     q.s.low = 0;
             *     q.s.high = n.s.low;
             *     r.s.high = 0;
             *     r.s.low = n.s.high;
             * }
             * else if (sr < n_udword_bits)  // 2 <= sr <= n_udword_bits - 1
             * {
             *     q.s.low = 0;
             *     q.s.high = n.s.low << (n_udword_bits - sr);
             *     r.s.high = n.s.high >> sr;
             *     r.s.low = (n.s.high << (n_udword_bits - sr)) | (n.s.low >> sr);
             * }
             * else              // n_udword_bits + 1 <= sr <= n_utword_bits - 1
             * {
             *     q.s.low = n.s.low << (n_utword_bits - sr);
             *     q.s.high = (n.s.high << (n_utword_bits - sr)) |
             *              (n.s.low >> (sr - n_udword_bits));
             *     r.s.high = 0;
             *     r.s.low = n.s.high >> (sr - n_udword_bits);
             * }
             */
            q.s.low =  (n.s.low << (n_utword_bits - sr)) &
                        ((di_int)(int)(n_udword_bits - sr) >> (n_udword_bits-1));
            q.s.high = ((n.s.low << ( n_udword_bits - sr))                        &
                                                ((di_int)(int)(sr - n_udword_bits - 1) >> (n_udword_bits-1))) |
                        (((n.s.high << (n_utword_bits - sr))                       |
                          (n.s.low >> (sr - n_udword_bits)))                         &
                         ((di_int)(int)(n_udword_bits - sr) >> (n_udword_bits-1)));
            r.s.high = (n.s.high >> sr) &
                        ((di_int)(int)(sr - n_udword_bits) >> (n_udword_bits-1));
            r.s.low =  ((n.s.high >> (sr - n_udword_bits))                        &
                                                ((di_int)(int)(n_udword_bits - sr - 1) >> (n_udword_bits-1))) |
                        (((n.s.high << (n_udword_bits - sr))                       |
                          (n.s.low >> sr))                                           &
                         ((di_int)(int)(sr - n_udword_bits) >> (n_udword_bits-1)));
        }
        else
        {
            /* K X
             * ---
             * K K
             */
            sr = __builtin_clzll(d.s.high) - __builtin_clzll(n.s.high);
            /*0 <= sr <= n_udword_bits - 1 or sr large */
            if (sr > n_udword_bits - 1)
            {
                                if (rem)
                    *rem = n.all;
                return 0;
            }
            ++sr;
            /* 1 <= sr <= n_udword_bits */
            /* q.all = n.all << (n_utword_bits - sr); */
            q.s.low = 0;
            q.s.high = n.s.low << (n_udword_bits - sr);
            /* r.all = n.all >> sr;
             * if (sr < n_udword_bits)
             * {
             *     r.s.high = n.s.high >> sr;
             *     r.s.low = (n.s.high << (n_udword_bits - sr)) | (n.s.low >> sr);
             * }
             * else
             * {
             *     r.s.high = 0;
             *     r.s.low = n.s.high;
             * }
             */
            r.s.high = (n.s.high >> sr) &
                        ((di_int)(int)(sr - n_udword_bits) >> (n_udword_bits-1));
            r.s.low = (n.s.high << (n_udword_bits - sr)) |
                        ((n.s.low >> sr)                   &
                         ((di_int)(int)(sr - n_udword_bits) >> (n_udword_bits-1)));
        }
    }
    /* Not a special case
     * q and r are initialized with:
     * q.all = n.all << (n_utword_bits - sr);
     * r.all = n.all >> sr;
     * 1 <= sr <= n_utword_bits - 1
     */
    su_int carry = 0;
    for (; sr > 0; --sr)
    {
        /* r:q = ((r:q)  << 1) | carry */
        r.s.high = (r.s.high << 1) | (r.s.low  >> (n_udword_bits - 1));
        r.s.low  = (r.s.low  << 1) | (q.s.high >> (n_udword_bits - 1));
        q.s.high = (q.s.high << 1) | (q.s.low  >> (n_udword_bits - 1));
        q.s.low  = (q.s.low  << 1) | carry;
        /* carry = 0;
         * if (r.all >= d.all)
         * {
         *     r.all -= d.all;
         *      carry = 1;
         * }
         */
        const ti_int s = (ti_int)(d.all - r.all - 1) >> (n_utword_bits - 1);
        carry = s & 1;
        r.all -= d.all & s;
    }
    q.all = (q.all << 1) | carry;
    if (rem)
        *rem = r.all;
    return q.all;
}

// missing, needed by __cairo_uint128_divrem in libcairo.a(cairo-wideint.o)
// http://www.publicsource.apple.com/source/clang/clang-137/src/projects/compiler-rt/lib/umodti3.c
tu_int __umodti3(tu_int a, tu_int b)
{
    tu_int r;
    __udivmodti4(a, b, &r);
    return r;
}

// missing, needed by __cairo_uint128_divrem in libcairo.a(cairo-wideint.o)
// http://www.publicsource.apple.com/source/clang/clang-137/src/projects/compiler-rt/lib/udivti3.c
tu_int __udivti3(tu_int a, tu_int b)
{
        return __udivmodti4(a, b, 0);
}

// missing, needed by __nl_log_untranslated in libintl.a(log.o) for 10.5
FILE* fopen$DARWIN_EXTSN(const char *filename, const char *mode)
{
        return fopen(filename, mode);
}

// missing, needed by _slab_allocator_alloc_chunk in libglib-2.0.a(libglib_2_0_la-gslice.o) for 10.5
// http://www.spinics.net/lists/fio/msg00700.html
// http://www.publicsource.apple.com/source/clang/clang-137/src/tools/clang/lib/Headers/mm_malloc.h
int posix_memalign(void **ptr, size_t align, size_t size)
{
        if (ptr)
        {
                *ptr = _mm_malloc (size, align);
                return 0;
        }

        return ENOMEM;
}
#endif  // PATCH_10_5

#ifdef PATCH_LIBPNG_EARLIER_THAN_1_5
// missing in libpng<1.5, needed by _Load_SBit_Png in libfreetype.a(sfnt.o), _error_callback in libfreetype.a(sfnt.o)
// http://stackoverflow.com/questions/5190554/unresolved-external-png-set-longjmp-fn-in-libpng
// http://cpansearch.perl.org/src/JTPALMER/Alien-SDL-1.439_1/patches/SDL_image-1.2.10-libpng15.patch
typedef jmp_buf* (*png_set_longjmp_fnPtr)(png_structp, void*, size_t);
png_set_longjmp_fnPtr   png_set_longjmp_fn = 0;
#endif


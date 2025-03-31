/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * This file is part of the TinyUSB stack.
 */

#ifndef _TUSB_COMPILER_H_
#define _TUSB_COMPILER_H_

/*------------------------------------------------------------------+
 | Stringification macros
 +------------------------------------------------------------------*/
#define TU_TOKEN(x)           x
#define TU_STRING(x)          #x                  /* stringify without expand */
#define TU_XSTRING(x)         TU_STRING(x)        /* expand then stringify */
#define TU_STRCAT(a, b)       a##b                /* concat without expand */
#define TU_STRCAT3(a, b, c)   a##b##c             /* concat without expand */
#define TU_XSTRCAT(a, b)      TU_STRCAT(a, b)     /* expand then concat */
#define TU_XSTRCAT3(a, b, c)  TU_STRCAT3(a, b, c) /* expand then concat 3 tokens */
#define TU_INCLUDE_PATH(_dir,_file) TU_XSTRING( TU_TOKEN(_dir)TU_TOKEN(_file) )

/*------------------------------------------------------------------+
 | Counter for compile-time asserts
 +------------------------------------------------------------------*/
#if defined __COUNTER__ && __COUNTER__ != __COUNTER__
  #define _TU_COUNTER_ __COUNTER__
#else
  #define _TU_COUNTER_ __LINE__
#endif

/*------------------------------------------------------------------+
 | Compile-time Assert
 +------------------------------------------------------------------*/
#if defined (__cplusplus) && __cplusplus >= 201103L
  #define TU_VERIFY_STATIC   static_assert
#elif defined (__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
  #define TU_VERIFY_STATIC   _Static_assert
#else
  #define TU_VERIFY_STATIC(const_expr, _mess) typedef char TU_XSTRCAT(_verify_static_, _TU_COUNTER_)[(const_expr) ? 1 : 0]
#endif

/*------------------------------------------------------------------+
 | Fuzzing definition (if needed)
 +------------------------------------------------------------------*/
#ifdef _FUZZ
  #define tu_static static __thread
#else
  #define tu_static static
#endif

#define TU_RESERVED           TU_XSTRCAT(reserved, _TU_COUNTER_)

/*------------------------------------------------------------------+
 | Endian macros
 +------------------------------------------------------------------*/
#define TU_LITTLE_ENDIAN (0x12u)
#define TU_BIG_ENDIAN    (0x21u)

/*------------------------------------------------------------------+
 | Count number of arguments in __VA_ARGS__
 +------------------------------------------------------------------*/
#define TU_ARGS_NUM(...)   _TU_NARG(_0, ##__VA_ARGS__, _RSEQ_N())
#define _TU_NARG(...)      _GET_NTH_ARG(__VA_ARGS__)
#define _GET_NTH_ARG( \
          _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
         _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, \
         _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
         _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, \
         _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
         _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
         _61, _62, _63, N, ...) N
#define _RSEQ_N() \
         62,61,60, 59,58,57,56,55,54,53,52,51,50, \
         49,48,47,46,45,44,43,42,41,40, 39,38,37,36,35,34,33,32,31,30, \
         29,28,27,26,25,24,23,22,21,20, 19,18,17,16,15,14,13,12,11,10, \
         9,8,7,6,5,4,3,2,1,0

/*------------------------------------------------------------------+
 | Apply a macro X to each argument with separator _s
 +------------------------------------------------------------------*/
#define TU_ARGS_APPLY(_X, _s, ...)   TU_XSTRCAT(_TU_ARGS_APPLY_, TU_ARGS_NUM(__VA_ARGS__))(_X, _s, __VA_ARGS__)
#define _TU_ARGS_APPLY_1(_X, _s, _a1)                                    _X(_a1)
#define _TU_ARGS_APPLY_2(_X, _s, _a1, _a2)                               _X(_a1) _s _X(_a2)
#define _TU_ARGS_APPLY_3(_X, _s, _a1, _a2, _a3)                          _X(_a1) _s _TU_ARGS_APPLY_2(_X, _s, _a2, _a3)
#define _TU_ARGS_APPLY_4(_X, _s, _a1, _a2, _a3, _a4)                     _X(_a1) _s _TU_ARGS_APPLY_3(_X, _s, _a2, _a3, _a4)
#define _TU_ARGS_APPLY_5(_X, _s, _a1, _a2, _a3, _a4, _a5)                _X(_a1) _s _TU_ARGS_APPLY_4(_X, _s, _a2, _a3, _a4, _a5)
#define _TU_ARGS_APPLY_6(_X, _s, _a1, _a2, _a3, _a4, _a5, _a6)           _X(_a1) _s _TU_ARGS_APPLY_5(_X, _s, _a2, _a3, _a4, _a5, _a6)
#define _TU_ARGS_APPLY_7(_X, _s, _a1, _a2, _a3, _a4, _a5, _a6, _a7)      _X(_a1) _s _TU_ARGS_APPLY_6(_X, _s, _a2, _a3, _a4, _a5, _a6, _a7)
#define _TU_ARGS_APPLY_8(_X, _s, _a1, _a2, _a3, _a4, _a5, _a6, _a7, _a8) _X(_a1) _s _TU_ARGS_APPLY_7(_X, _s, _a2, _a3, _a4, _a5, _a6, _a7, _a8)

/*------------------------------------------------------------------+
 | Macro for function default arguments
 +------------------------------------------------------------------*/
#define TU_GET_3RD_ARG(arg1, arg2, arg3, ...)        arg3
#define TU_FUNC_OPTIONAL_ARG(func, ...)   TU_XSTRCAT(func##_arg, TU_ARGS_NUM(__VA_ARGS__))(__VA_ARGS__)

/*------------------------------------------------------------------+
 | ESP32 Xtensa (GCC) specific definitions
 +------------------------------------------------------------------*/
#define TU_ATTR_ALIGNED(Bytes)        __attribute__ ((aligned(Bytes)))
#define TU_ATTR_SECTION(sec_name)     __attribute__ ((section(#sec_name)))
#define TU_ATTR_PACKED                __attribute__ ((packed))
#define TU_ATTR_WEAK                  __attribute__ ((weak))
#ifndef TU_ATTR_ALWAYS_INLINE
  #define TU_ATTR_ALWAYS_INLINE       __attribute__ ((always_inline))
#endif
#define TU_ATTR_DEPRECATED(mess)      __attribute__ ((deprecated(mess)))
#define TU_ATTR_UNUSED                __attribute__ ((unused))
#define TU_ATTR_USED                  __attribute__ ((used))
#if __GNUC__ < 5
  #define TU_ATTR_FALLTHROUGH         do {} while (0)
#else
  #if __has_attribute(__fallthrough__)
    #define TU_ATTR_FALLTHROUGH         __attribute__((fallthrough))
  #else
    #define TU_ATTR_FALLTHROUGH         do {} while (0)
  #endif
#endif

#define TU_ATTR_PACKED_BEGIN
#define TU_ATTR_PACKED_END
#define TU_ATTR_BIT_FIELD_ORDER_BEGIN
#define TU_ATTR_BIT_FIELD_ORDER_END

// Endian conversion: ESP32 is little-endian.
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  #define TU_BYTE_ORDER TU_LITTLE_ENDIAN
#else
  #define TU_BYTE_ORDER TU_BIG_ENDIAN
#endif

#define TU_BSWAP16(u16) (__builtin_bswap16(u16))
#define TU_BSWAP32(u32) (__builtin_bswap32(u32))

/*------------------------------------------------------------------+
 | Endian conversion macros
 +------------------------------------------------------------------*/
#define tu_htons(u16)  (TU_BSWAP16(u16))
#define tu_ntohs(u16)  (TU_BSWAP16(u16))
#define tu_htonl(u32)  (TU_BSWAP32(u32))
#define tu_ntohl(u32)  (TU_BSWAP32(u32))
#define tu_htole16(u16) (u16)
#define tu_le16toh(u16) (u16)
#define tu_htole32(u32) (u32)
#define tu_le32toh(u32) (u32)

#endif /* _TUSB_COMPILER_H_ */
/* Copyright (c) 2011-2012, Jon Mayo <jon@rm-f.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef JDM_COMPILER_H
#define JDM_COMPILER_H

#if defined __GNUC__ && __GNUC__ >= 3
/* function's return must be used or cast to void */
# define _must_check __attribute__((warn_unused_result))
/* mark a parameter as unused */
#define _unused __attribute__((unused))
#else
/* all other compilers */
# define _must_check
#define _unused
#endif

/* Thread Local Storage (TLS) storage class specifier */
#if defined(_MSC_VER)
/* Visual C++, Intel C/C++ (windows), Digital Mars C++, Borland C++ Builder */
# define _thread __declspec(thread)
#else
/* GCC, Intel C/C++ (linux), Sun Studio C/C++, IBM XL C/C++ */
# define _thread __thread
#endif

#endif

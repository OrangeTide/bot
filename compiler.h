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

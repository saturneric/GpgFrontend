
#ifndef GPGFRONTEND_TEST_EXPORT_H
#define GPGFRONTEND_TEST_EXPORT_H

#ifdef GPGFRONTEND_TEST_STATIC_DEFINE
#  define GPGFRONTEND_TEST_EXPORT
#  define GPGFRONTEND_TEST_NO_EXPORT
#else
#  ifndef GPGFRONTEND_TEST_EXPORT
#    ifdef gpgfrontend_test_EXPORTS
        /* We are building this library */
#      define GPGFRONTEND_TEST_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define GPGFRONTEND_TEST_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef GPGFRONTEND_TEST_NO_EXPORT
#    define GPGFRONTEND_TEST_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef GPGFRONTEND_TEST_DEPRECATED
#  define GPGFRONTEND_TEST_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef GPGFRONTEND_TEST_DEPRECATED_EXPORT
#  define GPGFRONTEND_TEST_DEPRECATED_EXPORT GPGFRONTEND_TEST_EXPORT GPGFRONTEND_TEST_DEPRECATED
#endif

#ifndef GPGFRONTEND_TEST_DEPRECATED_NO_EXPORT
#  define GPGFRONTEND_TEST_DEPRECATED_NO_EXPORT GPGFRONTEND_TEST_NO_EXPORT GPGFRONTEND_TEST_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef GPGFRONTEND_TEST_NO_DEPRECATED
#    define GPGFRONTEND_TEST_NO_DEPRECATED
#  endif
#endif

#endif /* GPGFRONTEND_TEST_EXPORT_H */

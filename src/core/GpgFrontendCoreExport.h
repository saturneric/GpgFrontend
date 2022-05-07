
#ifndef GPGFRONTEND_CORE_EXPORT_H
#define GPGFRONTEND_CORE_EXPORT_H

#ifdef GPGFRONTEND_CORE_STATIC_DEFINE
#  define GPGFRONTEND_CORE_EXPORT
#  define GPGFRONTEND_CORE_NO_EXPORT
#else
#  ifndef GPGFRONTEND_CORE_EXPORT
#    ifdef gpgfrontend_core_EXPORTS
        /* We are building this library */
#      define GPGFRONTEND_CORE_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define GPGFRONTEND_CORE_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef GPGFRONTEND_CORE_NO_EXPORT
#    define GPGFRONTEND_CORE_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef GPGFRONTEND_CORE_DEPRECATED
#  define GPGFRONTEND_CORE_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef GPGFRONTEND_CORE_DEPRECATED_EXPORT
#  define GPGFRONTEND_CORE_DEPRECATED_EXPORT GPGFRONTEND_CORE_EXPORT GPGFRONTEND_CORE_DEPRECATED
#endif

#ifndef GPGFRONTEND_CORE_DEPRECATED_NO_EXPORT
#  define GPGFRONTEND_CORE_DEPRECATED_NO_EXPORT GPGFRONTEND_CORE_NO_EXPORT GPGFRONTEND_CORE_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef GPGFRONTEND_CORE_NO_DEPRECATED
#    define GPGFRONTEND_CORE_NO_DEPRECATED
#  endif
#endif

#endif /* GPGFRONTEND_CORE_EXPORT_H */

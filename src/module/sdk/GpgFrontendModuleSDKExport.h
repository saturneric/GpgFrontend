
#ifndef GPGFRONTEND_MODULE_SDK_EXPORT_H
#define GPGFRONTEND_MODULE_SDK_EXPORT_H

#ifdef GPGFRONTEND_MODULE_SDK_STATIC_DEFINE
#  define GPGFRONTEND_MODULE_SDK_EXPORT
#  define GPGFRONTEND_MODULE_SDK_NO_EXPORT
#else
#  ifndef GPGFRONTEND_MODULE_SDK_EXPORT
#    ifdef gpgfrontend_module_sdk_EXPORTS
        /* We are building this library */
#      define GPGFRONTEND_MODULE_SDK_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define GPGFRONTEND_MODULE_SDK_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef GPGFRONTEND_MODULE_SDK_NO_EXPORT
#    define GPGFRONTEND_MODULE_SDK_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef GPGFRONTEND_MODULE_SDK_DEPRECATED
#  define GPGFRONTEND_MODULE_SDK_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef GPGFRONTEND_MODULE_SDK_DEPRECATED_EXPORT
#  define GPGFRONTEND_MODULE_SDK_DEPRECATED_EXPORT GPGFRONTEND_MODULE_SDK_EXPORT GPGFRONTEND_MODULE_SDK_DEPRECATED
#endif

#ifndef GPGFRONTEND_MODULE_SDK_DEPRECATED_NO_EXPORT
#  define GPGFRONTEND_MODULE_SDK_DEPRECATED_NO_EXPORT GPGFRONTEND_MODULE_SDK_NO_EXPORT GPGFRONTEND_MODULE_SDK_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef GPGFRONTEND_MODULE_SDK_NO_DEPRECATED
#    define GPGFRONTEND_MODULE_SDK_NO_DEPRECATED
#  endif
#endif

#endif /* GPGFRONTEND_MODULE_SDK_EXPORT_H */


#ifndef GPGFRONTEND_PLUGIN_EXPORT_H
#define GPGFRONTEND_PLUGIN_EXPORT_H

#ifdef GPGFRONTEND_PLUGIN_STATIC_DEFINE
#  define GPGFRONTEND_PLUGIN_EXPORT
#  define GPGFRONTEND_PLUGIN_NO_EXPORT
#else
#  ifndef GPGFRONTEND_PLUGIN_EXPORT
#    ifdef gpgfrontend_plugin_EXPORTS
        /* We are building this library */
#      define GPGFRONTEND_PLUGIN_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define GPGFRONTEND_PLUGIN_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef GPGFRONTEND_PLUGIN_NO_EXPORT
#    define GPGFRONTEND_PLUGIN_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef GPGFRONTEND_PLUGIN_DEPRECATED
#  define GPGFRONTEND_PLUGIN_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef GPGFRONTEND_PLUGIN_DEPRECATED_EXPORT
#  define GPGFRONTEND_PLUGIN_DEPRECATED_EXPORT GPGFRONTEND_PLUGIN_EXPORT GPGFRONTEND_PLUGIN_DEPRECATED
#endif

#ifndef GPGFRONTEND_PLUGIN_DEPRECATED_NO_EXPORT
#  define GPGFRONTEND_PLUGIN_DEPRECATED_NO_EXPORT GPGFRONTEND_PLUGIN_NO_EXPORT GPGFRONTEND_PLUGIN_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef GPGFRONTEND_PLUGIN_NO_DEPRECATED
#    define GPGFRONTEND_PLUGIN_NO_DEPRECATED
#  endif
#endif

#endif /* GPGFRONTEND_PLUGIN_EXPORT_H */

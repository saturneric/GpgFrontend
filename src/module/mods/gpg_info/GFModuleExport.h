
#ifndef GF_MODULE_EXPORT_H
#define GF_MODULE_EXPORT_H

#ifdef GF_MODULE_STATIC_DEFINE
#  define GF_MODULE_EXPORT
#  define GF_MODULE_NO_EXPORT
#else
#  ifndef GF_MODULE_EXPORT
#    ifdef mod_gpg_info_EXPORTS
        /* We are building this library */
#      define GF_MODULE_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define GF_MODULE_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef GF_MODULE_NO_EXPORT
#    define GF_MODULE_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef GF_MODULE_DEPRECATED
#  define GF_MODULE_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef GF_MODULE_DEPRECATED_EXPORT
#  define GF_MODULE_DEPRECATED_EXPORT GF_MODULE_EXPORT GF_MODULE_DEPRECATED
#endif

#ifndef GF_MODULE_DEPRECATED_NO_EXPORT
#  define GF_MODULE_DEPRECATED_NO_EXPORT GF_MODULE_NO_EXPORT GF_MODULE_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef GF_MODULE_NO_DEPRECATED
#    define GF_MODULE_NO_DEPRECATED
#  endif
#endif

#endif /* GF_MODULE_EXPORT_H */

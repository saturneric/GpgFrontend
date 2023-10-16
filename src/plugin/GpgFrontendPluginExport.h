/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
 *
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GpgFrontend is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GpgFrontend. If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from
 * the gpg4usb project, which is under GPL-3.0-or-later.
 *
 * All the source code of GpgFrontend was modified and released by
 * Saturneric <eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */
 
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

/**
 * Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
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

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "GFSDKBuffer.h"
#include "GFSDKGpgList.h"
#include "GFSDKGpgResult.h"
#include "GFSDKModuleModel.h"
#include "GFSDKVisibility.h"

/**
 * @file GFSDKModuleApi.h
 * @brief The module runtime ABI: one bootstrap symbol and two tables.
 *
 * WHAT THIS REPLACES. A module currently has to export TEN separate extern "C"
 * symbols, each resolved by name at load time, and link against ~130 global
 * SDK symbols besides. That arrangement makes three things hard that this one
 * makes easy:
 *
 *   - ABI NEGOTIATION. Ten independently-resolved symbols cannot express "I
 *     support host ABI 3 but not 4". A single entry point that is HANDED the
 *     host's ABI and returns NULL if it cannot work with it can.
 *   - CAPABILITY ENFORCEMENT. When every module links every symbol there is
 *     no place to stand to deny one. When the host hands over a table, it can
 *     hand over a table with the gpg group absent.
 *   - TESTING. A test can fill in a GFHostApi struct. Today it has to define
 *     ~20 real symbols in a stub translation unit and compile the module with
 *     GF_SDK_EXPORT stubbed out, and those stubs drift from the real SDK.
 *
 * GROWTH. Both tables begin with struct_size, written by whichever side
 * COMPILED the struct. The reader uses only the prefix both sides agree on,
 * so adding a field at the end is backwards compatible: an older module hands
 * over a smaller struct and still works, and an older host reads only what it
 * knows. Never reorder or repurpose an existing field -- only append.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* --- what the host provides to the module -------------------------------- */

/**
 * @brief The SDK, as a table rather than as global symbols.
 *
 * Grouped by area so that a capability decision has something to switch on:
 * a module without the "gpg" capability can be handed a table whose gpg
 * pointers are NULL, which is an enforcement point that does not exist when
 * every module links every symbol directly.
 *
 * A NULL function pointer means "not available to you"; a module must check
 * before calling rather than assuming, exactly as it would for any optional
 * interface.
 */
typedef struct GFHostApi {
  size_t struct_size;   /**< sizeof as the HOST compiled it */
  uint32_t abi_version; /**< the host's ABI generation */

  /* buffers -- always present, since everything else traffics in them */
  GFBufferRef (*buffer_new_from_bytes)(const void* data, size_t size);
  const void* (*buffer_data)(GFBufferView buf);
  size_t (*buffer_size)(GFBufferView buf);
  void (*buffer_zeroize)(GFBufferRef buf);
  void (*buffer_release)(GFBufferRef buf);

  /* gpg operations -- NULL when the module lacks the "gpg" capability */
  int (*gpg_sign)(int channel, const char* const* key_ids, size_t key_ids_size,
                  GFBufferView in, int sign_mode, int ascii,
                  GFGpgResultRef* out);
  int (*gpg_encrypt)(int channel, const char* const* key_ids,
                     size_t key_ids_size, GFBufferView in, int ascii,
                     GFGpgResultRef* out);
  int (*gpg_decrypt)(int channel, GFBufferView in, GFGpgResultRef* out);
  int (*gpg_verify)(int channel, GFBufferView in, GFBufferView signature,
                    GFGpgResultRef* out);

  /* result accessors */
  int (*result_status)(GFGpgResultRef r);
  uint32_t (*result_error)(GFGpgResultRef r);
  GFBufferView (*result_data)(GFGpgResultRef r);
  const char* (*result_capsule_id)(GFGpgResultRef r);
  const char* (*result_error_string)(GFGpgResultRef r);
  const char* (*result_hash_algo)(GFGpgResultRef r);
  GFBufferRef (*result_take_data)(GFGpgResultRef r);
  void (*result_release)(GFGpgResultRef r);

  /* logging -- always present; a module that cannot report is undebuggable */
  void (*log_debug)(const char* msg);
  void (*log_info)(const char* msg);
  void (*log_warn)(const char* msg);
  void (*log_error)(const char* msg);
} GFHostApi;

/**
 * @brief What the HOST verified about this module, handed to it at activate().
 *
 * Passed as GFModuleApi::activate's @c reserved argument, which was NULL until
 * this existed. A module built against an older host must tolerate NULL, and a
 * host older than a field must be detected through @ref struct_size -- this
 * table grows by APPENDING only, exactly like GFHostApi.
 *
 * WHY IT EXISTS. The host parses and signature-checks a module's manifest,
 * cross-checks its id and version against the loaded binary, and then had no
 * way to tell the module any of it. Every fact a module knew about itself came
 * from constants compiled into it, which is to say from an unverified source.
 *
 * OWNERSHIP. Everything here is BORROWED and valid only for the duration of
 * the activate() call. The host owns every pointer and frees nothing; the
 * strings point into storage in the caller's frame. A module that retains one
 * of these pointers past activate() has a dangling pointer -- copy what you
 * keep, before you return.
 *
 * TRUST. These fields only ever NARROW what a module does; none of them is a
 * grant. A capability list here is not permission to use a capability: that is
 * decided by what the host puts in GFHostApi, which is the only side the
 * module cannot replace.
 */
typedef struct GFModuleBootstrapInfo {
  size_t struct_size;   /**< sizeof as the HOST compiled it */
  uint32_t abi_version; /**< the host's ABI generation */
  uint32_t flags;       /**< GF_MODULE_BOOT_* */

  const char* module_id;      /**< the verified id, == GFModuleApi::module_id */
  const char* module_version; /**< the verified version */

  /** The signed manifest's translation context, or NULL when unverified. */
  const char* translation_context;

  /** The locale in effect at activation, e.g. "en_US". */
  const char* locale;

  const char* const* capabilities; /**< from the signed manifest */
  size_t capabilities_size;

  /** Event ids this module declared it subscribes to, UPPER-CASE. This is an
   *  allowlist: a module may subscribe to these and to nothing else. */
  const char* const* events;
  size_t events_size;
} GFModuleBootstrapInfo;

/** Every field above was backed by a verified signed manifest. When this is
 *  clear, the module was loaded loose (a development tree): the id and version
 *  are the module's own word for itself and nothing else is populated. */
#define GF_MODULE_BOOT_VERIFIED 0x1u

/* --- what the module provides to the host -------------------------------- */

/**
 * @brief A module's lifecycle, as a table.
 *
 * @ref module_id is the module's RUNTIME identity. It must be checked against
 * the identity in the module's signed manifest before activation: a package
 * whose manifest claims one id while its binary reports another is rejected,
 * because otherwise the signature covers a name nobody enforces.
 */
typedef struct GFModuleApi {
  size_t struct_size;   /**< sizeof as the MODULE compiled it */
  uint32_t abi_version; /**< the ABI the module implements */

  const char* module_id; /**< borrowed, static, never freed by the host */
  const char* version;   /**< borrowed, static */

  /** Called once after the module is accepted. @p host is borrowed and stays
   *  valid for the module's lifetime. Return 0 on success. */
  int (*activate)(const GFHostApi* host, void* reserved);

  /** Handle one event. Return 0 on success. */
  int (*execute)(GFModuleEvent* event);

  /** Cancel in-flight work and drop host registrations. Return 0 on success.
   *  The host waits for in-flight calls to return AFTER this, not before. */
  int (*deactivate)(void);

  /** Final teardown. No module code runs after this returns. */
  void (*unregister)(void);
} GFModuleApi;

/**
 * @brief The ONLY symbol a module is required to export.
 *
 * @param host_abi the host's ABI generation, so the module can decline a host
 *        it does not support by returning NULL -- negotiation rather than a
 *        crash on the first mismatched call.
 * @return a table with static storage duration, or NULL to decline. The host
 *         does not free it.
 */
GF_SDK_EXPORT const GFModuleApi* GFModuleGetApi(uint32_t host_abi);

/** Matching function-pointer type, for the host's symbol resolution. */
typedef const GFModuleApi* (*GFModuleGetApiFn)(uint32_t host_abi);

/**
 * @brief The host's own table, filled in with the real SDK entry points.
 *
 * Borrowed and valid for the life of the process.
 */
GF_SDK_EXPORT const GFHostApi* GFGetHostApi(void);

#ifdef __cplusplus
}
#endif

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

#include "GFSDKTypes.h"

/**
 * @file GFSDKHostApi.h
 * @brief What the host lets one module do, as capability groups.
 *
 * ## The one rule
 *
 * A module reaches host functionality through this table and through nothing
 * else. It does not link `gf_core` or `gf_ui`; the build refuses to produce a
 * module that does. The high-level `GFSDK*.h` spellings a module writes are
 * implemented ON TOP of these primitives, module-side, in `gf_sdk`
 * (`src/sdk/api`), which the module links statically -- so adding a
 * convenience helper is a change to `gf_sdk` and not a change to this ABI.
 *
 * ## Why a context, and not thread-local state
 *
 * Every primitive below takes a @ref GFHostContextRef as its first argument.
 * That context is minted by the host, per module, at activation, and carries
 * the module's identity, the set of groups it was granted, and a lifetime that
 * ends at unload.
 *
 * It exists because the obvious alternative does not work. The host already
 * brackets its own calls into module code with a thread-local record of whose
 * code is running (`host/private/GFHostAttribution.h`), and that record is
 * exactly absent where it would matter most: a module's own worker thread, or a
 * queued callback, has no host frame anywhere on its stack. Authorization
 * decided from the call stack would therefore be right for a call made inside
 * an event handler and wrong for the same call made from the thread that
 * handler started -- which is the normal shape of a module that does real work.
 *
 * Carrying the grant in an argument makes the answer the same on every thread.
 * The thread-local record keeps its own job (attributing handles, naming the
 * module in a log line); it is not the authority.
 *
 * The host validates a context pointer against its own registry before
 * reading anything through it, so an unknown, stale or invented value is
 * refused rather than followed. A module cannot widen its own grant either:
 * the granted set is in the host's record, not in anything the module holds.
 *
 * This is not protection against a hostile module. Native code in the same
 * process can read another module's live context pointer and call with it,
 * and the host will treat the call as that module's. What the context does
 * guarantee is that an honest module's calls are attributed and limited
 * correctly on every thread, and that handles issued to one module are
 * refused to every other.
 *
 * ## Growth
 *
 * Every struct here begins with `struct_size`, written by whichever side
 * COMPILED it. Fields may only be APPENDED. Reordering or repurposing one
 * makes an already-built module read the wrong slot with no diagnostic
 * anywhere, which is the failure this rule exists to prevent.
 *
 * A NULL group pointer means "this module was not granted this"; it is never
 * a neutral default, and a module must check before calling.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* --- the context --------------------------------------------------------- */

/**
 * @brief An opaque grant, minted per module, live while the module is active.
 *
 * Revoked when the module is deactivated and at shutdown; a call presenting a
 * revoked grant is refused. The record behind it is never freed, so holding
 * one past its revocation is safe -- it simply stops working.
 *
 * Borrowed: the host owns it and the module never releases it. Safe to copy
 * and to pass between the module's own threads, which is the entire point.
 */
typedef struct GFHostContextImpl* GFHostContextRef;

/**
 * @brief The groups a module may be granted.
 *
 * Only these are grantable. The always-present groups (buffer, log, app,
 * event, bootstrap) have no bit, because "granted" is not a question that
 * arises for them: a module that cannot allocate or report is not a module.
 */
#define GF_HOST_CAP_GPG (1u << 0)
#define GF_HOST_CAP_PGP (1u << 1)
#define GF_HOST_CAP_UI (1u << 2)
#define GF_HOST_CAP_EDITOR (1u << 3)
#define GF_HOST_CAP_STORAGE (1u << 4)
#define GF_HOST_CAP_PROCESS (1u << 5)
/** Module-owned native widgets in Host containers. Requires GF_HOST_CAP_UI. */
#define GF_HOST_CAP_UI_CUSTOM (1u << 6)

/* --- always-granted groups ----------------------------------------------- */

/**
 * @brief Buffers and raw memory. Always present.
 *
 * Everything else traffics in these, so withholding them would not restrict a
 * module, it would stop it existing.
 *
 * Passing the context to the allocators is what finally attributes a handle
 * created on a module's OWN thread, where no host frame exists to say whose
 * code is running.
 */
typedef struct GFHostBufferApi {
  size_t struct_size;

  GFBufferRef (*new_from_bytes)(GFHostContextRef ctx, const void* data,
                                size_t size);
  const void* (*data)(GFHostContextRef ctx, GFBufferView buf);
  size_t (*size)(GFHostContextRef ctx, GFBufferView buf);
  void (*zeroize)(GFHostContextRef ctx, GFBufferRef buf);
  void (*release)(GFHostContextRef ctx, GFBufferRef buf);
  size_t (*outstanding_count)(GFHostContextRef ctx);

  void* (*mem_alloc)(GFHostContextRef ctx, int arena, uint32_t size);
  void* (*mem_realloc)(GFHostContextRef ctx, int arena, void* p, uint32_t size);
  void (*mem_free)(GFHostContextRef ctx, int arena, void* p);
  char* (*mem_strdup)(GFHostContextRef ctx, int arena, const char* s);
} GFHostBufferApi;

/**
 * @brief Logging. Always present: a module that cannot report is undebuggable.
 *
 * One entry point rather than five, because trace/debug/info/warn/error
 * differed by an argument and nothing else. The module's identity comes from
 * the context, so it cannot be misreported.
 */
typedef struct GFHostLogApi {
  size_t struct_size;
  void (*write)(GFHostContextRef ctx, int severity, const char* file, int line,
                const char* function, const char* msg);
  int (*enabled)(GFHostContextRef ctx, int severity);
} GFHostLogApi;

/**
 * @brief Facts about the running application. Always present, all read-only.
 *
 * Each is a distinct fact rather than a lookup by code: `version` and
 * `is_flatpak` have nothing in common but their caller, and a
 * `get_fact(int)` would only have moved the type confusion into a cast.
 *
 * `GFCompareSoftwareVersion` is deliberately absent. Comparing two version
 * strings needs nothing from the host, so it belongs in the module-side SDK
 * where it costs no ABI at all.
 */
typedef struct GFHostAppApi {
  size_t struct_size;
  const char* (*version)(GFHostContextRef ctx);         /**< borrowed, static */
  const char* (*git_commit_hash)(GFHostContextRef ctx); /**< borrowed, static */
  const char* (*qt_env_version)(GFHostContextRef ctx);  /**< borrowed, static */
  const char* (*http_user_agent)(GFHostContextRef ctx); /**< borrowed, static */
  char* (*active_locale)(GFHostContextRef ctx); /**< owned; mem_free NORMAL */
  int (*is_flatpak)(GFHostContextRef ctx);
  int (*key_protection_level)(GFHostContextRef ctx);
} GFHostAppApi;

/**
 * @brief How a module joins the event system. Always present.
 *
 * Present unconditionally on purpose, because it answers a different question
 * from the rest of this table. The groups above and below say what a module
 * may actively DO; a subscription says what host activity it may OBSERVE or
 * take part in, and that is governed by the signed manifest's event allowlist,
 * enforced host-side. Making `event` grantable would conflate the two axes.
 */
typedef struct GFHostEventApi {
  size_t struct_size;
  /** Subscribe to @p event_id, decided at once: 0 means subscribed, and
   *  negative means refused -- an event the host does not fire, one the
   *  signed manifest does not declare, or a module that is not being
   *  activated or active. The refusal is logged. */
  int (*subscribe)(GFHostContextRef ctx, const char* event_id);
  /** Answer a trigger. @p answer->params is transferred to the host.
   *  Refused unless this module was delivered that trigger and has not
   *  answered it yet: one answer per delivery. */
  int (*answer)(GFHostContextRef ctx, const GFModuleEventAnswer* answer);
} GFHostEventApi;

/**
 * @brief Runtime bootstrap. Always present.
 *
 * Not an application capability, and kept out of `app` so it cannot be
 * mistaken for one: this is how the module runtime installs the module's own
 * translations before any hook runs. There is nothing here for a module author
 * to ask for.
 */
typedef struct GFHostBootstrapApi {
  size_t struct_size;
  int (*register_translator_reader)(GFHostContextRef ctx, const char* id,
                                    GFTranslatorDataReader reader);
} GFHostBootstrapApi;

/**
 * @brief Plain string lists. Always present.
 *
 * Its own group, and an ungated one, because a string list is a CONTAINER
 * rather than a subject. Two different capabilities produce one: `gpg`
 * returns keyring addresses in one, `storage` returns the register table's
 * child keys in one. Putting the accessors in either group would mean a
 * module holding the other could obtain a list it could not read -- or, worse,
 * could not release, which is a leak created by a permission boundary.
 *
 * The key-brief and recipient lists stay in `gpg`, where their CONTENTS
 * belong. Only the generic container is here.
 */
typedef struct GFHostListApi {
  size_t struct_size;
  size_t (*string_count)(GFHostContextRef ctx, GFStringListRef l);
  const char* (*string_at)(GFHostContextRef ctx, GFStringListRef l, size_t i);
  void (*string_release)(GFHostContextRef ctx, GFStringListRef l);
  /** Outstanding list handles of every kind held by this module. */
  size_t (*outstanding_count)(GFHostContextRef ctx);
} GFHostListApi;

/* --- grantable groups ---------------------------------------------------- */

/**
 * @brief Cryptographic operations and the keyring. Capability "gpg".
 *
 * The four operations stay typed and named: signing is not encrypting with a
 * different integer, and collapsing them would mean losing the compiler's
 * check on their genuinely different arguments.
 *
 * What did collapse: the twenty list accessors (now two row structs), the
 * three identical `const char*(GFGpgResultRef)` getters (now @ref result_text)
 * and the nine analyse entry points (now @ref analyse_result).
 */
typedef struct GFHostGpgApi {
  size_t struct_size;

  int (*current_channel)(GFHostContextRef ctx);

  int (*sign)(GFHostContextRef ctx, int channel, const char* const* key_ids,
              size_t key_ids_size, GFBufferView in, int sign_mode, int ascii,
              GFGpgResultRef* out);
  int (*encrypt)(GFHostContextRef ctx, int channel, const char* const* key_ids,
                 size_t key_ids_size, GFBufferView in, int ascii,
                 GFGpgResultRef* out);
  int (*decrypt)(GFHostContextRef ctx, int channel, GFBufferView in,
                 GFGpgResultRef* out);
  int (*verify)(GFHostContextRef ctx, int channel, GFBufferView in,
                GFBufferView signature, GFGpgResultRef* out);

  int (*result_status)(GFHostContextRef ctx, GFGpgResultRef r);
  uint32_t (*result_error)(GFHostContextRef ctx, GFGpgResultRef r);
  GFBufferView (*result_data)(GFHostContextRef ctx, GFGpgResultRef r);
  const char* (*result_text)(GFHostContextRef ctx, GFGpgResultRef r, int field);
  GFBufferRef (*result_take_data)(GFHostContextRef ctx, GFGpgResultRef r);
  void (*result_release)(GFHostContextRef ctx, GFGpgResultRef r);
  size_t (*result_outstanding_count)(GFHostContextRef ctx);

  /** @p want selects which members of @p out to produce; a member that was
   *  not asked for comes back NULL. The capsule is CONSUMED either way, so
   *  ask for everything needed in one call. */
  int (*analyse_result)(GFHostContextRef ctx, int channel, int operation,
                        uint32_t err, const char* capsule_id, uint32_t want,
                        GFGpgAnalysis* out);

  /** Owned; release with buffer->release. NULL when the key is unknown. */
  GFBufferRef (*public_key)(GFHostContextRef ctx, int channel,
                            const char* key_id, int ascii);
  /** The primary UID, split. Each out-parameter is OWNED and released with
   *  buffer->release; pass NULL for a part the caller does not want. The
   *  struct this replaced handed back three char* members to be freed one by
   *  one, which is the ownership shape this SDK exists to not have. */
  int (*key_primary_uid)(GFHostContextRef ctx, int channel, const char* key_id,
                         GFBufferRef* name, GFBufferRef* email,
                         GFBufferRef* comment);
  int (*export_key)(GFHostContextRef ctx, int channel, const char* key_id,
                    int ascii, GFBufferRef* out);
  /** @p parent is ignored: the Host parents its own result dialog. */
  int (*import_keys)(GFHostContextRef ctx, int channel, void* parent,
                     GFBufferView data);

  /* lists: three distinct handle types, deliberately. Funnelling unrelated
     collections through one void* handle would make it compile to hand a
     key-brief list to a recipient accessor. */
  int (*find_keys)(GFHostContextRef ctx, int channel, const char* email,
                   GFGpgKeyBriefListRef* out);
  size_t (*key_brief_count)(GFHostContextRef ctx, GFGpgKeyBriefListRef l);
  const GFGpgKeyBriefRow* (*key_brief_at)(GFHostContextRef ctx,
                                          GFGpgKeyBriefListRef l, size_t i);
  void (*key_brief_release)(GFHostContextRef ctx, GFGpgKeyBriefListRef l);

  int (*sniff_recipients)(GFHostContextRef ctx, int channel, GFBufferView in,
                          GFGpgRecipientListRef* out);
  size_t (*recipient_count)(GFHostContextRef ctx, GFGpgRecipientListRef l);
  const GFGpgRecipientRow* (*recipient_at)(GFHostContextRef ctx,
                                           GFGpgRecipientListRef l, size_t i);
  void (*recipient_release)(GFHostContextRef ctx, GFGpgRecipientListRef l);

  int (*list_addresses)(GFHostContextRef ctx, int channel, int secret_only,
                        GFStringListRef* out);
} GFHostGpgApi;

/**
 * @brief Reading OpenPGP structure with no keyring and no engine.
 *  Capability "pgp".
 *
 * Its own group rather than part of `gpg`: inspecting packet structure touches
 * no key material and runs no engine, so a module that only needs to describe
 * a message should not have to ask for the ability to decrypt one.
 */
typedef struct GFHostPgpApi {
  size_t struct_size;
  /** @p out owned; release with buffer->release. */
  int (*inspect)(GFHostContextRef ctx, GFBufferView in, GFBufferRef* out);
} GFHostPgpApi;

/**
 * @brief Putting things on the screen. Capability "ui".
 *
 * Reading the application's settings is NOT here. A module reading
 * configuration is
 * doing storage, and making it ask for the ability to open dialogs in order to
 * read a preference would be a grant that says more than it means. See
 * @ref GFHostStorageApi.
 */
typedef struct GFHostUiApi {
  size_t struct_size;

  /* --- retired -------------------------------------------------------------
   * Each of these handed a module a Host Qt object, or took one. They keep
   * their place so the table layout does not move, and every one of them now
   * refuses: NULL, -1 or 0, logged once per module with its replacement. A
   * module's UI is its script and its own native widgets (GFSDKUI.h). */

  void* (*create_object)(GFHostContextRef ctx, QObjectFactory factory,
                         void* data);                        /**< NULL */
  void* (*get_object)(GFHostContextRef ctx, const char* id); /**< NULL */
  int (*show_dialog)(GFHostContextRef ctx, void* dialog,
                     void* parent); /**< -1; a dialog mount + view.open */
  uint32_t (*theme_color)(GFHostContextRef ctx, int role,
                          void* widget); /**< 0; theme_color_role */
  /** Owned; release with buffer->release. */
  GFBufferRef (*user_file_path)(GFHostContextRef ctx);

  /* retired, -1: a settings mount, an editor mount, its `extensions` */
  int (*register_settings_page)(GFHostContextRef ctx,
                                const GFUISettingsPageSpec* spec);
  int (*unregister_settings_page)(GFHostContextRef ctx, const char* page_id);
  int (*register_tab_view)(GFHostContextRef ctx, const GFUITabViewSpec* spec);
  int (*unregister_tab_view)(GFHostContextRef ctx, const char* tab_type);
  int (*register_file_extension)(GFHostContextRef ctx, const char* extension,
                                 const char* event_prefix);

  /* --- appended ------------------------------------------------------------
   */

  /** A role colour from the application palette; no widget involved. */
  uint32_t (*theme_color_role)(GFHostContextRef ctx, int role);
} GFHostUiApi;

/**
 * @brief What the user is currently looking at. Capability "editor".
 *
 * Separate from `ui` because it is a different permission in substance: one is
 * "draw something", the other is "read the document open in front of the
 * user", which may be plaintext they just decrypted.
 */
typedef struct GFHostEditorApi {
  size_t struct_size;
  /** The current tab's exact octets, module views flushed back first. Owned;
   *  release with buffer->release. NULL when no text tab is open. */
  GFBufferRef (*take_current_content)(GFHostContextRef ctx);

  /* --- appended ------------------------------------------------------------
   */

  /** What the current document is -- id, type, title, path, modified -- as a
   *  CBOR map. Never its content. -1 when no document is open. */
  int (*current_document)(GFHostContextRef ctx, GFBufferRef* out_cbor);
} GFHostEditorApi;

/**
 * @brief Settings, caches and shared runtime state. Capability "storage".
 *
 * The three cache families became one set of three calls taking a
 * @ref GFStorageStore, because they differed in where a value lives and in
 * nothing else.
 */
typedef struct GFHostStorageApi {
  size_t struct_size;

  /** Retired: always NULL. Settings are read and written through
   *  setting_get/set/remove, scoped to the module's own group. */
  void* (*settings_root)(GFHostContextRef ctx);

  /** @p out owned on success. Returns 0 on a hit, negative when absent. */
  int (*cache_get)(GFHostContextRef ctx, int store, const char* key,
                   GFBufferRef* out);
  /** @p ttl_seconds <= 0 means no expiry. */
  int (*cache_set)(GFHostContextRef ctx, int store, const char* key,
                   GFBufferView value, int64_t ttl_seconds);
  int (*cache_remove)(GFHostContextRef ctx, int store, const char* key);

  /* The cross-module runtime register table.
   *
   * Text and bool are kept apart rather than funnelled through one call with
   * an encoding, because the table really does store typed values and a
   * lookup of the wrong type is a miss, not a conversion. What DID move
   * module-side is the convenience the old names were mostly about: these
   * report absence, and "or a default" is arithmetic the SDK wrapper does. */
  int (*state_get_text)(GFHostContextRef ctx, const char* ns, const char* key,
                        GFBufferRef* out);
  int (*state_set_text)(GFHostContextRef ctx, const char* ns, const char* key,
                        GFBufferView value);
  int (*state_get_bool)(GFHostContextRef ctx, const char* ns, const char* key,
                        int* out);
  int (*state_set_bool)(GFHostContextRef ctx, const char* ns, const char* key,
                        int value);
  /** Direct children of ns/key, as a string list. */
  int (*state_list_children)(GFHostContextRef ctx, const char* ns,
                             const char* key, GFStringListRef* out);

  /* --- appended ------------------------------------------------------------
   */

  /** One setting, as a CBOR value. @p scope is GF_SETTING_*: the module's
   *  own group, or a Host setting on the allowlist. -1 when absent. */
  int (*setting_get)(GFHostContextRef ctx, int scope, const char* key,
                     GFBufferRef* out_cbor);
  int (*setting_set)(GFHostContextRef ctx, int scope, const char* key,
                     GFBufferView cbor);
  int (*setting_remove)(GFHostContextRef ctx, int scope, const char* key);
} GFHostStorageApi;

/**
 * @brief Running an external program. Capability "process".
 *
 * One call, because the single-command entry point was the batch of one. It is
 * its own group and not part of `app` because executing a program is the most
 * consequential thing in this header.
 */
typedef struct GFHostProcessApi {
  size_t struct_size;
  int (*execute)(GFHostContextRef ctx, GFCommandExecuteContext** contexts,
                 size_t count);
} GFHostProcessApi;

/**
 * @brief Commands. Always present.
 *
 * Present for every module because a command carries its own requirement:
 * the registry checks the CALLER's grant against the capabilities each
 * command declares, which is finer than withholding the whole group. See
 * GFSDKCommand.h for the ownership rules, which every member follows.
 */
typedef struct GFHostCommandApi {
  size_t struct_size;

  int (*register_command)(GFHostContextRef ctx, const GFCommandSpec* spec);
  int (*unregister_command)(GFHostContextRef ctx, const char* id);
  int (*invoke)(GFHostContextRef ctx, const char* id, uint32_t flags,
                GFBufferRef args_cbor, GFBufferRef* blobs, size_t blob_count,
                GFCommandDoneFn done, void* user, uint64_t* out_call_id);
  int (*complete)(GFHostContextRef ctx, uint64_t call_id, int status,
                  GFBufferRef result_cbor, GFBufferRef* blobs,
                  size_t blob_count, const char* error);
  int (*cancel)(GFHostContextRef ctx, uint64_t call_id);
  int (*is_cancelled)(GFHostContextRef ctx, uint64_t call_id);
  int (*describe)(GFHostContextRef ctx, const char* id, GFBufferRef* out);
  int (*list)(GFHostContextRef ctx, const char* prefix, GFStringListRef* out);
  int (*query_state)(GFHostContextRef ctx, const char* id, uint32_t* out_bits);
} GFHostCommandApi;

/**
 * @brief The module's UI scripts. Capability "ui".
 *
 * A script is text, loaded into a sandboxed Lua state the Host keeps for the
 * module; see GFSDKUI.h. The runtime loads a module's embedded scripts by
 * itself, so a module rarely calls this directly.
 */
typedef struct GFHostScriptApi {
  size_t struct_size;
  int (*load)(GFHostContextRef ctx, const char* chunk_name,
              GFBufferView source);
} GFHostScriptApi;

/**
 * @brief Module-owned widgets in Host containers. Capability "ui.custom".
 *
 * The module -> Host half of the typed native widget interfaces. Every call
 * names an instance the calling module owns and is refused for any other,
 * or for one of another kind. See GFSDKTypes.h for the Host -> module half.
 */
typedef struct GFHostNativeWidgetApi {
  size_t struct_size;
  int (*register_widget)(GFHostContextRef ctx, const GFNativeWidgetSpec* spec);
  int (*unregister_widget)(GFHostContextRef ctx, const char* name);

  /* documents */
  int (*document_modified)(GFHostContextRef ctx, uint64_t instance);
  int (*document_show_source)(GFHostContextRef ctx, uint64_t instance,
                              int source);
  int (*document_request_crypto)(GFHostContextRef ctx, uint64_t instance,
                                 uint32_t op);
  int (*document_ops_changed)(GFHostContextRef ctx, uint64_t instance);

  /* settings pages */
  int (*settings_restart_needed)(GFHostContextRef ctx, uint64_t instance,
                                 int level);

  /* dialogs */
  int (*dialog_close)(GFHostContextRef ctx, uint64_t instance);
} GFHostNativeWidgetApi;

/* --- the table ----------------------------------------------------------- */

/**
 * @brief Everything one module may do, minted for that module alone.
 *
 * Handed to `activate()` and valid for the module's lifetime. A grantable
 * group the module did not declare, or was refused, is NULL.
 */
typedef struct GFHostApi {
  size_t struct_size;    /**< sizeof as the HOST compiled it */
  uint32_t abi_version;  /**< the host's ABI generation */
  uint32_t granted;      /**< GF_HOST_CAP_* bits, for diagnostics and tests */
  const char* module_id; /**< who this table was minted for; borrowed */
  GFHostContextRef context; /**< pass to every call below */

  /* always granted */
  const GFHostBufferApi* buffer;
  const GFHostLogApi* log;
  const GFHostAppApi* app;
  const GFHostEventApi* event;
  const GFHostBootstrapApi* bootstrap;
  const GFHostListApi* list;

  /* NULL unless the signed manifest declared the matching capability */
  const GFHostGpgApi* gpg;
  const GFHostPgpApi* pgp;
  const GFHostUiApi* ui;
  const GFHostEditorApi* editor;
  const GFHostStorageApi* storage;
  const GFHostProcessApi* process;

  /* --- appended; check struct_size before reading (GF_SDK_REQUIRE does) --- */

  /* always present: each command states the capabilities it needs */
  const GFHostCommandApi* command;

  /* NULL unless "ui" was granted */
  const GFHostScriptApi* script;

  /* NULL unless "ui.custom" was granted */
  const GFHostNativeWidgetApi* native;
} GFHostApi;

#ifdef __cplusplus
}
#endif

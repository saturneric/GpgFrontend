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

/**
 * @file GFSDKTypes.h
 * @brief Every type that crosses the module boundary, in one place.
 *
 * Types only: no function declarations and no `GFHostApi`. That is what makes
 * the layering above it acyclic.
 *
 *   GFSDKTypes.h      types
 *     GFSDKHostApi.h    the primitive tables, built out of those types
 *       GFSDKContext.h    what a module passes back on every call
 *         GFSDKGpg.h etc.   the public SDK, which takes a context
 *
 * It replaces GFSDKBasicModel.h, GFSDKUIModel.h and GFSDKModuleModel.h, which
 * were three files split by which header happened to need them first rather
 * than by anything about the types.
 *
 * GROWTH. Every struct here that a module and the host both compile begins
 * with `struct_size`, written by whichever side compiled it, and grows by
 * APPENDING only. Reordering or repurposing a field makes an already-built
 * module read the wrong slot with no diagnostic anywhere.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* --- octets -------------------------------------------------------------- */

/** Opaque owning handle. Exactly one GFBufferRelease per handle. */
typedef struct GFBufferImpl* GFBufferRef;

/** Opaque borrowed view. Cannot be released; valid only while its owner is. */
typedef const struct GFBufferImpl* GFBufferView;

/** Maximum length for SDK string operations (32 MiB). */
#define kGfStrlenMax ((int32_t)(1024 * 1024 * 32))

/** Which allocator a call should use. */
typedef enum GFMemoryArena {
  GF_ARENA_NORMAL = 0,
  GF_ARENA_SECURE = 1, /**< wiped on free */
} GFMemoryArena;

/**
 * @brief How loud a message is.
 *
 * Passed across the ABI as a plain `int`: a C enum's underlying type is
 * implementation-defined, and this SDK passes every enumerated argument as
 * `int`. The enum names the values; it is not itself an ABI type.
 */
typedef enum GFLogSeverity {
  GF_LOG_TRACE = 0,
  GF_LOG_DEBUG = 1,
  GF_LOG_INFO = 2,
  GF_LOG_WARN = 3,
  GF_LOG_ERROR = 4,
} GFLogSeverity;

/* --- crypto results ------------------------------------------------------ */

/** One crypto operation's outcome. Release with GFGpgResultRelease. */
typedef struct GFGpgResultImpl* GFGpgResultRef;

/**
 * @brief Status of the operation a result describes.
 *
 * The middle case is the one the old struct-based API got wrong by accident:
 * the result was allocated BEFORE the operation could fail, so a non-zero
 * return did not mean there was nothing to reclaim.
 */
typedef enum {
  GF_GPG_OK = 0,          /**< succeeded; data, where applicable, is present */
  GF_GPG_OP_FAILED = 1,   /**< the operation failed; the result explains why */
  GF_GPG_BAD_REQUEST = 2, /**< arguments were unusable; nothing was attempted */
} GFGpgResultStatus;

/**
 * @brief Success, in the gpg error space.
 *
 * Spelled here rather than taken from <gpg-error.h> so a module needs no
 * GnuPG headers to talk to the SDK. The value is fixed by the gpg error
 * encoding, and the rPGP engine reports through the same space.
 */
#define GF_GPG_ERR_NO_ERROR 0U

/** Which of a result's text fields to read. */
typedef enum GFGpgResultTextField {
  GF_GPG_RESULT_TEXT_CAPSULE_ID = 0,
  GF_GPG_RESULT_TEXT_ERROR_STRING = 1,
  GF_GPG_RESULT_TEXT_HASH_ALGO = 2,
} GFGpgResultTextField;

/** Which operation an analysis is describing. */
typedef enum GFGpgAnalyseOperation {
  GF_GPG_ANALYSE_ENCRYPT = 0,
  GF_GPG_ANALYSE_SIGN = 1,
  GF_GPG_ANALYSE_DECRYPT = 2,
  GF_GPG_ANALYSE_VERIFY = 3,
} GFGpgAnalyseOperation;

/** Which parts of an analysis a caller wants produced. */
#define GF_GPG_ANALYSE_WANT_REPORT (1u << 0)
#define GF_GPG_ANALYSE_WANT_CARDS (1u << 1)
#define GF_GPG_ANALYSE_WANT_INFO_JSON (1u << 2)

/**
 * @brief Everything the host can say about one finished crypto operation.
 *
 * Replaces nine entry points whose signatures were identical and which
 * differed only in the operation they named and in whether they also produced
 * the JSON. Every member is OWNED and may be NULL; free each with
 * GFMemFree(ctx, GF_ARENA_NORMAL, p).
 */
typedef struct GFGpgAnalysis {
  size_t struct_size;
  char* report;    /**< prose, for a person */
  char* cards;     /**< structured InfoBoard cards, JSON */
  char* info_json; /**< the full structured result, JSON */
} GFGpgAnalysis;

/* --- collections ---------------------------------------------------------
 *
 * Three distinct handle types on purpose. Funnelling unrelated collections
 * through one `void*` would make it compile to hand a key-brief list to a
 * recipient accessor, and would buy nothing: one teardown per OWNING object
 * is the goal, not one teardown for every object in the SDK.
 */

typedef struct GFGpgKeyBriefListImpl* GFGpgKeyBriefListRef;
typedef struct GFGpgRecipientListImpl* GFGpgRecipientListRef;
typedef struct GFStringListImpl* GFStringListRef;

/**
 * @brief One key from a key-brief list, borrowed.
 *
 * Replaces eleven accessors that differed only in which field they named. A
 * row says the same thing once, keeps C type checking, and grows by
 * appending, whereas a twelfth field used to mean a twelfth entry point.
 *
 * Every pointer is BORROWED and dies with the owning list. Never free one.
 */
typedef struct GFGpgKeyBriefRow {
  size_t struct_size;
  const char* fingerprint;
  const char* key_id;
  const char* uid;           /**< primary UID, "Name (Comment) <email>" */
  const char* matched_email; /**< the UID email that matched the query */
  int64_t expires_at;        /**< seconds since the epoch; 0 means never */
  /** Mirrors GpgFrontend::GpgKeyStatus:
   *  0 ok, 1 expiring soon, 2 expired, 3 revoked, 4 disabled. */
  int usability;
  int can_encrypt;
  int can_sign;
  /** Identity-binding facts, not usability ones. */
  int matched_uid_is_primary;
  int matched_uid_revoked;
} GFGpgKeyBriefRow;

/**
 * @brief One recipient of an encrypted message, borrowed.
 *
 * @ref key_id is what the message itself names, which is the only thing that
 * decides whether it can be opened.
 */
typedef struct GFGpgRecipientRow {
  size_t struct_size;
  const char* key_id; /**< 8-byte key id (v3 PKESK) or fingerprint (v6) */
  const char* pub_algo;
  const char* fingerprint; /**< of the key this resolved to; "" if none */
  const char* uid;         /**< primary UID of that key; "" if none */
  int key_found;
  int has_secret; /**< the only field answering "can this be opened here" */
  int hidden;     /**< sender used --hidden-recipient */
} GFGpgRecipientRow;

/* --- user interface ------------------------------------------------------ */

/**
 * @brief Factory for a QObject-derived GUI object.
 *
 * @param data user data forwarded from the call that registered the factory
 * @return the new QObject, or NULL on failure
 */
typedef void* (*QObjectFactory)(void* data);

/**
 * @brief A color's MEANING, resolved against a widget's palette by the host.
 *
 * Asking for a role rather than a value is what keeps a module's panel
 * looking like part of the application under both themes. Two conventions are
 * worth knowing: a negative state is de-emphasised rather than painted red,
 * and danger red is reserved for what cannot be undone, or for secrets about
 * to travel in the clear.
 */
typedef enum GFUIColorRole {
  GF_UI_COLOR_MUTED_TEXT = 0,
  GF_UI_COLOR_BORDER = 1,
  GF_UI_COLOR_WARNING = 2,
  GF_UI_COLOR_DANGER = 3,
  GF_UI_COLOR_ACCENT_POSITIVE = 4,
  GF_UI_COLOR_ACCENT_NEGATIVE = 5,
} GFUIColorRole;

/**
 * @brief A settings page a module contributes to the Settings dialog.
 *
 * A spec struct rather than six positional arguments, so a later field costs
 * an append here instead of a new entry point.
 *
 * @ref title and @ref keywords must be UNTRANSLATED source strings; the host
 * translates them in the "GTrC" context each time the dialog is built, so a
 * language change is picked up without re-registering.
 */
typedef struct GFUISettingsPageSpec {
  size_t struct_size;
  const char* page_id;    /**< unique, module-namespaced */
  const char* section_id; /**< "application" | "keys_engines" | "features"
                           *   | "system"; anything else makes its own */
  const char* title;
  const char* keywords;   /**< '\n'-separated, may be NULL */
  QObjectFactory factory; /**< invoked per dialog, on the GUI thread */
  void* data;             /**< handed to @ref factory every time; must outlive
                           *   the module's registration */
} GFUISettingsPageSpec;

/**
 * @brief A module-owned primary view for one tab type.
 *
 * The host still owns the page and its document; this supplies the widget
 * shown on top of it, with the plain editor reachable as "Raw Source".
 */
typedef struct GFUITabViewSpec {
  size_t struct_size;
  const char* tab_type;   /**< as in `EDIT_TAB_TYPE_<TYPE>_OP_*`, case-folded */
  QObjectFactory factory; /**< invoked per tab, on the GUI thread */
  void* data;
} GFUITabViewSpec;

/* --- storage ------------------------------------------------------------- */

/**
 * @brief Which store a key/value call addresses.
 *
 * They differ in how long a value lives and whether it is wiped, which is a
 * property of the store and not of the call.
 */
typedef enum GFStorageStore {
  GF_STORE_SESSION = 0,        /**< in memory, this run only */
  GF_STORE_DURABLE = 1,        /**< on disk, encrypted with the profile */
  GF_STORE_SECURE_DURABLE = 2, /**< on disk, flushed at once; for secrets */
} GFStorageStore;

/** Where a setting key is looked up. */
typedef enum GFSettingScope {
  GF_SETTING_MODULE = 0, /**< the module's own group; private to it */
  GF_SETTING_HOST = 1,   /**< a Host setting the Host shares, by full name */
} GFSettingScope;

/* --- events -------------------------------------------------------------- */

/** One named parameter of an event. */
typedef struct GFModuleEventParam {
  const char* name;
  const char* value;
  struct GFModuleEventParam* next; /**< NULL at end of list */
} GFModuleEventParam;

/** One event dispatched to a module. */
typedef struct GFModuleEvent {
  const char* id;         /**< unique event identifier, UPPER-CASE */
  const char* trigger_id; /**< identifies which trigger to answer */
  GFModuleEventParam* params;
} GFModuleEvent;

/**
 * @brief One module's reply to one event.
 *
 * The module used to build a whole GFModuleEvent for a reply, most of which
 * the host discarded. This carries only what the host reads.
 *
 * @ref params is TRANSFERRED: the host frees the list and every string in it,
 * on the delivered and the no-such-trigger paths alike. Nodes and names must
 * come from the normal arena and values from the secure arena, because a
 * value may be a secret.
 */
typedef struct GFModuleEventAnswer {
  size_t struct_size;
  const char* event_id;       /**< borrowed */
  const char* trigger_id;     /**< borrowed */
  GFModuleEventParam* params; /**< transferred */
} GFModuleEventAnswer;

/* --- external programs --------------------------------------------------- */

/**
 * @brief Called after a command finishes.
 *
 * @param data the context's user pointer
 * @param errcode the process exit code
 * @param out standard output, NUL-terminated
 * @param err standard error, NUL-terminated
 */
typedef void (*GFCommandExecuteCallback)(void* data, int errcode,
                                         const char* out, const char* err);

/** One command in a batch. Borrowed for the duration of the call. */
typedef struct {
  char* cmd; /**< command path or name */
  int32_t argc;
  char** argv; /**< argument array of length @ref argc */
  GFCommandExecuteCallback cb;
  void* data; /**< user context; the caller frees it */
} GFCommandExecuteContext;

/* --- translations -------------------------------------------------------- */

/* --- native widgets (see GFSDKUI.h) ------------------------------------- */

/** Which typed interface a native widget implements. */
typedef enum GFNativeWidgetKind {
  GF_NATIVE_DOCUMENT = 1, /**< the view for a document type */
  GF_NATIVE_SETTINGS = 2, /**< a page in the Settings dialog */
  GF_NATIVE_DIALOG = 3,   /**< a dialog the module opens */
} GFNativeWidgetKind;

/** The crypto operations a document may say apply to it, as bits. */
#define GF_CRYPTO_OP_ENCRYPT (1u << 0)
#define GF_CRYPTO_OP_DECRYPT (1u << 1)
#define GF_CRYPTO_OP_SIGN (1u << 2)
#define GF_CRYPTO_OP_VERIFY (1u << 3)
#define GF_CRYPTO_OP_ENCRYPT_SIGN (1u << 4)
#define GF_CRYPTO_OP_DECRYPT_VERIFY (1u << 5)

/**
 * @brief A document view: what the Host asks of it.
 *
 * Every call is on the GUI thread and names the instance; `user` is the
 * spec's. Buffers handed in are borrowed; buffers returned are the Host's.
 * Append-only.
 */
typedef struct GFDocumentWidgetOps {
  size_t struct_size;
  /** The document's bytes changed underneath the view; show them. */
  void (*load)(void* user, uint64_t instance, GFBufferView bytes);
  /** The document as the view now has it, or NULL for "unchanged". */
  GFBufferRef (*save)(void* user, uint64_t instance);
  /** 1 when the view holds edits the document does not have yet. */
  int (*is_dirty)(void* user, uint64_t instance);
  /** GF_CRYPTO_OP_* bits in @p out; 0 when the view has an opinion. */
  int (*crypto_ops)(void* user, uint64_t instance, uint32_t* out);
  /** A file name to offer when saving, or NULL. */
  GFBufferRef (*suggested_file_name)(void* user, uint64_t instance);
  /** A verification finished; @p json is the Host's result document. */
  void (*apply_verification)(void* user, uint64_t instance, GFBufferView json);
  /** Text to add to the body; 1 when taken. */
  int (*append_text)(void* user, uint64_t instance, GFBufferView utf8);
  /** A public key to attach; 1 when taken. */
  int (*attach_public_key)(void* user, uint64_t instance, GFBufferView key,
                           const char* name);
  /** The editor font the user chose. */
  void (*apply_font)(void* user, uint64_t instance, const char* family,
                     int point_size);
  /** Forget everything decrypted or parsed: the tab is closing. */
  void (*wipe_content)(void* user, uint64_t instance);
  /**
   * The Host is about to write @p bytes to a file. The view may check and
   * normalise them -- a mail's line endings, say -- and may ask the user.
   * @return 0 and the bytes to write in @p out (NULL: write @p bytes as
   *         they are), or 1 when the user cancelled
   */
  int (*prepare_save)(void* user, uint64_t instance, GFBufferView bytes,
                      GFBufferRef* out);
} GFDocumentWidgetOps;

/** A settings page: what the Host asks of it. Append-only. */
typedef struct GFSettingsWidgetOps {
  size_t struct_size;
  void (*load)(void* user, uint64_t instance);  /**< show stored values */
  int (*apply)(void* user, uint64_t instance);  /**< store them; 0 on success */
} GFSettingsWidgetOps;

/** A dialog: what the Host asks of it. Append-only. */
typedef struct GFDialogWidgetOps {
  size_t struct_size;
  /** It is being shown; @p args_cbor is what org.gpgfrontend.view.open had. */
  void (*opened)(void* user, uint64_t instance, GFBufferView args_cbor);
  /** The user is closing it; 1 to allow, 0 to keep it open. */
  int (*close_requested)(void* user, uint64_t instance);
} GFDialogWidgetOps;

/**
 * @brief A native widget, registered once, mounted by the module's script.
 *
 * The module creates the widget; the Host puts it in a container of its own
 * and never hands the module anything back but the instance number. Exactly
 * one ops table matches `kind`; the others are NULL.
 */
typedef struct GFNativeWidgetSpec {
  size_t struct_size;
  const char* name;  /**< the module's own name for it; "<module id>." is added */
  int kind;          /**< GFNativeWidgetKind */
  int multi_instance; /**< 1: a factory, one widget per mount instance */

  /* presentation, untranslated source strings in the "GTrC" context */
  const char* title;
  const char* keywords; /**< comma-separated */
  const char* suffix;   /**< documents: default file suffix, no dot */
  const char* filter;   /**< documents: file dialog filter */
  const char* icon;     /**< ":/..." */
  int width;
  int height;

  void* user;
  /** Build the widget for @p instance; a QWidget*, owned by the Host after. */
  void* (*create)(void* user, uint64_t instance, GFBufferView args_cbor);
  /** @p instance is gone; the module forgets it. */
  void (*destroyed)(void* user, uint64_t instance);

  const GFDocumentWidgetOps* document;
  const GFSettingsWidgetOps* settings;
  const GFDialogWidgetOps* dialog;
} GFNativeWidgetSpec;

/* --- commands (see GFSDKCommand.h) ------------------------------------- */

/* --- status ---------------------------------------------------------------- */

#define GF_CMD_OK 0
#define GF_CMD_E_UNKNOWN (-1)     /**< no command with that id */
#define GF_CMD_E_DENIED (-2)      /**< caller may not invoke or register it */
#define GF_CMD_E_BAD_ARGS (-3)    /**< arguments do not match the schema */
#define GF_CMD_E_DISABLED (-4)    /**< exists, but not enabled right now */
#define GF_CMD_E_UNAVAILABLE (-5) /**< its provider went away */
#define GF_CMD_E_CANCELLED (-6)   /**< cancelled before it completed */
#define GF_CMD_E_FAILED (-7)      /**< ran, and failed */

/* --- state ----------------------------------------------------------------- */

#define GF_CMD_STATE_ENABLED 0x1u
#define GF_CMD_STATE_VISIBLE 0x2u
#define GF_CMD_STATE_CHECKED 0x4u

/* --- callbacks ------------------------------------------------------------- */

/**
 * @brief A command finished. Called exactly once per successful invoke,
 *        unless the call was cancelled first; never after cancel returns.
 *
 * @p result_cbor and every element of @p blobs are the callee's to release.
 * @p error is borrowed for the duration of the call, and NULL on success.
 */
typedef void (*GFCommandDoneFn)(void* user, uint64_t call_id, int status,
                                GFBufferRef result_cbor, GFBufferRef* blobs,
                                size_t blob_count, const char* error);

/**
 * @brief Run a command this module provides.
 *
 * @p context_cbor describes who asked and in what situation; borrowed for the
 * call. @p args_cbor and every blob are the handler's to release. Complete the
 * call exactly once with GFCommandComplete, now or later, from any thread.
 */
typedef void (*GFCommandHandlerFn)(void* user, uint64_t call_id,
                                   GFBufferView context_cbor,
                                   GFBufferRef args_cbor, GFBufferRef* blobs,
                                   size_t blob_count);

/**
 * @brief Whether a command this module provides is enabled right now.
 *
 * Called on the GUI thread, often -- before every menu is shown -- so it must
 * be cheap. Returns GF_CMD_STATE_* bits.
 */
typedef uint32_t (*GFCommandStateFn)(void* user, GFBufferView context_cbor);

/** One command a module provides. Borrowed for the registering call. */
typedef struct GFCommandSpec {
  size_t struct_size;
  /** "<module_id>.<name>"; must also be listed in the signed manifest. */
  const char* id;
  /** CBOR descriptor: title, category, required capabilities, schemas. */
  GFBufferView descriptor_cbor;
  GFCommandHandlerFn handler;
  GFCommandStateFn state; /**< NULL: always enabled and visible */
  void* user;
} GFCommandSpec;

/**
 * @brief Supplies translation data for a locale.
 *
 * @param locale BCP-47 locale string, e.g. "en_US"
 * @param data set to a caller-owned buffer holding the raw data
 * @return 0 on success
 */
typedef int (*GFTranslatorDataReader)(const char* locale, char** data);

#ifdef __cplusplus
}
#endif

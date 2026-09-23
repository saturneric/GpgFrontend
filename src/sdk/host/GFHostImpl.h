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

#include <QString>

#include "GFSDKTypes.h"

namespace GpgFrontend {
class GFBuffer;
}  // namespace GpgFrontend

/**
 * @file GFHostImpl.h
 * @brief The host's own operations, behind the capability gate.
 *
 * ## Why these are not the public SDK
 *
 * They used to be. Every one of these was a `GF_SDK_EXPORT` symbol that a
 * module linked and called directly, which meant the capability table a
 * module was handed was advisory: a module refused `gpg` could call
 * `GFGpgDecrypt` by name and get it.
 *
 * Now they are ordinary C++ functions in a namespace, visible only inside the
 * host component. The public names live module-side, in `gf_sdk`, and reach
 * these only through a `GFHostApi` primitive that has already checked the
 * caller's context. There is exactly one host execution path.
 *
 * Nothing here takes a context: by the time one of these runs, authorization
 * has happened. Keeping the check in one layer rather than two is what stops
 * the two from disagreeing.
 *
 * ## What may include what
 *
 * The implementations behind this header are the ONLY code allowed to include
 * `core/` and `ui/` headers. Anything above the primitive boundary that
 * wanted to would be reaching around the gate.
 */

namespace gf_host {

/* --- octets and memory --------------------------------------------------- */

auto GFBufferNewFromBytes(const void* data, size_t size) -> GFBufferRef;
auto GFBufferData(GFBufferView buf) -> const void*;
auto GFBufferSize(GFBufferView buf) -> size_t;
void GFBufferZeroize(GFBufferRef buf);
void GFBufferRelease(GFBufferRef buf);
auto GFBufferOutstandingCount(const char* module_id) -> size_t;

auto GFAllocateMemory(uint32_t size) -> void*;
auto GFReallocateMemory(void* ptr, uint32_t size) -> void*;
void GFFreeMemory(void* ptr);
auto GFSecAllocateMemory(uint32_t size) -> void*;
auto GFSecReallocateMemory(void* ptr, uint32_t size) -> void*;
void GFSecFreeMemory(void* ptr);
auto GFModuleStrDup(const char* src) -> char*;
auto GFModuleSecStrDup(const char* src) -> char*;

/* --- logging ------------------------------------------------------------- */

void GFModuleLogAt(const char* module_id, int severity, const char* file,
                   int line, const char* function, const char* msg);
auto GFModuleLogEnabled(const char* module_id, int severity) -> int;

/* --- application facts --------------------------------------------------- */

auto GFProjectVersion() -> const char*;
auto GFProjectGitCommitHash() -> const char*;
auto GFQtEnvVersion() -> const char*;
auto GFHttpRequestUserAgent() -> const char*;
auto GFAppActiveLocale() -> char*;
auto GFIsFlatpakENV() -> int;
auto GFAppKeyProtectionLevel() -> int;
auto GFAppRegisterTranslatorReader(const char* id, GFTranslatorDataReader r)
    -> int;

/* --- events -------------------------------------------------------------- */

void GFModuleListenEvent(const char* module_id, const char* event_id);
void GFModuleTriggerModuleEventCallback(GFModuleEvent* event,
                                        const char* module_id,
                                        GFModuleEventParam* argv);

/* --- gpg operations and results ------------------------------------------ */

auto GFGpgCurrentGpgContextChannel() -> int;

auto GFGpgSign(int channel, const char* const* key_ids, size_t key_ids_size,
               GFBufferView in, int sign_mode, int ascii, GFGpgResultRef* out)
    -> int;
auto GFGpgEncrypt(int channel, const char* const* key_ids, size_t key_ids_size,
                  GFBufferView in, int ascii, GFGpgResultRef* out) -> int;
auto GFGpgDecrypt(int channel, GFBufferView in, GFGpgResultRef* out) -> int;
auto GFGpgVerify(int channel, GFBufferView in, GFBufferView signature,
                 GFGpgResultRef* out) -> int;

auto GFGpgResultStatusOf(GFGpgResultRef r) -> int;
auto GFGpgResultError(GFGpgResultRef r) -> uint32_t;
auto GFGpgResultData(GFGpgResultRef r) -> GFBufferView;
auto GFGpgResultCapsuleId(GFGpgResultRef r) -> const char*;
auto GFGpgResultErrorString(GFGpgResultRef r) -> const char*;
auto GFGpgResultHashAlgo(GFGpgResultRef r) -> const char*;
auto GFGpgResultTakeData(GFGpgResultRef r) -> GFBufferRef;
void GFGpgResultRelease(GFGpgResultRef r);
auto GFGpgResultOutstandingCount(const char* module_id) -> size_t;

auto GFAnalyseEncryptResultInfoByCapsule(int channel, uint32_t err,
                                         const char* capsule_id,
                                         const char** analyse,
                                         const char** cards,
                                         const char** info_json) -> int;
auto GFAnalyseSignResultInfoByCapsule(int channel, uint32_t err,
                                      const char* capsule_id,
                                      const char** analyse, const char** cards,
                                      const char** info_json) -> int;
auto GFAnalyseDecryptResultInfoByCapsule(int channel, uint32_t err,
                                         const char* capsule_id,
                                         const char** analyse,
                                         const char** cards,
                                         const char** info_json) -> int;
auto GFAnalyseVerifyResultInfoByCapsule(int channel, uint32_t err,
                                        const char* capsule_id,
                                        const char** analyse,
                                        const char** cards,
                                        const char** info_json) -> int;

/* --- keys ---------------------------------------------------------------- */

auto GFGpgPublicKey(int channel, const char* key_id, int ascii) -> GFBufferRef;
auto GFGpgExportKey(int channel, const char* key_id, int ascii,
                    GFBufferRef* out) -> int;
auto GFGpgImportKeys(int channel, void* parent, const char* data, int size)
    -> int;

/** The three parts of the primary UID, each owned by the caller and freed
 *  with GFFreeMemory. The struct this used to fill in is gone. */
auto GFGpgKeyPrimaryUidParts(int channel, const char* key_id, char** name,
                             char** email, char** comment) -> int;

/* --- collections --------------------------------------------------------- */

auto GFGpgFindKeys(int channel, const char* email, GFGpgKeyBriefListRef* out)
    -> int;
auto GFGpgKeyBriefListCount(GFGpgKeyBriefListRef l) -> size_t;
void GFGpgKeyBriefListRelease(GFGpgKeyBriefListRef l);

auto GFGpgSniffRecipients(int channel, GFBufferView in,
                          GFGpgRecipientListRef* out) -> int;
auto GFGpgRecipientListCount(GFGpgRecipientListRef l) -> size_t;
void GFGpgRecipientListRelease(GFGpgRecipientListRef l);

auto GFGpgListAddresses(int channel, int secret_only, GFStringListRef* out)
    -> int;
auto GFStringListCount(GFStringListRef l) -> size_t;
auto GFStringListAt(GFStringListRef l, size_t i) -> const char*;
void GFStringListRelease(GFStringListRef l);
auto GFGpgListOutstandingCount(const char* module_id) -> size_t;

/* --- openpgp structure --------------------------------------------------- */

auto GFPgpInspectData(GFBufferView in, char** out_json) -> int;

/* --- user interface ------------------------------------------------------ */

auto GFUIDefaultUserFilePath() -> char*;
auto GFUITakeCurrentEditorContent() -> GFBufferRef;
/// A role colour from the application palette, or 0 for an unknown role.
auto GFUIPaletteColor(int role) -> uint32_t;

/* --- storage ------------------------------------------------------------- */

/// The three module cache stores, scoped by module and store. @p module_id
/// must come from the caller's context. An empty value reads as absent.
auto GFModuleCacheGet(const QString& module_id, int store, const QString& key,
                      GpgFrontend::GFBuffer* out) -> bool;
auto GFModuleCacheSet(const QString& module_id, int store, const QString& key,
                      const GpgFrontend::GFBuffer& value, int64_t ttl_seconds)
    -> bool;
auto GFModuleCacheRemove(const QString& module_id, int store,
                         const QString& key) -> bool;

/* --- external programs --------------------------------------------------- */

void GFExecuteCommandBatchSync(GFCommandExecuteContext** contexts,
                               int32_t contexts_size);

}  // namespace gf_host

/**
 * @brief Hand gf_core the entry points it needs, once, at startup.
 *
 * Called from the application before any module can load. Not exported: the
 * caller is in the same binary. See GFHostBridgeInstall.cpp for why this is a
 * call rather than a load-time initializer.
 */
void GFHostApiInstallBridge();

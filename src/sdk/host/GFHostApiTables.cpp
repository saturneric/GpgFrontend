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

#include <QByteArray>
#include <QCborValue>
#include <QString>
#include <cstring>

#include "GFHostImpl.h"
#include "GFSDKBuildInfo.h"
#include "GFSDKHostApi.h"
#include "GFSDKModuleApi.h"
#include "core/function/GlobalSettingStation.h"
#include "core/module/ModuleManager.h"
#include "core/module/ModuleSettingsPolicy.h"
#include "ui/UIModuleManager.h"
#include "private/GFHostContext.h"
#include "private/GFHostGate.h"
#include "private/GFSDKGpgInternal.h"
#include "private/GFSDKPrivate.h"

/**
 * @file GFHostApiTables.cpp
 * @brief The host's side of the module ABI: the capability groups.
 *
 * Every function here is a thunk. It does three things and no more:
 *
 *   1. checks the caller's context against the capability the group needs,
 *      and counts the call as running until it returns,
 *   2. attributes this thread to that module, so handles it creates are
 *      recorded against it wherever it created them,
 *   3. forwards to the real implementation.
 *
 * The forwarding is deliberately dumb. "What a module may call" and "what the
 * SDK implements" are then the same list read two ways, and a new entry point
 * that is not wired here is simply unreachable from a module -- a visible
 * omission rather than a silent inconsistency.
 *
 * What is NOT here is as important. There are no convenience entry points, no
 * "or a default" variants, no formatting and no per-field accessors: those are
 * implemented module-side, in gf_sdk (src/sdk/api), over these primitives. That
 * is what lets the public SDK grow without this file changing.
 */

// Every primitive below forwards to one of these. They are the host's own
// operations, and this file is the only thing that may call them: the gate
// happens here, once, and nothing downstream re-checks it.
using namespace gf_host;  // NOLINT(build/namespaces)

namespace {

using gf_sdk_internal::BeginCall;
using gf_sdk_internal::CallTicket;
using gf_sdk_internal::EndCall;
using gf_sdk_internal::ScopedContextAttribution;

/// The GF_HOST_CAP_* values and ModuleCapability must agree; they are written
/// down in two places because gf_core must not include the SDK's headers.
static_assert(GF_HOST_CAP_GPG == 1U << 0, "capability bits moved");
static_assert(GF_HOST_CAP_PGP == 1U << 1, "capability bits moved");
static_assert(GF_HOST_CAP_UI == 1U << 2, "capability bits moved");
static_assert(GF_HOST_CAP_EDITOR == 1U << 3, "capability bits moved");
static_assert(GF_HOST_CAP_STORAGE == 1U << 4, "capability bits moved");
static_assert(GF_HOST_CAP_PROCESS == 1U << 5, "capability bits moved");
static_assert(GF_HOST_CAP_UI_CUSTOM == 1U << 6, "capability bits moved");

auto ModuleIdOf(GFHostContextRef ctx) -> QByteArray {
  return gf_sdk_internal::ContextModuleId(ctx).toUtf8();
}

/* --- buffer -------------------------------------------------------------- */

auto BufNewFromBytes(GFHostContextRef ctx, const void* data, size_t size)
    -> GFBufferRef {
  GATE(ctx, 0, "buffer.new_from_bytes", nullptr);
  return GFBufferNewFromBytes(data, size);
}

auto BufData(GFHostContextRef ctx, GFBufferView buf) -> const void* {
  GATE(ctx, 0, "buffer.data", nullptr);
  return GFBufferData(buf);
}

auto BufSize(GFHostContextRef ctx, GFBufferView buf) -> size_t {
  GATE(ctx, 0, "buffer.size", 0);
  return GFBufferSize(buf);
}

void BufZeroize(GFHostContextRef ctx, GFBufferRef buf) {
  GATE_VOID(ctx, 0, "buffer.zeroize");
  GFBufferZeroize(buf);
}

void BufRelease(GFHostContextRef ctx, GFBufferRef buf) {
  GATE_VOID(ctx, 0, "buffer.release");
  GFBufferRelease(buf);
}

auto BufOutstanding(GFHostContextRef ctx) -> size_t {
  GATE(ctx, 0, "buffer.outstanding_count", 0);
  const auto id = ModuleIdOf(ctx);
  return GFBufferOutstandingCount(id.constData());
}

auto MemAlloc(GFHostContextRef ctx, int arena, uint32_t size) -> void* {
  GATE(ctx, 0, "buffer.mem_alloc", nullptr);
  return arena == GF_ARENA_SECURE ? GFSecAllocateMemory(size)
                                  : GFAllocateMemory(size);
}

auto MemRealloc(GFHostContextRef ctx, int arena, void* p, uint32_t size)
    -> void* {
  GATE(ctx, 0, "buffer.mem_realloc", nullptr);
  return arena == GF_ARENA_SECURE ? GFSecReallocateMemory(p, size)
                                  : GFReallocateMemory(p, size);
}

void MemFree(GFHostContextRef ctx, int arena, void* p) {
  GATE_VOID(ctx, 0, "buffer.mem_free");
  if (arena == GF_ARENA_SECURE) {
    GFSecFreeMemory(p);
  } else {
    GFFreeMemory(p);
  }
}

auto MemStrDup(GFHostContextRef ctx, int arena, const char* s) -> char* {
  GATE(ctx, 0, "buffer.mem_strdup", nullptr);
  return arena == GF_ARENA_SECURE ? GFModuleSecStrDup(s) : GFModuleStrDup(s);
}

const GFHostBufferApi kBufferApi = {
    sizeof(GFHostBufferApi),
    &BufNewFromBytes,
    &BufData,
    &BufSize,
    &BufZeroize,
    &BufRelease,
    &BufOutstanding,
    &MemAlloc,
    &MemRealloc,
    &MemFree,
    &MemStrDup,
};

/* --- log ----------------------------------------------------------------- */

void LogWrite(GFHostContextRef ctx, int severity, const char* file, int line,
              const char* function, const char* msg) {
  GATE_VOID(ctx, 0, "log.write");
  const auto id = ModuleIdOf(ctx);
  GFModuleLogAt(id.constData(), severity, file, line, function, msg);
}

auto LogEnabled(GFHostContextRef ctx, int severity) -> int {
  GATE(ctx, 0, "log.enabled", 0);
  const auto id = ModuleIdOf(ctx);
  return GFModuleLogEnabled(id.constData(), severity);
}

const GFHostLogApi kLogApi = {sizeof(GFHostLogApi), &LogWrite, &LogEnabled};

/* --- app ----------------------------------------------------------------- */

auto AppVersion(GFHostContextRef ctx) -> const char* {
  GATE(ctx, 0, "app.version", nullptr);
  return GFProjectVersion();
}

auto AppGitCommitHash(GFHostContextRef ctx) -> const char* {
  GATE(ctx, 0, "app.git_commit_hash", nullptr);
  return GFProjectGitCommitHash();
}

auto AppQtEnvVersion(GFHostContextRef ctx) -> const char* {
  GATE(ctx, 0, "app.qt_env_version", nullptr);
  return GFQtEnvVersion();
}

auto AppUserAgent(GFHostContextRef ctx) -> const char* {
  GATE(ctx, 0, "app.http_user_agent", nullptr);
  return GFHttpRequestUserAgent();
}

auto AppActiveLocale(GFHostContextRef ctx) -> char* {
  GATE(ctx, 0, "app.active_locale", nullptr);
  return GFAppActiveLocale();
}

auto AppIsFlatpak(GFHostContextRef ctx) -> int {
  GATE(ctx, 0, "app.is_flatpak", 0);
  return GFIsFlatpakENV();
}

auto AppKeyProtectionLevel(GFHostContextRef ctx) -> int {
  GATE(ctx, 0, "app.key_protection_level", -1);
  return GFAppKeyProtectionLevel();
}

const GFHostAppApi kAppApi = {
    sizeof(GFHostAppApi), &AppVersion,
    &AppGitCommitHash,    &AppQtEnvVersion,
    &AppUserAgent,        &AppActiveLocale,
    &AppIsFlatpak,        &AppKeyProtectionLevel,
};

/* --- event --------------------------------------------------------------- */

auto EventSubscribe(GFHostContextRef ctx, const char* event_id) -> int {
  GATE(ctx, 0, "event.subscribe", -1);
  const auto id = ModuleIdOf(ctx);
  if (id.isEmpty() || event_id == nullptr) return -1;
  // The module id comes from the context, not from the caller: a module
  // subscribing on another module's behalf is not a thing that should be
  // expressible, and it used to be an argument.
  GFModuleListenEvent(id.constData(), event_id);
  return 0;
}

auto EventAnswer(GFHostContextRef ctx, const GFModuleEventAnswer* answer)
    -> int {
  GATE(ctx, 0, "event.answer", -1);
  if (answer == nullptr) return -1;

  // The host's reply path still wants a GFModuleEvent, and frees it. Building
  // it here rather than in the module means the module never has to know the
  // shape of a structure it only ever fills in to be thrown away.
  auto* reply =
      static_cast<GFModuleEvent*>(GFAllocateMemory(sizeof(GFModuleEvent)));
  if (reply == nullptr) return -1;
  reply->id =
      GFModuleStrDup(answer->event_id == nullptr ? "" : answer->event_id);
  reply->trigger_id =
      GFModuleStrDup(answer->trigger_id == nullptr ? "" : answer->trigger_id);
  reply->params = nullptr;

  const auto id = ModuleIdOf(ctx);
  GFModuleTriggerModuleEventCallback(reply, id.constData(), answer->params);
  return 0;
}

const GFHostEventApi kEventApi = {sizeof(GFHostEventApi), &EventSubscribe,
                                  &EventAnswer};

/* --- bootstrap ----------------------------------------------------------- */

auto BootRegisterTranslatorReader(GFHostContextRef ctx, const char* id,
                                  GFTranslatorDataReader reader) -> int {
  GATE(ctx, 0, "bootstrap.register_translator_reader", -1);
  // The module id comes from the context, as in EventSubscribe: a module must
  // not be able to replace another module's translations. @p id stays in the
  // signature for the ABI and is only checked.
  const auto module_id = ModuleIdOf(ctx);
  if (module_id.isEmpty() || reader == nullptr) return -1;
  if (id != nullptr && module_id != QByteArray(id)) {
    LOG_W() << "bootstrap.register_translator_reader: module" << module_id
            << "tried to register translations for" << id;
    return -1;
  }
  return GFAppRegisterTranslatorReader(module_id.constData(), reader);
}

const GFHostBootstrapApi kBootstrapApi = {sizeof(GFHostBootstrapApi),
                                          &BootRegisterTranslatorReader};

/* --- gpg ----------------------------------------------------------------- */

auto GpgCurrentChannel(GFHostContextRef ctx) -> int {
  GATE(ctx, GF_HOST_CAP_GPG, "gpg.current_channel", -1);
  return GFGpgCurrentGpgContextChannel();
}

auto GpgSign(GFHostContextRef ctx, int channel, const char* const* key_ids,
             size_t key_ids_size, GFBufferView in, int sign_mode, int ascii,
             GFGpgResultRef* out) -> int {
  GATE(ctx, GF_HOST_CAP_GPG, "gpg.sign", -1);
  return GFGpgSign(channel, key_ids, key_ids_size, in, sign_mode, ascii, out);
}

auto GpgEncrypt(GFHostContextRef ctx, int channel, const char* const* key_ids,
                size_t key_ids_size, GFBufferView in, int ascii,
                GFGpgResultRef* out) -> int {
  GATE(ctx, GF_HOST_CAP_GPG, "gpg.encrypt", -1);
  return GFGpgEncrypt(channel, key_ids, key_ids_size, in, ascii, out);
}

auto GpgDecrypt(GFHostContextRef ctx, int channel, GFBufferView in,
                GFGpgResultRef* out) -> int {
  GATE(ctx, GF_HOST_CAP_GPG, "gpg.decrypt", -1);
  return GFGpgDecrypt(channel, in, out);
}

auto GpgVerify(GFHostContextRef ctx, int channel, GFBufferView in,
               GFBufferView signature, GFGpgResultRef* out) -> int {
  GATE(ctx, GF_HOST_CAP_GPG, "gpg.verify", -1);
  return GFGpgVerify(channel, in, signature, out);
}

auto GpgResultStatus(GFHostContextRef ctx, GFGpgResultRef r) -> int {
  GATE(ctx, GF_HOST_CAP_GPG, "gpg.result_status", GF_GPG_BAD_REQUEST);
  return GFGpgResultStatusOf(r);
}

auto GpgResultError(GFHostContextRef ctx, GFGpgResultRef r) -> uint32_t {
  GATE(ctx, GF_HOST_CAP_GPG, "gpg.result_error", 0U);
  return GFGpgResultError(r);
}

auto GpgResultData(GFHostContextRef ctx, GFGpgResultRef r) -> GFBufferView {
  GATE(ctx, GF_HOST_CAP_GPG, "gpg.result_data", nullptr);
  return GFGpgResultData(r);
}

auto GpgResultText(GFHostContextRef ctx, GFGpgResultRef r, int field) -> const
    char* {
  // "" rather than NULL on every failure path: the header promises a string.
  GATE(ctx, GF_HOST_CAP_GPG, "gpg.result_text", "");
  switch (field) {
    case GF_GPG_RESULT_TEXT_CAPSULE_ID:
      return GFGpgResultCapsuleId(r);
    case GF_GPG_RESULT_TEXT_ERROR_STRING:
      return GFGpgResultErrorString(r);
    case GF_GPG_RESULT_TEXT_HASH_ALGO:
      return GFGpgResultHashAlgo(r);
    default:
      LOG_W() << "gpg.result_text: unknown field" << field;
      return "";
  }
}

auto GpgResultTakeData(GFHostContextRef ctx, GFGpgResultRef r) -> GFBufferRef {
  GATE(ctx, GF_HOST_CAP_GPG, "gpg.result_take_data", nullptr);
  return GFGpgResultTakeData(r);
}

void GpgResultRelease(GFHostContextRef ctx, GFGpgResultRef r) {
  GATE_VOID(ctx, GF_HOST_CAP_GPG, "gpg.result_release");
  GFGpgResultRelease(r);
}

auto GpgResultOutstanding(GFHostContextRef ctx) -> size_t {
  GATE(ctx, GF_HOST_CAP_GPG, "gpg.result_outstanding_count", 0);
  const auto id = ModuleIdOf(ctx);
  return GFGpgResultOutstandingCount(id.constData());
}

auto GpgAnalyseResult(GFHostContextRef ctx, int channel, int operation,
                      uint32_t err, const char* capsule_id, uint32_t want,
                      GFGpgAnalysis* out) -> int {
  GATE(ctx, GF_HOST_CAP_GPG, "gpg.analyse_result", -1);
  if (out == nullptr) return -1;

  const char* report = nullptr;
  const char* cards = nullptr;
  const char* info_json = nullptr;

  // A null out-parameter is how the underlying calls are told to skip
  // producing something, so `want` becomes three conditional addresses rather
  // than a branch per combination.
  const char** report_slot =
      (want & GF_GPG_ANALYSE_WANT_REPORT) != 0 ? &report : nullptr;
  const char** cards_slot =
      (want & GF_GPG_ANALYSE_WANT_CARDS) != 0 ? &cards : nullptr;
  const char** info_slot =
      (want & GF_GPG_ANALYSE_WANT_INFO_JSON) != 0 ? &info_json : nullptr;

  // The report is the one output the underlying calls always produce, so ask
  // for it even when the caller did not and discard it below. Passing null
  // there is not part of their contract.
  const char* discard = nullptr;
  if (report_slot == nullptr) report_slot = &discard;

  int status = -1;
  switch (operation) {
    case GF_GPG_ANALYSE_ENCRYPT:
      status = GFAnalyseEncryptResultInfoByCapsule(
          channel, err, capsule_id, report_slot, cards_slot, info_slot);
      break;
    case GF_GPG_ANALYSE_SIGN:
      status = GFAnalyseSignResultInfoByCapsule(
          channel, err, capsule_id, report_slot, cards_slot, info_slot);
      break;
    case GF_GPG_ANALYSE_DECRYPT:
      status = GFAnalyseDecryptResultInfoByCapsule(
          channel, err, capsule_id, report_slot, cards_slot, info_slot);
      break;
    case GF_GPG_ANALYSE_VERIFY:
      status = GFAnalyseVerifyResultInfoByCapsule(
          channel, err, capsule_id, report_slot, cards_slot, info_slot);
      break;
    default:
      LOG_W() << "gpg.analyse_result: unknown operation" << operation;
      return -1;
  }

  if (discard != nullptr) GFFreeMemory(const_cast<char*>(discard));

  out->struct_size = sizeof(GFGpgAnalysis);
  out->report = const_cast<char*>(report);
  out->cards = const_cast<char*>(cards);
  out->info_json = const_cast<char*>(info_json);
  return status;
}

auto GpgPublicKey(GFHostContextRef ctx, int channel, const char* key_id,
                  int ascii) -> GFBufferRef {
  GATE(ctx, GF_HOST_CAP_GPG, "gpg.public_key", nullptr);
  // Handed over as a buffer, like every other owned payload in this ABI: one
  // release, counted in the ledger, wiped when it dies.
  return GFGpgPublicKey(channel, key_id, ascii);
}

auto GpgKeyPrimaryUid(GFHostContextRef ctx, int channel, const char* key_id,
                      GFBufferRef* name, GFBufferRef* email,
                      GFBufferRef* comment) -> int {
  GATE(ctx, GF_HOST_CAP_GPG, "gpg.key_primary_uid", -1);

  char* n = nullptr;
  char* e = nullptr;
  char* c = nullptr;
  const auto rc = GFGpgKeyPrimaryUidParts(
      channel, key_id, name == nullptr ? nullptr : &n,
      email == nullptr ? nullptr : &e, comment == nullptr ? nullptr : &c);
  if (rc != 0) return rc;

  const auto adopt = [](char* text, GFBufferRef* out) {
    if (out == nullptr) return;
    *out = text == nullptr ? GFBufferNewFromBytes("", 0)
                           : GFBufferNewFromBytes(text, strlen(text));
    GFFreeMemory(text);
  };
  adopt(n, name);
  adopt(e, email);
  adopt(c, comment);
  return 0;
}

auto GpgExportKey(GFHostContextRef ctx, int channel, const char* key_id,
                  int ascii, GFBufferRef* out) -> int {
  GATE(ctx, GF_HOST_CAP_GPG, "gpg.export_key", -1);
  return GFGpgExportKey(channel, key_id, ascii, out);
}

auto GpgImportKeys(GFHostContextRef ctx, int channel, void* parent,
                   GFBufferView data) -> int {
  GATE(ctx, GF_HOST_CAP_GPG, "gpg.import_keys", -1);
  return GFGpgImportKeys(channel, parent,
                         static_cast<const char*>(GFBufferData(data)),
                         static_cast<int>(GFBufferSize(data)));
}

auto GpgFindKeys(GFHostContextRef ctx, int channel, const char* email,
                 GFGpgKeyBriefListRef* out) -> int {
  GATE(ctx, GF_HOST_CAP_GPG, "gpg.find_keys", -1);
  return GFGpgFindKeys(channel, email, out);
}

auto GpgKeyBriefCount(GFHostContextRef ctx, GFGpgKeyBriefListRef l) -> size_t {
  GATE(ctx, GF_HOST_CAP_GPG, "gpg.key_brief_count", 0);
  return GFGpgKeyBriefListCount(l);
}

auto GpgKeyBriefAt(GFHostContextRef ctx, GFGpgKeyBriefListRef l, size_t i)
    -> const GFGpgKeyBriefRow* {
  GATE(ctx, GF_HOST_CAP_GPG, "gpg.key_brief_at", nullptr);
  return gf_sdk_internal::KeyBriefRowAt(l, i);
}

void GpgKeyBriefRelease(GFHostContextRef ctx, GFGpgKeyBriefListRef l) {
  GATE_VOID(ctx, GF_HOST_CAP_GPG, "gpg.key_brief_release");
  GFGpgKeyBriefListRelease(l);
}

auto GpgSniffRecipients(GFHostContextRef ctx, int channel, GFBufferView in,
                        GFGpgRecipientListRef* out) -> int {
  GATE(ctx, GF_HOST_CAP_GPG, "gpg.sniff_recipients", -1);
  return GFGpgSniffRecipients(channel, in, out);
}

auto GpgRecipientCount(GFHostContextRef ctx, GFGpgRecipientListRef l)
    -> size_t {
  GATE(ctx, GF_HOST_CAP_GPG, "gpg.recipient_count", 0);
  return GFGpgRecipientListCount(l);
}

auto GpgRecipientAt(GFHostContextRef ctx, GFGpgRecipientListRef l, size_t i)
    -> const GFGpgRecipientRow* {
  GATE(ctx, GF_HOST_CAP_GPG, "gpg.recipient_at", nullptr);
  return gf_sdk_internal::RecipientRowAt(l, i);
}

void GpgRecipientRelease(GFHostContextRef ctx, GFGpgRecipientListRef l) {
  GATE_VOID(ctx, GF_HOST_CAP_GPG, "gpg.recipient_release");
  GFGpgRecipientListRelease(l);
}

auto GpgListAddresses(GFHostContextRef ctx, int channel, int secret_only,
                      GFStringListRef* out) -> int {
  GATE(ctx, GF_HOST_CAP_GPG, "gpg.list_addresses", -1);
  return GFGpgListAddresses(channel, secret_only, out);
}

const GFHostGpgApi kGpgApi = {
    sizeof(GFHostGpgApi),  &GpgCurrentChannel,   &GpgSign,
    &GpgEncrypt,           &GpgDecrypt,          &GpgVerify,
    &GpgResultStatus,      &GpgResultError,      &GpgResultData,
    &GpgResultText,        &GpgResultTakeData,   &GpgResultRelease,
    &GpgResultOutstanding, &GpgAnalyseResult,    &GpgPublicKey,
    &GpgKeyPrimaryUid,     &GpgExportKey,        &GpgImportKeys,
    &GpgFindKeys,          &GpgKeyBriefCount,    &GpgKeyBriefAt,
    &GpgKeyBriefRelease,   &GpgSniffRecipients,  &GpgRecipientCount,
    &GpgRecipientAt,       &GpgRecipientRelease, &GpgListAddresses,
};

/* --- list ---------------------------------------------------------------- */

auto ListStringCount(GFHostContextRef ctx, GFStringListRef l) -> size_t {
  GATE(ctx, 0, "list.string_count", 0);
  return GFStringListCount(l);
}

auto ListStringAt(GFHostContextRef ctx, GFStringListRef l, size_t i) -> const
    char* {
  GATE(ctx, 0, "list.string_at", nullptr);
  return GFStringListAt(l, i);
}

void ListStringRelease(GFHostContextRef ctx, GFStringListRef l) {
  GATE_VOID(ctx, 0, "list.string_release");
  GFStringListRelease(l);
}

auto ListOutstanding(GFHostContextRef ctx) -> size_t {
  GATE(ctx, 0, "list.outstanding_count", 0);
  const auto id = ModuleIdOf(ctx);
  return GFGpgListOutstandingCount(id.constData());
}

const GFHostListApi kListApi = {sizeof(GFHostListApi), &ListStringCount,
                                &ListStringAt, &ListStringRelease,
                                &ListOutstanding};

/* --- pgp ----------------------------------------------------------------- */

auto PgpInspect(GFHostContextRef ctx, GFBufferView in, GFBufferRef* out)
    -> int {
  GATE(ctx, GF_HOST_CAP_PGP, "pgp.inspect", -1);
  if (out == nullptr) return -1;
  *out = nullptr;

  // The reference case for the whole arrangement: the public SDK function is
  // module-side and calls this; this is the only code that may name
  // GpgFrontend::InspectOpenPGPData, and it does so one layer down.
  char* json = nullptr;
  const auto rc = GFPgpInspectData(in, &json);
  if (rc != 0 || json == nullptr) return rc == 0 ? -1 : rc;

  *out = GFBufferNewFromBytes(json, strlen(json));
  GFFreeMemory(json);
  return *out == nullptr ? -1 : 0;
}

const GFHostPgpApi kPgpApi = {sizeof(GFHostPgpApi), &PgpInspect};

/* --- ui ------------------------------------------------------------------ */

auto UiCreateObject(GFHostContextRef ctx, QObjectFactory factory, void* data)
    -> void* {
  GATE(ctx, GF_HOST_CAP_UI, "ui.create_object", nullptr);
  return GFUICreateGUIObject(factory, data);
}

auto UiGetObject(GFHostContextRef ctx, const char* id) -> void* {
  GATE(ctx, GF_HOST_CAP_UI, "ui.get_object", nullptr);
  return GFUIGetGUIObject(id);
}

auto UiShowDialog(GFHostContextRef ctx, void* dialog, void* parent) -> int {
  GATE(ctx, GF_HOST_CAP_UI, "ui.show_dialog", -1);
  return GFUIShowDialog(dialog, parent);
}

auto UiThemeColor(GFHostContextRef ctx, int role, void* widget) -> uint32_t {
  GATE(ctx, GF_HOST_CAP_UI, "ui.theme_color", 0U);
  switch (role) {
    case GF_UI_COLOR_MUTED_TEXT:
      return GFUIMutedTextColor(widget);
    case GF_UI_COLOR_BORDER:
      return GFUIBorderColor(widget);
    case GF_UI_COLOR_WARNING:
      return GFUIWarningColor(widget);
    case GF_UI_COLOR_DANGER:
      return GFUIDangerColor(widget);
    case GF_UI_COLOR_ACCENT_POSITIVE:
      return GFUIAccentColor(widget, 1);
    case GF_UI_COLOR_ACCENT_NEGATIVE:
      return GFUIAccentColor(widget, 0);
    default:
      LOG_W() << "ui.theme_color: unknown role" << role;
      return 0U;
  }
}

auto UiThemeColorRole(GFHostContextRef ctx, int role) -> uint32_t {
  GATE(ctx, GF_HOST_CAP_UI, "ui.theme_color_role", 0U);
  return GFUIPaletteColor(role);
}

auto UiUserFilePath(GFHostContextRef ctx) -> GFBufferRef {
  GATE(ctx, GF_HOST_CAP_UI, "ui.user_file_path", nullptr);
  auto* path = GFUIDefaultUserFilePath();
  if (path == nullptr) return nullptr;
  auto* buf = GFBufferNewFromBytes(path, strlen(path));
  GFFreeMemory(path);
  return buf;
}

auto UiRegisterSettingsPage(GFHostContextRef ctx,
                            const GFUISettingsPageSpec* spec) -> int {
  GATE(ctx, GF_HOST_CAP_UI, "ui.register_settings_page", -1);
  if (spec == nullptr ||
      spec->struct_size <
          offsetof(GFUISettingsPageSpec, data) + sizeof(void*)) {
    return -1;
  }
  return GFUIRegisterSettingsPage(spec->page_id, spec->section_id, spec->title,
                                  spec->keywords, spec->factory, spec->data);
}

auto UiUnregisterSettingsPage(GFHostContextRef ctx, const char* page_id)
    -> int {
  GATE(ctx, GF_HOST_CAP_UI, "ui.unregister_settings_page", -1);
  return GFUIUnregisterSettingsPage(page_id);
}

auto UiRegisterTabView(GFHostContextRef ctx, const GFUITabViewSpec* spec)
    -> int {
  GATE(ctx, GF_HOST_CAP_UI, "ui.register_tab_view", -1);
  if (spec == nullptr ||
      spec->struct_size < offsetof(GFUITabViewSpec, data) + sizeof(void*)) {
    return -1;
  }
  return GFUIRegisterTabPageView(spec->tab_type, spec->factory, spec->data);
}

auto UiUnregisterTabView(GFHostContextRef ctx, const char* tab_type) -> int {
  GATE(ctx, GF_HOST_CAP_UI, "ui.unregister_tab_view", -1);
  return GFUIUnregisterTabPageView(tab_type);
}

auto UiRegisterFileExtension(GFHostContextRef ctx, const char* extension,
                             const char* event_prefix) -> int {
  GATE(ctx, GF_HOST_CAP_UI, "ui.register_file_extension", -1);
  return GFUIRegisterFileExtensionHandleEvent(extension, event_prefix);
}

const GFHostUiApi kUiApi = {
    sizeof(GFHostUiApi),
    &UiCreateObject,
    &UiGetObject,
    &UiShowDialog,
    &UiThemeColor,
    &UiUserFilePath,
    &UiRegisterSettingsPage,
    &UiUnregisterSettingsPage,
    &UiRegisterTabView,
    &UiUnregisterTabView,
    &UiRegisterFileExtension,
    &UiThemeColorRole,
};

/* --- editor -------------------------------------------------------------- */

auto EditorTakeCurrentContent(GFHostContextRef ctx) -> GFBufferRef {
  GATE(ctx, GF_HOST_CAP_EDITOR, "editor.take_current_content", nullptr);
  return GFUITakeCurrentEditorContent();
}

auto EditorCurrentDocument(GFHostContextRef ctx, GFBufferRef* out) -> int {
  GATE(ctx, GF_HOST_CAP_EDITOR, "editor.current_document", -1);
  if (out == nullptr) return -1;
  *out = nullptr;
  const auto info = GpgFrontend::UI::CurrentDocumentInfo();
  if (!info.has_value()) return -1;
  const auto bytes = QCborValue(*info).toCbor();
  *out = GFBufferNewFromBytes(bytes.constData(),
                              static_cast<size_t>(bytes.size()));
  return *out == nullptr ? -1 : 0;
}

const GFHostEditorApi kEditorApi = {sizeof(GFHostEditorApi),
                                    &EditorTakeCurrentContent,
                                    &EditorCurrentDocument};

/* --- storage ------------------------------------------------------------- */

auto StorageSettingsRoot(GFHostContextRef ctx) -> void* {
  GATE(ctx, GF_HOST_CAP_STORAGE, "storage.settings_root", nullptr);
  return GFUIGlobalSettings();
}

auto StorageCacheGet(GFHostContextRef ctx, int store, const char* key,
                     GFBufferRef* out) -> int {
  GATE(ctx, GF_HOST_CAP_STORAGE, "storage.cache_get", -1);
  if (out == nullptr || key == nullptr) return -1;
  *out = nullptr;

  // Octets in, octets out: the value never passes through a C string or a
  // QString, so an embedded NUL or invalid UTF-8 survives, and a secret goes
  // from one wiping buffer to another without an ordinary copy in between.
  GpgFrontend::GFBuffer value;
  if (!GFModuleCacheGet(gf_sdk_internal::ContextModuleId(ctx), store,
                        QString::fromUtf8(key), &value)) {
    return -1;
  }

  *out = GFBufferNewFromBytes(value.Data(), value.Size());
  return *out == nullptr ? -1 : 0;
}

auto StorageCacheSet(GFHostContextRef ctx, int store, const char* key,
                     GFBufferView value, int64_t ttl_seconds) -> int {
  GATE(ctx, GF_HOST_CAP_STORAGE, "storage.cache_set", -1);
  if (key == nullptr) return -1;

  const auto* data = static_cast<const char*>(GFBufferData(value));
  const auto size = GFBufferSize(value);
  GpgFrontend::GFBuffer bytes(size);
  if (size > 0 && data != nullptr) memcpy(bytes.Data(), data, size);

  if (store != GF_STORE_SESSION && store != GF_STORE_DURABLE &&
      store != GF_STORE_SECURE_DURABLE) {
    LOG_W() << "storage.cache_set: unknown store" << store;
    return -1;
  }
  return GFModuleCacheSet(gf_sdk_internal::ContextModuleId(ctx), store,
                          QString::fromUtf8(key), bytes, ttl_seconds)
             ? 0
             : -1;
}

auto StorageCacheRemove(GFHostContextRef ctx, int store, const char* key)
    -> int {
  GATE(ctx, GF_HOST_CAP_STORAGE, "storage.cache_remove", -1);
  if (key == nullptr) return -1;
  return GFModuleCacheRemove(gf_sdk_internal::ContextModuleId(ctx), store,
                             QString::fromUtf8(key))
             ? 0
             : -1;
}

auto StorageStateGetText(GFHostContextRef ctx, const char* ns, const char* key,
                         GFBufferRef* out) -> int {
  GATE(ctx, GF_HOST_CAP_STORAGE, "storage.state_get_text", -1);
  if (out == nullptr || ns == nullptr || key == nullptr) return -1;
  *out = nullptr;

  // std::optional, not "or a default": the register table really can be asked
  // for something it does not hold, and collapsing that into the caller's
  // fallback is what made the old entry points unable to say "absent".
  const auto value = GpgFrontend::Module::RetrieveRTValueTyped<QString>(
      QString::fromUtf8(ns), QString::fromUtf8(key));
  if (!value.has_value()) return -1;

  const auto utf8 = value->toUtf8();
  *out =
      GFBufferNewFromBytes(utf8.constData(), static_cast<size_t>(utf8.size()));
  return *out == nullptr ? -1 : 0;
}

auto StorageStateSetText(GFHostContextRef ctx, const char* ns, const char* key,
                         GFBufferView value) -> int {
  GATE(ctx, GF_HOST_CAP_STORAGE, "storage.state_set_text", -1);
  if (ns == nullptr || key == nullptr) return -1;

  const auto* data = static_cast<const char*>(GFBufferData(value));
  const auto size = GFBufferSize(value);
  const auto text = QString::fromUtf8(data == nullptr ? "" : data,
                                      static_cast<qsizetype>(size));
  return GpgFrontend::Module::UpsertRTValue(QString::fromUtf8(ns).toLower(),
                                            QString::fromUtf8(key).toLower(),
                                            text)
             ? 0
             : -1;
}

auto StorageStateGetBool(GFHostContextRef ctx, const char* ns, const char* key,
                         int* out) -> int {
  GATE(ctx, GF_HOST_CAP_STORAGE, "storage.state_get_bool", -1);
  if (out == nullptr || ns == nullptr || key == nullptr) return -1;

  const auto value = GpgFrontend::Module::RetrieveRTValueTyped<bool>(
      QString::fromUtf8(ns), QString::fromUtf8(key));
  if (!value.has_value()) return -1;
  *out = *value ? 1 : 0;
  return 0;
}

auto StorageStateSetBool(GFHostContextRef ctx, const char* ns, const char* key,
                         int value) -> int {
  GATE(ctx, GF_HOST_CAP_STORAGE, "storage.state_set_bool", -1);
  if (ns == nullptr || key == nullptr) return -1;
  return GpgFrontend::Module::UpsertRTValue(QString::fromUtf8(ns).toLower(),
                                            QString::fromUtf8(key).toLower(),
                                            value != 0)
             ? 0
             : -1;
}

auto StorageStateListChildren(GFHostContextRef ctx, const char* ns,
                              const char* key, GFStringListRef* out) -> int {
  GATE(ctx, GF_HOST_CAP_STORAGE, "storage.state_list_children", -1);
  if (out == nullptr || ns == nullptr || key == nullptr) return -1;
  return gf_sdk_internal::NewStringList(
      GpgFrontend::Module::ListRTChildKeys(QString::fromUtf8(ns).toLower(),
                                           QString::fromUtf8(key).toLower()),
      out);
}

/// The full settings key a module may use, or empty -- which is a refusal,
/// logged once here so the policy's answer is visible where it bites.
auto SettingKey(GFHostContextRef ctx, int scope, const char* key, bool write)
    -> QString {
  if (key == nullptr) return {};
  if (scope != GF_SETTING_MODULE && scope != GF_SETTING_HOST) return {};
  const auto module = gf_sdk_internal::ContextModuleId(ctx);
  const auto full = GpgFrontend::Module::ResolveModuleSettingKey(
      module, static_cast<GpgFrontend::Module::ModuleSettingScope>(scope),
      QString::fromUtf8(key), write);
  if (full.isEmpty()) {
    LOG_W() << "module" << module << "may not" << (write ? "write" : "read")
            << "setting" << key << "in scope" << scope;
  }
  return full;
}

auto StorageSettingGet(GFHostContextRef ctx, int scope, const char* key,
                       GFBufferRef* out) -> int {
  GATE(ctx, GF_HOST_CAP_STORAGE, "storage.setting_get", -1);
  if (out == nullptr) return -1;
  *out = nullptr;
  const auto full = SettingKey(ctx, scope, key, false);
  if (full.isEmpty()) return -1;

  const auto settings = GpgFrontend::GetSettings();
  if (!settings.contains(full)) return -1;
  const auto bytes = QCborValue::fromVariant(settings.value(full)).toCbor();
  *out = GFBufferNewFromBytes(bytes.constData(),
                              static_cast<size_t>(bytes.size()));
  return *out == nullptr ? -1 : 0;
}

auto StorageSettingSet(GFHostContextRef ctx, int scope, const char* key,
                       GFBufferView cbor) -> int {
  GATE(ctx, GF_HOST_CAP_STORAGE, "storage.setting_set", -1);
  const auto full = SettingKey(ctx, scope, key, true);
  if (full.isEmpty()) return -1;

  const auto* data = static_cast<const char*>(GFBufferData(cbor));
  const auto size = GFBufferSize(cbor);
  if (data == nullptr || size == 0) return -1;
  QCborParserError error{};
  const auto value = QCborValue::fromCbor(
      QByteArray(data, static_cast<qsizetype>(size)), &error);
  if (error.error != QCborError::NoError) return -1;

  // A fresh QSettings per call: the object is not thread-safe, and a module
  // calls from its own threads. It writes on destruction, which is here.
  auto settings = GpgFrontend::GetSettings();
  settings.setValue(full, value.toVariant());
  return 0;
}

auto StorageSettingRemove(GFHostContextRef ctx, int scope, const char* key)
    -> int {
  GATE(ctx, GF_HOST_CAP_STORAGE, "storage.setting_remove", -1);
  const auto full = SettingKey(ctx, scope, key, true);
  if (full.isEmpty()) return -1;
  auto settings = GpgFrontend::GetSettings();
  settings.remove(full);
  return 0;
}

const GFHostStorageApi kStorageApi = {
    sizeof(GFHostStorageApi),  &StorageSettingsRoot, &StorageCacheGet,
    &StorageCacheSet,          &StorageCacheRemove,  &StorageStateGetText,
    &StorageStateSetText,      &StorageStateGetBool, &StorageStateSetBool,
    &StorageStateListChildren, &StorageSettingGet,   &StorageSettingSet,
    &StorageSettingRemove,
};

/* --- process ------------------------------------------------------------- */

auto ProcessExecute(GFHostContextRef ctx, GFCommandExecuteContext** contexts,
                    size_t count) -> int {
  GATE(ctx, GF_HOST_CAP_PROCESS, "process.execute", -1);
  if (contexts == nullptr || count == 0) return -1;
  GFExecuteCommandBatchSync(contexts, static_cast<int32_t>(count));
  return 0;
}

const GFHostProcessApi kProcessApi = {sizeof(GFHostProcessApi),
                                      &ProcessExecute};

}  // namespace

namespace gf_sdk_internal {

void FillHostApiGroups(GFHostApi& table, uint32_t granted) {
  // Always granted. Withholding any of these would not restrict a module, it
  // would stop it being one.
  table.buffer = &kBufferApi;
  table.log = &kLogApi;
  table.app = &kAppApi;
  table.event = &kEventApi;
  table.bootstrap = &kBootstrapApi;
  table.list = &kListApi;

  table.gpg = (granted & GF_HOST_CAP_GPG) != 0 ? &kGpgApi : nullptr;
  table.pgp = (granted & GF_HOST_CAP_PGP) != 0 ? &kPgpApi : nullptr;
  table.ui = (granted & GF_HOST_CAP_UI) != 0 ? &kUiApi : nullptr;
  table.editor = (granted & GF_HOST_CAP_EDITOR) != 0 ? &kEditorApi : nullptr;
  table.storage = (granted & GF_HOST_CAP_STORAGE) != 0 ? &kStorageApi : nullptr;
  table.process = (granted & GF_HOST_CAP_PROCESS) != 0 ? &kProcessApi : nullptr;

  // Always present: every command states the capabilities its CALLER needs,
  // and the registry checks them per call.
  table.command = &kCommandApi;
}

}  // namespace gf_sdk_internal

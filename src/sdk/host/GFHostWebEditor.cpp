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

/**
 * @file GFHostWebEditor.cpp
 * @brief Web editors: the consumer's calls and the engine's, checked, and a
 *        module's engine adapted for the service.
 */

#include <QWidget>

#include "GFHostImpl.h"
#include "GFSDKHostApi.h"
#include "core/model/GFBuffer.h"
#include "private/GFHostContext.h"
#include "private/GFHostEnter.h"
#include "private/GFHostGate.h"
#include "private/GFHostTransfer.h"
#include "private/GFHostVersioned.h"
#include "ui/html/WebEditorDocument.h"
#include "ui/html/WebEditorService.h"

namespace {

using gf_sdk_internal::EnterModule;
using GpgFrontend::GFBuffer;
using GpgFrontend::UI::WebEditorEngine;
using GpgFrontend::UI::WebEditorProviderInfo;
using GpgFrontend::UI::WebEditorService;

static_assert(GF_HTML_VIEW_OK == WebEditorService::kOK);
static_assert(GF_HTML_VIEW_E_UNAVAILABLE == WebEditorService::kUNAVAILABLE);
static_assert(GF_HTML_VIEW_E_RESTART_REQUIRED ==
              WebEditorService::kRESTART_REQUIRED);
static_assert(GF_HTML_VIEW_E_DENIED == WebEditorService::kDENIED);
static_assert(GF_HTML_VIEW_E_INVALID == WebEditorService::kINVALID);
static_assert(GF_HTML_VIEW_E_TOO_LARGE == WebEditorService::kTOO_LARGE);
static_assert(GF_HTML_VIEW_E_FAILED == WebEditorService::kFAILED);
static_assert(GF_HTML_VIEW_E_NOT_FOUND == WebEditorService::kNOT_FOUND);
static_assert(GF_WEB_EDITOR_EVENT_READY == WebEditorService::kEVENT_READY);
static_assert(GF_WEB_EDITOR_EVENT_MESSAGE == WebEditorService::kEVENT_MESSAGE);
static_assert(GF_WEB_EDITOR_EVENT_LINK == WebEditorService::kEVENT_LINK);
static_assert(GF_WEB_EDITOR_EVENT_LOST == WebEditorService::kEVENT_LOST);
static_assert(GF_WEB_EDITOR_MAX_MESSAGE_BYTES ==
              GpgFrontend::UI::WebEditor::kMaxMessageBytes);
static_assert(GF_WEB_EDITOR_MAX_RESOURCE_BYTES ==
              GpgFrontend::UI::WebEditor::kMaxResourceBytes);
static_assert(GF_WEB_EDITOR_MAX_TOTAL_RESOURCE_BYTES ==
              GpgFrontend::UI::WebEditor::kMaxTotalResourceBytes);

constexpr uint32_t kEditorCaps =
    GF_HOST_CAP_UI | GF_HOST_CAP_UI_CUSTOM | GF_HOST_CAP_WEB_EDITOR;
constexpr uint32_t kProviderCaps =
    GF_HOST_CAP_UI | GF_HOST_CAP_UI_CUSTOM | GF_HOST_CAP_WEB_EDITOR_PROVIDER;

auto Bytes(const char* json, size_t len) -> QByteArray {
  if (json == nullptr) return {};
  return {json, static_cast<qsizetype>(len)};
}

/// A module's engine, for the service. Every call enters the module through
/// its gate; once the module is withdrawn none does.
class ModuleEngine : public WebEditorEngine {
 public:
  ModuleEngine(QByteArray module, const GFWebEditorProviderOps& ops, void* user)
      : module_(std::move(module)), ops_(ops), user_(user) {}

  auto Create(quint64 editor, const QString& session) -> QWidget* override {
    const auto s = session.toUtf8();
    return gf_sdk_internal::AdoptModuleWidget(
        module_, [&] { return ops_.create(user_, editor, s.constData()); });
  }

  auto Post(quint64 editor, const QByteArray& json) -> bool override {
    bool ok = false;
    EnterModule(module_, [&] {
      ok = ops_.post(user_, editor, json.constData(),
                     static_cast<size_t>(json.size())) == 0;
    });
    return ok;
  }

  void Destroyed(quint64 editor) override {
    if (ops_.destroyed == nullptr) return;
    EnterModule(module_, [&] { ops_.destroyed(user_, editor); });
  }

 private:
  QByteArray module_;
  GFWebEditorProviderOps ops_;
  void* user_;
};

/* --- consumers ----------------------------------------------------------- */

auto Available(GFHostContextRef ctx) -> int {
  GATE(ctx, kEditorCaps, "web_editor.available", GF_HTML_VIEW_E_DENIED);
  return WebEditorService::Instance().Available();
}

auto Create(GFHostContextRef ctx, uint64_t instance, void* slot,
            GFWebEditorEventFn fn, void* user, uint64_t* out_editor) -> int {
  GATE(ctx, kEditorCaps, "web_editor.create", GF_HTML_VIEW_E_DENIED);
  if (out_editor != nullptr) *out_editor = 0;
  const auto module = gf_sdk_internal::ContextModuleId(ctx);
  WebEditorService::EventFn events;
  if (fn != nullptr) {
    const auto m = module.toUtf8();
    events = [m, fn, user](quint64 editor, int event, const QByteArray& text) {
      EnterModule(m, [&] {
        fn(user, editor, event, text.isNull() ? nullptr : text.constData(),
           static_cast<size_t>(text.size()));
      });
    };
  }
  quint64 editor = 0;
  const auto rc = WebEditorService::Instance().Create(
      module, instance, slot, std::move(events), &editor);
  if (out_editor != nullptr) *out_editor = editor;
  return rc;
}

auto Post(GFHostContextRef ctx, uint64_t editor, const char* json, size_t len)
    -> int {
  GATE(ctx, kEditorCaps, "web_editor.post", GF_HTML_VIEW_E_DENIED);
  return WebEditorService::Instance().Post(
      gf_sdk_internal::ContextModuleId(ctx), editor, Bytes(json, len));
}

auto PutResource(GFHostContextRef ctx, uint64_t editor, const char* path,
                 const char* mime, GFBufferRef data) -> int {
  // The Host's from here, whatever happens next.
  auto taken = gf_sdk_internal::TakeIfGiven(data, "web_editor.put_resource");
  GATE(ctx, kEditorCaps, "web_editor.put_resource", GF_HTML_VIEW_E_DENIED);
  if (!taken.has_value() || path == nullptr || mime == nullptr) {
    return GF_HTML_VIEW_E_INVALID;
  }
  return WebEditorService::Instance().PutResource(
      gf_sdk_internal::ContextModuleId(ctx), editor, QString::fromUtf8(path),
      QString::fromUtf8(mime), std::move(*taken));
}

auto RemoveResource(GFHostContextRef ctx, uint64_t editor, const char* path)
    -> int {
  GATE(ctx, kEditorCaps, "web_editor.remove_resource", GF_HTML_VIEW_E_DENIED);
  if (path == nullptr) return GF_HTML_VIEW_E_INVALID;
  return WebEditorService::Instance().RemoveResource(
      gf_sdk_internal::ContextModuleId(ctx), editor, QString::fromUtf8(path));
}

auto Destroy(GFHostContextRef ctx, uint64_t editor) -> int {
  GATE(ctx, kEditorCaps, "web_editor.destroy", GF_HTML_VIEW_E_DENIED);
  return WebEditorService::Instance().Destroy(
      gf_sdk_internal::ContextModuleId(ctx), editor);
}

/* --- engines ------------------------------------------------------------- */

auto RegisterProvider(GFHostContextRef ctx, const GFWebEditorProviderSpec* spec)
    -> int {
  GATE(ctx, kProviderCaps, "web_editor_provider.register_provider",
       GF_HTML_VIEW_E_DENIED);
  if (spec == nullptr || spec->struct_size < sizeof(size_t)) {
    return GF_HTML_VIEW_E_INVALID;
  }
  const auto s = gf_sdk_internal::CopyVersioned(*spec);
  if (s.interface_version != GF_WEB_EDITOR_PROVIDER_V1 || s.ops == nullptr ||
      s.name == nullptr || *s.name == '\0') {
    return GF_HTML_VIEW_E_INVALID;
  }
  const auto ops = gf_sdk_internal::CopyVersioned(*s.ops);
  if (ops.create == nullptr || ops.post == nullptr) {
    return GF_HTML_VIEW_E_INVALID;
  }
  const auto module = gf_sdk_internal::ContextModuleId(ctx);
  WebEditorProviderInfo info;
  info.owner = module;
  info.name = QString::fromUtf8(s.name);
  info.priority = WebEditorService::kModulePriority;
  info.restart_required =
      (s.features & GF_HTML_PROVIDER_FEATURE_RESTART_REQUIRED) != 0;
  info.engine = std::make_shared<ModuleEngine>(module.toUtf8(), ops, s.user);
  return WebEditorService::Instance().RegisterProvider(std::move(info));
}

auto UnregisterProvider(GFHostContextRef ctx, const char* name) -> int {
  GATE(ctx, kProviderCaps, "web_editor_provider.unregister_provider",
       GF_HTML_VIEW_E_DENIED);
  if (name == nullptr) return GF_HTML_VIEW_E_INVALID;
  return WebEditorService::Instance().UnregisterProvider(
      gf_sdk_internal::ContextModuleId(ctx), QString::fromUtf8(name));
}

auto Resolve(GFHostContextRef ctx, uint64_t editor, const char* url,
             GFBufferRef* out_data, GFBufferRef* out_mime) -> int {
  GATE(ctx, kProviderCaps, "web_editor_provider.resolve",
       GF_HTML_VIEW_E_DENIED);
  if (out_data != nullptr) *out_data = nullptr;
  if (out_mime != nullptr) *out_mime = nullptr;
  if (url == nullptr || out_data == nullptr || out_mime == nullptr) {
    return GF_HTML_VIEW_E_INVALID;
  }
  const auto module = gf_sdk_internal::ContextModuleId(ctx);
  GFBuffer data;
  QString mime;
  const auto rc = WebEditorService::Instance().Resolve(
      module, editor, QString::fromUtf8(url), &data, &mime);
  if (rc != WebEditorService::kOK) return rc;
  *out_data = gf_sdk_internal::NewBufferFor(data, module, "web_editor.data");
  *out_mime = gf_sdk_internal::NewBufferFor(GFBuffer(mime.toUtf8()), module,
                                            "web_editor.mime");
  return GF_HTML_VIEW_OK;
}

auto Message(GFHostContextRef ctx, uint64_t editor, const char* json,
             size_t len) -> int {
  GATE(ctx, kProviderCaps, "web_editor_provider.message",
       GF_HTML_VIEW_E_DENIED);
  return WebEditorService::Instance().Message(
      gf_sdk_internal::ContextModuleId(ctx), editor, Bytes(json, len));
}

auto LinkActivated(GFHostContextRef ctx, uint64_t editor, const char* url)
    -> int {
  GATE(ctx, kProviderCaps, "web_editor_provider.link_activated",
       GF_HTML_VIEW_E_DENIED);
  if (url == nullptr) return GF_HTML_VIEW_E_INVALID;
  return WebEditorService::Instance().LinkActivated(
      gf_sdk_internal::ContextModuleId(ctx), editor, QString::fromUtf8(url));
}

auto Ready(GFHostContextRef ctx, uint64_t editor) -> int {
  GATE(ctx, kProviderCaps, "web_editor_provider.ready", GF_HTML_VIEW_E_DENIED);
  return WebEditorService::Instance().Ready(
      gf_sdk_internal::ContextModuleId(ctx), editor);
}

}  // namespace

namespace gf_sdk_internal {

const GFHostWebEditorApi kWebEditorApi = {
    sizeof(GFHostWebEditorApi),
    &Available,
    &Create,
    &Post,
    &PutResource,
    &RemoveResource,
    &Destroy,
};

const GFHostWebEditorProviderApi kWebEditorProviderApi = {
    sizeof(GFHostWebEditorProviderApi),
    &RegisterProvider,
    &UnregisterProvider,
    &Resolve,
    &Message,
    &LinkActivated,
    &Ready,
};

}  // namespace gf_sdk_internal

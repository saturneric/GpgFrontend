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

#include "GFModuleRuntimeWebEditor.h"

#include <QHash>
#include <list>

#include "GFModuleRuntimeBoot.h"
#include "GFSDKBuffer.h"
#include "GFSDKWebEditor.h"
#include "include/GFModule.h"
#include "include/GFModuleNativeWidget.h"
#include "include/GFModuleWebEditor.h"

/**
 * @file GFModuleRuntimeWebEditor.cpp
 * @brief Web editors and their engines, as C callbacks the Host calls. Both
 * maps belong to the GUI thread.
 */

namespace {

using gf::ui::WebEditor;
using gf::ui::WebEditorEngine;
using gf::ui::WebEditorStatus;

auto EditorHandlers() -> QHash<quint64, WebEditor::Handlers>& {
  static QHash<quint64, WebEditor::Handlers> h;
  return h;
}

struct Registration {
  QByteArray name;
  std::unique_ptr<WebEditorEngine> engine;
  GFWebEditorProviderOps ops{};
  GFWebEditorProviderSpec spec{};
};

auto Registrations() -> std::list<Registration>& {
  static std::list<Registration> r;
  return r;
}

void OnEvent(void* /*user*/, uint64_t editor, int event, const char* text,
             size_t len) {
  const auto it = EditorHandlers().constFind(editor);
  if (it == EditorHandlers().constEnd()) return;
  const auto handlers = *it;  // a handler may destroy the editor
  switch (event) {
    case GF_WEB_EDITOR_EVENT_READY:
      if (handlers.ready) handlers.ready();
      break;
    case GF_WEB_EDITOR_EVENT_MESSAGE:
      if (handlers.message && text != nullptr) {
        handlers.message(QByteArray(text, static_cast<qsizetype>(len)));
      }
      break;
    case GF_WEB_EDITOR_EVENT_LINK:
      if (handlers.link && text != nullptr) {
        handlers.link(QString::fromUtf8(text, static_cast<qsizetype>(len)));
      }
      break;
    case GF_WEB_EDITOR_EVENT_LOST:
      EditorHandlers().remove(editor);
      if (handlers.lost) handlers.lost();
      break;
    default:
      break;
  }
}

auto EngineOf(void* user) -> WebEditorEngine* {
  return static_cast<Registration*>(user)->engine.get();
}

auto OpCreate(void* user, uint64_t editor, const char* session) -> void* {
  return EngineOf(user)->Create(editor, QString::fromUtf8(session));
}

auto OpPost(void* user, uint64_t editor, const char* json, size_t len) -> int {
  return EngineOf(user)->Post(editor,
                              QByteArray(json, static_cast<qsizetype>(len)))
             ? 0
             : -1;
}

void OpDestroyed(void* user, uint64_t editor) {
  EngineOf(user)->Destroyed(editor);
}

}  // namespace

namespace gf::ui {

auto WebEditorAvailability() -> WebEditorStatus {
  return static_cast<WebEditorStatus>(
      GFWebEditorAvailable(gf::runtime::SdkContext()));
}

WebEditor::~WebEditor() { Destroy(); }

auto WebEditor::Create(const NativeWidgetBase& owner, QWidget* slot,
                       Handlers handlers) -> WebEditorStatus {
  Destroy();
  uint64_t editor = 0;
  const auto rc = GFWebEditorCreate(gf::runtime::SdkContext(), owner.Instance(),
                                    slot, &OnEvent, nullptr, &editor);
  if (rc != GF_HTML_VIEW_OK) return static_cast<WebEditorStatus>(rc);
  editor_ = editor;
  EditorHandlers().insert(editor_, std::move(handlers));
  return WebEditorStatus::kOK;
}

auto WebEditor::still_live() -> bool {
  if (IsLive() && EditorHandlers().contains(editor_)) return true;
  editor_ = 0;
  return false;
}

auto WebEditor::Post(const QByteArray& json) -> WebEditorStatus {
  if (!still_live()) return WebEditorStatus::kINVALID;
  return static_cast<WebEditorStatus>(
      GFWebEditorPost(gf::runtime::SdkContext(), editor_, json.constData(),
                      static_cast<size_t>(json.size())));
}

auto WebEditor::PutResource(const QString& path, const QString& mime,
                            const QByteArray& data) -> WebEditorStatus {
  if (!still_live()) return WebEditorStatus::kINVALID;
  auto* ctx = gf::runtime::SdkContext();
  auto* ref = GFBufferNewFromBytes(ctx, data.constData(),
                                   static_cast<size_t>(data.size()));
  return static_cast<WebEditorStatus>(GFWebEditorPutResource(
      ctx, editor_, path.toUtf8().constData(), mime.toUtf8().constData(), ref));
}

auto WebEditor::RemoveResource(const QString& path) -> WebEditorStatus {
  if (!still_live()) return WebEditorStatus::kINVALID;
  return static_cast<WebEditorStatus>(GFWebEditorRemoveResource(
      gf::runtime::SdkContext(), editor_, path.toUtf8().constData()));
}

void WebEditor::Destroy() {
  if (!IsLive()) return;
  const auto editor = editor_;
  editor_ = 0;
  if (EditorHandlers().remove(editor) > 0) {
    GFWebEditorDestroy(gf::runtime::SdkContext(), editor);
  }
}

auto WebEditorEngine::Resolve(quint64 editor, const QString& url,
                              QByteArray* data, QString* mime) -> bool {
  auto* ctx = gf::runtime::SdkContext();
  GFBufferRef d = nullptr;
  GFBufferRef m = nullptr;
  const auto rc =
      GFWebEditorProviderResolve(ctx, editor, url.toUtf8().constData(), &d, &m);
  if (rc != GF_HTML_VIEW_OK) return false;
  if (data != nullptr) *data = gf::runtime::ViewBytes(d);
  if (mime != nullptr) *mime = QString::fromUtf8(gf::runtime::ViewBytes(m));
  GFBufferRelease(ctx, d);
  GFBufferRelease(ctx, m);
  return true;
}

void WebEditorEngine::Message(quint64 editor, const QByteArray& json) {
  GFWebEditorProviderMessage(gf::runtime::SdkContext(), editor,
                             json.constData(),
                             static_cast<size_t>(json.size()));
}

void WebEditorEngine::LinkActivated(quint64 editor, const QString& url) {
  GFWebEditorProviderLinkActivated(gf::runtime::SdkContext(), editor,
                                   url.toUtf8().constData());
}

void WebEditorEngine::Ready(quint64 editor) {
  GFWebEditorProviderReady(gf::runtime::SdkContext(), editor);
}

auto RegisterWebEditorEngine(const char* name, uint32_t features,
                             std::unique_ptr<WebEditorEngine> engine) -> bool {
  if (engine == nullptr || name == nullptr) return false;
  auto& reg = Registrations().emplace_back();
  reg.name = name;
  reg.engine = std::move(engine);
  reg.ops.struct_size = sizeof(GFWebEditorProviderOps);
  reg.ops.create = &OpCreate;
  reg.ops.post = &OpPost;
  reg.ops.destroyed = &OpDestroyed;
  auto& s = reg.spec;
  s.struct_size = sizeof(GFWebEditorProviderSpec);
  s.interface_version = GF_WEB_EDITOR_PROVIDER_V1;
  s.name = reg.name.constData();
  s.features = features;
  s.user = &reg;
  s.ops = &reg.ops;
  if (GFWebEditorProviderRegister(gf::runtime::SdkContext(), &s) !=
      GF_HTML_VIEW_OK) {
    LOG_ERROR(QString("the host refused web editor engine %1")
                  .arg(QString::fromUtf8(name)));
    Registrations().pop_back();
    return false;
  }
  return true;
}

}  // namespace gf::ui

namespace gf::runtime {

void ForgetWebEditors() {
  RunOnGuiThread([]() { EditorHandlers().clear(); });
}

void ResetWebEditorEngines() { Registrations().clear(); }

}  // namespace gf::runtime

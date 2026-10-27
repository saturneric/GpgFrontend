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

#include "GFModuleRuntimeHtmlView.h"

#include <QCoreApplication>
#include <QHash>
#include <QThread>
#include <list>

#include "GFModuleRuntimeBoot.h"
#include "GFSDKHtmlView.h"
#include "include/GFModule.h"
#include "include/GFModuleHtmlView.h"

/**
 * @file GFModuleRuntimeHtmlView.cpp
 * @brief HTML views and renderers, as C callbacks the Host calls.
 *
 * Both maps belong to the GUI thread, where every call touching them runs.
 */

namespace {

using gf::ui::HtmlView;
using gf::ui::HtmlViewRenderer;
using gf::ui::HtmlViewStatus;

auto ViewHandlers() -> QHash<quint64, HtmlView::Handlers>& {
  static QHash<quint64, HtmlView::Handlers> h;
  return h;
}

auto PartRequests() -> QHash<quint64, HtmlViewRenderer::PartDone>& {
  static QHash<quint64, HtmlViewRenderer::PartDone> r;
  return r;
}

struct Registration {
  QByteArray name;
  std::unique_ptr<HtmlViewRenderer> renderer;
  GFHtmlViewProviderOps ops{};
  GFHtmlViewProviderSpec spec{};
};

/// Stable addresses: the Host holds `user` pointers into this list.
auto Registrations() -> std::list<Registration>& {
  static std::list<Registration> r;
  return r;
}

auto Bytes(GFBufferView view) -> QByteArray {
  auto* ctx = gf::runtime::SdkContext();
  const auto* data = static_cast<const char*>(GFBufferData(ctx, view));
  return data == nullptr
             ? QByteArray()
             : QByteArray(data, static_cast<int>(GFBufferSize(ctx, view)));
}

// --- consumer events

void OnEvent(void* /*user*/, uint64_t view, int event, int value,
             const char* text) {
  const auto it = ViewHandlers().constFind(view);
  if (it == ViewHandlers().constEnd()) return;
  const auto handlers = *it;  // a handler may destroy the view
  switch (event) {
    case GF_HTML_VIEW_EVENT_LOADED:
      if (handlers.loaded) handlers.loaded(value != 0);
      break;
    case GF_HTML_VIEW_EVENT_LINK:
      if (handlers.link) handlers.link(QString::fromUtf8(text));
      break;
    case GF_HTML_VIEW_EVENT_LOST:
      // The Host has already forgotten it; so does the module.
      ViewHandlers().remove(view);
      if (handlers.lost) handlers.lost();
      break;
    default:
      break;
  }
}

// --- renderer ops

auto RendererOf(void* user) -> HtmlViewRenderer* {
  return static_cast<Registration*>(user)->renderer.get();
}

auto OpCreate(void* user, uint64_t view, const GFHtmlViewPolicy* policy)
    -> void* {
  if (policy == nullptr) return nullptr;
  return RendererOf(user)->Create(view, *policy);
}

auto OpLoad(void* user, uint64_t view, const char* session,
            GFBufferView document) -> int {
  return RendererOf(user)->Load(view, QString::fromUtf8(session),
                                Bytes(document))
             ? 0
             : -1;
}

void OpClear(void* user, uint64_t view) { RendererOf(user)->Clear(view); }

void OpDestroyed(void* user, uint64_t view) {
  RendererOf(user)->Destroyed(view);
}

void OnPartDone(void* /*user*/, uint64_t request, int status, const char* mime,
                GFBufferRef data) {
  auto* ctx = gf::runtime::SdkContext();
  const auto bytes = data == nullptr ? QByteArray() : Bytes(data);
  if (data != nullptr) GFBufferRelease(ctx, data);
  const auto done = PartRequests().take(request);
  if (done) done(status, QString::fromUtf8(mime), bytes);
}

}  // namespace

namespace gf::ui {

auto HtmlViewAvailability() -> HtmlViewStatus {
  return static_cast<HtmlViewStatus>(
      GFHtmlViewAvailable(gf::runtime::SdkContext(), GF_HTML_VIEW_PRESET_MAIL));
}

HtmlView::~HtmlView() { Destroy(); }

auto HtmlView::Create(const NativeWidgetBase& owner, QWidget* slot,
                      Handlers handlers) -> HtmlViewStatus {
  Destroy();
  uint64_t view = 0;
  const auto rc =
      GFHtmlViewCreate(gf::runtime::SdkContext(), owner.Instance(), slot,
                       GF_HTML_VIEW_PRESET_MAIL, &OnEvent, nullptr, &view);
  if (rc != GF_HTML_VIEW_OK) return static_cast<HtmlViewStatus>(rc);
  view_ = view;
  ViewHandlers().insert(view_, std::move(handlers));
  return HtmlViewStatus::kOK;
}

auto HtmlView::Load(const QString& html, const QList<HtmlPart>& parts,
                    int* dropped) -> HtmlViewStatus {
  if (dropped != nullptr) *dropped = 0;
  if (!IsLive() || !ViewHandlers().contains(view_)) {
    view_ = 0;  // lost meanwhile
    return HtmlViewStatus::kINVALID;
  }
  auto* ctx = gf::runtime::SdkContext();
  const auto body = html.toUtf8();
  auto* body_ref = GFBufferNewFromBytes(ctx, body.constData(),
                                        static_cast<size_t>(body.size()));

  QList<QByteArray> strings;  // alive across the call
  QList<GFHtmlPart> c_parts;
  for (const auto& part : parts) {
    strings.append(part.cid.toUtf8());
    const auto& cid = strings.constLast();
    strings.append(part.mime.toUtf8());
    const auto& mime = strings.constLast();
    GFHtmlPart c{};
    c.struct_size = sizeof(c);
    c.cid = cid.constData();
    c.mime = mime.constData();
    c.data = GFBufferNewFromBytes(ctx, part.data.constData(),
                                  static_cast<size_t>(part.data.size()));
    c_parts.append(c);
  }

  size_t n = 0;
  const auto rc = GFHtmlViewLoad(ctx, view_, body_ref, c_parts.constData(),
                                 static_cast<size_t>(c_parts.size()), &n);
  if (dropped != nullptr) *dropped = static_cast<int>(n);
  return static_cast<HtmlViewStatus>(rc);
}

void HtmlView::Clear() {
  if (IsLive()) GFHtmlViewClear(gf::runtime::SdkContext(), view_);
}

void HtmlView::Destroy() {
  if (!IsLive()) return;
  const auto view = view_;
  view_ = 0;
  if (ViewHandlers().remove(view) > 0) {
    GFHtmlViewDestroy(gf::runtime::SdkContext(), view);
  }
}

auto HtmlViewRenderer::RequestPart(quint64 view, const QString& session,
                                   const QString& cid, PartDone done)
    -> quint64 {
  if (!done) return 0;
  uint64_t request = 0;
  const auto rc = GFHtmlProviderRequestPart(
      gf::runtime::SdkContext(), view, session.toUtf8().constData(),
      cid.toUtf8().constData(), &OnPartDone, nullptr, &request);
  if (rc != GF_HTML_VIEW_OK) return 0;
  // Recorded before anything can answer: the Host never answers inside the
  // request itself.
  PartRequests().insert(request, std::move(done));
  return request;
}

void HtmlViewRenderer::CancelPart(quint64 request) {
  if (PartRequests().remove(request) > 0) {
    GFHtmlProviderCancelPart(gf::runtime::SdkContext(), request);
  }
}

void HtmlViewRenderer::LinkActivated(quint64 view, const QString& url) {
  GFHtmlProviderLinkActivated(gf::runtime::SdkContext(), view,
                              url.toUtf8().constData());
}

void HtmlViewRenderer::Loaded(quint64 view, bool ok) {
  GFHtmlProviderLoaded(gf::runtime::SdkContext(), view, ok ? 1 : 0);
}

auto RegisterHtmlViewRenderer(const char* name, uint32_t enforceable,
                              uint32_t features,
                              std::unique_ptr<HtmlViewRenderer> renderer)
    -> bool {
  if (renderer == nullptr || name == nullptr) return false;
  auto& reg = Registrations().emplace_back();
  reg.name = name;
  reg.renderer = std::move(renderer);
  reg.ops.struct_size = sizeof(GFHtmlViewProviderOps);
  reg.ops.create = &OpCreate;
  reg.ops.load = &OpLoad;
  reg.ops.clear = &OpClear;
  reg.ops.destroyed = &OpDestroyed;
  auto& s = reg.spec;
  s.struct_size = sizeof(GFHtmlViewProviderSpec);
  s.interface_version = GF_HTML_VIEW_PROVIDER_V1;
  s.name = reg.name.constData();
  s.enforceable = enforceable;
  s.features = features;
  s.user = &reg;
  s.ops = &reg.ops;

  if (GFHtmlProviderRegister(gf::runtime::SdkContext(), &s) !=
      GF_HTML_VIEW_OK) {
    LOG_ERROR(QString("the host refused HTML renderer %1")
                  .arg(QString::fromUtf8(name)));
    Registrations().pop_back();
    return false;
  }
  return true;
}

}  // namespace gf::ui

namespace gf::runtime {

void ForgetHtmlViews() {
  // The Host has withdrawn this module's views and drained its renderers
  // (or is about to, on the GUI thread, ahead of this): nothing calls in.
  auto forget = []() {
    ViewHandlers().clear();
    PartRequests().clear();
  };
  auto* app = QCoreApplication::instance();
  if (app == nullptr || QThread::currentThread() == app->thread()) {
    forget();
  } else {
    QMetaObject::invokeMethod(app, forget, Qt::QueuedConnection);
  }
}

void ResetHtmlRenderers() {
  // The previous activation's renderers. Their widgets were the Host's to
  // destroy, and it did when it drained them; the objects can go now.
  Registrations().clear();
}

}  // namespace gf::runtime

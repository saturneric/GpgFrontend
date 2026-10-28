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

#include "WebEditorService.h"

#include <QFile>
#include <QPointer>
#include <QRandomGenerator>

#include "core/module/ModuleLibraryPin.h"
#include "ui/html/EmbeddedViewCommon.h"
#include "ui/html/HtmlViewDocument.h"
#include "ui/html/WebEditorDocument.h"

namespace GpgFrontend::UI {

struct WebEditorService::Provider {
  WebEditorProviderInfo info;
  bool draining = false;
};

struct WebEditorService::Editor {
  quint64 id = 0;
  QString consumer;
  quint64 instance = 0;
  QString session;
  std::shared_ptr<Provider> provider;
  QPointer<QWidget> frame;
  QMetaObject::Connection frame_gone;
  EventFn events;
  QHash<QString, std::pair<QString, GFBuffer>> resources;  ///< path -> mime
  qint64 resource_bytes = 0;
};

namespace {

using Embedded::OnGuiThread;

void AskThenOpen(const QUrl& url) {
  Embedded::AskThenOpen(
      url, {WebEditorService::tr("Link Not Opened"),
            WebEditorService::tr("This kind of link cannot be opened:\n\n%1"),
            WebEditorService::tr("Open Link?"),
            WebEditorService::tr("The document links to this address:\n\n%1"
                                 "\n\nOpen it in your browser?")});
}

}  // namespace

auto WebEditorService::Instance() -> WebEditorService& {
  static auto* service = new WebEditorService();
  return *service;
}

WebEditorService::WebEditorService() : link_handler_(&AskThenOpen) {
  if (auto* app = QCoreApplication::instance()) moveToThread(app->thread());
}

auto WebEditorService::AssetRoot(const QString& consumer) const -> QString {
  const auto it = test_asset_roots_.constFind(consumer);
  if (it != test_asset_roots_.constEnd()) return *it;
  return u":/gf_module/%1/web/"_s.arg(consumer);
}

void WebEditorService::SetAssetRootForTest(const QString& consumer,
                                           const QString& dir) {
  if (dir.isEmpty()) {
    test_asset_roots_.remove(consumer);
  } else {
    test_asset_roots_.insert(consumer, dir);
  }
}

/* --- engines ------------------------------------------------------------- */

auto WebEditorService::RegisterProvider(WebEditorProviderInfo info) -> int {
  if (info.engine == nullptr || info.name.isEmpty()) return kINVALID;
  // An engine, once loaded, cannot be unloaded: see ModuleLibraryPin.h.
  if (!info.owner.isEmpty()) Module::PinModuleLibrary(info.owner);
  if (!OnGuiThread()) {
    QMetaObject::invokeMethod(
        this, [this, info = std::move(info)]() { register_now(info); },
        Qt::QueuedConnection);
    return kOK;
  }
  return register_now(std::move(info));
}

auto WebEditorService::register_now(WebEditorProviderInfo info) -> int {
  for (const auto& p : providers_) {
    if (p->info.owner == info.owner && p->info.name == info.name) {
      return kINVALID;
    }
  }
  auto provider = std::make_shared<Provider>();
  provider->info = std::move(info);
  providers_.append(provider);
  return kOK;
}

auto WebEditorService::UnregisterProvider(const QString& owner,
                                          const QString& name) -> int {
  if (!OnGuiThread()) {
    QMetaObject::invokeMethod(
        this, [this, owner, name]() { drain(owner, name); },
        Qt::QueuedConnection);
    return kOK;
  }
  for (const auto& p : providers_) {
    if (p->info.owner == owner && p->info.name == name) {
      drain(owner, name);
      return kOK;
    }
  }
  return kINVALID;
}

void WebEditorService::drain(const QString& owner, const QString& name) {
  QList<std::shared_ptr<Provider>> leaving;
  for (const auto& p : providers_) {
    if (p->info.owner == owner && (name.isEmpty() || p->info.name == name)) {
      leaving.append(p);
    }
  }
  for (const auto& provider : leaving) {
    provider->draining = true;
    QList<quint64> served;
    for (const auto& e : std::as_const(editors_)) {
      if (e->provider == provider) served.append(e->id);
    }
    for (const auto id : served) destroy(id, true);
    providers_.removeAll(provider);
  }
}

/* --- consumers ----------------------------------------------------------- */

auto WebEditorService::Available() const -> int {
  bool any = false;
  for (const auto& p : providers_) {
    if (p->draining) continue;
    any = true;
    if (!p->info.restart_required) return kOK;
  }
  return any ? kRESTART_REQUIRED : kUNAVAILABLE;
}

auto WebEditorService::Create(const QString& caller, quint64 instance,
                              const void* slot, EventFn events,
                              quint64* out_editor) -> int {
  if (out_editor != nullptr) *out_editor = 0;
  if (!OnGuiThread() || out_editor == nullptr || slot == nullptr) {
    return kINVALID;
  }
  auto* target = Embedded::FindSlot(caller, instance, slot);
  if (target == nullptr) return kDENIED;
  if (!Embedded::IsEmptySlot(target)) return kINVALID;
  // No page of its own, nothing to load.
  if (!QFile::exists(AssetRoot(caller) + u"index.html"_s)) return kINVALID;

  const auto provider =
      Embedded::PickProvider(providers_, [](const Provider&) { return true; });
  if (provider == nullptr) {
    const auto status = Available();
    return status == kOK ? kUNAVAILABLE : status;
  }

  auto e = std::make_shared<Editor>();
  e->id = new_id();
  e->consumer = caller;
  e->instance = instance;
  e->session = HtmlView::NewSession();
  e->provider = provider;
  e->events = std::move(events);
  // Registered before the engine is asked, so its first Resolve finds it.
  editors_.insert(e->id, e);

  auto* widget = provider->info.engine->Create(e->id, e->session);
  if (widget == nullptr) {
    editors_.remove(e->id);
    return kFAILED;
  }
  auto* frame = Embedded::MountInSlot(target, widget, u"GFWebEditorFrame"_s);
  e->frame = frame;
  const auto id = e->id;
  // The consumer may delete its own slot; the editor goes with it.
  e->frame_gone = connect(frame, &QObject::destroyed, this,
                          [this, id]() { frame_gone(id); });
  *out_editor = id;
  return kOK;
}

auto WebEditorService::Post(const QString& caller, quint64 editor,
                            const QByteArray& json) -> int {
  if (!OnGuiThread()) return kINVALID;
  auto* e = consumer_editor(caller, editor);
  if (e == nullptr) return kDENIED;
  if (json.size() > WebEditor::kMaxMessageBytes) return kTOO_LARGE;
  if (!WebEditor::IsValidMessage(json)) return kINVALID;
  return e->provider->info.engine->Post(editor, json) ? kOK : kFAILED;
}

auto WebEditorService::PutResource(const QString& caller, quint64 editor,
                                   const QString& path, const QString& mime,
                                   GFBuffer data) -> int {
  if (!OnGuiThread()) return kINVALID;
  auto* e = consumer_editor(caller, editor);
  if (e == nullptr) return kDENIED;
  if (!WebEditor::IsCleanPath(path) ||
      !WebEditor::IsAllowedResourceMime(mime)) {
    return kINVALID;
  }
  const auto size = static_cast<qint64>(data.Size());
  if (size > WebEditor::kMaxResourceBytes) return kTOO_LARGE;
  const auto old = e->resources.value(path).second.Size();
  if (e->resource_bytes - static_cast<qint64>(old) + size >
      WebEditor::kMaxTotalResourceBytes) {
    return kTOO_LARGE;
  }
  e->resource_bytes += size - static_cast<qint64>(old);
  e->resources.insert(path, {mime.toLower(), std::move(data)});
  return kOK;
}

auto WebEditorService::RemoveResource(const QString& caller, quint64 editor,
                                      const QString& path) -> int {
  if (!OnGuiThread()) return kINVALID;
  auto* e = consumer_editor(caller, editor);
  if (e == nullptr) return kDENIED;
  const auto it = e->resources.find(path);
  if (it == e->resources.end()) return kNOT_FOUND;
  e->resource_bytes -= static_cast<qint64>(it->second.Size());
  e->resources.erase(it);
  return kOK;
}

auto WebEditorService::Destroy(const QString& caller, quint64 editor) -> int {
  if (!OnGuiThread()) return kINVALID;
  if (consumer_editor(caller, editor) == nullptr) return kDENIED;
  destroy(editor, false);
  return kOK;
}

/* --- engines ask --------------------------------------------------------- */

auto WebEditorService::Resolve(const QString& provider, quint64 editor,
                               const QString& url, GFBuffer* out_data,
                               QString* out_mime) -> int {
  if (!OnGuiThread() || out_data == nullptr || out_mime == nullptr) {
    return kINVALID;
  }
  auto* e = provider_editor(provider, editor);
  if (e == nullptr) return kDENIED;
  const auto request = WebEditor::ParseRequest(url);
  // Another editor's session -- or a stale one of this editor's -- is not
  // found, not "denied": the page learns nothing about what exists.
  if (!request.has_value() || request->session != e->session) {
    return kNOT_FOUND;
  }

  if (!request->asset) {
    const auto it = e->resources.constFind(request->path);
    if (it == e->resources.constEnd()) return kNOT_FOUND;
    *out_mime = it->first;
    *out_data = it->second;
    return kOK;
  }

  const auto mime = WebEditor::AssetMime(request->path);
  if (mime.isEmpty()) return kNOT_FOUND;
  QFile file(AssetRoot(e->consumer) + request->path);
  if (!file.open(QIODevice::ReadOnly)) return kNOT_FOUND;
  if (file.size() > WebEditor::kMaxAssetBytes) return kTOO_LARGE;
  auto bytes = file.readAll();
  if (request->path == u"index.html"_s) bytes = WebEditor::InjectPolicy(bytes);
  *out_mime = mime;
  *out_data = GFBuffer(bytes);
  return kOK;
}

auto WebEditorService::Message(const QString& provider, quint64 editor,
                               const QByteArray& json) -> int {
  if (!OnGuiThread()) return kINVALID;
  auto* e = provider_editor(provider, editor);
  if (e == nullptr) return kDENIED;
  if (json.size() > WebEditor::kMaxMessageBytes) return kTOO_LARGE;
  // Checked before anyone hears it; its content is never logged.
  if (!WebEditor::IsValidMessage(json)) {
    LOG_W() << "web editor" << editor << "sent a malformed message of"
            << json.size() << "bytes; dropped";
    return kINVALID;
  }
  if (e->events) e->events(editor, kEVENT_MESSAGE, json);
  return kOK;
}

auto WebEditorService::LinkActivated(const QString& provider, quint64 editor,
                                     const QString& url) -> int {
  if (!OnGuiThread()) return kINVALID;
  auto* e = provider_editor(provider, editor);
  if (e == nullptr) return kDENIED;
  const QUrl parsed(url, QUrl::StrictMode);
  if (!parsed.isValid() || parsed.scheme().isEmpty()) return kINVALID;
  if (e->events) e->events(editor, kEVENT_LINK, url.toUtf8());
  QMetaObject::invokeMethod(
      this,
      [this, parsed]() {
        if (link_handler_) link_handler_(parsed);
      },
      Qt::QueuedConnection);
  return kOK;
}

auto WebEditorService::Ready(const QString& provider, quint64 editor) -> int {
  if (!OnGuiThread()) return kINVALID;
  auto* e = provider_editor(provider, editor);
  if (e == nullptr) return kDENIED;
  if (e->events) e->events(editor, kEVENT_READY, {});
  return kOK;
}

/* --- lifecycle ----------------------------------------------------------- */

void WebEditorService::WithdrawModule(const QString& module) {
  if (module.isEmpty()) return;
  QList<quint64> embedded;
  for (const auto& e : std::as_const(editors_)) {
    if (e->consumer == module) embedded.append(e->id);
  }
  for (const auto id : embedded) destroy(id, false);
  drain(module, {});
}

void WebEditorService::InstanceGone(quint64 instance) {
  QList<quint64> inside;
  for (const auto& e : std::as_const(editors_)) {
    if (e->instance == instance) inside.append(e->id);
  }
  for (const auto id : inside) destroy(id, false);
}

void WebEditorService::SetLinkHandler(LinkHandler handler) {
  link_handler_ = handler ? std::move(handler) : LinkHandler(&AskThenOpen);
}

auto WebEditorService::ProviderOwners() const -> QStringList {
  QStringList owners;
  for (const auto& p : providers_) owners.append(p->info.owner);
  return owners;
}

/* --- internals ----------------------------------------------------------- */

void WebEditorService::destroy(quint64 editor, bool tell_consumer) {
  const auto e = editors_.take(editor);
  if (e == nullptr) return;
  // Every resource goes with it, wiped as its buffers are released.
  e->resources.clear();
  if (!e->frame.isNull()) {
    disconnect(e->frame_gone);
    e->frame->hide();
    e->frame->deleteLater();
  }
  if (e->provider != nullptr) e->provider->info.engine->Destroyed(editor);
  if (tell_consumer && e->events) e->events(editor, kEVENT_LOST, {});
}

void WebEditorService::frame_gone(quint64 editor) {
  const auto e = editors_.take(editor);
  if (e == nullptr) return;
  e->resources.clear();
  if (e->provider != nullptr) e->provider->info.engine->Destroyed(editor);
}

auto WebEditorService::new_id() const -> quint64 {
  // Random rather than counted: a handle names nothing to anyone but its
  // owner, and should not be guessable from the last one either.
  for (;;) {
    const auto id = QRandomGenerator::system()->generate64();
    if (id != 0 && !editors_.contains(id)) return id;
  }
}

auto WebEditorService::find(quint64 editor) -> Editor* {
  const auto it = editors_.constFind(editor);
  return it == editors_.constEnd() ? nullptr : it->get();
}

auto WebEditorService::consumer_editor(const QString& caller, quint64 editor)
    -> Editor* {
  auto* e = find(editor);
  return e != nullptr && e->consumer == caller ? e : nullptr;
}

auto WebEditorService::provider_editor(const QString& provider, quint64 editor)
    -> Editor* {
  auto* e = find(editor);
  if (e == nullptr || e->provider == nullptr) return nullptr;
  return e->provider->info.owner == provider ? e : nullptr;
}

}  // namespace GpgFrontend::UI

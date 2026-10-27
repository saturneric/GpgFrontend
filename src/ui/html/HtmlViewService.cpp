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

#include "HtmlViewService.h"

#include <QDesktopServices>
#include <QMessageBox>
#include <QPointer>
#include <QRandomGenerator>
#include <QThread>
#include <QVBoxLayout>

#include "core/module/ModuleLibraryPin.h"
#include "ui/lua/NativeInstances.h"

namespace GpgFrontend::UI {

struct HtmlViewService::Provider {
  HtmlViewProviderInfo info;
  bool draining = false;
};

struct HtmlViewService::View {
  quint64 id = 0;
  QString consumer;
  quint64 instance = 0;
  std::shared_ptr<Provider> provider;
  HtmlView::Policy policy;
  QPointer<QWidget> frame;
  QMetaObject::Connection frame_gone;
  EventFn events;
  /// The document now shown; empty when nothing is. A part request is only
  /// honoured for the current one.
  QString session;
  QHash<QString, HtmlViewPart> parts;  ///< by NormalizeCid()
};

struct HtmlViewService::Pending {
  quint64 id = 0;
  quint64 view = 0;
  QString session;
  QString cid;
  QString provider;
  PartDoneFn done;
};

namespace {

auto OnGuiThread() -> bool {
  auto* app = QCoreApplication::instance();
  return app != nullptr && QThread::currentThread() == app->thread();
}

auto Covers(const HtmlViewProviderInfo& info, const HtmlView::Policy& policy)
    -> bool {
  return (info.enforceable & policy.required) == policy.required;
}

/// The Host's question, with the address exactly as it would be opened.
/// Fully encoded, so an internationalised host name shows as its punycode
/// rather than as whatever it was chosen to look like.
void AskThenOpen(const QUrl& url) {
  const auto scheme = url.scheme().toLower();
  const auto shown = url.toString(QUrl::FullyEncoded);
  auto* parent = QApplication::activeWindow();
  if (scheme != u"http"_s && scheme != u"https"_s && scheme != u"mailto"_s) {
    QMessageBox::information(
        parent, HtmlViewService::tr("Link Not Opened"),
        HtmlViewService::tr("This kind of link cannot be opened from a "
                            "message:\n\n%1")
            .arg(shown));
    return;
  }
  const auto answer = QMessageBox::question(
      parent, HtmlViewService::tr("Open Link?"),
      HtmlViewService::tr("The message links to this address:\n\n%1\n\nOpen "
                          "it in your browser?")
          .arg(shown),
      QMessageBox::Open | QMessageBox::Cancel, QMessageBox::Cancel);
  if (answer == QMessageBox::Open) QDesktopServices::openUrl(url);
}

}  // namespace

auto HtmlViewService::Instance() -> HtmlViewService& {
  static auto* service = new HtmlViewService();
  return *service;
}

HtmlViewService::HtmlViewService() : link_handler_(&AskThenOpen) {
  // Part completions are queued to this object; whichever thread first asked
  // for the service, it lives where every view does.
  if (auto* app = QCoreApplication::instance()) moveToThread(app->thread());
}

/* --- renderers ----------------------------------------------------------- */

auto HtmlViewService::RegisterProvider(HtmlViewProviderInfo info) -> int {
  if (info.renderer == nullptr || info.name.isEmpty()) return kINVALID;
  // An engine, once loaded, cannot be unloaded: see ModuleLibraryPin.h.
  // Here and now, from whatever thread: the pin must hold before the module
  // could be deactivated, which it may be before the registration lands.
  if (!info.owner.isEmpty()) Module::PinModuleLibrary(info.owner);

  // A module registers while it activates, on its runner thread; the
  // registry is the GUI thread's. Queued behind anything already posted, so
  // a withdrawal queued after it still finds it to drain.
  if (!OnGuiThread()) {
    QMetaObject::invokeMethod(
        this, [this, info = std::move(info)]() { register_now(info); },
        Qt::QueuedConnection);
    return kOK;
  }
  return register_now(std::move(info));
}

auto HtmlViewService::register_now(HtmlViewProviderInfo info) -> int {
  for (const auto& p : providers_) {
    if (p->info.owner == info.owner && p->info.name == info.name) {
      LOG_W() << "HTML renderer" << info.name << "of" << info.owner
              << "is registered already; refused";
      return kINVALID;
    }
  }
  auto provider = std::make_shared<Provider>();
  provider->info = std::move(info);
  providers_.append(provider);
  return kOK;
}

auto HtmlViewService::UnregisterProvider(const QString& owner,
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

void HtmlViewService::drain(const QString& owner, const QString& name) {
  // An empty name drains every renderer @p owner provides.
  QList<std::shared_ptr<Provider>> leaving;
  for (const auto& p : providers_) {
    if (p->info.owner == owner && (name.isEmpty() || p->info.name == name)) {
      leaving.append(p);
    }
  }

  for (const auto& provider : leaving) {
    // 1. No new view lands on it from here on.
    provider->draining = true;

    QList<quint64> served;
    for (const auto& v : std::as_const(views_)) {
      if (v->provider == provider) served.append(v->id);
    }

    // 2. Nothing it asked for is answered.
    for (const auto id : served) drop_pending_for(id);

    // 3. Every view it serves goes, and whoever embedded it hears so.
    for (const auto id : served) destroy(id, true);

    // 4. Then it is forgotten.
    providers_.removeAll(provider);
  }
}

/* --- consumers ----------------------------------------------------------- */

auto HtmlViewService::select(const HtmlView::Policy& policy) const
    -> const Provider* {
  const Provider* best = nullptr;
  for (const auto& p : providers_) {
    if (p->draining || p->info.restart_required) continue;
    if (!Covers(p->info, policy)) continue;
    if (best == nullptr || p->info.priority > best->info.priority) {
      best = p.get();
    }
  }
  return best;
}

auto HtmlViewService::Available(int preset) const -> int {
  const auto policy = HtmlView::PolicyFor(preset);
  if (!policy.has_value()) return kINVALID;

  bool any = false;
  bool covering = false;
  for (const auto& p : providers_) {
    if (p->draining) continue;
    any = true;
    if (!Covers(p->info, *policy)) continue;
    covering = true;
    if (!p->info.restart_required) return kOK;
  }
  if (!any) return kUNAVAILABLE;
  return covering ? kRESTART_REQUIRED : kPOLICY;
}

auto HtmlViewService::CreateView(const QString& caller, quint64 instance,
                                 const void* slot, int preset, EventFn events,
                                 quint64* out_view) -> int {
  if (out_view != nullptr) *out_view = 0;
  if (!OnGuiThread() || out_view == nullptr || slot == nullptr) {
    return kINVALID;
  }
  const auto policy = HtmlView::PolicyFor(preset);
  if (!policy.has_value()) return kINVALID;

  // Who is asking is the grant's answer, never an argument's; and the
  // instance must be one the Host made for that module, still alive.
  const auto live = NativeInstances::Instance().Find(instance);
  if (!live.has_value() || live->owner != caller || live->widget.isNull()) {
    return kDENIED;
  }
  QWidget* root = live->widget.data();

  // The slot is the caller's pointer, so it is not followed until it has
  // been found, by address, in the instance's own tree. Asking the slot for
  // its ancestors would dereference whatever the caller passed.
  QWidget* target = nullptr;
  if (slot == root) {
    target = root;
  } else {
    for (auto* child : root->findChildren<QWidget*>()) {
      if (child == slot) {
        target = child;
        break;
      }
    }
  }
  if (target == nullptr) return kDENIED;
  if (target->layout() != nullptr && target->layout()->count() > 0) {
    return kINVALID;  // the view fills its slot; the slot must be empty
  }

  const auto* chosen = select(*policy);
  if (chosen == nullptr) {
    const auto status = Available(preset);
    return status == kOK ? kUNAVAILABLE : status;
  }
  std::shared_ptr<Provider> provider;
  for (const auto& p : providers_) {
    if (p.get() == chosen) provider = p;
  }

  const auto id = new_id();
  auto* widget = provider->info.renderer->Create(id, *policy);
  if (widget == nullptr) return kFAILED;

  auto* frame = new QWidget(target);
  frame->setObjectName(u"GFHtmlViewFrame"_s);
  auto* frame_layout = new QVBoxLayout(frame);
  frame_layout->setContentsMargins(0, 0, 0, 0);
  frame_layout->setSpacing(0);
  widget->setParent(frame);
  frame_layout->addWidget(widget);
  auto* slot_layout = target->layout();
  if (slot_layout == nullptr) {
    slot_layout = new QVBoxLayout(target);
    slot_layout->setContentsMargins(0, 0, 0, 0);
    slot_layout->setSpacing(0);
  }
  slot_layout->addWidget(frame);

  auto v = std::make_shared<View>();
  v->id = id;
  v->consumer = caller;
  v->instance = instance;
  v->provider = provider;
  v->policy = *policy;
  v->frame = frame;
  v->events = std::move(events);
  // The consumer may delete its own slot; the view goes with it.
  v->frame_gone = connect(frame, &QObject::destroyed, this,
                          [this, id]() { frame_gone(id); });
  views_.insert(id, v);
  *out_view = id;
  return kOK;
}

auto HtmlViewService::Load(const QString& caller, quint64 view,
                           const QByteArray& html_utf8,
                           QList<HtmlViewPart> parts, int* out_dropped) -> int {
  if (out_dropped != nullptr) *out_dropped = 0;
  if (!OnGuiThread()) return kINVALID;
  auto* v = consumer_view(caller, view);
  if (v == nullptr) return kDENIED;
  if (html_utf8.size() > v->policy.max_document_bytes) return kTOO_LARGE;

  // Only what may be shown at all is kept, and nothing past the limits:
  // the rest is dropped and counted, not refused, because the body is still
  // worth showing without it.
  QHash<QString, HtmlViewPart> kept;
  qint64 total = 0;
  int dropped = 0;
  for (auto& part : parts) {
    const auto cid = HtmlView::NormalizeCid(part.cid);
    const auto size = static_cast<qint64>(part.data.Size());
    if (cid.isEmpty() || kept.contains(cid) ||
        !HtmlView::IsAllowedPartMime(part.mime) ||
        size > v->policy.max_part_bytes ||
        total + size > v->policy.max_total_part_bytes) {
      ++dropped;
      continue;
    }
    total += size;
    part.cid = cid;
    kept.insert(cid, part);
  }

  // A new document is a new session: whatever the old one still had in
  // flight is answered STALE when it arrives.
  v->parts = std::move(kept);
  v->session = HtmlView::NewSession();
  const auto document = HtmlView::BuildDocument(
      QString::fromUtf8(html_utf8), v->session,
      v->provider->info.frame_same_origin);
  if (!v->provider->info.renderer->Load(view, v->session, document)) {
    v = find_view(view);  // the renderer may have been re-entered
    if (v != nullptr) {
      v->session.clear();
      v->parts.clear();
    }
    return kFAILED;
  }
  if (out_dropped != nullptr) *out_dropped = dropped;
  return kOK;
}

auto HtmlViewService::Clear(const QString& caller, quint64 view) -> int {
  if (!OnGuiThread()) return kINVALID;
  auto* v = consumer_view(caller, view);
  if (v == nullptr) return kDENIED;
  v->session.clear();
  v->parts.clear();
  v->provider->info.renderer->Clear(view);
  return kOK;
}

auto HtmlViewService::DestroyView(const QString& caller, quint64 view) -> int {
  if (!OnGuiThread()) return kINVALID;
  if (consumer_view(caller, view) == nullptr) return kDENIED;
  destroy(view, false);
  return kOK;
}

/* --- what a renderer may ask --------------------------------------------- */

auto HtmlViewService::RequestPart(const QString& provider, quint64 view,
                                  const QString& session, const QString& cid,
                                  PartDoneFn done, quint64* out_request)
    -> int {
  if (out_request != nullptr) *out_request = 0;
  if (!OnGuiThread() || !done || out_request == nullptr) return kINVALID;
  auto* v = provider_view(provider, view);
  if (v == nullptr) return kDENIED;
  // Only the document now shown, and only under the name it was given.
  if (v->session.isEmpty() || session != v->session) return kDENIED;

  auto p = std::make_shared<Pending>();
  p->id = new_id();
  p->view = view;
  p->session = session;
  p->cid = cid;
  p->provider = provider;
  p->done = std::move(done);
  pending_.insert(p->id, p);
  const auto id = p->id;
  // Never inside the request: a renderer must not be re-entered before it
  // has even recorded what it asked for.
  QMetaObject::invokeMethod(
      this, [this, id]() { deliver(id); }, Qt::QueuedConnection);
  *out_request = id;
  return kOK;
}

void HtmlViewService::deliver(quint64 request) {
  const auto p = pending_.take(request);
  if (p == nullptr) return;  // cancelled, or its view is gone
  auto* v = find_view(p->view);
  if (v == nullptr) return;

  if (v->session != p->session) {
    p->done(request, kPART_STALE, {}, {});
    return;
  }
  const auto it = v->parts.constFind(HtmlView::NormalizeCid(p->cid));
  if (it == v->parts.constEnd()) {
    p->done(request, kPART_NOT_FOUND, {}, {});
    return;
  }
  if (static_cast<qint64>(it->data.Size()) > v->policy.max_part_bytes) {
    p->done(request, kPART_TOO_LARGE, {}, {});
    return;
  }
  // Copied out first: the callback may load, clear or destroy this view.
  const auto mime = it->mime;
  const auto data = it->data;
  p->done(request, kPART_OK, mime, data);
}

auto HtmlViewService::CancelPart(const QString& provider, quint64 request)
    -> int {
  if (!OnGuiThread()) return kINVALID;
  const auto it = pending_.constFind(request);
  if (it == pending_.constEnd()) return kINVALID;
  if ((*it)->provider != provider) return kDENIED;
  pending_.erase(it);
  return kOK;
}

auto HtmlViewService::LinkActivated(const QString& provider, quint64 view,
                                    const QString& url) -> int {
  if (!OnGuiThread()) return kINVALID;
  auto* v = provider_view(provider, view);
  if (v == nullptr) return kDENIED;
  const QUrl parsed(url, QUrl::StrictMode);
  if (!parsed.isValid() || parsed.scheme().isEmpty()) return kINVALID;

  if (v->events) v->events(view, kEVENT_LINK, 0, url);
  // Queued: the question is modal, and the renderer is mid-callback.
  QMetaObject::invokeMethod(
      this,
      [this, parsed]() {
        if (link_handler_) link_handler_(parsed);
      },
      Qt::QueuedConnection);
  return kOK;
}

auto HtmlViewService::Loaded(const QString& provider, quint64 view, bool ok)
    -> int {
  if (!OnGuiThread()) return kINVALID;
  auto* v = provider_view(provider, view);
  if (v == nullptr) return kDENIED;
  if (v->events) v->events(view, kEVENT_LOADED, ok ? 1 : 0, {});
  return kOK;
}

/* --- lifecycle ----------------------------------------------------------- */

void HtmlViewService::WithdrawModule(const QString& module) {
  if (module.isEmpty()) return;
  QList<quint64> embedded;
  for (const auto& v : std::as_const(views_)) {
    if (v->consumer == module) embedded.append(v->id);
  }
  // Its own views: the renderer is told; the module, whose gate is shut,
  // is not.
  for (const auto id : embedded) destroy(id, false);
  drain(module, {});
}

void HtmlViewService::InstanceGone(quint64 instance) {
  QList<quint64> inside;
  for (const auto& v : std::as_const(views_)) {
    if (v->instance == instance) inside.append(v->id);
  }
  for (const auto id : inside) destroy(id, false);
}

void HtmlViewService::SetLinkHandler(LinkHandler handler) {
  link_handler_ = handler ? std::move(handler) : LinkHandler(&AskThenOpen);
}

/* --- internals ----------------------------------------------------------- */

auto HtmlViewService::find_view(quint64 view) -> View* {
  const auto it = views_.constFind(view);
  return it == views_.constEnd() ? nullptr : it->get();
}

auto HtmlViewService::consumer_view(const QString& caller, quint64 view)
    -> View* {
  auto* v = find_view(view);
  return v != nullptr && v->consumer == caller ? v : nullptr;
}

auto HtmlViewService::provider_view(const QString& provider, quint64 view)
    -> View* {
  auto* v = find_view(view);
  if (v == nullptr || v->provider == nullptr) return nullptr;
  return v->provider->info.owner == provider ? v : nullptr;
}

void HtmlViewService::drop_pending_for(quint64 view) {
  for (auto it = pending_.begin(); it != pending_.end();) {
    if ((*it)->view == view) {
      it = pending_.erase(it);
    } else {
      ++it;
    }
  }
}

void HtmlViewService::destroy(quint64 view, bool tell_consumer) {
  const auto v = views_.take(view);
  if (v == nullptr) return;
  drop_pending_for(view);
  v->parts.clear();
  v->session.clear();
  if (!v->frame.isNull()) {
    disconnect(v->frame_gone);
    v->frame->hide();
    v->frame->deleteLater();
  }
  if (v->provider != nullptr) v->provider->info.renderer->Destroyed(view);
  if (tell_consumer && v->events) v->events(view, kEVENT_LOST, 0, {});
}

void HtmlViewService::frame_gone(quint64 view) {
  const auto v = views_.take(view);
  if (v == nullptr) return;
  drop_pending_for(view);
  v->parts.clear();
  if (v->provider != nullptr) v->provider->info.renderer->Destroyed(view);
}

auto HtmlViewService::new_id() -> quint64 {
  // Random rather than counted: a handle names nothing to anyone but its
  // owner, and should not be guessable from the last one either.
  for (;;) {
    const auto id = QRandomGenerator::system()->generate64();
    if (id != 0 && !views_.contains(id) && !pending_.contains(id)) return id;
  }
}

auto HtmlViewService::ProviderOwners() const -> QStringList {
  QStringList owners;
  for (const auto& p : providers_) owners.append(p->info.owner);
  return owners;
}

auto HtmlViewService::ViewCount() const -> int {
  return static_cast<int>(views_.size());
}

auto HtmlViewService::PendingPartCount() const -> int {
  return static_cast<int>(pending_.size());
}

auto HtmlViewService::CurrentSession(quint64 view) const -> QString {
  const auto it = views_.constFind(view);
  return it == views_.constEnd() ? QString() : (*it)->session;
}

}  // namespace GpgFrontend::UI

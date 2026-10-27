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

#include <functional>
#include <memory>

#include "core/model/GFBuffer.h"
#include "ui/html/HtmlViewDocument.h"

namespace GpgFrontend::UI {

/**
 * @brief A renderer, as the service drives it.
 *
 * The built-in platform engines implement this directly; a module's renderer
 * reaches it through the SDK adapter. Every call is on the GUI thread.
 */
class GF_UI_EXPORT HtmlViewRenderer {
 public:
  virtual ~HtmlViewRenderer() = default;

  /// Build @p view's widget, enforcing all of @p policy; nullptr refuses.
  /// The service owns the widget afterwards.
  virtual auto Create(quint64 view, const HtmlView::Policy& policy)
      -> QWidget* = 0;
  /// Show @p document, whose parts are requested under @p session.
  virtual auto Load(quint64 view, const QString& session,
                    const QByteArray& document) -> bool = 0;
  virtual void Clear(quint64 view) = 0;
  /// The view is gone; its widget is already scheduled for deletion.
  virtual void Destroyed(quint64 view) = 0;
};

/// One renderer, as registered.
struct GF_UI_EXPORT HtmlViewProviderInfo {
  QString owner;  ///< the module id; empty for the Host's own engine
  QString name;
  int priority = 0;               ///< kBuiltInPriority or kModulePriority
  uint32_t enforceable = 0;       ///< HtmlView::PolicyBit
  bool restart_required = false;  ///< installed, not usable until a restart
  /// The engine needs the mail frame to carry allow-same-origin to load the
  /// part scheme at all (WebView2). Safe only because no script ever runs;
  /// see HtmlView::SandboxTokens. Opaque origin otherwise.
  bool frame_same_origin = false;
  std::shared_ptr<HtmlViewRenderer> renderer;
};

/// A part of the message, as a consumer hands it over. The bytes stay in a
/// GFBuffer from the consumer to the renderer, so the one copy there is is
/// still wiped when the last holder lets go.
struct GF_UI_EXPORT HtmlViewPart {
  QString cid;
  QString mime;
  GFBuffer data;
};

/**
 * @brief Every HTML view, every renderer, and what each may touch.
 *
 * Everything that decides safety is here: which policy a view is under,
 * which renderer may serve it, which slot it may sit in, which document it
 * shows and which parts that document may ask for. Renderers only enforce;
 * consumers only ask.
 *
 * GUI thread only, every member, except registering and unregistering a
 * renderer: a module does that while it activates, on its own thread.
 */
class GF_UI_EXPORT HtmlViewService : public QObject {
  Q_OBJECT

 public:
  /// GF_HTML_VIEW_* status codes.
  enum Status : int {
    kOK = 0,
    kUNAVAILABLE = -1,
    kRESTART_REQUIRED = -2,
    kPOLICY = -3,
    kDENIED = -4,
    kINVALID = -5,
    kTOO_LARGE = -6,
    kFAILED = -7,
  };

  /// GF_HTML_PART_* outcomes.
  enum PartStatus : int {
    kPART_OK = 0,
    kPART_NOT_FOUND = 1,
    kPART_TOO_LARGE = 2,
    kPART_STALE = 3,
  };

  /// GF_HTML_VIEW_EVENT_*.
  enum Event : int {
    kEVENT_LOADED = 1,
    kEVENT_LINK = 2,
    kEVENT_LOST = 3,
  };

  static constexpr int kBuiltInPriority = 100;
  static constexpr int kModulePriority = 50;

  /// What a consumer hears about its view.
  using EventFn = std::function<void(quint64 view, int event, int value,
                                     const QString& text)>;
  /// How a part request ended, for the renderer that made it.
  using PartDoneFn = std::function<void(
      quint64 request, int status, const QString& mime, const GFBuffer& data)>;
  /// What happens to a link the user activated: by default the Host asks,
  /// showing the URL as written, and opens it outside the application.
  using LinkHandler = std::function<void(const QUrl& url)>;

  static auto Instance() -> HtmlViewService&;

  /* --- renderers --- */

  /// From any thread; off the GUI thread it lands there, queued.
  auto RegisterProvider(HtmlViewProviderInfo info) -> int;
  /// Drain, then forget: no new view, pending parts dropped, every view it
  /// serves destroyed (its consumer told LOST), then unregistered.
  auto UnregisterProvider(const QString& owner, const QString& name) -> int;

  /* --- consumers --- */

  [[nodiscard]] auto Available(int preset) const -> int;
  auto CreateView(const QString& caller, quint64 instance, const void* slot,
                  int preset, EventFn events, quint64* out_view) -> int;
  auto Load(const QString& caller, quint64 view, const QByteArray& html_utf8,
            QList<HtmlViewPart> parts, int* out_dropped) -> int;
  auto Clear(const QString& caller, quint64 view) -> int;
  auto DestroyView(const QString& caller, quint64 view) -> int;

  /* --- what a renderer may ask --- */

  auto RequestPart(const QString& provider, quint64 view,
                   const QString& session, const QString& cid, PartDoneFn done,
                   quint64* out_request) -> int;
  auto CancelPart(const QString& provider, quint64 request) -> int;
  auto LinkActivated(const QString& provider, quint64 view, const QString& url)
      -> int;
  auto Loaded(const QString& provider, quint64 view, bool ok) -> int;

  /* --- lifecycle --- */

  /// Part of withdrawing @p module: its views are destroyed and every
  /// renderer it provides is drained. Nothing enters it while this runs.
  void WithdrawModule(const QString& module);
  /// A native widget instance is gone; so are the views inside it.
  void InstanceGone(quint64 instance);

  void SetLinkHandler(LinkHandler handler);

  /* --- for tests --- */

  [[nodiscard]] auto ViewCount() const -> int;
  /// Who provides a renderer now; the Host's own engine is the empty name.
  [[nodiscard]] auto ProviderOwners() const -> QStringList;
  [[nodiscard]] auto PendingPartCount() const -> int;
  [[nodiscard]] auto CurrentSession(quint64 view) const -> QString;

 private:
  struct View;
  struct Pending;
  struct Provider;

  HtmlViewService();

  auto register_now(HtmlViewProviderInfo info) -> int;
  [[nodiscard]] auto select(const HtmlView::Policy& policy) const
      -> const Provider*;
  [[nodiscard]] auto find_view(quint64 view) -> View*;
  /// The view @p caller embedded, or nullptr.
  auto consumer_view(const QString& caller, quint64 view) -> View*;
  /// The view @p provider serves, or nullptr.
  auto provider_view(const QString& provider, quint64 view) -> View*;
  void deliver(quint64 request);
  void drop_pending_for(quint64 view);
  /// Remove @p view: the renderer is told, the frame goes, its parts are
  /// wiped. @p tell_consumer: whether the consumer hears LOST.
  void destroy(quint64 view, bool tell_consumer);
  void frame_gone(quint64 view);
  void drain(const QString& owner, const QString& name);
  [[nodiscard]] auto new_id() -> quint64;

  QHash<quint64, std::shared_ptr<View>> views_;
  QHash<quint64, std::shared_ptr<Pending>> pending_;
  QList<std::shared_ptr<Provider>> providers_;
  LinkHandler link_handler_;
};

}  // namespace GpgFrontend::UI

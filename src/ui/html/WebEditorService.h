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

/**
 * @file WebEditorService.h
 * @brief Editable pages of a module's own, in its native widgets.
 *
 * The HTML view's sibling, kept apart so that neither's guarantees depend on
 * the other: a mail view runs no script at all; an editor runs exactly the
 * script its module shipped, and nothing it is given.
 *
 * The Host owns what the page may load (the caller's embedded WEB_ASSETS and
 * the resources it put, nothing else), the policy in front of it, which
 * module owns which editor, and every message that crosses: each is checked
 * before it is passed on, and none is ever logged.
 */

namespace GpgFrontend::UI {

class GF_UI_EXPORT WebEditorEngine {
 public:
  virtual ~WebEditorEngine() = default;
  /// Build @p editor's widget, loading gfasset://<session>/index.html.
  virtual auto Create(quint64 editor, const QString& session) -> QWidget* = 0;
  /// Deliver one JSON object to the page.
  virtual auto Post(quint64 editor, const QByteArray& json) -> bool = 0;
  /// The editor is gone; its widget is already scheduled for deletion.
  virtual void Destroyed(quint64 editor) = 0;
};

struct GF_UI_EXPORT WebEditorProviderInfo {
  QString owner;  ///< the module id; empty for the Host's own engine
  QString name;
  int priority = 0;
  bool restart_required = false;
  std::shared_ptr<WebEditorEngine> engine;
};

class GF_UI_EXPORT WebEditorService : public QObject {
  Q_OBJECT

 public:
  /// GF_HTML_VIEW_* codes; an editor has no policy to miss, so no kPOLICY.
  enum Status : int {
    kOK = 0,
    kUNAVAILABLE = -1,
    kRESTART_REQUIRED = -2,
    kDENIED = -4,
    kINVALID = -5,
    kTOO_LARGE = -6,
    kFAILED = -7,
    kNOT_FOUND = -8,
  };

  /// GF_WEB_EDITOR_EVENT_*.
  enum Event : int {
    kEVENT_READY = 1,
    kEVENT_MESSAGE = 2,
    kEVENT_LINK = 3,
    kEVENT_LOST = 4,
  };

  static constexpr int kBuiltInPriority = 100;
  static constexpr int kModulePriority = 50;

  using EventFn =
      std::function<void(quint64 editor, int event, const QByteArray& text)>;
  using LinkHandler = std::function<void(const QUrl& url)>;

  static auto Instance() -> WebEditorService&;

  /* --- engines --- */

  /// From any thread; off the GUI thread it lands there, queued.
  auto RegisterProvider(WebEditorProviderInfo info) -> int;
  auto UnregisterProvider(const QString& owner, const QString& name) -> int;

  /* --- consumers --- */

  [[nodiscard]] auto Available() const -> int;
  auto Create(const QString& caller, quint64 instance, const void* slot,
              EventFn events, quint64* out_editor) -> int;
  auto Post(const QString& caller, quint64 editor, const QByteArray& json)
      -> int;
  auto PutResource(const QString& caller, quint64 editor, const QString& path,
                   const QString& mime, GFBuffer data) -> int;
  auto RemoveResource(const QString& caller, quint64 editor,
                      const QString& path) -> int;
  auto Destroy(const QString& caller, quint64 editor) -> int;

  /* --- what an engine may ask --- */

  /// What the page loads: an asset of the consumer's own, or a resource it
  /// put, for the editor's CURRENT session only.
  auto Resolve(const QString& provider, quint64 editor, const QString& url,
               GFBuffer* out_data, QString* out_mime) -> int;
  auto Message(const QString& provider, quint64 editor, const QByteArray& json)
      -> int;
  auto LinkActivated(const QString& provider, quint64 editor,
                     const QString& url) -> int;
  auto Ready(const QString& provider, quint64 editor) -> int;

  /* --- lifecycle --- */

  void WithdrawModule(const QString& module);
  void InstanceGone(quint64 instance);
  void SetLinkHandler(LinkHandler handler);

  /* --- for tests --- */

  [[nodiscard]] auto ProviderOwners() const -> QStringList;
  /// Tests only: read @p consumer's assets from @p dir (ending in '/');
  /// empty restores the embedded root.
  void SetAssetRootForTest(const QString& consumer, const QString& dir);

 private:
  struct Editor;
  struct Provider;

  WebEditorService();

  /// Where an editor's assets are read from, for @p consumer: its embedded
  /// WEB_ASSETS, unless a test has pointed it elsewhere.
  [[nodiscard]] auto AssetRoot(const QString& consumer) const -> QString;
  auto register_now(WebEditorProviderInfo info) -> int;
  void drain(const QString& owner, const QString& name);
  void destroy(quint64 editor, bool tell_consumer);
  void frame_gone(quint64 editor);
  [[nodiscard]] auto new_id() const -> quint64;
  [[nodiscard]] auto find(quint64 editor) -> Editor*;
  auto consumer_editor(const QString& caller, quint64 editor) -> Editor*;
  auto provider_editor(const QString& provider, quint64 editor) -> Editor*;

  QList<std::shared_ptr<Provider>> providers_;
  QHash<quint64, std::shared_ptr<Editor>> editors_;
  QHash<QString, QString> test_asset_roots_;
  LinkHandler link_handler_;
};

}  // namespace GpgFrontend::UI

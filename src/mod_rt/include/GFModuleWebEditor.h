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

#include <QByteArray>
#include <QString>
#include <functional>
#include <memory>

#include "GFSDKTypes.h"

// Declared, not included: see GFModuleNativeWidget.h.
class QWidget;

/**
 * @file GFModuleWebEditor.h
 * @brief An editable page of the module's own, from C++: embedding one, or
 * providing the engine behind it.
 *
 * Embedding (capability "web.editor"): the page is the module's WEB_ASSETS
 * index.html; the module and the page exchange JSON objects.
 * @code
 * editor_.Create(*this, slot_, {.message = [this](const QByteArray& json) {
 *   ...
 * }});
 * editor_.Post(R"({"type":"load","markdown":"# hi"})");
 * @endcode
 *
 * Providing (capability "web.editor.provider") is for engine modules:
 * subclass WebEditorEngine and register it on activation.
 */

namespace gf::ui {

class NativeWidgetBase;

/// GF_HTML_VIEW_* status; the same meanings.
enum class WebEditorStatus : int {
  kOK = GF_HTML_VIEW_OK,
  kUNAVAILABLE = GF_HTML_VIEW_E_UNAVAILABLE,
  kRESTART_REQUIRED = GF_HTML_VIEW_E_RESTART_REQUIRED,
  kDENIED = GF_HTML_VIEW_E_DENIED,
  kINVALID = GF_HTML_VIEW_E_INVALID,
  kTOO_LARGE = GF_HTML_VIEW_E_TOO_LARGE,
  kFAILED = GF_HTML_VIEW_E_FAILED,
  kNOT_FOUND = GF_HTML_VIEW_E_NOT_FOUND,
};

/// Whether an editor could be created now.
[[nodiscard]] auto WebEditorAvailability() -> WebEditorStatus;

class WebEditor {
 public:
  struct Handlers {
    std::function<void()> ready;
    /// One JSON object from the page, already checked by the Host.
    std::function<void(const QByteArray& json)> message;
    /// The user activated a link; the Host has already decided what to do.
    std::function<void(const QString& url)> link;
    /// The engine went away and took the editor with it.
    std::function<void()> lost;
  };

  WebEditor() = default;
  ~WebEditor();
  WebEditor(const WebEditor&) = delete;
  auto operator=(const WebEditor&) -> WebEditor& = delete;

  auto Create(const NativeWidgetBase& owner, QWidget* slot, Handlers handlers)
      -> WebEditorStatus;
  auto Post(const QByteArray& json) -> WebEditorStatus;
  /// Serve @p data at gfres://<session>/@p path; images only.
  auto PutResource(const QString& path, const QString& mime,
                   const QByteArray& data) -> WebEditorStatus;
  auto RemoveResource(const QString& path) -> WebEditorStatus;
  void Destroy();
  [[nodiscard]] auto IsLive() const -> bool { return editor_ != 0; }

 private:
  /// Still the Host's: an editor it lost is forgotten here too.
  auto still_live() -> bool;

  quint64 editor_ = 0;
};

class WebEditorEngine {
 public:
  virtual ~WebEditorEngine() = default;

  /// Build the widget and load gfasset://<session>/index.html.
  virtual auto Create(quint64 editor, const QString& session) -> QWidget* = 0;
  virtual auto Post(quint64 editor, const QByteArray& json) -> bool = 0;
  virtual void Destroyed(quint64 /*editor*/) {}

 protected:
  /// What the page asked for; false when the editor does not serve it.
  static auto Resolve(quint64 editor, const QString& url, QByteArray* data,
                      QString* mime) -> bool;
  static void Message(quint64 editor, const QByteArray& json);
  static void LinkActivated(quint64 editor, const QString& url);
  static void Ready(quint64 editor);
};

/// Provide @p engine, at activation. @p features: GF_HTML_PROVIDER_FEATURE_*.
auto RegisterWebEditorEngine(const char* name, uint32_t features,
                             std::unique_ptr<WebEditorEngine> engine) -> bool;

}  // namespace gf::ui

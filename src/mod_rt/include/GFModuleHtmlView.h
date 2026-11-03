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
#include <QList>
#include <QString>
#include <functional>
#include <memory>

#include "GFSDKTypes.h"

// Declared, not included: see GFModuleNativeWidget.h.
class QWidget;

/**
 * @file GFModuleHtmlView.h
 * @brief Read-only HTML views, from C++: embedding one, or rendering them.
 *
 * Embedding (capability "html.view"):
 * @code
 * if (gf::ui::HtmlViewAvailability() == gf::ui::HtmlViewStatus::kOK) {
 *   view_.Create(*this, slot_, {.link = [](const QString& url) { ... }});
 *   view_.Load(body, parts);
 * }
 * @endcode
 *
 * `*this` is the module's native widget (GFModuleNativeWidget.h) the slot
 * lives in. The view is the Host's: the module never learns which engine
 * draws it, and is told when it goes away.
 *
 * Rendering (capability "html.provider") is for engine modules: subclass
 * HtmlViewRenderer and register it on activation.
 */

namespace gf::ui {

class NativeWidgetBase;

/// GF_HTML_VIEW_* status.
enum class HtmlViewStatus : int {
  kOK = GF_HTML_VIEW_OK,
  kUNAVAILABLE = GF_HTML_VIEW_E_UNAVAILABLE,
  kRESTART_REQUIRED = GF_HTML_VIEW_E_RESTART_REQUIRED,
  kPOLICY = GF_HTML_VIEW_E_POLICY,
  kDENIED = GF_HTML_VIEW_E_DENIED,
  kINVALID = GF_HTML_VIEW_E_INVALID,
  kTOO_LARGE = GF_HTML_VIEW_E_TOO_LARGE,
  kFAILED = GF_HTML_VIEW_E_FAILED,
};

/// One inline part of the message.
struct HtmlPart {
  QString cid;  ///< the Content-ID, brackets or not
  QString mime;
  QByteArray data;
};

/// Whether a mail view could be created now.
[[nodiscard]] auto HtmlViewAvailability() -> HtmlViewStatus;

/**
 * @brief One HTML view, embedded in a widget of the module's own.
 *
 * Destroying the object destroys the view. GUI thread only.
 */
class HtmlView {
 public:
  struct Handlers {
    std::function<void(bool shown)> loaded;
    /// The user activated a link; the Host has already decided what to do.
    std::function<void(const QString& url)> link;
    /// The renderer went away and took the view with it.
    std::function<void()> lost;
  };

  HtmlView() = default;
  ~HtmlView();
  HtmlView(const HtmlView&) = delete;
  auto operator=(const HtmlView&) -> HtmlView& = delete;

  /// Put a view into @p slot, an empty widget inside @p owner.
  auto Create(const NativeWidgetBase& owner, QWidget* slot, Handlers handlers)
      -> HtmlViewStatus;
  /// Show @p html, with @p parts available to it.
  auto Load(const QString& html, const QList<HtmlPart>& parts,
            int* dropped = nullptr) -> HtmlViewStatus;
  void Clear();
  void Destroy();
  [[nodiscard]] auto IsLive() const -> bool { return view_ != 0; }

 private:
  quint64 view_ = 0;
};

/**
 * @brief An engine, for modules that provide one.
 *
 * Every call is on the GUI thread. A renderer enforces every bit of the
 * policy it is created under with its engine's own mechanisms, and refuses
 * (returns nullptr from Create) when it cannot.
 */
class HtmlViewRenderer {
 public:
  /// How a part request ended: GF_HTML_PART_*, and the bytes when OK.
  using PartDone = std::function<void(int status, const QString& mime,
                                      const QByteArray& data)>;

  virtual ~HtmlViewRenderer() = default;

  virtual auto Create(quint64 view, const GFHtmlViewPolicy& policy)
      -> QWidget* = 0;
  virtual auto Load(quint64 view, const QString& session,
                    const QByteArray& document) -> bool = 0;
  virtual void Clear(quint64 /*view*/) {}
  virtual void Destroyed(quint64 /*view*/) {}

 protected:
  /// Ask for a part; the request id, or 0 when refused. @p done runs later,
  /// never inside this call, and never after CancelPart() or Destroyed().
  static auto RequestPart(quint64 view, const QString& session,
                          const QString& cid, PartDone done) -> quint64;
  static void CancelPart(quint64 request);
  static void LinkActivated(quint64 view, const QString& url);
  static void Loaded(quint64 view, bool ok);
};

/// Provide @p renderer, at activation. @p enforceable: GF_HTML_POLICY_*
/// bits; @p features: GF_HTML_PROVIDER_FEATURE_*.
auto RegisterHtmlViewRenderer(const char* name, uint32_t enforceable,
                              uint32_t features,
                              std::unique_ptr<HtmlViewRenderer> renderer)
    -> bool;

}  // namespace gf::ui

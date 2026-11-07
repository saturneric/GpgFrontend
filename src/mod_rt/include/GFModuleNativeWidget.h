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
#include <QCborMap>
#include <QString>
#include <functional>
#include <optional>
#include <type_traits>

#include "GFSDKTypes.h"

// Declared, not included: this header is part of GFModule.h, which some
// module targets build without QtGui. std::is_base_of needs only the
// derived class complete.
class QFont;
class QWidget;

/**
 * @file GFModuleNativeWidget.h
 * @brief A module's own widgets, for its UI script to mount.
 *
 * @code
 * class InspectorWidget : public QWidget, public gf::ui::DialogWidget { ... };
 *
 * // in on_activate:
 * gf::ui::RegisterNativeWidget<InspectorWidget>(
 *     "inspector", {GC_TR("OpenPGP Inspector"), "", "", "", "", 900, 640},
 *     [](const QCborMap& args) { return new InspectorWidget(); });
 * @endcode
 *
 * @code{.lua}
 * local inspector = ui.mount { id = "inspector", anchor = ui.anchor.dialog {},
 *                              widget = native.widget("inspector") }
 * @endcode
 *
 * Inside the widget and its children, Qt is the module's to use as it
 * likes. What is NOT the module's is anything above it: the container, the
 * Host window, the Host's editor. The widget's parent and window() are
 * whatever the Host chose and are not part of this contract.
 *
 * Each base class is one typed interface -- named methods, never a method
 * found by name at run time -- and its protected notifiers are the only way
 * back to the Host.
 */

namespace gf::ui {

/// Common to the three kinds: which instance the Host made this for.
class NativeWidgetBase {
 public:
  virtual ~NativeWidgetBase() = default;
  [[nodiscard]] auto Instance() const -> quint64 { return instance_; }

 private:
  friend struct NativeWidgetAccess;
  quint64 instance_ = 0;
};

/// The view for a document type, in a Host editor tab.
class DocumentWidget : public NativeWidgetBase {
 public:
  /// The document changed underneath the view -- opened, or replaced by a
  /// crypto result. Show these bytes.
  virtual void Load(const QByteArray& bytes) = 0;
  /// The document as the view has it now; nullopt when nothing changed.
  virtual auto Save() -> std::optional<QByteArray> { return std::nullopt; }
  [[nodiscard]] virtual auto IsDirty() const -> bool { return false; }
  /// GF_CRYPTO_OP_* bits that mean something for this content, or nullopt
  /// for "no opinion" -- every operation is then offered.
  [[nodiscard]] virtual auto CryptoOperations() const
      -> std::optional<uint32_t> {
    return std::nullopt;
  }
  [[nodiscard]] virtual auto SuggestedFileName() const -> QString {
    return {};
  }
  virtual void ApplyVerification(const QByteArray& /*json*/) {}
  virtual auto AppendText(const QString& /*text*/) -> bool { return false; }
  virtual auto AttachPublicKey(const QByteArray& /*key*/,
                               const QString& /*name*/) -> bool {
    return false;
  }
  virtual void ApplyFont(const QFont& /*font*/) {}
  /// The tab is closing: forget everything decrypted or parsed.
  virtual void WipeContent() {}

  /// What PrepareSave() decided.
  struct SaveDecision {
    bool cancelled = false;
    std::optional<QByteArray> bytes;  ///< nullopt: write them unchanged
  };
  /// The Host is about to write @p bytes. Normalise them, or ask the user.
  virtual auto PrepareSave(const QByteArray& /*bytes*/) -> SaveDecision {
    return {};
  }

  /// Why the raw document may not be edited now -- signed bytes, say -- or
  /// nullopt when the user may unlock it. Asked by the Host's source view.
  [[nodiscard]] virtual auto SourceLockReason() const
      -> std::optional<QString> {
    return std::nullopt;
  }

  // --- a document the module persists itself (persistence = "module") ---

  /// Open the file at @p path; the Host has not read it. false: refused.
  virtual auto OpenPath(const QString& /*path*/) -> bool { return false; }
  /// Save, or Save As. true: taken; NotifySaved() says when it is done.
  virtual auto SaveRequested(bool /*save_as*/) -> bool { return false; }
  /// The tab or the application is closing. false keeps it open. May ask.
  virtual auto CloseRequested() -> bool { return true; }
  /// The user asked to find text in the document.
  virtual void FindRequested() {}
  /// The user pressed Encrypt: encrypt the document yourself. true: taken.
  /// Offered while CryptoOperations() includes GF_CRYPTO_OP_ENCRYPT.
  virtual auto EncryptRequested() -> bool { return false; }
  /// The user pressed Decrypt; what that does is the view's. true: taken.
  /// Offered while CryptoOperations() includes GF_CRYPTO_OP_DECRYPT.
  virtual auto DecryptRequested() -> bool { return false; }
  /// The tooltip for the toolbar action of @p op (one GF_CRYPTO_OP_* bit)
  /// when the view gives it its own meaning; nullopt keeps the usual one.
  [[nodiscard]] virtual auto CryptoOperationTip(uint32_t /*op*/) const
      -> std::optional<QString> {
    return std::nullopt;
  }
  /// false while Save and Save As mean nothing for the document.
  [[nodiscard]] virtual auto SaveAllowed() const -> bool { return true; }
  /// The user already chose, in the Host's dialog, to save (@p save) or
  /// discard; do it without asking. true: the tab may close; false: keep
  /// it. nullopt (the default): not handled, CloseRequested() is asked.
  virtual auto CloseDecided(bool /*save*/) -> std::optional<bool> {
    return std::nullopt;
  }
  /// The application is quitting and keeps unsaved tabs for the next start:
  /// bring what Save() returns up to date now, without writing a file or
  /// asking. true: done, the tab is cached and not asked about; false (the
  /// default): the user is asked as for any close.
  virtual auto PrepareRecovery() -> bool { return false; }

 protected:
  /// An edit happened; the tab now has unsaved changes.
  void NotifyModified();
  /// Ask the Host to show the raw document (true) or this view (false).
  void ShowSource(bool source);
  /// Ask the Host to run a crypto operation on this document.
  void RequestCrypto(uint32_t op);
  /// CryptoOperations() would now answer differently.
  void NotifyOpsChanged();
  /// Saved to @p path; the tab now reads @p title and is no longer modified.
  void NotifySaved(const QString& path, const QString& title);
  /// A crypto operation the view ran itself, for the Status Panel.
  struct CryptoReport {
    int channel = 0;
    int operation = 0;   ///< a GFGpgAnalyseOperation
    uint32_t err = 0;    ///< the error the operation returned
    QString capsule_id;  ///< the result's; empty: it never ran (see reason)
    QString input_hash;  ///< SHA-256 of the input, hex
    int output = GF_DOCUMENT_OUTPUT_NOT_PRODUCED;  ///< a GFDocumentOutput
    QString reason;  ///< why it stopped, or the output was not used
  };
  /// The Host analyses the result and reports it as its own Encrypt and
  /// Decrypt are reported. Call it once per operation: it takes the capsule.
  void NotifyResult(const CryptoReport& report);
};

/// A page in the Host's Settings dialog.
class SettingsWidget : public NativeWidgetBase {
 public:
  virtual void LoadSettings() = 0;
  /// Store what the page shows. false: something could not be stored.
  virtual auto ApplySettings() -> bool = 0;

 protected:
  void NotifyRestartNeeded(int level);
};

/// A dialog, in a Host dialog frame, opened with org.gpgfrontend.view.open.
class DialogWidget : public NativeWidgetBase {
 public:
  virtual void Opened(const QCborMap& /*args*/) {}
  /// The user is closing it; false keeps it open.
  virtual auto CloseRequested() -> bool { return true; }

 protected:
  void CloseDialog();
};

/// Presentation, as untranslated source strings (GTrC::Noop). The Host
/// translates them in the registering module's translation context.
struct NativeWidgetMeta {
  const char* title = "";
  const char* keywords = "";  ///< comma-separated, for the Settings search
  const char* suffix = "";    ///< documents: default file suffix, no dot
  const char* filter = "";    ///< documents: file dialog filter
  const char* icon = "";      ///< ":/..."
  int width = 0;
  int height = 0;
};

namespace detail {

/// Build one widget; also report its typed side.
using MakeFn = std::function<QWidget*(const QCborMap&, NativeWidgetBase**)>;

auto RegisterNativeWidget(const char* name, int kind, bool multi,
                          const NativeWidgetMeta& meta, MakeFn make) -> bool;

template <typename W>
constexpr auto KindOf() -> int {
  static_assert(std::is_base_of_v<QWidget, W>,
                "a native widget is a QWidget");
  constexpr int kinds = (std::is_base_of_v<DocumentWidget, W> ? 1 : 0) +
                        (std::is_base_of_v<SettingsWidget, W> ? 1 : 0) +
                        (std::is_base_of_v<DialogWidget, W> ? 1 : 0);
  static_assert(kinds == 1,
                "a native widget implements exactly one of DocumentWidget, "
                "SettingsWidget and DialogWidget");
  if constexpr (std::is_base_of_v<DocumentWidget, W>) return GF_NATIVE_DOCUMENT;
  if constexpr (std::is_base_of_v<SettingsWidget, W>) return GF_NATIVE_SETTINGS;
  return GF_NATIVE_DIALOG;
}

template <typename W, typename F>
auto Make(F factory) -> MakeFn {
  return [factory = std::move(factory)](const QCborMap& args,
                                        NativeWidgetBase** base) -> QWidget* {
    W* w = factory(args);
    *base = w;
    return w;
  };
}

}  // namespace detail

/// A widget with one live instance at a time: a dialog, a settings page.
template <typename W, typename F>
auto RegisterNativeWidget(const char* name, const NativeWidgetMeta& meta,
                          F factory) -> bool {
  return detail::RegisterNativeWidget(name, detail::KindOf<W>(), false, meta,
                                      detail::Make<W>(std::move(factory)));
}

/// A widget with one instance per mount: a document view, one per tab.
template <typename W, typename F>
auto RegisterNativeWidgetFactory(const char* name, const NativeWidgetMeta& meta,
                                 F factory) -> bool {
  return detail::RegisterNativeWidget(name, detail::KindOf<W>(), true, meta,
                                      detail::Make<W>(std::move(factory)));
}

}  // namespace gf::ui

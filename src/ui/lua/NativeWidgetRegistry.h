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

#include <QCborMap>
#include <QMutex>
#include <functional>
#include <optional>

namespace GpgFrontend::UI {

/// The three typed interfaces a native widget may implement.
enum class NativeWidgetKind { kDOCUMENT = 1, kSETTINGS = 2, kDIALOG = 3 };

/// What the Host may ask of a document view. Adapted from the C ops table by
/// the SDK host layer, which enters module code properly for each call.
struct GF_UI_EXPORT NativeDocumentOps {
  std::function<void(quint64, const QByteArray&)> load;
  std::function<std::optional<QByteArray>(quint64)> save;  ///< nullopt: same
  std::function<bool(quint64)> is_dirty;
  std::function<std::optional<uint32_t>(quint64)> crypto_ops;
  std::function<QString(quint64)> suggested_file_name;
  std::function<void(quint64, const QByteArray&)> apply_verification;
  std::function<bool(quint64, const QString&)> append_text;
  std::function<bool(quint64, const QByteArray&, const QString&)>
      attach_public_key;
  std::function<void(quint64, const QString&, int)> apply_font;
  std::function<void(quint64)> wipe_content;
  /// nullopt: the user cancelled. Inner nullopt: write the bytes unchanged.
  std::function<std::optional<std::optional<QByteArray>>(quint64,
                                                         const QByteArray&)>
      prepare_save;
  /// nullopt: the source may be unlocked; otherwise why it may not.
  std::function<std::optional<QString>(quint64)> source_lock;
};

struct GF_UI_EXPORT NativeSettingsOps {
  std::function<void(quint64)> load;
  std::function<bool(quint64)> apply;
};

struct GF_UI_EXPORT NativeDialogOps {
  std::function<void(quint64, const QCborMap&)> opened;
  std::function<bool(quint64)> close_requested;
};

/**
 * @brief A widget a module registered from C++, for its UI script to mount.
 *
 * The module owns the widget and its whole tree; the Host owns the container
 * it is mounted in. What crosses is a module widget pointer, from the module
 * to the Host, and never the other way: the Host hands the module an opaque
 * instance number, not an object of its own.
 */
struct GF_UI_EXPORT NativeWidgetEntry {
  QString owner;  ///< module id
  QString id;     ///< "<module id>.<name>"
  NativeWidgetKind kind = NativeWidgetKind::kDIALOG;
  bool multi_instance = false;  ///< a factory: one widget per mount instance

  // Presentation, untranslated, in the module's "GTrC" context.
  QString title;
  QString keywords;
  QString suffix;
  QString filter;
  QString icon;
  int width = 0;
  int height = 0;

  /// Build the widget for one instance, on the GUI thread. The Host takes
  /// the result as a QWidget and places it in a container of its own.
  std::function<QWidget*(quint64 instance, const QCborMap& args)> create;
  /// The instance is going away; the module forgets it. Optional.
  std::function<void(quint64 instance)> destroyed;

  /// The typed interface matching `kind`; the other two stay empty.
  NativeDocumentOps document;
  NativeSettingsOps settings;
  NativeDialogOps dialog;
};

class GF_UI_EXPORT NativeWidgetRegistry {
 public:
  static auto Instance() -> NativeWidgetRegistry&;

  /// A module registers only inside its own namespace -- its id and one
  /// dotless name -- once per id.
  auto Register(NativeWidgetEntry entry) -> bool;
  auto Unregister(const QString& owner, const QString& id) -> bool;
  [[nodiscard]] auto Find(const QString& id)
      -> std::optional<NativeWidgetEntry>;
  void RemoveAllFor(const QString& owner);

 private:
  QMutex mutex_;
  QHash<QString, NativeWidgetEntry> entries_;
};

}  // namespace GpgFrontend::UI

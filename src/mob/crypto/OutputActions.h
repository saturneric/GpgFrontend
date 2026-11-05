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

#include <QObject>
#include <QtQml/qqmlregistration.h>
#include <chrono>

namespace GpgFrontend::Mobile {

/**
 * @brief Copying and sharing an operation's text, for QML.
 *
 * Sharing reports what the platform could say about it, once, with
 * SignalShared: "handed_off" (a target was chosen), "dismissed" (not
 * confirmed either way), "failed" or "unsupported" -- the last two mean the
 * text went nowhere and the page should offer Copy instead. Nothing falls
 * back to the clipboard by itself.
 */
class OutputActions : public QObject {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(bool canShare READ CanShare CONSTANT)

 public:
  /// How long a sensitive copy stays on the clipboard, at most.
  static constexpr std::chrono::seconds kSensitiveCopyExpiry{60};

  explicit OutputActions(QObject* parent = nullptr);

  [[nodiscard]] auto CanShare() const -> bool;

  /**
   * @brief Copy @p text; when @p sensitive (decrypted text), marked so and
   * cleared after kSensitiveCopyExpiry unless something newer replaced it.
   */
  Q_INVOKABLE void copy(const QString& text, bool sensitive);

  /// Share @p text through the platform's share sheet.
  Q_INVOKABLE void share(const QString& text, const QString& title = {});

  /**
   * @brief Share @p content as a file named @p file_name, of @p mime_type,
   * through the platform's share sheet.
   *
   * The file is written into a directory of its own among the shared files
   * (ShareRetentionStore), so a later share of the same name never replaces
   * one a receiver may still be reading, and it stays readable for the
   * store's time -- not only until the share sheet closes: a receiver may
   * read it later. For content that is not secret (a public key).
   */
  Q_INVOKABLE void shareFile(const QString& content, const QString& file_name,
                             const QString& mime_type);

  /// shareFile()'s first part, for tests: the file kept for sharing, or
  /// empty when it could not be written.
  static auto KeepForSharing(const QByteArray& content,
                             const QString& file_name) -> QString;

 signals:
  void SignalShared(const QString& outcome);
};

}  // namespace GpgFrontend::Mobile

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
#include <QUrl>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>
#include <functional>
#include <optional>

#include "core/model/GFBuffer.h"

namespace GpgFrontend::Mobile {

/**
 * @brief Import keys from text (a pasted armored key block) or from a picked
 * file.
 *
 * Runs the core's KeyImportExportOperation on the GPG task runner and
 * reports {considered, imported, unchanged, notImported, secretImported,
 * fingerprints}, or an empty map when nothing could be read;
 * SignalImported asks the key list to be refreshed.
 */
class KeyImportController : public QObject {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(bool busy READ Busy NOTIFY SignalBusyChanged)
  Q_PROPERTY(QVariantMap result READ Result NOTIFY SignalResultChanged)

 public:
  explicit KeyImportController(QObject* parent = nullptr);

  [[nodiscard]] auto Busy() const -> bool;
  [[nodiscard]] auto Result() const -> QVariantMap;

  Q_INVOKABLE void importText(const QString& text);

  /// Import from @p url: a local file, or a document a picker returned.
  Q_INVOKABLE void importFile(const QUrl& url);

 signals:
  void SignalBusyChanged();
  void SignalResultChanged();
  void SignalFinished(bool imported);

 private:
  bool busy_ = false;
  QVariantMap result_;

  void run(std::function<std::optional<GFBuffer>()> read);
};

}  // namespace GpgFrontend::Mobile

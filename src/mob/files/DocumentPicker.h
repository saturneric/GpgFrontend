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

#include <QtQml/qqmlregistration.h>

#include <QList>
#include <QObject>
#include <QUrl>

namespace GpgFrontend::Mobile {

/**
 * @brief The system's picker for documents to read, where the platform has
 * one of its own (PickDocuments(): Android).
 *
 * Used by OpenDialog.qml, which falls back to Qt Quick's FileDialog where
 * pick() says there is none.
 */
class DocumentPicker : public QObject {
  Q_OBJECT
  QML_ELEMENT

 public:
  explicit DocumentPicker(QObject* parent = nullptr);

  /**
   * @brief Show the picker for one document, or several when @p multiple.
   * picked() follows once it is left, with what was picked (nothing when
   * it was left without a choice).
   * @return false when there is no such picker here; nothing follows.
   */
  Q_INVOKABLE bool pick(bool multiple);

 signals:
  void picked(const QList<QUrl>& files);
};

}  // namespace GpgFrontend::Mobile

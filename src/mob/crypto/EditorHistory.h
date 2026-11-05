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

#include <QObject>
#include <QQuickTextDocument>

class QTextDocument;

namespace GpgFrontend::Mobile {

/**
 * @brief Changes to an editor's text that Undo takes back in one step.
 *
 * Setting a TextArea's text drops its undo history, and removing then
 * inserting makes two steps. replaceAll() does both inside one edit block
 * of the editor's own document, so the history is the one typing uses:
 * Undo brings the text before back, Redo the new one, and a later edit
 * ends the redo as for any edit.
 */
class EditorHistory : public QObject {
  Q_OBJECT
  QML_ELEMENT

 public:
  explicit EditorHistory(QObject* parent = nullptr);

  /// Put @p text in place of everything in @p document, as one undo step.
  /// False when there is no document.
  Q_INVOKABLE bool replaceAll(QQuickTextDocument* document,
                              const QString& text);

  /// Forget @p document's undo and redo history: what was there must not
  /// come back (decrypted text, wiped).
  Q_INVOKABLE bool forget(QQuickTextDocument* document);

  /// The same on a plain QTextDocument.
  static auto ReplaceAll(QTextDocument* document, const QString& text) -> bool;
};

}  // namespace GpgFrontend::Mobile

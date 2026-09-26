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

class QTextDocument;

namespace GpgFrontend::UI {

/**
 * @brief Whether an operation result (encrypt, decrypt, sign, ...) replaces
 * the editor text as one undoable step (basic/undoable_operations, off by
 * default).
 *
 * Off is the safer default: undo history is a second, unwipeable copy of the
 * previous text, and after an encryption that copy is the plaintext.
 *
 * @return bool
 */
auto GF_UI_EXPORT IsOperationUndoEnabled() -> bool;

/**
 * @brief Replace everything in @p doc with @p text.
 *
 * Undoable, it is a single edit block, so one Ctrl+Z brings the previous text
 * back. Otherwise it goes in with undo disabled, which also discards the
 * existing history, as setPlainText() has always done here. Either way the
 * document's undo/redo setting is left as it was.
 *
 * @param doc target document, may be null
 * @param text the new content
 * @param undoable keep the previous text on the undo stack
 */
void GF_UI_EXPORT ReplaceDocumentText(QTextDocument* doc, const QString& text,
                                      bool undoable);

}  // namespace GpgFrontend::UI

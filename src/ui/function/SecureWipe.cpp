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

#include "SecureWipe.h"

#include <QTextDocument>

namespace GpgFrontend::UI {

void WipeTextDocument(QTextDocument* doc) {
  if (doc == nullptr) return;

  // Order matters: with undo still enabled the clear would be recorded, and the
  // secret would come back on Ctrl+Z. Disabling it discards both stacks.
  const bool undo_was_enabled = doc->isUndoRedoEnabled();
  doc->setUndoRedoEnabled(false);

  doc->clear();
  doc->clearUndoRedoStacks();
  doc->setModified(false);

  doc->setUndoRedoEnabled(undo_was_enabled);
}

}  // namespace GpgFrontend::UI

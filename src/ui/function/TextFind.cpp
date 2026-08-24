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

#include "ui/function/TextFind.h"

#include <QTextCursor>
#include <QTextDocument>

namespace GpgFrontend::UI {

auto FindInDocument(QTextDocument* doc, const QTextCursor& from,
                    const QString& needle, bool backward) -> QTextCursor {
  if (doc == nullptr) return from;

  if (needle.isEmpty()) {
    // Drops the highlight and leaves the caret where the match began, so
    // clearing the search box undoes the search instead of freezing its last
    // result on screen.
    auto collapsed = from;
    collapsed.setPosition(from.selectionStart());
    return collapsed;
  }

  QTextDocument::FindFlags flags = QTextDocument::FindCaseSensitively;
  if (backward) flags |= QTextDocument::FindBackward;

  if (const auto found = doc->find(needle, from, flags); !found.isNull()) {
    return found;
  }

  // Wrap around: forwards resumes at the top, backwards at the very end. The
  // end has to be reached through a cursor, because QTextCursor::End passed
  // where an int offset is expected is read as the number 11 rather than as the
  // end of the document.
  QTextCursor edge(doc);
  edge.movePosition(backward ? QTextCursor::End : QTextCursor::Start);

  const auto wrapped = doc->find(needle, edge, flags);
  return wrapped.isNull() ? from : wrapped;
}

}  // namespace GpgFrontend::UI

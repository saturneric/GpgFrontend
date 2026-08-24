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

namespace GpgFrontend::UI {

/**
 * @brief The cursor a find should leave behind.
 *
 * Wraps around the end of the document it started from, and answers the two
 * cases the caller cannot express as a search:
 *
 * An empty @p needle collapses the selection rather than matching nothing.
 * QTextDocument::find() returns a null cursor for an empty string, so a caller
 * that only applies non-null results leaves the previous match highlighted, and
 * emptying a search box appears to keep searching for text that is no longer
 * there.
 *
 * A @p needle that is nowhere in the document leaves @p from untouched, so a
 * search that fails does not strand the caret outside the document.
 *
 * @param doc document to search, may be null
 * @param from cursor to search onwards from; a selection is searched past its
 *             end going forwards and past its start going backwards
 * @param needle text to look for, matched case sensitively
 * @param backward true to search towards the start of the document
 * @return the cursor to apply, never null
 */
auto GF_UI_EXPORT FindInDocument(QTextDocument* doc, const QTextCursor& from,
                                 const QString& needle, bool backward)
    -> QTextCursor;

}  // namespace GpgFrontend::UI

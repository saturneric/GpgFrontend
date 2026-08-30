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

#include "core/profile/ProfilePackage.h"
#include "ui/widgets/MetaListPanel.h"

namespace GpgFrontend::UI {

/**
 * @brief Describe a profile file, in rows, once for the whole application.
 *
 * Every place that puts a package in front of someone -- the Open/Import
 * question, the passphrase prompt, the write-back question -- was describing
 * the same file in its own words, which is how three descriptions of one thing
 * come to disagree. This is that description.
 *
 * The order is the point: what this application established for itself first,
 * then what the file claims about itself under a heading that says so. A fact
 * and a claim shown as one list read as two facts.
 *
 * Deliberately short. Format version, minimum reader and the header digest are
 * left out because nobody deciding whether to type a passphrase can act on
 * them, and protection is left out too now that every file this application
 * writes is sealed -- unless the header claims it is not, which is the one
 * thing about a file worth interrupting someone over.
 *
 * @param file the file itself; need not exist
 * @param header what its header claims, ignored unless @p header_readable
 * @param header_readable whether the header parsed at all
 * @return the rows, facts before claims
 */
auto GF_UI_EXPORT BuildProfilePackageRows(const QFileInfo &file,
                                          const ProfilePackageHeader &header,
                                          bool header_readable)
    -> QVector<MetaListRow>;

/**
 * @brief Describe a file this application is about to write.
 *
 * The mirror of the rows above, for the other direction: where the file goes,
 * and what the volume it goes to has room for.
 *
 * @param file the destination; need not exist yet
 * @param free_bytes what the volume has free, or negative when unknown
 * @return the rows
 */
auto GF_UI_EXPORT BuildProfilePackageDestinationRows(const QFileInfo &file,
                                                     qint64 free_bytes)
    -> QVector<MetaListRow>;

}  // namespace GpgFrontend::UI

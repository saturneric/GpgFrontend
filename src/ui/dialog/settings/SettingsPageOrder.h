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

class QWidget;

namespace GpgFrontend::UI {

/**
 * @brief One settings page, before it has a row in the dialog.
 *
 * Deliberately holds nothing but data: keeping the ordering rules away from the
 * dialog is what lets them be exercised without a QWidget.
 */
struct SettingsPageDescriptor {
  QWidget* page{nullptr};  ///< never dereferenced by the ordering code
  QString title;           ///< sidebar row and page heading
  QString section_id;      ///< canonical section key
  QStringList keywords;    ///< extra search terms, beyond the title
};

/**
 * @brief The canonical section keys, in the order they appear in the sidebar.
 *
 * @return const QStringList& application, keys_engines, features, system
 */
auto GF_UI_EXPORT SettingsSectionOrder() -> const QStringList&;

/**
 * @brief Group @p descriptors by section, following @p canonical_sections.
 *
 * Module pages register while their module activates, long before the Settings
 * dialog exists, so they always arrive after every built-in page. The dialog
 * emits a section header the first time it sees a section, which means an
 * unsorted late arrival would appear under whichever header happened to come
 * last rather than its own. Grouping here makes each section contiguous, so
 * that rule keeps holding.
 *
 * Order within a section is preserved, so registration order stays the
 * tiebreak. Sections missing from @p canonical_sections are appended after the
 * known ones, in the order they were first seen.
 *
 * @param descriptors pages in the order they were collected
 * @param canonical_sections section keys in their intended display order
 * @return QVector<SettingsPageDescriptor> the same pages, grouped
 */
auto GF_UI_EXPORT OrderSettingsPageDescriptors(
    QVector<SettingsPageDescriptor> descriptors,
    const QStringList& canonical_sections) -> QVector<SettingsPageDescriptor>;

}  // namespace GpgFrontend::UI

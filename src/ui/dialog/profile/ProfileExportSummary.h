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
 * @brief One thing a profile file carries.
 *
 * Named rather than keyed by string so that adding an area to the format is a
 * compile error here instead of a row that quietly stops being listed.
 */
enum class ProfileExportArea {
  kSettings,
  kSavedState,
  kProfileKey,  ///< the profile's own application key
  kKeyDatabases,
  kWorkspace,
};

/**
 * @brief One line of the "what goes in" list.
 */
struct ProfileExportContentsRow {
  ProfileExportArea area = ProfileExportArea::kSettings;
  QString label;
  QString icon;  ///< a :/icons/… path
  qint64 bytes = 0;
  /// The workspace is the only area a user chooses; everything else travels
  /// because the file would not be a profile without it.
  bool optional = false;
  bool included = true;
};

/**
 * @brief Everything a profile file will carry, listed.
 *
 * The single place that knows how MeasureProfileAreas()' five keys become rows,
 * so the list on screen and the total under it cannot drift apart — and so that
 * an area cannot go unlisted again. The profile's own key was measured but
 * never shown, which is exactly the thing that makes an unprotected file
 * dangerous.
 *
 * An area measuring zero still gets a row: "empty" and "left out" are different
 * facts, and someone checking this against their own folder needs both.
 *
 * @param areas what MeasureProfileAreas() returned
 * @param include_workspace whether the user asked for their own files too
 * @return one row per area, in a fixed order
 */
auto GF_UI_EXPORT BuildProfileExportContents(const QMap<QString, qint64>& areas,
                                             bool include_workspace)
    -> QVector<ProfileExportContentsRow>;

/**
 * @brief What the rows that travel add up to.
 *
 * Uncompressed: this is the sum of what goes in, not a prediction of the file
 * that comes out, which is gzip'd and therefore unknowable before it is packed.
 *
 * @param rows what BuildProfileExportContents() returned
 * @return total bytes of the included rows
 */
auto GF_UI_EXPORT TotalProfileExportBytes(
    const QVector<ProfileExportContentsRow>& rows) -> qint64;

/**
 * @brief What the user has chosen so far.
 */
struct ProfileExportChoice {
  bool has_destination = false;
  bool include_workspace = false;
  bool protect_with_passphrase = true;
  bool passphrase_acceptable = false;  ///< from EvaluateSecretEntry()
  qint64 total_bytes = 0;
  qint64 free_bytes = -1;  ///< -1 when unknown, or no destination yet
};

/**
 * @brief Something the user should read before pressing the button.
 */
enum class ProfileExportWarning {
  kUnprotected,               ///< the profile's own key travels in the clear
  kUnprotectedWithWorkspace,  ///< …and so do the user's own files
  kMayNotFit,
};

/**
 * @brief Whether the export can go ahead, and what to say either way.
 */
struct ProfileExportReadiness {
  bool can_export = false;
  QVector<ProfileExportWarning> warnings;  ///< most severe first
};

/**
 * @brief Decide whether the choices so far add up to an export.
 *
 * Running short of disk is reported but never blocks. The estimate sums the
 * areas uncompressed while the payload is gzip'd, so "too tight" very often
 * still fits — and the packing has its own refusal, in
 * ProfilePackagePayloadCap(), which is where a real one belongs. This is a
 * courtesy ahead of that, and reads like one.
 *
 * @param choice what has been chosen
 * @return whether to enable the button, and what to warn about
 */
auto GF_UI_EXPORT EvaluateProfileExport(const ProfileExportChoice& choice)
    -> ProfileExportReadiness;

/**
 * @brief Wording for a warning.
 *
 * @param warning what to describe
 * @return translated text
 */
auto GF_UI_EXPORT DescribeProfileExportWarning(ProfileExportWarning warning)
    -> QString;

/**
 * @brief One sentence saying what is about to happen, for just above the
 * button.
 *
 * Immediately before an act with no undo, this puts the user's understanding
 * and the program's actual state side by side in one line — which is what
 * catches "I thought I had ticked the workspace box".
 *
 * @param choice what has been chosen
 * @param destination the file name to name
 * @return the sentence, or empty while there is nothing to say yet
 */
auto GF_UI_EXPORT DescribeProfileExport(const ProfileExportChoice& choice,
                                        const QString& destination) -> QString;

}  // namespace GpgFrontend::UI

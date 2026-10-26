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

#include "ui/widgets/MetaListPanel.h"

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
    -> QList<ProfileExportContentsRow>;

/**
 * @brief What the rows that travel add up to.
 *
 * Uncompressed: this is the sum of what goes in, not a prediction of the file
 * that comes out, which is gzip'd and therefore unknowable before it is packed.
 *
 * @param rows what BuildProfileExportContents() returned
 * @return total bytes of the included rows
 */
auto GF_UI_EXPORT
TotalProfileExportBytes(const QList<ProfileExportContentsRow>& rows) -> qint64;

/**
 * @brief What the user has chosen so far.
 */
struct ProfileExportChoice {
  bool has_destination = false;
  bool include_workspace = false;
  bool passphrase_acceptable = false;  ///< from EvaluateSecretEntry()
  qint64 total_bytes = 0;
  qint64 free_bytes = -1;  ///< -1 when unknown, or no destination yet
};

/**
 * @brief Something the user should read before pressing the button.
 *
 * Only one of these left. A profile file is always sealed with a passphrase
 * now, so the two warnings about handing over an unprotected copy of the
 * profile's own key describe a file this application can no longer write.
 */
enum class ProfileExportWarning {
  kMayNotFit,
};

/**
 * @brief Whether the export can go ahead, and what to say either way.
 */
struct ProfileExportReadiness {
  bool can_export = false;
  QList<ProfileExportWarning> warnings;  ///< most severe first
};

/**
 * @brief Decide whether the choices so far add up to an export.
 *
 * Running short of disk is reported but never blocks. The estimate sums the
 * areas uncompressed while the payload is gzip'd, so "too tight" very often
 * still fits, and the packing itself has no size limit to fail against: a
 * streamed body is never held whole, so an export that genuinely cannot fit
 * fails on the write, where the real refusal belongs. This is a courtesy ahead
 * of that, and reads like one.
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
 * @brief The contents list as rows, total included.
 *
 * The dialog holds no wording of its own: what goes in, what it adds up to and
 * what never travels are decided here, where they can be asserted.
 *
 * Read-only. The workspace is chosen by the one checkbox beside the list, and
 * a second checkbox for the same thing inside it would be two answers to one
 * question.
 *
 * @param rows what BuildProfileExportContents() returned
 * @return the rows to hand a MetaListPanel
 */
auto GF_UI_EXPORT ToMetaListRows(const QList<ProfileExportContentsRow>& rows)
    -> QList<MetaListRow>;

/**
 * @brief How the file is sealed, in one sentence.
 *
 * Named rather than asserted. Someone who knows what these are can check the
 * claim; someone who does not still reads a specific mechanism rather than a
 * reassurance. The keychain is mentioned because its absence is the surprising
 * part. The Argon2id parameters are deliberately left out: they will change,
 * and a number nobody can evaluate is noise.
 *
 * @return translated sentence
 */
auto GF_UI_EXPORT DescribeExportProtection() -> QString;

/**
 * @brief Where the export wizard is.
 */
enum class ProfileExportStep {
  kContents,
  kProtection,
  kSave,
};

/// How many steps the wizard has, for "Step 2 of 3".
constexpr int kProfileExportStepCount = 3;

/**
 * @brief What a step is called in the indicator above it.
 *
 * @param step the step
 * @return translated title
 */
auto GF_UI_EXPORT ProfileExportStepTitle(ProfileExportStep step) -> QString;

/**
 * @brief Whether the primary button may leave a step.
 *
 * Contents has nothing to get wrong. Protection needs a passphrase the panel
 * accepts. Save is the export itself, so it asks EvaluateProfileExport() and
 * nothing else: the wizard cannot be more lenient than the rules it replaces.
 *
 * @param step the step being left
 * @param choice what has been chosen so far
 * @return true when the step may be left forwards
 */
auto GF_UI_EXPORT CanAdvanceProfileExport(ProfileExportStep step,
                                          const ProfileExportChoice& choice)
    -> bool;

/**
 * @brief One line saying what goes in and what it comes to.
 *
 * The detailed list stays one click away; this is the line that answers the
 * question most people opened the dialog with.
 *
 * @param rows what BuildProfileExportContents() returned
 * @return translated sentence
 */
auto GF_UI_EXPORT DescribeProfileExportContents(
    const QList<ProfileExportContentsRow>& rows) -> QString;

/**
 * @brief Whether there are workspace files to offer at all.
 *
 * @param rows what BuildProfileExportContents() returned
 * @return true when the workspace measures more than nothing
 */
auto GF_UI_EXPORT
HasProfileExportWorkspace(const QList<ProfileExportContentsRow>& rows) -> bool;

/**
 * @brief The workspace checkbox's text: its size, or why it is unavailable.
 *
 * @param rows what BuildProfileExportContents() returned
 * @return translated text
 */
auto GF_UI_EXPORT DescribeProfileExportWorkspaceOption(
    const QList<ProfileExportContentsRow>& rows) -> QString;

/**
 * @brief What the last step shows before the button with no undo.
 *
 * Plain words for the protection rather than the cipher's name: the mechanism
 * is on the step that chose it, and here the reader only needs to know that it
 * is in place.
 *
 * The destination is not a row here: it has a field of its own beside the
 * button that chooses it.
 *
 * @param display_name what the profile is called
 * @param choice what has been chosen so far
 * @return the rows to hand a MetaListPanel
 */
auto GF_UI_EXPORT BuildProfileExportReviewRows(
    const QString& display_name, const ProfileExportChoice& choice)
    -> QList<MetaListRow>;

/**
 * @brief The quiet line under the destination: how much room the volume has.
 *
 * @param free_bytes what QStorageInfo reported, or -1 when unknown
 * @return translated text, or empty when there is nothing true to say
 */
auto GF_UI_EXPORT DescribeProfileExportFreeSpace(qint64 free_bytes) -> QString;

/**
 * @brief The rows the "exported" dialog names the written file with.
 *
 * The size is the file's own, read after the write: by now it is a fact, not
 * the uncompressed estimate the wizard could only offer before packing.
 *
 * @param file the package that was written
 * @param file_bytes its size on disk, or -1 when it could not be read
 * @return the rows to hand a MetaListPanel
 */
auto GF_UI_EXPORT BuildProfileExportDoneRows(const QFileInfo& file,
                                             qint64 file_bytes)
    -> QList<MetaListRow>;

}  // namespace GpgFrontend::UI

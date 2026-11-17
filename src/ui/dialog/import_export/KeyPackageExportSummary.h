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

#include "core/function/KeyPackageOperator.h"
#include "ui/widgets/MetaListPanel.h"

namespace GpgFrontend::UI {

/// What a key package is saved as. The import dialog filters on the same two.
inline constexpr auto kKeyPackageExtension = ".gfpack";
inline constexpr auto kKeyPackageKeyFileExtension = ".key";

/**
 * @brief Where the key package wizard is.
 */
enum class KeyPackageExportStep {
  kContents,
  kLocation,    ///< where the package goes, and its key file beside it
  kProtection,  ///< the PIN for the key file, and the export itself
};

/// How many steps the wizard has, for "Step 2 of 3".
constexpr int kKeyPackageExportStepCount = 3;

/**
 * @brief One key the wizard was handed, as much of it as the lists show.
 *
 * A plain struct rather than the key itself, so what the lists say about a key
 * can be asserted without a key database.
 */
struct KeyPackageKeyEntry {
  QString name;
  QString email;
  QString id;
  bool has_private = false;
};

/**
 * @brief What the user has chosen so far.
 *
 * Never the PIN itself: only whether the panel accepts it. A struct that
 * carried the secret would carry it into every rule and every row built here.
 */
struct KeyPackageExportChoice {
  int selected = 0;      ///< keys the wizard was handed
  int with_private = 0;  ///< how many of those have a private part

  /// Drop the keys that have no private part. A filter on the key set, not a
  /// "public only" switch: what is exported of each key is the other option.
  bool only_with_private = false;
  /// Export the private part of every key that has one, beside the public
  /// part every key always contributes.
  bool include_private = false;

  bool pin_acceptable = false;  ///< from SecretEntryPanel::Acceptable()

  QString key_file_path;  ///< where the sealed passphrase goes, as chosen
  QString name;           ///< the package's file name, without extension
  QString location;       ///< the folder the package goes into
  bool location_writable = false;

  /// Write the older version 2 format, for GpgFrontend versions that cannot
  /// read version 3. Only a PIN can protect it.
  bool legacy_format = false;
  /// What protects a version 3 package.
  KeyPackageProtection protection = KeyPackageProtection::kPIN_FILE;
  /// Write the private keys without their own passphrase (version 3 only).
  bool remove_passphrase = false;
  /// Copy gfpipe into the chosen folder too, ready to run: the package is
  /// named "default", and a plain key is saved beside it.
  bool for_gfpipe = false;
};

/**
 * @brief Something that keeps the wizard from going on, in the order the
 * steps ask for things.
 */
enum class KeyPackageExportProblem {
  kNoKeys,
  kInvalidName,
  kNoLocation,
  kLocationNotWritable,
  kNoKeyFile,
  kSameFile,
  kNoPin,
  kGfPipeNeedsNoPin,  ///< gfpipe cannot read a PIN-protected package
};

/**
 * @brief What a step is called in the header above it.
 *
 * @param step the step
 * @return translated title
 */
auto GF_UI_EXPORT KeyPackageExportStepTitle(KeyPackageExportStep step)
    -> QString;

/**
 * @brief Whether a key survives the content filter.
 *
 * @param has_private whether the key has a private part
 * @param choice what has been chosen
 * @return true when the key goes into the package
 */
auto GF_UI_EXPORT KeyPackageKeepsKey(bool has_private,
                                     const KeyPackageExportChoice& choice)
    -> bool;

/**
 * @brief How many keys the package will hold.
 *
 * @param choice what has been chosen
 * @return the count after the filter
 */
auto GF_UI_EXPORT KeyPackageExportCount(const KeyPackageExportChoice& choice)
    -> int;

/**
 * @brief Whether private key material will actually be written.
 *
 * Asking for private keys when none of the selected keys has one writes none,
 * and the summary must not claim otherwise.
 *
 * @param choice what has been chosen
 * @return true when at least one private part goes in
 */
auto GF_UI_EXPORT
KeyPackageIncludesPrivate(const KeyPackageExportChoice& choice) -> bool;

/**
 * @brief What actually protects the package: a version 2 package is always
 * protected by a PIN, whatever was picked for version 3.
 * @param choice what has been chosen
 * @return the protection that will be written
 */
auto GF_UI_EXPORT KeyPackageEffectiveProtection(
    const KeyPackageExportChoice& choice) -> KeyPackageProtection;

/**
 * @brief Whether the package is written with a separate key file.
 * @param choice what has been chosen
 * @return true for a PIN-protected package
 */
auto GF_UI_EXPORT KeyPackageUsesKeyFile(const KeyPackageExportChoice& choice)
    -> bool;

/**
 * @brief Whether private keys are written without their passphrase.
 * Only for version 3, and only when private keys go in at all.
 * @param choice what has been chosen
 * @return true when the passphrases are removed
 */
auto GF_UI_EXPORT
KeyPackageRemovesPassphrase(const KeyPackageExportChoice& choice) -> bool;

/**
 * @brief What one protection means, as one sentence beside its option.
 * @param protection the protection
 * @return translated sentence
 */
auto GF_UI_EXPORT
DescribeKeyPackageProtectionOption(KeyPackageProtection protection) -> QString;

/**
 * @brief Whether "Include private keys" can be offered at all.
 *
 * @param choice what has been chosen
 * @return true when a selected key has a private part
 */
auto GF_UI_EXPORT CanIncludePrivateKeys(const KeyPackageExportChoice& choice)
    -> bool;

/**
 * @brief Whether "Only export keys with a private key" would change anything.
 *
 * Offered only when it can neither empty the set nor be a no-op: some selected
 * keys have a private part and some do not.
 *
 * @param choice what has been chosen
 * @return true when the filter is worth offering
 */
auto GF_UI_EXPORT CanFilterToPrivateKeys(const KeyPackageExportChoice& choice)
    -> bool;

/**
 * @brief Whether a package name can be a file name on every platform.
 *
 * @param name what was typed
 * @return true when it is usable
 */
auto GF_UI_EXPORT IsValidKeyPackageName(const QString& name) -> bool;

/**
 * @brief A path with its extension, appended only when missing.
 *
 * @param path what the picker returned
 * @param extension kKeyPackageExtension or kKeyPackageKeyFileExtension
 * @return the path the file will be written to
 */
auto GF_UI_EXPORT WithKeyPackageExtension(const QString& path,
                                          const QString& extension) -> QString;

/**
 * @brief Where the package will be written.
 *
 * @param choice what has been chosen
 * @return the absolute path, or empty while the name or location is missing
 */
auto GF_UI_EXPORT KeyPackagePath(const KeyPackageExportChoice& choice)
    -> QString;

/**
 * @brief Where the key file goes unless the user puts it somewhere else:
 * beside the package, under the package's name.
 *
 * @param choice what has been chosen
 * @return the path, or empty while the name or location is missing
 */
auto GF_UI_EXPORT
DefaultKeyPackageKeyFilePath(const KeyPackageExportChoice& choice) -> QString;

/**
 * @brief What keeps a step from being left forwards, most costly first.
 *
 * Each step only answers for itself and the steps before it, so the last one
 * cannot be more lenient than the first.
 *
 * @param step the step being left
 * @param choice what has been chosen
 * @return the problems; empty when the step may be left
 */
auto GF_UI_EXPORT KeyPackageExportProblems(KeyPackageExportStep step,
                                           const KeyPackageExportChoice& choice)
    -> QList<KeyPackageExportProblem>;

/**
 * @brief Whether the primary button may leave a step.
 *
 * @param step the step being left
 * @param choice what has been chosen
 * @return true when nothing blocks it
 */
auto GF_UI_EXPORT CanAdvanceKeyPackageExport(
    KeyPackageExportStep step, const KeyPackageExportChoice& choice) -> bool;

/**
 * @brief Wording for a problem.
 *
 * @param problem what to describe
 * @return translated text
 */
auto GF_UI_EXPORT
DescribeKeyPackageExportProblem(KeyPackageExportProblem problem) -> QString;

/**
 * @brief One line saying how many keys there are and how many are private.
 *
 * @param choice what has been chosen
 * @return translated sentence
 */
auto GF_UI_EXPORT
DescribeKeyPackageContents(const KeyPackageExportChoice& choice) -> QString;

/**
 * @brief Why the content options are greyed out, when they are.
 *
 * @param choice what has been chosen
 * @return translated sentence, or empty while both can be offered
 */
auto GF_UI_EXPORT DescribeKeyPackageContentLimits(
    const KeyPackageExportChoice& choice) -> QString;

/**
 * @brief One line of the key table on the first step.
 */
struct KeyPackageKeyRow {
  QString name;
  QString email;
  QString id;
  QString exported;  ///< what of the key goes in, or why it does not
  bool left_out = false;
};

/**
 * @brief The key table's column headings, in the order of KeyPackageKeyRow.
 *
 * @return translated headings
 */
auto GF_UI_EXPORT KeyPackageKeyColumns() -> QStringList;

/**
 * @brief The keys as table rows, each saying what of it goes in.
 *
 * Keys the filter drops stay listed and marked: "left out" is a fact someone
 * checking the table against their keyring needs to see.
 *
 * @param keys the keys the wizard was handed
 * @param choice what has been chosen
 * @return one row per key, in the order given
 */
auto GF_UI_EXPORT BuildKeyPackageKeyRows(const QList<KeyPackageKeyEntry>& keys,
                                         const KeyPackageExportChoice& choice)
    -> QList<KeyPackageKeyRow>;

/**
 * @brief What the last step shows before the button with no undo.
 *
 * Says whether a PIN is set, never anything about it.
 *
 * @param choice what has been chosen
 * @return the rows to hand a MetaListPanel
 */
auto GF_UI_EXPORT BuildKeyPackageReviewRows(
    const KeyPackageExportChoice& choice) -> QList<MetaListRow>;

/**
 * @brief The rows the "exported" dialog names the written files with.
 *
 * @param package_path the package that was written
 * @param key_path the key file that was written; empty when there is none
 * @param gfpipe_path the gfpipe copied beside it; empty when none was
 * @param key_text_path the plain key saved for gfpipe; empty when none was
 * @return the rows to hand a MetaListPanel
 */
auto GF_UI_EXPORT BuildKeyPackageExportDoneRows(
    const QString& package_path, const QString& key_path,
    const QString& gfpipe_path = {}, const QString& key_text_path = {})
    -> QList<MetaListRow>;

/**
 * @brief How the package is sealed, as one sentence for the curious.
 *
 * @param choice what has been chosen
 * @return translated sentence
 */
auto GF_UI_EXPORT
DescribeKeyPackageProtection(const KeyPackageExportChoice& choice) -> QString;

}  // namespace GpgFrontend::UI

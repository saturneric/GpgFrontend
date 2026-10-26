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

/// Largest key package the import reads. A package holds exported keys, which
/// are small; anything this big is something else.
inline constexpr qint64 kMaxKeyPackageBytes = 32LL * 1024 * 1024;

/// Largest key file the import reads. It holds one sealed passphrase.
inline constexpr qint64 kMaxKeyPackageKeyFileBytes = 1024LL * 1024;

/**
 * @brief Where the key package import wizard is.
 */
enum class KeyPackageImportStep {
  kFiles,   ///< the package and its key file
  kUnlock,  ///< the PIN, which opens the package
  kKeys,    ///< the keys inside, which to import, and the import itself
};

/// How many steps the wizard has, for "Step 2 of 3".
constexpr int kKeyPackageImportStepCount = 3;

/**
 * @brief What is known about one chosen file, read from disk by the dialog.
 *
 * A plain struct so the rules can be asserted without touching the disk.
 */
struct KeyPackageImportFile {
  QString path;
  bool is_file = false;   ///< exists and is a regular file
  bool readable = false;  ///< can be opened for reading
  qint64 bytes = 0;
};

/**
 * @brief What the user has chosen so far. Never the PIN itself.
 */
struct KeyPackageImportChoice {
  KeyPackageImportFile package;
  KeyPackageImportFile key_file;
  bool pin_entered = false;
  int keys_selected = 0;  ///< ticked on the last step
};

/**
 * @brief One key found in the opened package, and what this key database
 * already holds of it.
 */
struct KeyPackageImportKey {
  KeyPackageKey key;
  bool present = false;          ///< the key is already in the key database
  bool present_private = false;  ///< ...with its private key
};

/**
 * @brief One line of the key table on the last step.
 */
struct KeyPackageImportKeyRow {
  QString name;
  QString email;
  QString id;
  /// What importing it would bring here, private key included or not.
  QString status;
  bool already_present = false;  ///< nothing new but updates, at most
};

/**
 * @brief Something that keeps the wizard from going on, in the order the
 * steps ask for things.
 */
enum class KeyPackageImportProblem {
  kNoPackage,
  kPackageUnreadable,
  kPackageTooLarge,
  kNoKeyFile,
  kKeyFileUnreadable,
  kKeyFileTooLarge,
  kSameFile,
  kNoPin,
  kNoKeysSelected,
};

/**
 * @brief Why an import attempt failed, in words, and whether retyping the PIN
 * could fix it.
 */
struct KeyPackageImportFailure {
  QString message;
  bool retry_pin = false;  ///< the PIN, or the pairing of the files, is wrong
};

/**
 * @brief What a step is called in the header above it.
 *
 * @param step the step
 * @return translated title
 */
auto GF_UI_EXPORT KeyPackageImportStepTitle(KeyPackageImportStep step)
    -> QString;

/**
 * @brief Where an export would have put the key file for this package: beside
 * it, under the same name.
 *
 * @param package_path the chosen package
 * @return the sibling key file path, or empty for an empty package path
 */
auto GF_UI_EXPORT KeyPackageImportKeyFileFor(const QString& package_path)
    -> QString;

/**
 * @brief What keeps a step from being left forwards, in the order they are
 * asked for.
 *
 * @param step the step being left
 * @param choice what has been chosen
 * @return the problems; empty when the step may be left
 */
auto GF_UI_EXPORT KeyPackageImportProblems(KeyPackageImportStep step,
                                           const KeyPackageImportChoice& choice)
    -> QList<KeyPackageImportProblem>;

/**
 * @brief Whether the primary button may leave a step.
 *
 * @param step the step being left
 * @param choice what has been chosen
 * @return true when nothing blocks it
 */
auto GF_UI_EXPORT CanAdvanceKeyPackageImport(
    KeyPackageImportStep step, const KeyPackageImportChoice& choice) -> bool;

/**
 * @brief Wording for a problem.
 *
 * @param problem what to describe
 * @return translated text
 */
auto GF_UI_EXPORT
DescribeKeyPackageImportProblem(KeyPackageImportProblem problem) -> QString;

/**
 * @brief Turn what KeyPackageOperator::ImportKeyPackage() reported into words.
 *
 * The operator reports in untranslated English, one string per stage. A key
 * file that does not open with the PIN is the common case, and the one the
 * user can fix on the spot, so it is told apart from the rest.
 *
 * @param error the operator's message
 * @return the message to show, and whether to ask for the PIN again
 */
auto GF_UI_EXPORT DescribeKeyPackageImportError(const QString& error)
    -> KeyPackageImportFailure;

/**
 * @brief The key table's column headings, in the order of
 * KeyPackageImportKeyRow, after the checkbox.
 *
 * @return translated headings
 */
auto GF_UI_EXPORT KeyPackageImportKeyColumns() -> QStringList;

/**
 * @brief The keys in the package as table rows.
 *
 * Says, in one column, what importing each key would bring here and whether
 * that includes a private key, so a key that is already present in full can
 * be left unticked knowingly.
 *
 * @param keys the keys found, with what is already here
 * @return one row per key, in the order given
 */
auto GF_UI_EXPORT BuildKeyPackageImportKeyRows(
    const QList<KeyPackageImportKey>& keys) -> QList<KeyPackageImportKeyRow>;

/**
 * @brief One line saying how many keys are ticked.
 *
 * @param selected how many are ticked
 * @param total how many the package holds
 * @return translated sentence
 */
auto GF_UI_EXPORT DescribeKeyPackageImportSelection(int selected, int total)
    -> QString;

/**
 * @brief What the unlock step shows above the button.
 *
 * @param choice what has been chosen
 * @return the rows to hand a MetaListPanel
 */
auto GF_UI_EXPORT BuildKeyPackageImportReviewRows(
    const KeyPackageImportChoice& choice) -> QList<MetaListRow>;

}  // namespace GpgFrontend::UI

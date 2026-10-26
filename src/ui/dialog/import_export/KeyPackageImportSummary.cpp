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

#include "ui/dialog/import_export/KeyPackageImportSummary.h"

#include "ui/dialog/import_export/KeyPackageExportSummary.h"

namespace GpgFrontend::UI {

namespace {

auto SamePath(const QString& a, const QString& b) -> bool {
#if defined(Q_OS_WINDOWS) || defined(Q_OS_MACOS)
  constexpr auto kCase = Qt::CaseInsensitive;
#else
  constexpr auto kCase = Qt::CaseSensitive;
#endif
  return QDir::cleanPath(a).compare(QDir::cleanPath(b), kCase) == 0;
}

}  // namespace

auto KeyPackageImportStepTitle(KeyPackageImportStep step) -> QString {
  switch (step) {
    case KeyPackageImportStep::kFiles:
      return QObject::tr("Files");
    case KeyPackageImportStep::kUnlock:
      return QObject::tr("Unlock");
    case KeyPackageImportStep::kKeys:
      return QObject::tr("Keys");
  }
  return {};
}

auto KeyPackageImportKeyFileFor(const QString& package_path) -> QString {
  if (package_path.isEmpty()) return {};

  auto base = package_path;
  if (base.endsWith(kKeyPackageExtension, Qt::CaseInsensitive)) {
    base.chop(static_cast<int>(qstrlen(kKeyPackageExtension)));
  }
  return base + kKeyPackageKeyFileExtension;
}

auto KeyPackageImportProblems(KeyPackageImportStep step,
                              const KeyPackageImportChoice& choice)
    -> QList<KeyPackageImportProblem> {
  QList<KeyPackageImportProblem> problems;

  const auto& package = choice.package;
  if (package.path.isEmpty()) {
    problems.append(KeyPackageImportProblem::kNoPackage);
  } else if (!package.is_file || !package.readable) {
    problems.append(KeyPackageImportProblem::kPackageUnreadable);
  } else if (package.bytes > kMaxKeyPackageBytes) {
    problems.append(KeyPackageImportProblem::kPackageTooLarge);
  }

  const auto& key_file = choice.key_file;
  if (key_file.path.isEmpty()) {
    problems.append(KeyPackageImportProblem::kNoKeyFile);
  } else if (!key_file.is_file || !key_file.readable) {
    problems.append(KeyPackageImportProblem::kKeyFileUnreadable);
  } else if (key_file.bytes > kMaxKeyPackageKeyFileBytes) {
    problems.append(KeyPackageImportProblem::kKeyFileTooLarge);
  }

  if (!package.path.isEmpty() && !key_file.path.isEmpty() &&
      SamePath(package.path, key_file.path)) {
    problems.append(KeyPackageImportProblem::kSameFile);
  }
  if (step == KeyPackageImportStep::kFiles) return problems;

  if (!choice.pin_entered) problems.append(KeyPackageImportProblem::kNoPin);
  if (step == KeyPackageImportStep::kUnlock) return problems;

  if (choice.keys_selected <= 0) {
    problems.append(KeyPackageImportProblem::kNoKeysSelected);
  }
  return problems;
}

auto CanAdvanceKeyPackageImport(KeyPackageImportStep step,
                                const KeyPackageImportChoice& choice) -> bool {
  return KeyPackageImportProblems(step, choice).isEmpty();
}

auto DescribeKeyPackageImportProblem(KeyPackageImportProblem problem)
    -> QString {
  switch (problem) {
    case KeyPackageImportProblem::kNoPackage:
      return QObject::tr("Choose the key package to import.");
    case KeyPackageImportProblem::kPackageUnreadable:
      return QObject::tr(
          "The key package cannot be read. Make sure it is a regular file you "
          "can open.");
    case KeyPackageImportProblem::kPackageTooLarge:
      return QObject::tr("This file is too large to be a key package.");
    case KeyPackageImportProblem::kNoKeyFile:
      return QObject::tr("Choose the key file that came with the package.");
    case KeyPackageImportProblem::kKeyFileUnreadable:
      return QObject::tr(
          "The key file cannot be read. Make sure it is a regular file you can "
          "open.");
    case KeyPackageImportProblem::kKeyFileTooLarge:
      return QObject::tr("This file is too large to be a key file.");
    case KeyPackageImportProblem::kSameFile:
      return QObject::tr(
          "The key package and the key file are the same file. Choose the key "
          "file that came with the package.");
    case KeyPackageImportProblem::kNoPin:
      return QObject::tr("Enter the PIN that protects the key file.");
    case KeyPackageImportProblem::kNoKeysSelected:
      return QObject::tr("Tick at least one key to import.");
  }
  return {};
}

auto DescribeKeyPackageImportError(const QString& error)
    -> KeyPackageImportFailure {
  // The operator's own strings, stage by stage. Matched exactly: a reworded
  // stage falls through to the generic message rather than to a wrong one.
  if (error == u"Decrypt the Key of Key Package Failed"_s) {
    return {QObject::tr("The PIN is wrong, or this key file belongs to another "
                        "package. Check both and try again."),
            true};
  }
  if (error == u"Decrypt the Key Package Failed"_s) {
    return {QObject::tr("The key file opened, but it does not belong to this "
                        "package. Choose the key file exported with it."),
            false};
  }
  if (error == u"Read Key Package Failed"_s) {
    return {QObject::tr("The key package could not be read."), false};
  }
  if (error == u"Read the Key of Key Package Failed"_s) {
    return {QObject::tr("The key file could not be read."), false};
  }
  if (error == u"No Selected Key Found in the Key Package"_s) {
    return {QObject::tr("None of the ticked keys could be read from the "
                        "package."),
            false};
  }
  if (error == u"Import Key Data in the Key Package Failed"_s) {
    return {QObject::tr("The package opened, but its keys could not be "
                        "imported into this key database."),
            false};
  }
  return {QObject::tr("The key package could not be imported."), false};
}

auto KeyPackageImportKeyColumns() -> QStringList {
  return {QObject::tr("Name"), QObject::tr("Email"), QObject::tr("Key ID"),
          QObject::tr("Status")};
}

auto BuildKeyPackageImportKeyRows(const QList<KeyPackageImportKey>& keys)
    -> QList<KeyPackageImportKeyRow> {
  QList<KeyPackageImportKeyRow> rows;

  for (const auto& found : keys) {
    const auto& key = found.key;
    KeyPackageImportKeyRow row;
    row.name = key.name;
    row.email = key.email;
    row.id = key.key_id;

    if (!found.present) {
      row.status = key.has_secret ? QObject::tr("New, with private key")
                                  : QObject::tr("New");
    } else if (key.has_secret && !found.present_private) {
      row.status = QObject::tr("Adds the private key");
    } else {
      // Still offered: importing it again may bring newer signatures or
      // user IDs.
      row.status = QObject::tr("Already here");
      row.already_present = true;
    }

    rows.append(row);
  }

  return rows;
}

auto DescribeKeyPackageImportSelection(int selected, int total) -> QString {
  // Spelled out per number instead of a "%n key(s)" plural: without a
  // translator loaded Qt keeps the literal "(s)".
  if (total == 1) {
    return selected == 1 ? QObject::tr("The package holds 1 key; it is ticked.")
                         : QObject::tr(
                               "The package holds 1 key; it is not "
                               "ticked.");
  }
  return QObject::tr("%1 of %2 keys ticked.").arg(selected).arg(total);
}

auto BuildKeyPackageImportReviewRows(const KeyPackageImportChoice& choice)
    -> QList<MetaListRow> {
  const auto row = [](const QString& caption, const QString& path) {
    MetaListRow r;
    r.caption = caption;
    r.value = path.isEmpty() ? QObject::tr("Not chosen yet")
                             : QDir::toNativeSeparators(path);
    r.dimmed = path.isEmpty();
    r.path = !path.isEmpty();
    return r;
  };

  return {row(QObject::tr("Key package"), choice.package.path),
          row(QObject::tr("Key file"), choice.key_file.path)};
}

}  // namespace GpgFrontend::UI

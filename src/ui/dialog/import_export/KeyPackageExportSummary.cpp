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

#include "ui/dialog/import_export/KeyPackageExportSummary.h"

namespace GpgFrontend::UI {

namespace {

/// Characters no file name may carry on at least one supported platform.
constexpr std::array kForbiddenNameCharacters = {
    u'/', u'\\', u':', u'*', u'?', u'"', u'<', u'>', u'|',
};

/// Past this a name is no longer something a person typed on purpose, and
/// some file systems start refusing it.
constexpr int kMaxNameLength = 200;

auto SamePath(const QString& a, const QString& b) -> bool {
#if defined(Q_OS_WINDOWS) || defined(Q_OS_MACOS)
  constexpr auto kCase = Qt::CaseInsensitive;
#else
  constexpr auto kCase = Qt::CaseSensitive;
#endif
  return QDir::cleanPath(a).compare(QDir::cleanPath(b), kCase) == 0;
}

/// Spelled out per number instead of a "%n key(s)" plural: without a translator
/// loaded Qt keeps the literal "(s)".
auto DescribeKeyCount(int count) -> QString {
  return count == 1 ? QObject::tr("1 key") : QObject::tr("%1 keys").arg(count);
}

}  // namespace

auto KeyPackageExportStepTitle(KeyPackageExportStep step) -> QString {
  switch (step) {
    case KeyPackageExportStep::kContents:
      return QObject::tr("Contents");
    case KeyPackageExportStep::kLocation:
      return QObject::tr("Location");
    case KeyPackageExportStep::kProtection:
      return QObject::tr("Protection");
  }
  return {};
}

auto KeyPackageKeepsKey(bool has_private, const KeyPackageExportChoice& choice)
    -> bool {
  return has_private || !choice.only_with_private;
}

auto KeyPackageExportCount(const KeyPackageExportChoice& choice) -> int {
  return choice.only_with_private ? choice.with_private : choice.selected;
}

auto KeyPackageIncludesPrivate(const KeyPackageExportChoice& choice) -> bool {
  return choice.include_private && choice.with_private > 0;
}

auto KeyPackageEffectiveProtection(const KeyPackageExportChoice& choice)
    -> KeyPackageProtection {
  return choice.legacy_format ? KeyPackageProtection::kPIN_FILE
                              : choice.protection;
}

auto KeyPackageUsesKeyFile(const KeyPackageExportChoice& choice) -> bool {
  return KeyPackageEffectiveProtection(choice) ==
         KeyPackageProtection::kPIN_FILE;
}

auto KeyPackageRemovesPassphrase(const KeyPackageExportChoice& choice) -> bool {
  return choice.remove_passphrase && !choice.legacy_format &&
         KeyPackageIncludesPrivate(choice);
}

auto DescribeKeyPackageProtectionOption(KeyPackageProtection protection)
    -> QString {
  switch (protection) {
    case KeyPackageProtection::kPIN_FILE:
      return QObject::tr(
          "Recommended for moving keys to another device. Importing needs the "
          "package, its key file and the PIN.");
    case KeyPackageProtection::kPLAIN:
      return QObject::tr(
          "Opens with a key shown once after the export, for an environment "
          "variable or a CI secret. Anyone with the key and the package has "
          "the keys.");
  }
  return {};
}

auto CanIncludePrivateKeys(const KeyPackageExportChoice& choice) -> bool {
  return choice.with_private > 0;
}

auto CanFilterToPrivateKeys(const KeyPackageExportChoice& choice) -> bool {
  return choice.with_private > 0 && choice.with_private < choice.selected;
}

auto IsValidKeyPackageName(const QString& name) -> bool {
  if (name.isEmpty() || name.size() > kMaxNameLength) return false;
  if (name == "." || name == "..") return false;

  // Windows drops a trailing dot or space without saying so, and a leading
  // space is a name nobody can tell apart from the one without it.
  if (name != name.trimmed() || name.endsWith('.')) return false;

  return std::none_of(name.begin(), name.end(), [](QChar c) {
    return c.unicode() < 0x20 ||
           std::find(kForbiddenNameCharacters.begin(),
                     kForbiddenNameCharacters.end(),
                     c.unicode()) != kForbiddenNameCharacters.end();
  });
}

auto WithKeyPackageExtension(const QString& path, const QString& extension)
    -> QString {
  if (path.isEmpty()) return {};
  return path.endsWith(extension, Qt::CaseInsensitive) ? path
                                                       : path + extension;
}

auto KeyPackagePath(const KeyPackageExportChoice& choice) -> QString {
  if (choice.location.isEmpty() || !IsValidKeyPackageName(choice.name)) {
    return {};
  }
  return QDir(choice.location)
      .filePath(WithKeyPackageExtension(choice.name, kKeyPackageExtension));
}

auto DefaultKeyPackageKeyFilePath(const KeyPackageExportChoice& choice)
    -> QString {
  if (choice.location.isEmpty() || !IsValidKeyPackageName(choice.name)) {
    return {};
  }
  return QDir(choice.location)
      .filePath(
          WithKeyPackageExtension(choice.name, kKeyPackageKeyFileExtension));
}

auto KeyPackageExportProblems(KeyPackageExportStep step,
                              const KeyPackageExportChoice& choice)
    -> QList<KeyPackageExportProblem> {
  QList<KeyPackageExportProblem> problems;

  if (KeyPackageExportCount(choice) <= 0) {
    problems.append(KeyPackageExportProblem::kNoKeys);
  }
  if (step == KeyPackageExportStep::kContents) return problems;

  if (!IsValidKeyPackageName(choice.name)) {
    problems.append(KeyPackageExportProblem::kInvalidName);
  }
  if (choice.location.isEmpty()) {
    problems.append(KeyPackageExportProblem::kNoLocation);
  } else if (!choice.location_writable) {
    problems.append(KeyPackageExportProblem::kLocationNotWritable);
  }
  const auto uses_key_file = KeyPackageUsesKeyFile(choice);
  if (uses_key_file && choice.key_file_path.isEmpty()) {
    problems.append(KeyPackageExportProblem::kNoKeyFile);
  }

  // The two files are the two halves of the lock. Written to one path, the
  // second write would replace the first and leave nothing to import.
  const auto package = KeyPackagePath(choice);
  if (uses_key_file && !package.isEmpty() && !choice.key_file_path.isEmpty() &&
      SamePath(package, choice.key_file_path)) {
    problems.append(KeyPackageExportProblem::kSameFile);
  }
  if (step == KeyPackageExportStep::kLocation) return problems;

  switch (KeyPackageEffectiveProtection(choice)) {
    case KeyPackageProtection::kPIN_FILE:
      if (choice.for_gfpipe) {
        problems.append(KeyPackageExportProblem::kGfPipeNeedsNoPin);
      } else if (!choice.pin_acceptable) {
        problems.append(KeyPackageExportProblem::kNoPin);
      }
      break;
    case KeyPackageProtection::kPLAIN:
      break;
  }

  return problems;
}

auto CanAdvanceKeyPackageExport(KeyPackageExportStep step,
                                const KeyPackageExportChoice& choice) -> bool {
  return KeyPackageExportProblems(step, choice).isEmpty();
}

auto DescribeKeyPackageExportProblem(KeyPackageExportProblem problem)
    -> QString {
  switch (problem) {
    case KeyPackageExportProblem::kNoKeys:
      return QObject::tr("There are no keys to export.");
    case KeyPackageExportProblem::kNoPin:
      return QObject::tr("Choose a PIN to protect the key file.");
    case KeyPackageExportProblem::kNoKeyFile:
      return QObject::tr("Choose where to save the key file.");
    case KeyPackageExportProblem::kInvalidName:
      return QObject::tr(
          "Enter a package name. It cannot contain / \\ : * ? \" < > | or end "
          "with a dot or a space.");
    case KeyPackageExportProblem::kNoLocation:
      return QObject::tr("Choose where to save the key package.");
    case KeyPackageExportProblem::kLocationNotWritable:
      return QObject::tr("This folder cannot be written to. Choose another.");
    case KeyPackageExportProblem::kSameFile:
      return QObject::tr(
          "The key package and the key file would be the same file. Choose a "
          "different name or location for one of them.");
    case KeyPackageExportProblem::kGfPipeNeedsNoPin:
      return QObject::tr(
          "gfpipe cannot open a package protected by a PIN. Choose a plain "
          "key.");
  }
  return {};
}

auto DescribeKeyPackageContents(const KeyPackageExportChoice& choice)
    -> QString {
  const auto selected =
      choice.selected == 1
          ? QObject::tr("1 key selected")
          : QObject::tr("%1 keys selected").arg(choice.selected);

  if (choice.with_private == 0) {
    return QObject::tr("%1, none with a private key.").arg(selected);
  }
  if (choice.with_private == choice.selected) {
    return choice.selected == 1
               ? QObject::tr("%1, with its private key.").arg(selected)
               : QObject::tr("%1, all with a private key.").arg(selected);
  }
  return QObject::tr("%1, %2 with a private key.")
      .arg(selected)
      .arg(choice.with_private);
}

auto DescribeKeyPackageContentLimits(const KeyPackageExportChoice& choice)
    -> QString {
  if (CanIncludePrivateKeys(choice)) return {};
  return QObject::tr(
      "None of the selected keys has a private key, so only public keys can "
      "be exported.");
}

auto KeyPackageKeyColumns() -> QStringList {
  return {QObject::tr("Name"), QObject::tr("Email"), QObject::tr("Key ID"),
          QObject::tr("Exported")};
}

auto BuildKeyPackageKeyRows(const QList<KeyPackageKeyEntry>& keys,
                            const KeyPackageExportChoice& choice)
    -> QList<KeyPackageKeyRow> {
  QList<KeyPackageKeyRow> rows;

  for (const auto& key : keys) {
    KeyPackageKeyRow row{key.name, key.email, key.id, {}, false};

    if (!KeyPackageKeepsKey(key.has_private, choice)) {
      row.exported = QObject::tr("Left out: no private key");
      row.left_out = true;
    } else if (key.has_private && KeyPackageIncludesPrivate(choice)) {
      row.exported = QObject::tr("Public and private");
    } else {
      row.exported = QObject::tr("Public");
    }

    rows.append(row);
  }

  return rows;
}

auto BuildKeyPackageReviewRows(const KeyPackageExportChoice& choice)
    -> QList<MetaListRow> {
  QList<MetaListRow> rows;

  const auto count = KeyPackageExportCount(choice);
  rows.append(
      {.caption = QObject::tr("Keys"), .value = DescribeKeyCount(count)});

  const auto with_private = KeyPackageIncludesPrivate(choice);
  rows.append({.caption = QObject::tr("Private keys"),
               .value = with_private ? QObject::tr("Included")
                                     : QObject::tr("Not included"),
               .danger = with_private});

  if (with_private && !choice.legacy_format) {
    const auto removed = KeyPackageRemovesPassphrase(choice);
    rows.append({.caption = QObject::tr("Passphrases"),
                 .value = removed ? QObject::tr("Removed from the private keys")
                                  : QObject::tr("Kept"),
                 .danger = removed});
  }

  // Never a row that reads as protected when it is not: the rows are built
  // from the choice, not from where the wizard happens to be.
  QString protection;
  bool sealed = false;
  switch (KeyPackageEffectiveProtection(choice)) {
    case KeyPackageProtection::kPIN_FILE:
      sealed = choice.pin_acceptable && !choice.key_file_path.isEmpty() &&
               !choice.for_gfpipe;
      protection = sealed ? QObject::tr("Key file protected by your PIN")
                          : QObject::tr("Not set up yet");
      break;
    case KeyPackageProtection::kPLAIN:
      sealed = true;
      protection =
          choice.for_gfpipe
              ? QObject::tr("Plain key, saved beside the package for gfpipe")
              : QObject::tr("Plain key, shown after the export");
      break;
  }
  rows.append({.caption = QObject::tr("Protection"),
               .value = protection,
               .danger = !sealed});

  rows.append({.caption = QObject::tr("Format"),
               .value = choice.legacy_format
                            ? QObject::tr("Version 2, for older versions")
                            : QObject::tr("Version 3")});

  const auto package = KeyPackagePath(choice);
  rows.append({.caption = QObject::tr("Key package"),
               .value = package.isEmpty() ? QObject::tr("Not chosen yet")
                                          : QDir::toNativeSeparators(package),
               .dimmed = package.isEmpty(),
               .path = !package.isEmpty()});

  if (choice.for_gfpipe) {
    rows.append({.caption = QObject::tr("gfpipe"),
                 .value = QObject::tr("Copied into the folder, ready to run")});
  }

  if (KeyPackageUsesKeyFile(choice)) {
    rows.append({.caption = QObject::tr("Key file"),
                 .value = choice.key_file_path.isEmpty()
                              ? QObject::tr("Not chosen yet")
                              : QDir::toNativeSeparators(choice.key_file_path),
                 .dimmed = choice.key_file_path.isEmpty(),
                 .path = !choice.key_file_path.isEmpty()});
  }

  return rows;
}

auto BuildKeyPackageExportDoneRows(const QString& package_path,
                                   const QString& key_path,
                                   const QString& gfpipe_path,
                                   const QString& key_text_path)
    -> QList<MetaListRow> {
  QList<MetaListRow> rows = {
      {.caption = QObject::tr("Key package"),
       .value = QDir::toNativeSeparators(package_path),
       .path = true},
  };
  if (!key_path.isEmpty()) {
    rows.append(
        {.caption = QObject::tr("Key file"),
         .value = QDir::toNativeSeparators(key_path),
         .detail = QObject::tr("Needed with the PIN to import the package."),
         .path = true});
  }
  if (!key_text_path.isEmpty()) {
    rows.append({.caption = QObject::tr("Key"),
                 .value = QDir::toNativeSeparators(key_text_path),
                 .detail = QObject::tr("Opens the package. Anyone with this "
                                       "file and the package has the keys."),
                 .danger = true,
                 .path = true});
  }
  if (!gfpipe_path.isEmpty()) {
    rows.append({.caption = QObject::tr("gfpipe"),
                 .value = QDir::toNativeSeparators(gfpipe_path),
                 .path = true});
  }
  return rows;
}

auto DescribeKeyPackageProtection(const KeyPackageExportChoice& choice)
    -> QString {
  if (choice.legacy_format) {
    return QObject::tr(
        "The keys are encrypted with a random 256-character passphrase "
        "(XChaCha20-Poly1305). That passphrase is sealed with your PIN "
        "(Argon2id) and written to the key file. Importing needs the package, "
        "the key file and the PIN.");
  }
  switch (choice.protection) {
    case KeyPackageProtection::kPIN_FILE:
      return QObject::tr(
          "The keys are encrypted with a random key (XChaCha20-Poly1305). "
          "That key "
          "is sealed with your PIN (Argon2id) and written to the key file. "
          "Importing needs the package, the key file and the PIN.");
    case KeyPackageProtection::kPLAIN:
      return choice.for_gfpipe
                 ? QObject::tr(
                       "The keys are encrypted with a random key "
                       "(XChaCha20-Poly1305), sealed with a key saved beside "
                       "the package as default.gfkey.")
                 : QObject::tr(
                       "The keys are encrypted with a random key "
                       "(XChaCha20-Poly1305), sealed with a key you are shown "
                       "once after the export.");
  }
  return {};
}

}  // namespace GpgFrontend::UI

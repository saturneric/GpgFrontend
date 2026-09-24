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

#include "core/module/ModuleNamespace.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

namespace GpgFrontend::Module {

namespace {

/// How much of the leaf survives, and how much hash follows it.
///
/// 24 + 1 + 20 is 45 characters at most, which leaves room inside every
/// filesystem limit this project meets, including a Windows path already deep
/// inside an installation directory.
constexpr qsizetype kMaxLeafChars = 24;
constexpr qsizetype kSuffixHexChars = 20;  // 80 bits

}  // namespace

auto IsValidModuleId(const QString& module_id) -> bool {
  static const QRegularExpression kShape(
      QStringLiteral(R"(^[a-z][a-z0-9_]*(\.[a-z][a-z0-9_]*)+$)"));
  return module_id.size() <= 200 && kShape.match(module_id).hasMatch();
}

auto IsReservedModuleId(const QString& module_id) -> bool {
  return module_id.startsWith(QStringLiteral("com.bktus.gpgfrontend."));
}

auto IsOwnedName(const QString& owner, const QString& id) -> bool {
  static const QRegularExpression kLocal(QStringLiteral("^[a-z0-9_]+$"));
  if (owner.isEmpty() || !id.startsWith(owner + '.')) return false;
  return kLocal.match(id.mid(owner.size() + 1)).hasMatch();
}

auto ModuleIdLeaf(const QString& module_id) -> QString {
  const auto last_dot = module_id.lastIndexOf('.');
  const auto tail = last_dot < 0 ? module_id : module_id.mid(last_dot + 1);

  QString leaf;
  leaf.reserve(tail.size());
  for (const auto c : tail.toLower()) {
    // Underscore is the one substitution rather than a deletion: module ids
    // use it as a word separator, and dropping it would turn `ver_check` into
    // `vercheck`, which reads as a different word.
    const auto mapped = c == u'_' ? QChar(u'-') : c;
    if ((mapped >= u'a' && mapped <= u'z') ||
        (mapped >= u'0' && mapped <= u'9') || mapped == u'-') {
      leaf.append(mapped);
    }
  }

  while (leaf.startsWith(u'-')) leaf.remove(0, 1);
  while (leaf.endsWith(u'-')) leaf.chop(1);

  // A component that does not begin with a letter is prefixed rather than
  // rejected: an id is free to end in a digit, and refusing one here would
  // turn a naming choice into a build failure a long way from its cause.
  if (leaf.isEmpty() || leaf.front() < u'a' || leaf.front() > u'z') {
    leaf.prepend(u'm');
  }

  if (leaf.size() > kMaxLeafChars) leaf.truncate(kMaxLeafChars);
  // Truncation can strand a separator at the end, which would produce a key
  // with two adjacent dashes once the suffix is joined on.
  while (leaf.endsWith(u'-')) leaf.chop(1);

  return leaf;
}

auto ModuleDirectoryKey(const QString& module_id) -> QString {
  if (module_id.isEmpty()) return {};

  // Over the id exactly as the manifest spells it, in UTF-8. Not over the
  // leaf, and not over any normalised form: two ids that differ anywhere at
  // all must land on different keys, and the leaf has already thrown
  // information away by the time it is computed.
  const auto digest =
      QCryptographicHash::hash(module_id.toUtf8(), QCryptographicHash::Sha256);
  const auto suffix = QString::fromLatin1(digest.toHex().left(kSuffixHexChars));

  return ModuleIdLeaf(module_id) + u'-' + suffix;
}

auto ModuleNativeRootFor(const QString& descriptor_path) -> QString {
  // Purely textual, on the cleaned absolute path. QDir::cd()/cdUp() would be
  // the obvious way to walk this and are the wrong one: they check that the
  // directory exists, so the answer would depend on what happens to be on
  // disk, and a test could not ask about a layout it is not running on.
  const auto full = QDir::cleanPath(QFileInfo(descriptor_path).absolutePath());
  const auto parts = full.split(u'/', Qt::SkipEmptyParts);
  if (parts.isEmpty()) return full + "/" + kModuleNativeDirName;

  const auto& namespace_key = parts.constLast();

  // The macOS bundle is the only layout that splits a namespace across two
  // trees: Apple wants data under Resources and executable code under
  // Frameworks. Both levels must match -- a namespace that merely happens to
  // sit under some directory called Resources is not a bundle, and answering
  // as though it were would point the resolver at nothing.
  if (parts.size() >= 3 && parts.at(parts.size() - 2) == "modules" &&
      parts.at(parts.size() - 3) == "Resources") {
    const auto contents =
        QStringList(parts.mid(0, parts.size() - 3)).join(u'/');
    return "/" + contents + "/" + QString::fromUtf8(kAppleModuleNativeRoot) +
           "/" + namespace_key;
  }

  return full + "/" + QString::fromUtf8(kModuleNativeDirName);
}

}  // namespace GpgFrontend::Module

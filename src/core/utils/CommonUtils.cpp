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

#include "CommonUtils.h"

#include "core/utils/MemoryUtils.h"

namespace GpgFrontend {

auto BeautifyFingerprint(QString fingerprint) -> QString {
  auto len = fingerprint.size();
  QString buffer;
  QTextStream out(&buffer);
  decltype(len) count = 0;
  while (count < len) {
    if ((count != 0U) && ((count % 5) == 0)) out << " ";
    out << fingerprint[count];
    count++;
  }
  return out.readAll();
}

auto GFCompareSoftwareVersion(const QString& a, const QString& b) -> int {
  auto remove_prefix = [](const QString& version) {
    return version.startsWith('v') ? version.mid(1) : version;
  };

  auto real_version_a = remove_prefix(a);
  auto real_version_b = remove_prefix(b);

  QStringList split_a = real_version_a.split('.');
  QStringList split_b = real_version_b.split('.');

  const auto min_depth = std::min(split_a.size(), split_b.size());

  for (int i = 0; i < min_depth; ++i) {
    int const num_a = split_a[i].toInt();
    int const num_b = split_b[i].toInt();

    if (num_a != num_b) {
      return (num_a > num_b) ? 1 : -1;
    }
  }

  if (split_a.size() != split_b.size()) {
    return (split_a.size() > split_b.size()) ? 1 : -1;
  }

  return 0;
}

auto GFSoftwareVersionGreaterThan(const QString& a, const QString& b) -> bool {
  if (a.isEmpty()) return false;
  return GFCompareSoftwareVersion(a, b) > 0;
}

auto GFStrDup(const QString& s) -> char* {
  if (s.isEmpty()) return nullptr;

  auto u_s = s.toUtf8();
  auto* c_s = static_cast<char*>(SecureMalloc((u_s.size() + 1) * sizeof(char)));

  memcpy(c_s, u_s.constData(), u_s.size());
  c_s[u_s.size()] = '\0';
  return c_s;
}

auto GFUnStrDup(char* s) -> QString {
  auto q_s = QString::fromUtf8(s == nullptr ? "" : s);
  if (s != nullptr) SecureFree(static_cast<void*>(const_cast<char*>(s)));
  return q_s;
}

auto GFUnStrDup(const char* s) -> QString {
  return GFUnStrDup(const_cast<char*>(s));
}

auto GPGFRONTEND_CORE_EXPORT IsFlatpakENV() -> bool {
  return QString::fromLocal8Bit(qgetenv("container")) == "flatpak";
}

auto GPGFRONTEND_CORE_EXPORT ParseHexEncodedVersionTuple(const QString& s)
    -> int {
  // s is a hex-encoded, unsigned int-packed version tuple,
  // i.e. each byte represents one part of the version tuple
  bool ok;
  const auto version = s.toUtf8().toUInt(&ok, 16);
  return ok ? static_cast<int>(version) : -1;
}

const auto kReEmail = QRegularExpression{
    R"((?:[a-z0-9!#$%&'*+/=?^_`{|}~-]+(?:\.[a-z0-9!#$%&'*+/=?^_`{|}~-]+)*|"(?:[\x01-\x08\x0b\x0c\x0e-\x1f\x21\x23-\x5b\x5d-\x7f]|\\[\x01-\x09\x0b\x0c\x0e-\x7f])*")@(?:(?:[a-z0-9](?:[a-z0-9-]*[a-z0-9])?\.)+[a-z0-9](?:[a-z0-9-]*[a-z0-9])?|\[(?:(?:(2(5[0-5]|[0-4][0-9])|1[0-9][0-9]|[1-9]?[0-9]))\.){3}(?:(2(5[0-5]|[0-4][0-9])|1[0-9][0-9]|[1-9]?[0-9])|[a-z0-9-]*[a-z0-9]:(?:[\x01-\x08\x0b\x0c\x0e-\x1f\x21-\x5a\x53-\x7f]|\\[\x01-\x09\x0b\x0c\x0e-\x7f])+)\]))"};

auto GPGFRONTEND_CORE_EXPORT IsEmailAddress(const QString& str) -> bool {
  return kReEmail.match(str).hasMatch();
}
}  // namespace GpgFrontend
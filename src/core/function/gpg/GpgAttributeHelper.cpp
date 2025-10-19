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

#include "GpgAttributeHelper.h"

namespace {
auto StripHeader(QVector<GpgFrontend::GpgAttrInfo>& infos) -> void {
  for (auto& a : infos) {
    if (a.blob.size() < 16) continue;
    const auto* p = reinterpret_cast<const uchar*>(a.blob.constData());

    int hdr_len = p[0] | (p[1] << 8);
    if (hdr_len < 16 || a.blob.size() < hdr_len) continue;
    uchar version = p[2];
    uchar enc = p[3];  // 0x01 == JPEG
    a.payload = a.blob.mid(hdr_len);

    if (enc == 0x01 ||
        (a.payload.size() >= 3 && static_cast<uchar>(a.payload[0]) == 0xFF &&
         static_cast<uchar>(a.payload[1]) == 0xD8 &&
         static_cast<uchar>(a.payload[2]) == 0xFF)) {
      a.ext = "jpg";
    } else {
      a.ext = "bin";
    }
    Q_UNUSED(version);
    a.valid = true;
  }
}
}  // namespace

namespace GpgFrontend {
GpgAttributeHelper::GpgAttributeHelper(int channel)
    : GpgFrontend::SingletonFunctionObject<GpgAttributeHelper>(channel) {}

GpgAttributeHelper::~GpgAttributeHelper() = default;

auto GpgAttributeHelper::GetAttributes(const QString& key_id)
    -> QContainer<GpgAttrInfo> {
  auto [exit_code, out, err] = gce_.GpgExecuteSync({{},
                                                    {
                                                        "--batch",
                                                        "--no-tty",
                                                        "--attribute-fd",
                                                        "2",
                                                        "--status-fd",
                                                        "1",
                                                        "--logger-fd",
                                                        "1",
                                                        "--list-keys",
                                                        key_id,
                                                    },
                                                    {}});
  auto attr_bin_data = err;
  auto attr_status_data = out;

  LOG_D() << "GPG attribute status data size: " << attr_status_data.size()
          << " bytes.";
  LOG_D() << "GPG attribute binary data size: " << attr_bin_data.size()
          << " bytes.";

  QVector<GpgFrontend::GpgAttrInfo> v;
  const QStringList lines =
      QString::fromUtf8(attr_status_data).split('\n', Qt::SkipEmptyParts);

  for (const QString& line : lines) {
    if (!line.startsWith("[GNUPG:] ATTRIBUTE ")) continue;

    QString rest = line.mid(QString("[GNUPG:] ATTRIBUTE ").size());
    const QStringList cols = rest.split(' ', Qt::SkipEmptyParts);
    if (cols.size() < 8) continue;

    GpgFrontend::GpgAttrInfo a;
    a.fpr = cols[0];
    a.octets = cols[1].toLongLong();
    a.type = cols[2].toInt();
    a.index = cols[3].toInt();
    a.count = cols[4].toInt();
    a.ts = cols[5].toLongLong();
    a.exp = cols[6].toLongLong();
    a.flags = cols[7].toInt();

    v.push_back(a);
  }

  bool is_valid = true;
  qint64 off = 0;
  for (auto& a : v) {
    if (off + a.octets > attr_bin_data.size()) {
      LOG_W() << "Attribute blob size exceeds available binary data size.";
      is_valid = false;
      break;
    }

    LOG_D() << "Splitting attribute blob: FPR=" << a.fpr
            << ", Octets=" << a.octets << ", Type=" << a.type
            << ", Index=" << a.index << ", Count=" << a.count << ", TS=" << a.ts
            << ", Exp=" << a.exp << ", Flags=" << a.flags;
    a.blob = attr_bin_data.mid(off, a.octets);
    off += a.octets;
  }

  LOG_D() << "Total attribute binary data size: " << attr_bin_data.size()
          << " bytes, processed size: " << off << " bytes.";

  if (is_valid) StripHeader(v);
  return v;
}

}  // namespace GpgFrontend
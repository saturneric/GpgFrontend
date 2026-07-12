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

#include "core/function/ImContactBundle.h"

#include "core/function/X25519.h"

namespace GpgFrontend {

namespace {

constexpr uint8_t kBundleVersion = 0x01;
constexpr int kKeyLen = X25519::kPubKeyLen;

void AppendU16BE(QByteArray& out, quint16 v) {
  out.append(static_cast<char>((v >> 8) & 0xFF));
  out.append(static_cast<char>(v & 0xFF));
}

void AppendU32BE(QByteArray& out, quint32 v) {
  out.append(static_cast<char>((v >> 24) & 0xFF));
  out.append(static_cast<char>((v >> 16) & 0xFF));
  out.append(static_cast<char>((v >> 8) & 0xFF));
  out.append(static_cast<char>(v & 0xFF));
}

auto ReadU16BE(const QByteArray& in, int off) -> quint16 {
  return static_cast<quint16>(
      (static_cast<uint8_t>(in[off]) << 8) | static_cast<uint8_t>(in[off + 1]));
}

auto ReadU32BE(const QByteArray& in, int off) -> quint32 {
  return (static_cast<quint32>(static_cast<uint8_t>(in[off])) << 24) |
         (static_cast<quint32>(static_cast<uint8_t>(in[off + 1])) << 16) |
         (static_cast<quint32>(static_cast<uint8_t>(in[off + 2])) << 8) |
         static_cast<quint32>(static_cast<uint8_t>(in[off + 3]));
}

// Serialize the body (everything the signature covers). When include_sig is
// true the detached PGP signature is appended, producing the full wire bundle.
auto SerializeImpl(const ImContactBundle& b, bool include_sig) -> QByteArray {
  QByteArray out;
  out.append(static_cast<char>(kBundleVersion));
  out.append(b.ik_pub.ConvertToQByteArray());
  AppendU32BE(out, b.spk_id);
  out.append(b.spk_pub.ConvertToQByteArray());

  AppendU16BE(out, static_cast<quint16>(b.opks.size()));
  for (const auto& opk : b.opks) {
    AppendU32BE(out, opk.id);
    out.append(opk.pub.ConvertToQByteArray());
  }

  const QByteArray fpr = b.pgp_fpr.toUtf8();
  AppendU16BE(out, static_cast<quint16>(fpr.size()));
  out.append(fpr);

  if (include_sig) {
    AppendU32BE(out, static_cast<quint32>(b.pgp_sig.size()));
    out.append(b.pgp_sig);
  }
  return out;
}

}  // namespace

auto ImContactBundle::SignedPortion() const -> GFBuffer {
  return GFBuffer(SerializeImpl(*this, false));
}

auto ImContactBundle::Serialize() const -> GFBuffer {
  return GFBuffer(SerializeImpl(*this, true));
}

auto ImContactBundle::Parse(const GFBuffer& bytes)
    -> std::optional<ImContactBundle> {
  const QByteArray in = bytes.ConvertToQByteArray();
  int off = 0;
  const auto avail = [&](int n) { return off + n <= in.size(); };

  if (!avail(1) || static_cast<uint8_t>(in[off]) != kBundleVersion) return {};
  off += 1;

  ImContactBundle b;
  if (!avail(kKeyLen)) return {};
  b.ik_pub = GFBuffer(in.mid(off, kKeyLen));
  off += kKeyLen;

  if (!avail(4)) return {};
  b.spk_id = ReadU32BE(in, off);
  off += 4;

  if (!avail(kKeyLen)) return {};
  b.spk_pub = GFBuffer(in.mid(off, kKeyLen));
  off += kKeyLen;

  if (!avail(2)) return {};
  const quint16 opk_count = ReadU16BE(in, off);
  off += 2;
  for (quint16 i = 0; i < opk_count; ++i) {
    if (!avail(4 + kKeyLen)) return {};
    ImOneTimePrekey opk;
    opk.id = ReadU32BE(in, off);
    off += 4;
    opk.pub = GFBuffer(in.mid(off, kKeyLen));
    off += kKeyLen;
    b.opks.push_back(opk);
  }

  if (!avail(2)) return {};
  const quint16 fpr_len = ReadU16BE(in, off);
  off += 2;
  if (!avail(fpr_len)) return {};
  b.pgp_fpr = QString::fromUtf8(in.mid(off, fpr_len));
  off += fpr_len;

  if (!avail(4)) return {};
  const quint32 sig_len = ReadU32BE(in, off);
  off += 4;
  if (!avail(static_cast<int>(sig_len))) return {};
  b.pgp_sig = in.mid(off, static_cast<int>(sig_len));
  off += static_cast<int>(sig_len);

  return b;
}

}  // namespace GpgFrontend

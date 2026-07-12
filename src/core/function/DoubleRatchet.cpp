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

#include "core/function/DoubleRatchet.h"

#include <sodium.h>

#include "core/utils/CommonUtils.h"

namespace GpgFrontend {

namespace {

constexpr int kKeyLen = 32;
constexpr int kNonceLen = crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;  // 24
constexpr int kTagLen = crypto_aead_xchacha20poly1305_ietf_ABYTES;       // 16

// Domain-separation strings for the three KDFs.
constexpr std::string_view kRootDomain = "GF-DR-root-v1";
constexpr std::string_view kMsgDomain = "GF-DR-msg-v1";
constexpr char kChainCtx[crypto_kdf_CONTEXTBYTES] = {'G', 'F', '-', 'D',
                                                     'R', '-', 'c', 'k'};

auto ToQBA(const GFBuffer& b) -> QByteArray { return b.ConvertToQByteArray(); }

void AppendU32BE(QByteArray& out, quint32 v) {
  out.append(static_cast<char>((v >> 24) & 0xFF));
  out.append(static_cast<char>((v >> 16) & 0xFF));
  out.append(static_cast<char>((v >> 8) & 0xFF));
  out.append(static_cast<char>(v & 0xFF));
}

auto ReadU32BE(const QByteArray& in, int off) -> quint32 {
  return (static_cast<quint32>(static_cast<uint8_t>(in[off])) << 24) |
         (static_cast<quint32>(static_cast<uint8_t>(in[off + 1])) << 16) |
         (static_cast<quint32>(static_cast<uint8_t>(in[off + 2])) << 8) |
         static_cast<quint32>(static_cast<uint8_t>(in[off + 3]));
}

// Root KDF: (rk', ck) = BLAKE2b(key = rk, msg = domain || dh_out), split 32/32.
auto KdfRk(const GFBuffer& rk, const GFBuffer& dh)
    -> std::optional<std::pair<GFBuffer, GFBuffer>> {
  if (rk.Size() != kKeyLen || dh.Size() != kKeyLen) return {};

  QByteArray in(kRootDomain.data(), static_cast<qsizetype>(kRootDomain.size()));
  in.append(ToQBA(dh));

  std::array<unsigned char, 64> out{};
  if (crypto_generichash(out.data(), out.size(),
                         reinterpret_cast<const unsigned char*>(in.constData()),
                         static_cast<unsigned long long>(in.size()),
                         reinterpret_cast<const unsigned char*>(rk.Data()),
                         rk.Size()) != 0) {
    return {};
  }

  GFBuffer new_rk(reinterpret_cast<const char*>(out.data()), kKeyLen);
  GFBuffer ck(reinterpret_cast<const char*>(out.data() + kKeyLen), kKeyLen);
  sodium_memzero(out.data(), out.size());
  return std::make_pair(new_rk, ck);
}

// Chain KDF: mk = KDF(ck, id=1), ck' = KDF(ck, id=2), via crypto_kdf (BLAKE2b).
auto KdfCk(const GFBuffer& ck) -> std::optional<std::pair<GFBuffer, GFBuffer>> {
  if (ck.Size() != kKeyLen) return {};

  GFBuffer mk(kKeyLen);
  GFBuffer new_ck(kKeyLen);
  const auto* key = reinterpret_cast<const unsigned char*>(ck.Data());
  if (crypto_kdf_derive_from_key(reinterpret_cast<unsigned char*>(mk.Data()),
                                 kKeyLen, 1, kChainCtx, key) != 0 ||
      crypto_kdf_derive_from_key(reinterpret_cast<unsigned char*>(new_ck.Data()),
                                 kKeyLen, 2, kChainCtx, key) != 0) {
    mk.Zeroize();
    new_ck.Zeroize();
    return {};
  }
  return std::make_pair(new_ck, mk);
}

// Expand a message key into an AEAD key and nonce.
auto DeriveMsgKeyNonce(const GFBuffer& mk)
    -> std::optional<std::pair<GFBuffer, GFBuffer>> {
  std::array<unsigned char, kKeyLen + kNonceLen> out{};
  if (crypto_generichash(
          out.data(), out.size(),
          reinterpret_cast<const unsigned char*>(kMsgDomain.data()),
          static_cast<unsigned long long>(kMsgDomain.size()),
          reinterpret_cast<const unsigned char*>(mk.Data()), mk.Size()) != 0) {
    return {};
  }
  GFBuffer enc_key(reinterpret_cast<const char*>(out.data()), kKeyLen);
  GFBuffer nonce(reinterpret_cast<const char*>(out.data() + kKeyLen), kNonceLen);
  sodium_memzero(out.data(), out.size());
  return std::make_pair(enc_key, nonce);
}

auto AeadSeal(const GFBuffer& mk, const QByteArray& ad, const QByteArray& pt)
    -> std::optional<QByteArray> {
  auto kn = DeriveMsgKeyNonce(mk);
  if (!kn) return {};
  auto& [enc_key, nonce] = *kn;

  QByteArray ct(pt.size() + kTagLen, Qt::Uninitialized);
  unsigned long long clen = 0;
  const int rc = crypto_aead_xchacha20poly1305_ietf_encrypt(
      reinterpret_cast<unsigned char*>(ct.data()), &clen,
      reinterpret_cast<const unsigned char*>(pt.constData()),
      static_cast<unsigned long long>(pt.size()),
      reinterpret_cast<const unsigned char*>(ad.constData()),
      static_cast<unsigned long long>(ad.size()), nullptr,
      reinterpret_cast<const unsigned char*>(nonce.Data()),
      reinterpret_cast<const unsigned char*>(enc_key.Data()));
  enc_key.Zeroize();
  nonce.Zeroize();
  if (rc != 0) return {};
  ct.resize(static_cast<qsizetype>(clen));
  return ct;
}

auto AeadOpen(const GFBuffer& mk, const QByteArray& ad, const QByteArray& ct)
    -> std::optional<QByteArray> {
  if (ct.size() < kTagLen) return {};

  auto kn = DeriveMsgKeyNonce(mk);
  if (!kn) return {};
  auto& [enc_key, nonce] = *kn;

  QByteArray pt(ct.size() - kTagLen, Qt::Uninitialized);
  unsigned long long mlen = 0;
  const int rc = crypto_aead_xchacha20poly1305_ietf_decrypt(
      reinterpret_cast<unsigned char*>(pt.data()), &mlen, nullptr,
      reinterpret_cast<const unsigned char*>(ct.constData()),
      static_cast<unsigned long long>(ct.size()),
      reinterpret_cast<const unsigned char*>(ad.constData()),
      static_cast<unsigned long long>(ad.size()),
      reinterpret_cast<const unsigned char*>(nonce.Data()),
      reinterpret_cast<const unsigned char*>(enc_key.Data()));
  enc_key.Zeroize();
  nonce.Zeroize();
  if (rc != 0) return {};
  pt.resize(static_cast<qsizetype>(mlen));
  return pt;
}

// Build the full AEAD associated data: outer AD || serialized ratchet header.
auto FullAd(const GFBuffer& ad, const RatchetHeader& header) -> QByteArray {
  QByteArray full = ToQBA(ad);
  full.append(ToQBA(DoubleRatchet::SerializeHeader(header)));
  return full;
}

// Advance the receiving chain up to index `until`, banking skipped keys.
auto SkipMessageKeys(RatchetState& w, quint32 until) -> bool {
  if (w.ckr.Empty()) return true;  // no receiving chain yet: nothing to skip
  if (w.nr + DoubleRatchet::kMaxSkip < until) return false;

  const QByteArray dhr = ToQBA(w.dhr_pub);
  while (w.nr < until) {
    auto step = KdfCk(w.ckr);
    if (!step) return false;
    w.ckr = step->first;
    w.skipped.insert(qMakePair(dhr, w.nr), step->second);
    w.nr += 1;
  }
  return true;
}

// Perform a DH ratchet step for a newly seen peer ratchet public key.
auto DhRatchet(RatchetState& w, const RatchetHeader& header) -> bool {
  w.pn = w.ns;
  w.ns = 0;
  w.nr = 0;
  w.dhr_pub = header.dh_pub;

  auto dh1 = X25519::DH(w.dhs.priv, w.dhr_pub);
  if (!dh1) return false;
  auto rk_ckr = KdfRk(w.rk, *dh1);
  if (!rk_ckr) return false;
  w.rk = rk_ckr->first;
  w.ckr = rk_ckr->second;

  auto kp = X25519::Generate();
  if (!kp) return false;
  w.dhs = *kp;

  auto dh2 = X25519::DH(w.dhs.priv, w.dhr_pub);
  if (!dh2) return false;
  auto rk_cks = KdfRk(w.rk, *dh2);
  if (!rk_cks) return false;
  w.rk = rk_cks->first;
  w.cks = rk_cks->second;
  return true;
}

}  // namespace

auto DoubleRatchet::SerializeHeader(const RatchetHeader& header) -> GFBuffer {
  QByteArray out = header.dh_pub.ConvertToQByteArray();
  AppendU32BE(out, header.pn);
  AppendU32BE(out, header.n);
  return GFBuffer(out);
}

auto DoubleRatchet::ParseHeader(const GFBuffer& bytes)
    -> std::optional<RatchetHeader> {
  if (static_cast<int>(bytes.Size()) != kHeaderLen) return {};
  const QByteArray b = bytes.ConvertToQByteArray();
  RatchetHeader h;
  h.dh_pub = GFBuffer(b.left(X25519::kPubKeyLen));
  h.pn = ReadU32BE(b, X25519::kPubKeyLen);
  h.n = ReadU32BE(b, X25519::kPubKeyLen + 4);
  return h;
}

auto DoubleRatchet::InitSender(const GFBuffer& sk,
                               const GFBuffer& peer_ratchet_pub)
    -> std::optional<RatchetState> {
  if (sk.Size() != kKeyLen) return {};

  auto kp = X25519::Generate();
  if (!kp) return {};

  auto dh = X25519::DH(kp->priv, peer_ratchet_pub);
  if (!dh) return {};

  auto rk_cks = KdfRk(sk, *dh);
  if (!rk_cks) return {};

  RatchetState st;
  st.dhs = *kp;
  st.dhr_pub = peer_ratchet_pub;
  st.rk = rk_cks->first;
  st.cks = rk_cks->second;
  return st;
}

auto DoubleRatchet::InitReceiver(const GFBuffer& sk,
                                 const X25519KeyPair& own_ratchet_kp)
    -> RatchetState {
  RatchetState st;
  st.dhs = own_ratchet_kp;
  st.rk = sk;
  return st;
}

auto DoubleRatchet::Encrypt(RatchetState& st, const GFBuffer& plaintext,
                            const GFBuffer& ad, RatchetHeader& out_header)
    -> GFBufferOrNone {
  if (st.cks.Empty()) {
    LOG_E() << "ratchet encrypt with no sending chain";
    return {};
  }

  auto step = KdfCk(st.cks);
  if (!step) return {};

  RatchetHeader header;
  header.dh_pub = st.dhs.pub;
  header.pn = st.pn;
  header.n = st.ns;

  auto ct = AeadSeal(step->second, FullAd(ad, header), ToQBA(plaintext));
  step->second.Zeroize();
  if (!ct) return {};

  st.cks = step->first;
  st.ns += 1;
  out_header = header;
  return GFBuffer(*ct);
}

auto DoubleRatchet::Decrypt(RatchetState& st, const RatchetHeader& header,
                            const GFBuffer& ciphertext, const GFBuffer& ad)
    -> GFBufferOrNone {
  const QByteArray ct = ToQBA(ciphertext);
  const QByteArray full_ad = FullAd(ad, header);

  // Work on a copy; commit to `st` only if decryption succeeds, so a forged
  // token can never desynchronise the live session.
  RatchetState w = st;

  // 1. A message key banked earlier for an out-of-order message.
  const auto skipped_key = qMakePair(ToQBA(header.dh_pub), header.n);
  if (w.skipped.contains(skipped_key)) {
    auto pt = AeadOpen(w.skipped.value(skipped_key), full_ad, ct);
    if (!pt) return {};
    w.skipped.remove(skipped_key);
    st = w;
    return GFBuffer(*pt);
  }

  // 2. New peer ratchet key -> skip the tail of the old chain and DH-ratchet.
  if (w.dhr_pub != header.dh_pub) {
    if (!SkipMessageKeys(w, header.pn)) return {};
    if (!DhRatchet(w, header)) return {};
  }

  // 3. Skip within the current receiving chain, then derive this message key.
  if (!SkipMessageKeys(w, header.n)) return {};
  if (w.ckr.Empty()) return {};

  auto step = KdfCk(w.ckr);
  if (!step) return {};
  auto pt = AeadOpen(step->second, full_ad, ct);
  step->second.Zeroize();
  if (!pt) return {};

  w.ckr = step->first;
  w.nr += 1;
  st = w;
  return GFBuffer(*pt);
}

auto DoubleRatchet::SerializeState(const RatchetState& st) -> GFBuffer {
  QByteArray out;
  out.append(static_cast<char>(0x01));  // state format version

  const auto put_key = [&out](const GFBuffer& k) {
    out.append(k.ConvertToQByteArray());
  };
  const auto put_opt = [&out, &put_key](const GFBuffer& k) {
    if (k.Empty()) {
      out.append(static_cast<char>(0x00));
    } else {
      out.append(static_cast<char>(0x01));
      put_key(k);
    }
  };

  put_key(st.dhs.priv);
  put_key(st.dhs.pub);
  put_opt(st.dhr_pub);
  put_key(st.rk);
  put_opt(st.cks);
  put_opt(st.ckr);
  AppendU32BE(out, st.ns);
  AppendU32BE(out, st.nr);
  AppendU32BE(out, st.pn);

  AppendU32BE(out, static_cast<quint32>(st.skipped.size()));
  for (auto it = st.skipped.constBegin(); it != st.skipped.constEnd(); ++it) {
    out.append(it.key().first);  // dh_pub (32)
    AppendU32BE(out, it.key().second);
    const QByteArray mk = it.value().ConvertToQByteArray();
    AppendU32BE(out, static_cast<quint32>(mk.size()));
    out.append(mk);
  }
  return GFBuffer(out);
}

auto DoubleRatchet::DeserializeState(const GFBuffer& bytes)
    -> std::optional<RatchetState> {
  const QByteArray in = bytes.ConvertToQByteArray();
  int off = 0;
  const auto avail = [&](int n) { return off + n <= in.size(); };

  if (!avail(1) || static_cast<uint8_t>(in[off]) != 0x01) return {};
  off += 1;

  const auto read_key = [&](GFBuffer& k) -> bool {
    if (!avail(kKeyLen)) return false;
    k = GFBuffer(in.mid(off, kKeyLen));
    off += kKeyLen;
    return true;
  };
  const auto read_opt = [&](GFBuffer& k) -> bool {
    if (!avail(1)) return false;
    const auto flag = static_cast<uint8_t>(in[off]);
    off += 1;
    if (flag == 0) {
      k = GFBuffer();
      return true;
    }
    return read_key(k);
  };

  RatchetState st;
  if (!read_key(st.dhs.priv) || !read_key(st.dhs.pub) ||
      !read_opt(st.dhr_pub) || !read_key(st.rk) || !read_opt(st.cks) ||
      !read_opt(st.ckr)) {
    return {};
  }
  if (!avail(12)) return {};
  st.ns = ReadU32BE(in, off);
  st.nr = ReadU32BE(in, off + 4);
  st.pn = ReadU32BE(in, off + 8);
  off += 12;

  if (!avail(4)) return {};
  const quint32 n_skipped = ReadU32BE(in, off);
  off += 4;
  for (quint32 i = 0; i < n_skipped; ++i) {
    if (!avail(kKeyLen + 4 + 4)) return {};
    const QByteArray dh = in.mid(off, kKeyLen);
    off += kKeyLen;
    const quint32 n = ReadU32BE(in, off);
    off += 4;
    const quint32 mk_len = ReadU32BE(in, off);
    off += 4;
    if (!avail(static_cast<int>(mk_len))) return {};
    st.skipped.insert(qMakePair(dh, n),
                      GFBuffer(in.mid(off, static_cast<int>(mk_len))));
    off += static_cast<int>(mk_len);
  }
  return st;
}

}  // namespace GpgFrontend

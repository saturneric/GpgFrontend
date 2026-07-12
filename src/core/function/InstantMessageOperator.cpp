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

#include "core/function/InstantMessageOperator.h"

#include <sodium.h>

#include <QCryptographicHash>
#include <QRegularExpression>
#include <array>

#include "core/function/DoubleRatchet.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/ImContactBundle.h"
#include "core/function/ImHandshake.h"
#include "core/function/ImSessionStore.h"
#include "core/function/SecureRandomGenerator.h"
#include "core/function/X25519.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/utils/CommonUtils.h"

namespace GpgFrontend {

namespace {

// ---------------------------------------------------------------------------
// Base58 (Bitcoin/IPFS alphabet). Purely alphanumeric, no 0 O I l, and nothing
// a messenger turns into markdown/link/soft-wrap, so a token stays one word.
// ---------------------------------------------------------------------------

constexpr char kB58Alphabet[] =
    "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

auto Base58DigitValue(char c) -> int {
  for (int i = 0; i < 58; ++i) {
    if (kB58Alphabet[i] == c) return i;
  }
  return -1;
}

auto Base58Encode(const QByteArray& in) -> QString {
  QVector<quint16> digits;
  for (const char ch : in) {
    int carry = static_cast<uint8_t>(ch);
    for (auto& d : digits) {
      carry += d << 8;
      d = static_cast<quint16>(carry % 58);
      carry /= 58;
    }
    while (carry > 0) {
      digits.append(static_cast<quint16>(carry % 58));
      carry /= 58;
    }
  }

  QString out;
  for (const char ch : in) {
    if (ch != 0) break;
    out.append(QLatin1Char(kB58Alphabet[0]));
  }
  for (auto i = digits.size() - 1; i >= 0; --i) {
    out.append(QLatin1Char(kB58Alphabet[digits[i]]));
  }
  return out;
}

auto Base58Decode(const QString& in, bool& ok) -> QByteArray {
  ok = false;
  QVector<quint16> bytes;
  for (const QChar qc : in) {
    const int val = Base58DigitValue(qc.toLatin1());
    if (val < 0) return {};

    int carry = val;
    for (auto& b : bytes) {
      carry += b * 58;
      b = static_cast<quint16>(carry & 0xFF);
      carry >>= 8;
    }
    while (carry > 0) {
      bytes.append(static_cast<quint16>(carry & 0xFF));
      carry >>= 8;
    }
  }

  QByteArray out;
  for (const QChar qc : in) {
    if (qc != QLatin1Char(kB58Alphabet[0])) break;
    out.append('\0');
  }
  for (auto i = bytes.size() - 1; i >= 0; --i) {
    out.append(static_cast<char>(bytes[i]));
  }
  ok = true;
  return out;
}

// ---------------------------------------------------------------------------
// Password book: a shared 256-byte table, phrase-derived (Argon2id) or default.
// Now used only as an extra shared secret mixed into the X3DH handshake.
// ---------------------------------------------------------------------------

constexpr uint64_t kDefaultWeyl = 0x7F49BA797C9E3175ULL;
constexpr uint64_t kDefaultMul = 0xBF18475B4C6DEE59ULL;
constexpr auto kBookPhraseKey = "im/password_book_phrase";

constexpr std::array<unsigned char, crypto_pwhash_SALTBYTES> kBookKdfSalt = {
    'G', 'F', '-', 'I', 'M', '-', 'b', 'o',
    'o', 'k', '-', 's', 'a', 'l', 't', '1'};
constexpr unsigned long long kBookKdfOps = 3;
constexpr size_t kBookKdfMem = 65536ULL * 1024ULL;  // 64 MiB

auto DeriveBookConstants(const QString& phrase, uint64_t& weyl, uint64_t& mul)
    -> bool {
  if (!EnsureSodiumInit()) return false;

  const QByteArray pw = phrase.toUtf8();
  std::array<unsigned char, 16> out{};
  const int rc =
      crypto_pwhash(out.data(), out.size(), pw.constData(),
                    static_cast<unsigned long long>(pw.size()),
                    kBookKdfSalt.data(), kBookKdfOps, kBookKdfMem,
                    crypto_pwhash_ALG_ARGON2ID13);
  if (rc != 0) {
    LOG_E() << "argon2id derivation for instant-messaging book failed";
    return false;
  }

  const auto read8 = [&out](int off) {
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) v = (v << 8) | out[off + i];
    return v;
  };
  weyl = read8(0) | 1ULL;
  mul = read8(8) | 1ULL;
  return true;
}

auto BuildBook(uint64_t weyl, uint64_t mul) -> std::array<uint8_t, 256> {
  std::array<uint8_t, 256> sub{};
  for (int i = 0; i < 256; ++i) sub[i] = static_cast<uint8_t>(i);

  uint64_t s = weyl;
  for (int i = 255; i > 0; --i) {
    s += weyl;
    uint64_t z = s;
    z = (z ^ (z >> 30)) * mul;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    z = z ^ (z >> 31);
    const int j = static_cast<int>(z % static_cast<uint64_t>(i + 1));
    std::swap(sub[i], sub[j]);
  }
  return sub;
}

// The active book's raw bytes, mixed into the X3DH secret so both parties must
// share the same phrase (or both use the default).
auto ActiveBookBytes() -> GFBuffer {
  const auto phrase = GetSettings().value(kBookPhraseKey).toString().trimmed();
  uint64_t weyl = kDefaultWeyl;
  uint64_t mul = kDefaultMul;
  if (!phrase.isEmpty()) {
    uint64_t w = 0;
    uint64_t m = 0;
    if (DeriveBookConstants(phrase, w, m)) {
      weyl = w;
      mul = m;
    }
  }
  const auto sub = BuildBook(weyl, mul);
  return GFBuffer(QByteArray(reinterpret_cast<const char*>(sub.data()),
                             static_cast<qsizetype>(sub.size())));
}

// ---------------------------------------------------------------------------
// Wire container.
// ---------------------------------------------------------------------------

constexpr uint8_t kVersion = 0x01;
constexpr uint8_t kTypeNormal = 0x00;
constexpr uint8_t kTypePfsMsg = 0x01;
constexpr uint8_t kTypePfsInit = 0x02;

constexpr uint8_t kNormalFlagHasBundle = 0x01;

constexpr int kSessionIdLen = 8;
constexpr int kKeyLen = X25519::kPubKeyLen;
constexpr int kHeaderLen = DoubleRatchet::kHeaderLen;  // 40
// version|type|sid|ik_pub|ek_pub|spk_id|opk_id
constexpr int kInitFieldsLen = 2 + kSessionIdLen + kKeyLen + kKeyLen + 4 + 4;
constexpr int kPfsMsgAdLen = 2 + kSessionIdLen;  // version|type|sid

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

// Assemble our signed prekey bundle (key material from the store + PGP sig).
auto BuildSignedBundle(int channel, const GpgKeyPtr& signer)
    -> std::optional<ImContactBundle> {
  if (signer == nullptr) return {};
  auto bundle = ImSessionStore::GetInstance().PublicBundleKeys();
  bundle.pgp_fpr = signer->Fingerprint();
  auto sig = ImHandshake::SignDetached(channel, signer, bundle.SignedPortion());
  if (!sig) return {};
  bundle.pgp_sig = *sig;
  return bundle;
}

auto RandomSessionId() -> QByteArray {
  auto r = SecureRandomGenerator::Generate(kSessionIdLen);
  return r ? r->ConvertToQByteArray() : QByteArray(kSessionIdLen, '\0');
}

// Whether a fingerprint is one of our own keys — i.e. we are messaging
// ourselves, so the two ratchet ends must be kept in separate session slots.
auto IsSelfIdentity(int channel, const QString& fpr) -> bool {
  auto key = GpgKeyRepository::GetInstance(channel).GetKey(fpr);
  return key.IsGood() && key.IsPrivateKey();
}

// Strip whitespace a messenger may have injected; reject armored PGP text.
auto Normalize(const QString& text, QString& compact) -> bool {
  if (text.contains(QLatin1String("-----BEGIN PGP"))) return false;
  compact = text;
  compact.remove(QRegularExpression(QStringLiteral("\\s")));
  return !compact.isEmpty();
}

}  // namespace

auto InstantMessageOperator::ChooseSendMode(const QString& peer_fpr)
    -> SendMode {
  if (peer_fpr.isEmpty()) return SendMode::kNORMAL;
  auto& store = ImSessionStore::GetInstance();
  if (store.LoadSession(peer_fpr) || store.LoadPeerBundle(peer_fpr)) {
    return SendMode::kPFS;
  }
  return SendMode::kNORMAL;
}

auto InstantMessageOperator::ShouldAttachBundle(const QString& peer_fpr)
    -> bool {
  if (peer_fpr.isEmpty()) return true;
  return !ImSessionStore::GetInstance().IsPfsConfirmed(peer_fpr);
}

auto InstantMessageOperator::EncodeNormal(int channel, const GpgKeyPtr& signer,
                                          const GFBuffer& pgp_message,
                                          bool attach_bundle) -> QString {
  const auto pgp = pgp_message.ConvertToQByteArray();
  if (pgp.isEmpty()) return {};

  QByteArray bundle_bytes;
  uint8_t flags = 0;
  if (attach_bundle) {
    if (auto b = BuildSignedBundle(channel, signer)) {
      bundle_bytes = b->Serialize().ConvertToQByteArray();
      flags |= kNormalFlagHasBundle;
    }
  }

  QByteArray frame;
  frame.append(static_cast<char>(kVersion));
  frame.append(static_cast<char>(kTypeNormal));
  frame.append(static_cast<char>(flags));
  AppendU32BE(frame, static_cast<quint32>(bundle_bytes.size()));
  frame.append(bundle_bytes);
  frame.append(pgp);
  return Base58Encode(frame);
}

auto InstantMessageOperator::EncodePfs(int channel, const GpgKeyPtr& signer,
                                       const QString& peer_fpr,
                                       const GFBuffer& plaintext) -> QString {
  auto& store = ImSessionStore::GetInstance();

  // Steady state: an established session -> compact PFS_MSG.
  if (auto session = store.LoadSession(peer_fpr)) {
    auto sid = store.GetPeerSessionId(peer_fpr);
    if (!sid) return {};

    QByteArray ad;
    ad.append(static_cast<char>(kVersion));
    ad.append(static_cast<char>(kTypePfsMsg));
    ad.append(*sid);

    RatchetHeader header;
    auto ct = DoubleRatchet::Encrypt(*session, plaintext, GFBuffer(ad), header);
    if (!ct) return {};
    store.SaveSession(peer_fpr, *session);

    QByteArray frame = ad;
    frame.append(DoubleRatchet::SerializeHeader(header).ConvertToQByteArray());
    frame.append(ct->ConvertToQByteArray());
    return Base58Encode(frame);
  }

  // First message: need the peer's bundle to run X3DH -> PFS_INIT.
  auto bundle = store.LoadPeerBundle(peer_fpr);
  if (!bundle) return {};  // caller falls back to NORMAL

  const auto identity = store.LoadOrCreateIdentity();
  const auto book = ActiveBookBytes();

  ImHandshake::InitPayload payload;
  auto state = ImHandshake::Initiate(identity, *bundle, book, payload);
  if (!state) return {};

  const QByteArray sid = RandomSessionId();

  QByteArray fields;  // version|type|sid|ik_pub|ek_pub|spk_id|opk_id
  fields.append(static_cast<char>(kVersion));
  fields.append(static_cast<char>(kTypePfsInit));
  fields.append(sid);
  fields.append(payload.ik_pub.ConvertToQByteArray());
  fields.append(payload.ek_pub.ConvertToQByteArray());
  AppendU32BE(fields, payload.spk_id);
  AppendU32BE(fields, payload.opk_id);

  RatchetHeader header;
  auto ct = DoubleRatchet::Encrypt(*state, plaintext, GFBuffer(fields), header);
  if (!ct) return {};

  const QByteArray header_bytes =
      DoubleRatchet::SerializeHeader(header).ConvertToQByteArray();

  QByteArray transcript = fields;
  transcript.append(header_bytes);
  auto sig = ImHandshake::SignDetached(channel, signer, GFBuffer(transcript));
  if (!sig) return {};

  QByteArray frame = fields;
  frame.append(header_bytes);
  AppendU32BE(frame, static_cast<quint32>(sig->size()));
  frame.append(*sig);
  frame.append(ct->ConvertToQByteArray());

  store.SaveSession(peer_fpr, *state);
  store.SavePeerSessionId(peer_fpr, sid);
  store.SetSessionPeer(sid, peer_fpr);
  return Base58Encode(frame);
}

auto InstantMessageOperator::Decode(int channel, const QString& text)
    -> DecodeResult {
  DecodeResult r;

  QString compact;
  if (!Normalize(text, compact)) return r;  // kNOT_TOKEN

  bool ok = false;
  const QByteArray frame = Base58Decode(compact, ok);
  if (!ok || frame.size() < 2 || static_cast<uint8_t>(frame[0]) != kVersion) {
    return r;  // kNOT_TOKEN
  }

  auto& store = ImSessionStore::GetInstance();
  const auto type = static_cast<uint8_t>(frame[1]);

  if (type == kTypeNormal) {
    if (frame.size() < 3 + 4) {
      r.status = DecodeStatus::kMALFORMED;
      return r;
    }
    const uint8_t flags = static_cast<uint8_t>(frame[2]);
    const quint32 bundle_len = ReadU32BE(frame, 3);
    int off = 3 + 4;
    if (frame.size() < off + static_cast<int>(bundle_len)) {
      r.status = DecodeStatus::kMALFORMED;
      return r;
    }
    if ((flags & kNormalFlagHasBundle) != 0 && bundle_len > 0) {
      const auto bundle =
          ImContactBundle::Parse(GFBuffer(frame.mid(off, bundle_len)));
      if (bundle) {
        auto signer_fpr = ImHandshake::VerifyDetached(
            channel, bundle->SignedPortion(), bundle->pgp_sig);
        if (signer_fpr && *signer_fpr == bundle->pgp_fpr) {
          // Recovery: a peer that lost its state and re-keyed (new identity key)
          // re-sends a bundle. A different identity key means any session we
          // still hold is dead, so drop it — our next message then transparently
          // re-establishes forward secrecy with the new identity. (A reordered
          // copy of the same bundle keeps the same key, so it is a no-op.)
          auto existing = store.LoadPeerBundle(bundle->pgp_fpr);
          if (existing && existing->ik_pub != bundle->ik_pub) {
            store.DeleteSession(bundle->pgp_fpr);
            store.ClearPfsConfirmed(bundle->pgp_fpr);
          }
          store.SavePeerBundle(*bundle);
          r.peer_fpr = bundle->pgp_fpr;
          r.imported_bundle = true;
        }
      }
    }
    off += static_cast<int>(bundle_len);
    r.pgp_message = GFBuffer(frame.mid(off));
    r.status = DecodeStatus::kNORMAL_OK;
    return r;
  }

  if (type == kTypePfsMsg) {
    if (frame.size() < kPfsMsgAdLen + kHeaderLen) {
      r.status = DecodeStatus::kMALFORMED;
      return r;
    }
    const QByteArray sid = frame.mid(2, kSessionIdLen);
    auto peer = store.SessionIdToPeer(sid);
    if (!peer) {
      r.status = DecodeStatus::kNO_SESSION;
      return r;
    }
    auto header =
        DoubleRatchet::ParseHeader(GFBuffer(frame.mid(kPfsMsgAdLen, kHeaderLen)));
    if (!header) {
      r.status = DecodeStatus::kMALFORMED;
      return r;
    }
    auto session = store.LoadSession(*peer);
    if (!session) {
      r.status = DecodeStatus::kNO_SESSION;
      return r;
    }

    // Self-messaging: a token whose ratchet key is our primary's own sending key
    // was authored by us, so it must be decrypted with the loopback (other-end)
    // session rather than the one that produced it.
    const bool use_loopback = header->dh_pub == session->dhs.pub;
    if (use_loopback) {
      session = store.LoadLoopbackSession(*peer);
      if (!session) {
        r.status = DecodeStatus::kNO_SESSION;
        return r;
      }
    }

    const GFBuffer ad(frame.left(kPfsMsgAdLen));
    const GFBuffer ct(frame.mid(kPfsMsgAdLen + kHeaderLen));
    auto pt = DoubleRatchet::Decrypt(*session, *header, ct, ad);
    if (!pt) {
      r.status = DecodeStatus::kUNDECRYPTABLE;
      return r;
    }
    if (use_loopback) {
      store.SaveLoopbackSession(*peer, *session);
    } else {
      store.SaveSession(*peer, *session);
    }
    store.MarkPfsConfirmed(*peer);
    r.status = DecodeStatus::kPFS_OK;
    r.plaintext = *pt;
    r.peer_fpr = *peer;
    return r;
  }

  if (type == kTypePfsInit) {
    if (frame.size() < kInitFieldsLen + kHeaderLen + 4) {
      r.status = DecodeStatus::kMALFORMED;
      return r;
    }
    int off = 2;
    const QByteArray sid = frame.mid(off, kSessionIdLen);
    off += kSessionIdLen;
    ImHandshake::InitPayload payload;
    payload.ik_pub = GFBuffer(frame.mid(off, kKeyLen));
    off += kKeyLen;
    payload.ek_pub = GFBuffer(frame.mid(off, kKeyLen));
    off += kKeyLen;
    payload.spk_id = ReadU32BE(frame, off);
    off += 4;
    payload.opk_id = ReadU32BE(frame, off);
    off += 4;  // == kInitFieldsLen

    const QByteArray header_bytes = frame.mid(off, kHeaderLen);
    off += kHeaderLen;
    const quint32 sig_len = ReadU32BE(frame, off);
    off += 4;
    if (frame.size() < off + static_cast<int>(sig_len)) {
      r.status = DecodeStatus::kMALFORMED;
      return r;
    }
    const QByteArray sig = frame.mid(off, static_cast<int>(sig_len));
    off += static_cast<int>(sig_len);
    const QByteArray ct = frame.mid(off);

    QByteArray transcript = frame.left(kInitFieldsLen);
    transcript.append(header_bytes);
    auto signer_fpr =
        ImHandshake::VerifyDetached(channel, GFBuffer(transcript), sig);
    if (!signer_fpr) {
      r.status = DecodeStatus::kHANDSHAKE_AUTH_FAILED;
      return r;
    }
    const QString peer_fpr = *signer_fpr;

    const auto identity = store.LoadOrCreateIdentity();
    auto own_spk = store.GetSignedPrekeyById(payload.spk_id);
    if (!own_spk) {
      r.status = DecodeStatus::kNO_SESSION;  // our prekey rotated away
      return r;
    }
    std::optional<X25519KeyPair> own_opk;
    if (payload.opk_id != 0) {
      own_opk = store.ConsumeOneTimePrekey(payload.opk_id);
      if (!own_opk) {
        r.status = DecodeStatus::kNO_SESSION;  // one-time prekey already used
        return r;
      }
    }

    const auto book = ActiveBookBytes();
    auto state =
        ImHandshake::Accept(identity, *own_spk, own_opk, payload, book);
    if (!state) {
      r.status = DecodeStatus::kNO_SESSION;
      return r;
    }

    auto header = DoubleRatchet::ParseHeader(GFBuffer(header_bytes));
    if (!header) {
      r.status = DecodeStatus::kMALFORMED;
      return r;
    }
    const GFBuffer ad(frame.left(kInitFieldsLen));
    auto pt = DoubleRatchet::Decrypt(*state, *header, GFBuffer(ct), ad);
    if (!pt) {
      r.status = DecodeStatus::kUNDECRYPTABLE;
      return r;
    }

    // If we already hold a session for this fingerprint and it is our own key,
    // we are messaging ourselves: keep this (responder) end in the loopback slot
    // so it does not clobber the sender session we just created.
    const bool loopback =
        IsSelfIdentity(channel, peer_fpr) && store.LoadSession(peer_fpr);
    if (loopback) {
      store.SaveLoopbackSession(peer_fpr, *state);
    } else {
      store.SaveSession(peer_fpr, *state);
      store.SavePeerSessionId(peer_fpr, sid);
    }
    store.SetSessionPeer(sid, peer_fpr);
    store.MarkPfsConfirmed(peer_fpr);
    r.status = DecodeStatus::kPFS_OK;
    r.plaintext = *pt;
    r.peer_fpr = peer_fpr;
    return r;
  }

  return r;  // unknown type -> kNOT_TOKEN
}

auto InstantMessageOperator::Contains(const QString& text) -> bool {
  QString compact;
  if (!Normalize(text, compact)) return false;
  bool ok = false;
  const QByteArray frame = Base58Decode(compact, ok);
  if (!ok || frame.size() < 2 || static_cast<uint8_t>(frame[0]) != kVersion) {
    return false;
  }
  const auto type = static_cast<uint8_t>(frame[1]);
  return type == kTypeNormal || type == kTypePfsMsg || type == kTypePfsInit;
}

auto InstantMessageOperator::ExportBundle(int channel, const GpgKeyPtr& signer)
    -> QString {
  auto bundle = BuildSignedBundle(channel, signer);
  if (!bundle) return {};
  return Base58Encode(bundle->Serialize().ConvertToQByteArray());
}

auto InstantMessageOperator::ImportBundle(int channel, const QString& text)
    -> QString {
  QString compact;
  if (!Normalize(text, compact)) return {};
  bool ok = false;
  const QByteArray raw = Base58Decode(compact, ok);
  if (!ok) return {};

  auto bundle = ImContactBundle::Parse(GFBuffer(raw));
  if (!bundle) return {};

  auto signer_fpr = ImHandshake::VerifyDetached(channel, bundle->SignedPortion(),
                                                bundle->pgp_sig);
  if (!signer_fpr || *signer_fpr != bundle->pgp_fpr) return {};

  if (!ImSessionStore::GetInstance().SavePeerBundle(*bundle)) return {};
  return bundle->pgp_fpr;
}

auto InstantMessageOperator::FormatVersion() -> int { return kVersion; }

}  // namespace GpgFrontend

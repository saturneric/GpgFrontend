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

#include <QMutex>
#include <QRegularExpression>
#include <array>

#include "core/function/GlobalSettingStation.h"
#include "core/function/SecureRandomGenerator.h"
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
// It is the whitening key: everything below is derived from it plus a fresh
// per-message seed.
// ---------------------------------------------------------------------------

constexpr uint64_t kDefaultWeyl = 0x7F49BA797C9E3175ULL;
constexpr uint64_t kDefaultMul = 0xBF18475B4C6DEE59ULL;
constexpr auto kBookPhraseKey = "im/password_book_phrase";

constexpr std::array<unsigned char, crypto_pwhash_SALTBYTES> kBookKdfSalt = {
    'G', 'F', '-', 'I', 'M', '-', 'b', 'o',
    'o', 'k', '-', 's', 'a', 'l', 't', '1'};
constexpr unsigned long long kBookKdfOps = 3;
constexpr size_t kBookKdfMem = 65536ULL * 1024ULL;  // 64 MiB

// Per-message whitening KDF: intentionally heavier than the (memoized) phrase
// KDF above, because it runs on EVERY encode/decode and is the anti-detection
// cost. A large-scale scanner that holds the default book still has to pay one
// full Argon2id per candidate Base58 message — there is no cheap pre-filter and
// no precomputation (the per-message seed is the salt, so no rainbow table and
// no cross-message amortization). Sized so a single detect/decode is ~0.1-0.2s
// and memory-bound (hard to parallelize on GPU/ASIC at platform scale). This is
// paid symmetrically by legitimate peers; the protection is scale — the scanner
// tests billions of messages, a user tests one.
constexpr unsigned long long kMasterKdfOps = 3;
constexpr size_t kMasterKdfMem = 128ULL * 1024ULL * 1024ULL;  // 128 MiB

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

// The active book's raw bytes. Deriving it from a phrase runs Argon2id, so the
// result is memoized by phrase — Decode()/Contains() call this on every token.
auto ActiveBookBytes() -> GFBuffer {
  static QMutex mutex;
  static bool has_cache = false;
  static QString cached_phrase;
  static GFBuffer cached_book;

  const auto phrase = GetSettings().value(kBookPhraseKey).toString().trimmed();

  QMutexLocker locker(&mutex);
  if (has_cache && phrase == cached_phrase) return cached_book;

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
  cached_book = GFBuffer(QByteArray(reinterpret_cast<const char*>(sub.data()),
                                    static_cast<qsizetype>(sub.size())));
  cached_phrase = phrase;
  has_cache = true;
  return cached_book;
}

// ---------------------------------------------------------------------------
// Whitened wire container. There is NO cleartext marker: the whole inner frame
// (a book-derived recognition tag, the header and the OpenPGP payload, plus
// secret-controlled random padding) is XORed with a keystream and shuffled by a
// permutation, both derived — via a memory-hard KDF — from the book and a fresh
// per-message seed. Only the seed travels in the clear, and it is uniform
// random.
//
//   wire = Base58( seed(16) || permute( inner XOR keystream ) )
//   inner = tag(16) | version(1) | type(1) | payload_len(u32) | payload | pad
// ---------------------------------------------------------------------------

constexpr uint8_t kVersion = 0x02;
constexpr uint8_t kTypeNormal = 0x00;

constexpr int kSeedLen = 16;  // == Argon2 salt length; ChaCha20 nonce is the
                              // first 8 bytes of it.
constexpr int kTagLen = 16;   // secret book-derived recognition value
constexpr int kHeaderLen = kTagLen + 1 + 1 + 4;  // tag|version|type|payload_len
constexpr int kBucket = 64;   // length is rounded up to this before jitter
constexpr int kMinFrame = kHeaderLen;
constexpr int kPrefixLen = kTagLen + 4;  // PRG bytes read before the length (m)
                                         // is known: tag + jitter u32

static_assert(kSeedLen == crypto_pwhash_SALTBYTES);
static_assert(kSeedLen >= crypto_stream_chacha20_NONCEBYTES);

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

auto RoundUp(int v, int mult) -> int { return ((v + mult - 1) / mult) * mult; }

// Memory-hard master key from the book (as password) and the seed (as salt).
// This is what makes even *detecting* a token cost an Argon2id per candidate
// book — there is no cheap pre-filter.
auto DeriveMaster(const GFBuffer& book, const QByteArray& seed,
                  std::array<unsigned char, 32>& master) -> bool {
  if (!EnsureSodiumInit()) return false;
  const auto book_bytes = book.ConvertToQByteArray();
  const int rc = crypto_pwhash(
      master.data(), master.size(), book_bytes.constData(),
      static_cast<unsigned long long>(book_bytes.size()),
      reinterpret_cast<const unsigned char*>(seed.constData()), kMasterKdfOps,
      kMasterKdfMem, crypto_pwhash_ALG_ARGON2ID13);
  return rc == 0;
}

// Deterministic keystream: ChaCha20(master) with the seed's first 8 bytes as
// nonce. The tag, jitter, permutation material and XOR keystream are disjoint
// slices of this one stream.
auto Keystream(const std::array<unsigned char, 32>& master,
               const QByteArray& seed, int len) -> QByteArray {
  QByteArray out(len, Qt::Uninitialized);
  crypto_stream_chacha20(
      reinterpret_cast<unsigned char*>(out.data()),
      static_cast<unsigned long long>(len),
      reinterpret_cast<const unsigned char*>(seed.constData()), master.data());
  return out;
}

// Keyed Fisher-Yates permutation P of [0, m) driven by 4 bytes per step.
auto BuildPermutation(const QByteArray& material, int m) -> QVector<int> {
  QVector<int> p(m);
  for (int i = 0; i < m; ++i) p[i] = i;
  int off = 0;
  for (int i = m - 1; i > 0; --i) {
    const quint32 r = ReadU32BE(material, off);
    off += 4;
    const int j = static_cast<int>(r % static_cast<quint32>(i + 1));
    std::swap(p[i], p[j]);
  }
  return p;
}

// Strip whitespace a messenger may have injected; reject armored PGP text.
auto Normalize(const QString& text, QString& compact) -> bool {
  if (text.contains(QLatin1String("-----BEGIN PGP"))) return false;
  compact = text;
  compact.remove(QRegularExpression(QStringLiteral("\\s")));
  return !compact.isEmpty();
}

}  // namespace

auto InstantMessageOperator::Encode(const GFBuffer& pgp_message) -> QString {
  const auto pgp = pgp_message.ConvertToQByteArray();
  if (pgp.isEmpty()) return {};

  const auto book = ActiveBookBytes();

  auto seed_buf = SecureRandomGenerator::Generate(kSeedLen);
  if (!seed_buf) return {};
  const QByteArray seed = seed_buf->ConvertToQByteArray();

  std::array<unsigned char, 32> master{};
  if (!DeriveMaster(book, seed, master)) return {};

  // Read the tag and jitter first; they fix the padded frame length m.
  const QByteArray prefix = Keystream(master, seed, kPrefixLen);
  const QByteArray tag = prefix.left(kTagLen);
  const quint32 jitter = ReadU32BE(prefix, kTagLen);

  const int content = kHeaderLen + static_cast<int>(pgp.size());
  const int m = RoundUp(content, kBucket) +
                static_cast<int>(jitter % static_cast<quint32>(2 * kBucket));
  const int pad_len = m - content;

  QByteArray inner;
  inner.reserve(m);
  inner.append(tag);
  inner.append(static_cast<char>(kVersion));
  inner.append(static_cast<char>(kTypeNormal));
  AppendU32BE(inner, static_cast<quint32>(pgp.size()));
  inner.append(pgp);
  if (pad_len > 0) {
    auto pad = SecureRandomGenerator::Generate(pad_len);
    if (!pad) {
      sodium_memzero(master.data(), master.size());
      return {};
    }
    inner.append(pad->ConvertToQByteArray());
  }

  const QByteArray stream = Keystream(master, seed, kPrefixLen + 4 * m + m);
  const QByteArray perm_material = stream.mid(kPrefixLen, 4 * m);
  const QByteArray xor_ks = stream.mid(kPrefixLen + 4 * m, m);
  sodium_memzero(master.data(), master.size());

  QByteArray t(m, Qt::Uninitialized);
  for (int i = 0; i < m; ++i) {
    t[i] = static_cast<char>(inner[i] ^ xor_ks[i]);
  }

  const QVector<int> p = BuildPermutation(perm_material, m);
  QByteArray cipher(m, Qt::Uninitialized);
  for (int i = 0; i < m; ++i) cipher[i] = t[p[i]];

  QByteArray wire = seed;
  wire.append(cipher);
  return Base58Encode(wire);
}

auto InstantMessageOperator::Decode(const QString& text) -> DecodeResult {
  DecodeResult r;

  QString compact;
  if (!Normalize(text, compact)) return r;

  bool ok = false;
  const QByteArray wire = Base58Decode(compact, ok);
  if (!ok || wire.size() < kSeedLen + kMinFrame) return r;

  const QByteArray seed = wire.left(kSeedLen);
  const QByteArray cipher = wire.mid(kSeedLen);
  const int m = static_cast<int>(cipher.size());

  const auto book = ActiveBookBytes();
  std::array<unsigned char, 32> master{};
  if (!DeriveMaster(book, seed, master)) return r;

  const QByteArray stream = Keystream(master, seed, kPrefixLen + 4 * m + m);
  sodium_memzero(master.data(), master.size());
  const QByteArray tag_expected = stream.left(kTagLen);
  const QByteArray perm_material = stream.mid(kPrefixLen, 4 * m);
  const QByteArray xor_ks = stream.mid(kPrefixLen + 4 * m, m);

  const QVector<int> p = BuildPermutation(perm_material, m);
  QByteArray t(m, Qt::Uninitialized);
  for (int i = 0; i < m; ++i) t[p[i]] = cipher[i];

  QByteArray inner(m, Qt::Uninitialized);
  for (int i = 0; i < m; ++i) {
    inner[i] = static_cast<char>(t[i] ^ xor_ks[i]);
  }

  // Recognition: the leading bytes must equal the secret, book-derived tag.
  if (sodium_memcmp(inner.constData(), tag_expected.constData(), kTagLen) != 0) {
    return r;  // not one of ours (or a different book)
  }
  if (static_cast<uint8_t>(inner[kTagLen]) != kVersion) return r;
  if (static_cast<uint8_t>(inner[kTagLen + 1]) != kTypeNormal) return r;

  const quint32 payload_len = ReadU32BE(inner, kTagLen + 2);
  if (kHeaderLen + static_cast<qint64>(payload_len) > m) return r;

  r.pgp_message = GFBuffer(inner.mid(kHeaderLen, static_cast<int>(payload_len)));
  r.ok = true;
  return r;
}

auto InstantMessageOperator::Contains(const QString& text) -> bool {
  return Decode(text).ok;
}

auto InstantMessageOperator::FormatVersion() -> int { return kVersion; }

}  // namespace GpgFrontend

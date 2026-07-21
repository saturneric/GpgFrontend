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
#include <algorithm>
#include <array>
#include <cstring>
#include <string_view>

#include "core/function/CacheManager.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/SecureRandomGenerator.h"
#include "core/utils/CommonUtils.h"

namespace GpgFrontend {

namespace {

// ---------------------------------------------------------------------------
// Base58 (Bitcoin/IPFS alphabet). Purely alphanumeric, no 0 O I l, and nothing
// a messenger turns into markdown/link/soft-wrap, so a token stays one word.
// ---------------------------------------------------------------------------

constexpr std::string_view kB58Alphabet =
    "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

// Reverse of kB58Alphabet: byte value -> digit, or -1 when not in the alphabet.
// Built at compile time so decoding — and the shape pre-filter in Normalize() —
// costs one table lookup per character instead of a scan of the alphabet.
constexpr uint8_t kB58Invalid = 0xFF;

constexpr auto MakeB58Reverse() -> std::array<uint8_t, 256> {
  std::array<uint8_t, 256> t{};
  for (auto& v : t) v = kB58Invalid;
  for (size_t i = 0; i < kB58Alphabet.size(); ++i) {
    t[static_cast<uint8_t>(kB58Alphabet[i])] = static_cast<uint8_t>(i);
  }
  return t;
}
constexpr std::array<uint8_t, 256> kB58Reverse = MakeB58Reverse();

// Whether @p qc is a Base58 character. ASCII-only by construction: every
// character outside 0x00-0x7F is rejected before the table is consulted.
constexpr auto IsBase58Char(QChar qc) -> bool {
  return qc.unicode() <= 0x7F && kB58Reverse[qc.unicode()] != kB58Invalid;
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
    if (!IsBase58Char(qc)) return {};
    int carry = kB58Reverse[qc.unicode()];
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

// A book is a 256-byte permutation, so the space is log2(256!) ≈ 1684 bits.
// Both the phrase-derived and the default book are seeded with a full 256-bit
// key and shuffled from a ChaCha20 keystream, so neither is the weak one: the
// default is exactly as strong a permutation as anything a phrase produces.
// (What the default cannot give is *secrecy*: it ships in every copy of
// GpgFrontend, so only a shared phrase hides messages from someone who knows
// this program.)
constexpr size_t kBookSeedLen = 32;
using BookSeed = std::array<unsigned char, kBookSeedLen>;

// Fixed, from the platform CSPRNG at authoring time.
constexpr BookSeed kDefaultBookSeed = {
    0x97, 0x47, 0x00, 0x3E, 0xDB, 0x08, 0xA8, 0x4F, 0x72, 0xE6, 0xAE,
    0xAA, 0xA3, 0x15, 0x54, 0xF5, 0x4C, 0x65, 0x03, 0x7B, 0x0E, 0x75,
    0x2A, 0xE1, 0x52, 0x59, 0x8B, 0x46, 0xB5, 0x20, 0x64, 0xF0};

// The phrase is a secret: it lives in the encrypted durable cache, not in the
// plaintext settings file. kLegacyBookPhraseSettingsKey is the settings key
// older versions wrote it to, kept only to migrate it out (and erase it).
constexpr auto kBookPhraseCacheKey = "im/password_book_phrase";
constexpr auto kLegacyBookPhraseSettingsKey = "im/password_book_phrase";

// The cache flushes only non-empty values to disk, so "no phrase" cannot be
// stored as an empty blob, or clearing the phrase would not survive a restart.
// The blob is therefore a one-byte version prefix plus the UTF-8 phrase, and a
// cleared phrase is the prefix alone.
constexpr char kBookPhraseBlobVersion = '\x01';

auto EncodeBookPhraseBlob(const QString& phrase) -> GFBuffer {
  QByteArray blob(1, kBookPhraseBlobVersion);
  blob.append(phrase.toUtf8());
  return GFBuffer(blob);
}

auto DecodeBookPhraseBlob(const GFBuffer& blob) -> QString {
  const auto raw = blob.ConvertToQByteArray();
  if (raw.isEmpty() || raw.at(0) != kBookPhraseBlobVersion) return {};
  return QString::fromUtf8(raw.mid(1)).trimmed();
}

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

auto DeriveBookSeed(const QString& phrase, BookSeed& seed) -> bool {
  if (!EnsureSodiumInit()) return false;

  const QByteArray pw = phrase.toUtf8();
  const int rc = crypto_pwhash(seed.data(), seed.size(), pw.constData(),
                               static_cast<unsigned long long>(pw.size()),
                               kBookKdfSalt.data(), kBookKdfOps, kBookKdfMem,
                               crypto_pwhash_ALG_ARGON2ID13);
  if (rc != 0) {
    LOG_E() << "argon2id derivation for instant-messaging book failed";
    return false;
  }
  return true;
}

// Fisher-Yates over a ChaCha20 keystream: every one of the 256! permutations is
// reachable, and the seed is the only thing bounding the entropy. Draws are 64
// bits wide, so the modulo bias is below 2^-56, far under anything that could
// bias the resulting table.
auto BuildBook(const BookSeed& seed) -> std::array<uint8_t, 256> {
  std::array<uint8_t, 256> sub{};
  for (int i = 0; i < 256; ++i) sub[i] = static_cast<uint8_t>(i);

  constexpr std::array<unsigned char, crypto_stream_chacha20_NONCEBYTES>
      kBookStreamNonce = {'G', 'F', '-', 'I', 'M', '-', 'b', 'k'};

  std::array<unsigned char, 255 * sizeof(uint64_t)> keystream{};
  crypto_stream_chacha20(keystream.data(), keystream.size(),
                         kBookStreamNonce.data(), seed.data());

  for (int i = 255; i > 0; --i) {
    uint64_t z = 0;
    std::memcpy(&z, keystream.data() + (255 - i) * sizeof(uint64_t), sizeof(z));
    const int j = static_cast<int>(z % static_cast<uint64_t>(i + 1));
    std::swap(sub[i], sub[j]);
  }

  sodium_memzero(keystream.data(), keystream.size());
  return sub;
}

// The configured phrase, read from the encrypted durable cache. A phrase left
// in the plaintext settings file by an older version is moved into the cache
// and erased from settings the first time we look.
auto ReadBookPhrase() -> QString {
  auto settings = GetSettings();
  if (settings.contains(kLegacyBookPhraseSettingsKey)) {
    const auto legacy =
        settings.value(kLegacyBookPhraseSettingsKey).toString().trimmed();
    settings.remove(kLegacyBookPhraseSettingsKey);
    settings.sync();
    if (!legacy.isEmpty()) {
      CacheManager::GetInstance().SaveSecDurableCache(
          kBookPhraseCacheKey, EncodeBookPhraseBlob(legacy), true);
      return legacy;
    }
  }

  return DecodeBookPhraseBlob(
      CacheManager::GetInstance().LoadSecDurableCache(kBookPhraseCacheKey));
}

// The raw book bytes for a phrase. Deriving one runs Argon2id, so results are
// memoized by phrase: Decode() calls this on every token, and the settings UI
// previews the fingerprint of phrases the user types.
auto BookBytesFor(const QString& phrase) -> GFBuffer {
  static QMutex mutex;
  static QHash<QString, GFBuffer> cache;

  QMutexLocker locker(&mutex);
  if (const auto it = cache.constFind(phrase); it != cache.constEnd()) {
    return *it;
  }

  BookSeed seed = kDefaultBookSeed;
  if (!phrase.isEmpty()) {
    BookSeed derived{};
    if (DeriveBookSeed(phrase, derived)) seed = derived;
  }
  const auto sub = BuildBook(seed);
  auto book = GFBuffer(QByteArray(reinterpret_cast<const char*>(sub.data()),
                                  static_cast<qsizetype>(sub.size())));

  // Bounded: a user tries a handful of phrases per session, and every entry
  // holds a derived secret we would rather not keep around indefinitely.
  if (cache.size() >= 8) cache.clear();
  cache.insert(phrase, book);
  return book;
}

// The active book's raw bytes.
auto ActiveBookBytes() -> GFBuffer { return BookBytesFor(ReadBookPhrase()); }

// Short, domain-separated digest of a book. See BookFingerprint()'s docs for
// why it is truncated and must never travel on the wire.
auto FingerprintOfBook(const GFBuffer& book) -> QString {
  QCryptographicHash hash(QCryptographicHash::Sha256);
  hash.addData(QByteArrayLiteral("GpgFrontend-IM-book-fingerprint-v1"));
  hash.addData(book.ConvertToQByteArray());

  const auto hex = hash.result().left(4).toHex().toUpper();
  return QString("%1-%2").arg(QString::fromLatin1(hex.left(4)),
                              QString::fromLatin1(hex.mid(4, 4)));
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
//
// The padding is at least 30% of the content and the total is then quantized
// onto a ladder of Base58 lengths (see SnapFrameBytes), so the token's length
// is a coarse, keyed function of the payload's rather than a reflection of it.
// ---------------------------------------------------------------------------

// 0x03: books are now full 256-bit-seeded permutations, so every book,
// the default included, differs from the 0x02 ones. Old tokens do not decode.
constexpr uint8_t kVersion = 0x03;
constexpr uint8_t kTypeNormal = 0x00;

constexpr int kSeedLen = 16;  // == Argon2 salt length; ChaCha20 nonce is the
                              // first 8 bytes of it.
constexpr int kTagLen = 16;   // secret book-derived recognition value
constexpr int kHeaderLen = kTagLen + 1 + 1 + 4;  // tag|version|type|payload_len
constexpr int kMinFrame = kHeaderLen;
constexpr int kPrefixLen = kTagLen + 4;  // PRG bytes read before the length (m)
                                         // is known: tag + jitter u32

// Random padding, as a fraction of the real content, in 1024ths: 30.0% to
// 50.0%. Drawn from the keystream, so the amount is itself a secret of the
// book — an observer cannot subtract a known overhead to recover the true
// payload size, and repeated sends of the same message land on different
// lengths.
constexpr quint32 kPadFracMin = 307;   // 307/1024 ≈ 30.0%
constexpr quint32 kPadFracSpan = 206;  // up to 512/1024 = 50.0%

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

// Wire lengths (in bytes) whose Base58 encodings are lengths that already
// occur in the wild, so a token that fits one is not merely "some Base58
// blob" but the same size as things people paste into chats every day:
//
//   25 -> 34 chars  P2PKH / P2SH address        32 -> 44 chars  Solana pubkey
//   37 -> 51 chars  WIF (uncompressed)          38 -> 52 chars  WIF
//   39 -> 53 chars  BIP38 encrypted key         64 -> 88 chars  Ed25519 sig
//   82 -> 111 chars xpub / xprv
//
// An OpenPGP message is much larger than any of these, so in practice only a
// contrived payload lands on one; they are the bottom of the ladder so that it
// happens whenever it can, and cost nothing when it cannot.
constexpr std::array<int, 7> kClassicFrameBytes = {25, 32, 37, 38, 39, 64, 82};

// Past the classical rungs the ladder grows by an eighth per step. That caps
// what snapping costs (≤12.5% on top of the random padding) while keeping the
// set of reachable token lengths small: many different messages quantize onto
// the same rung, so the length of a token says little about the length of what
// is inside it.
auto SnapFrameBytes(int n) -> int {
  for (const int c : kClassicFrameBytes) {
    if (n <= c) return c;
  }
  int v = kClassicFrameBytes.back();
  while (v < n) v += (v + 7) / 8;
  return v;
}

// ---------------------------------------------------------------------------
// Input bounds.
//
// Base58Decode is O(n^2) in the length of its input, and Decode() is handed
// whatever happens to be in the editor, so an unbounded input is a self-
// inflicted denial of service. The cap is deliberately generous: it sits far
// above any message a chat app will carry, and its only job is to stop a
// pathological paste.
//
// The two directions have to agree. If Encode() could emit a token longer than
// Decode() accepts, two copies of GpgFrontend would silently fail to talk to
// each other — so the encode-side limit is *derived* from the decode-side one
// and the agreement is a static_assert, not a comment.
// ---------------------------------------------------------------------------

constexpr int kMaxTokenChars = 16384;

// 732/1000 under-estimates log(58)/log(256) = 0.7322476, so this never
// over-states how many bytes kMaxTokenChars characters can carry.
constexpr int kMaxWireBytes = kMaxTokenChars * 732 / 1000;

// Base58 never shrinks its input (58 < 256), so n characters decode to at most
// n bytes: a token shorter than the smallest wire Decode() could accept can be
// rejected on length alone, without decoding anything.
constexpr int kMinTokenChars = kSeedLen + kMinFrame;

// The largest wire Encode() can produce for a payload of @p payload bytes.
// Mirrors the length arithmetic in Encode():
//   content = kHeaderLen + payload
//   padded  = content + (content * frac)/1024,  frac <=
//   kPadFracMin+kPadFracSpan
//           <= content + content/2                        (the fraction caps at
//           1/2)
//   wire    = SnapFrameBytes(kSeedLen + padded)
// and SnapFrameBytes stops at the first rung >= n, each rung being the previous
// one plus an eighth, so it returns at most (n-1) + (n+6)/8.
constexpr auto WorstCaseWireBytes(int payload) -> int {
  const int content = kHeaderLen + payload;
  const int padded = content + (content / 2);
  const int n = kSeedLen + padded;
  return (n - 1) + ((n + 6) / 8);
}

// Round, with margin: WorstCaseWireBytes(7000) = 11867 against a 11992 budget.
constexpr int kMaxPayloadBytes = 7000;

static_assert(WorstCaseWireBytes(kMaxPayloadBytes) <= kMaxWireBytes,
              "Encode() may emit a wire longer than Decode() accepts");

// And the other direction: 1366/1000 over-estimates log(256)/log(58) =
// 1.3656552, so a wire of kMaxWireBytes never Base58s past kMaxTokenChars.
static_assert((kMaxWireBytes * 1366 / 1000) + 1 <= kMaxTokenChars,
              "Encode() may emit a token longer than Decode() accepts");

// Memory-hard master key from the book (as password) and the seed (as salt).
// This is what makes even *detecting* a token cost an Argon2id per candidate
// book — there is no cheap pre-filter.
auto DeriveMaster(const GFBuffer& book, const QByteArray& seed,
                  std::array<unsigned char, 32>& master) -> bool {
  if (!EnsureSodiumInit()) return false;
  const auto book_bytes = book.ConvertToQByteArray();
  const int rc =
      crypto_pwhash(master.data(), master.size(), book_bytes.constData(),
                    static_cast<unsigned long long>(book_bytes.size()),
                    reinterpret_cast<const unsigned char*>(seed.constData()),
                    kMasterKdfOps, kMasterKdfMem, crypto_pwhash_ALG_ARGON2ID13);
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

// Strip whitespace a messenger may have injected, reject armored PGP text, and
// test the token's public shape.
//
// This is the ONLY pre-filter, and it deliberately tests nothing an observer
// does not already compute for free: the compacted length and whether every
// character is in the Base58 alphabet. It must never consult the book, the tag
// or any derived value — a book-dependent pre-filter would hand a large-scale
// scanner exactly the cheap discriminator this design spends one memory-hard
// derivation per candidate to deny it. Everything that survives this test still
// pays a full Argon2id, so the adversary's cost per candidate is unchanged.
//
// What it does buy us is local: ordinary text now stops at its first space or
// punctuation mark instead of reaching the O(n^2) decoder and the 128 MiB KDF.
auto Normalize(const QString& text, QString& compact) -> bool {
  if (text.contains(QLatin1String("-----BEGIN PGP"))) return false;

  compact.clear();
  compact.reserve(std::min<qsizetype>(text.size(), kMaxTokenChars));
  for (const QChar qc : text) {
    if (qc.isSpace()) continue;
    if (!IsBase58Char(qc)) return false;
    if (compact.size() == kMaxTokenChars) return false;
    compact.append(qc);
  }
  return compact.size() >= kMinTokenChars;
}

}  // namespace

auto InstantMessageOperator::Encode(const GFBuffer& pgp_message) -> QString {
  const auto pgp = pgp_message.ConvertToQByteArray();
  if (pgp.isEmpty()) return {};

  // Refuse anything Decode() would then refuse to read back. Callers should
  // check MaxPayloadBytes() first so they can say something useful about it;
  // this is the invariant guard for everyone else.
  if (pgp.size() > kMaxPayloadBytes) {
    LOG_W() << "instant-messaging payload too long:" << pgp.size()
            << "limit:" << kMaxPayloadBytes;
    return {};
  }

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

  // Length: pad by a keystream-chosen 30-50% of the content, then snap the
  // whole wire (seed included, since that is what gets Base58'd) up onto the
  // length ladder.
  const int content = kHeaderLen + static_cast<int>(pgp.size());
  const int padded =
      content + static_cast<int>((static_cast<qint64>(content) *
                                  (kPadFracMin + (jitter % kPadFracSpan))) /
                                 1024);
  const int m = std::max(padded, SnapFrameBytes(kSeedLen + padded) - kSeedLen);
  const int pad_len = m - content;

  // Unreachable given the payload check above; see the WorstCaseWireBytes
  // static_assert. Kept so a future change to the padding or the ladder cannot
  // quietly start emitting tokens Decode() refuses.
  if (kSeedLen + m > kMaxWireBytes) {
    LOG_E() << "instant-messaging frame exceeds the wire budget:" << m;
    sodium_memzero(master.data(), master.size());
    return {};
  }

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
  if (!ok || wire.size() < kSeedLen + kMinFrame ||
      wire.size() > kMaxWireBytes) {
    return r;
  }

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
  if (sodium_memcmp(inner.constData(), tag_expected.constData(), kTagLen) !=
      0) {
    return r;  // not one of ours (or a different book)
  }
  if (static_cast<uint8_t>(inner[kTagLen]) != kVersion) return r;
  if (static_cast<uint8_t>(inner[kTagLen + 1]) != kTypeNormal) return r;

  const quint32 payload_len = ReadU32BE(inner, kTagLen + 2);
  if (kHeaderLen + static_cast<qint64>(payload_len) > m) return r;

  r.pgp_message =
      GFBuffer(inner.mid(kHeaderLen, static_cast<int>(payload_len)));
  r.ok = true;
  return r;
}

auto InstantMessageOperator::FormatVersion() -> int { return kVersion; }

auto InstantMessageOperator::MaxPayloadBytes() -> int {
  return kMaxPayloadBytes;
}

auto InstantMessageOperator::BookConfigured() -> bool {
  return !ReadBookPhrase().isEmpty();
}

auto InstantMessageOperator::BookPhrase() -> QString {
  return ReadBookPhrase();
}

void InstantMessageOperator::SetBookPhrase(const QString& phrase) {
  const auto trimmed = phrase.trimmed();

  // Drop the legacy plaintext copy if it is still there, so clearing the
  // phrase here cannot be undone by a later migration read.
  auto settings = GetSettings();
  if (settings.contains(kLegacyBookPhraseSettingsKey)) {
    settings.remove(kLegacyBookPhraseSettingsKey);
    settings.sync();
  }

  CacheManager::GetInstance().SaveSecDurableCache(
      kBookPhraseCacheKey, EncodeBookPhraseBlob(trimmed), true);
}

// A short, domain-separated digest of the active book, so two peers can
// eyeball that they are on the same book without revealing the phrase.
//
// Truncated on purpose: this value identifies a book, and anyone who learns
// it can test candidate phrases offline (derive book, hash, compare). Eight
// hex digits keep it comparable by eye while leaving enough collisions that a
// match is evidence, not proof. The dictionary attack still costs one
// Argon2id (64 MiB) per candidate: the same wall the rest of the design
// leans on. Never put this on the wire.
auto InstantMessageOperator::GeneratePhrase() -> QString {
  constexpr int kPhraseLen = 256;
  // Largest multiple of 58 that fits in a byte: values at or above it are
  // rejected rather than folded, so every character is uniform.
  constexpr int kRejectAbove = 232;

  QString out;
  out.reserve(kPhraseLen);
  while (out.size() < kPhraseLen) {
    const auto buffer =
        SecureRandomGenerator::Generate(kPhraseLen - out.size());
    if (!buffer) {
      LOG_E() << "csprng unavailable, cannot generate a book phrase";
      return {};
    }

    const auto bytes = buffer->ConvertToQByteArray();
    for (const char byte : bytes) {
      const auto value = static_cast<uint8_t>(byte);
      if (value >= kRejectAbove) continue;
      out.append(QLatin1Char(kB58Alphabet[value % 58]));
    }
  }
  return out;
}

auto InstantMessageOperator::BookFingerprint() -> QString {
  return FingerprintOfBook(ActiveBookBytes());
}

auto InstantMessageOperator::BookFingerprintOf(const QString& phrase)
    -> QString {
  return FingerprintOfBook(BookBytesFor(phrase.trimmed()));
}

}  // namespace GpgFrontend

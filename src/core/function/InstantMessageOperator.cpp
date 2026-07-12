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
#include <QRandomGenerator>
#include <QRegularExpression>
#include <array>
#include <utility>

#include "core/function/GlobalSettingStation.h"
#include "core/utils/CommonUtils.h"

namespace GpgFrontend {

namespace {

// A real encrypted message is far larger; this rejects tiny accidental matches.
constexpr int kImMinBytes = 16;

// Base58 alphabet (the canonical Bitcoin/IPFS ordering). Purely alphanumeric
// and deliberately omits the visually ambiguous 0 O I l; no '-', '_', '+', '/',
// '=' — nothing a messenger can turn into markdown (italic/strikethrough), a
// link, or a soft-wrap break, so the token stays one unbreakable, copy-paste-
// safe word.
constexpr char kB58Alphabet[] =
    "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

auto Base58DigitValue(char c) -> int {
  for (int i = 0; i < 58; ++i) {
    if (kB58Alphabet[i] == c) return i;
  }
  return -1;
}

// Big-integer base-256 -> base-58 conversion (leading zero bytes are preserved
// as leading '1' characters). O(n^2); intended for chat-sized messages.
auto Base58Encode(const QByteArray& in) -> QString {
  QVector<quint16> digits;  // little-endian base-58 digits
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
  QVector<quint16> bytes;  // little-endian base-256
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
// Password book + per-message random seed ("random layer").
//
// The book is a shared 256-byte table used as key material — never applied to
// the message directly. Each encryption draws a fresh random seed and derives a
// keystream SHA-256(book || seed || counter) that is XORed with the raw
// message, so the book's structure never appears in the output and never
// repeats across messages. It is an obfuscation/format layer, NOT cryptographic
// protection — real confidentiality is the OpenPGP encryption underneath. The
// envelope is versioned and identifies the book by hash, so the format can
// evolve and support alternative books later.
// ---------------------------------------------------------------------------

constexpr uint8_t kImVersion = 0x01;  // envelope version, for evolution
constexpr int kBookIdLen = 4;         // SHA-256(book) prefix, identifies a book
constexpr int kSeedLen = 16;          // fresh random seed per message
constexpr int kHeaderLen =
    1 + kBookIdLen + kSeedLen;  // version + book_id + seed

struct PasswordBook {
  std::array<uint8_t, 256>
      sub{};      // book content, used as keystream key material
  QByteArray id;  // first kBookIdLen bytes of SHA-256(sub)
};

// The two mixer constants of the splitmix64 shuffle that builds a book: a Weyl
// increment and a multiplier. These are the shared DEFAULT, so every stock
// GpgFrontend produces the same default book and interoperates out of the box.
// A user-provided phrase derives its own pair (see DeriveBookConstants), giving
// that user a private book while default installs still talk to each other.
constexpr uint64_t kDefaultWeyl = 0x7F49BA797C9E3175ULL;
constexpr uint64_t kDefaultMul = 0xBF18475B4C6DEE59ULL;
constexpr auto kBookPhraseKey = "im/password_book_phrase";

// Fixed, non-secret salt: the derivation must be deterministic so the same
// phrase yields the same book on every machine. The salt only domain-separates
// this KDF from other libsodium uses in GpgFrontend.
constexpr std::array<unsigned char, crypto_pwhash_SALTBYTES> kBookKdfSalt = {
    'G', 'F', '-', 'I', 'M', '-', 'b', 'o',
    'o', 'k', '-', 's', 'a', 'l', 't', '1'};

// Argon2id work factors (mirroring AESCryptoHelper): memory-hard so the phrase
// cannot be cheaply brute-forced from a captured message's public book id.
constexpr unsigned long long kBookKdfOps = 3;
constexpr size_t kBookKdfMem = 65536ULL * 1024ULL;  // 64 MiB

// Derive the two 64-bit book constants from a free-text phrase using Argon2id
// (kept odd, as a good Weyl increment / bijective multiplier want to be).
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

// Build a book from its two constants: a permutation of 0..255 produced by a
// seeded splitmix64 Fisher-Yates shuffle, plus the SHA-256 id of that table.
auto BuildBook(uint64_t weyl, uint64_t mul) -> PasswordBook {
  PasswordBook book;
  for (int i = 0; i < 256; ++i) book.sub[i] = static_cast<uint8_t>(i);

  uint64_t s = weyl;
  for (int i = 255; i > 0; --i) {
    s += weyl;
    uint64_t z = s;
    z = (z ^ (z >> 30)) * mul;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    z = z ^ (z >> 31);
    const int j = static_cast<int>(z % static_cast<uint64_t>(i + 1));
    std::swap(book.sub[i], book.sub[j]);
  }

  const QByteArray sub_bytes(reinterpret_cast<const char*>(book.sub.data()),
                             static_cast<qsizetype>(book.sub.size()));
  book.id = QCryptographicHash::hash(sub_bytes, QCryptographicHash::Sha256)
                .left(kBookIdLen);
  return book;
}

// The shared default book — always available so stock installs interoperate.
auto DefaultBook() -> const PasswordBook& {
  static const PasswordBook kBook = BuildBook(kDefaultWeyl, kDefaultMul);
  return kBook;
}

// The book used for encoding: the phrase-derived one if configured, else the
// default. Cached by phrase so we rebuild only when the setting changes.
auto ConfiguredBook() -> const PasswordBook& {
  const auto phrase = GetSettings().value(kBookPhraseKey).toString().trimmed();
  if (phrase.isEmpty()) return DefaultBook();

  static QString cached_phrase;
  static PasswordBook cached_book;
  static bool cached = false;
  if (!cached || cached_phrase != phrase) {
    uint64_t weyl = 0;
    uint64_t mul = 0;
    if (!DeriveBookConstants(phrase, weyl, mul)) {
      // Derivation failed (e.g. libsodium unavailable); fall back to the shared
      // default book rather than silently producing an inconsistent one.
      return DefaultBook();
    }
    cached_book = BuildBook(weyl, mul);
    cached_phrase = phrase;
    cached = true;
  }
  return cached_book;
}

// Resolve a book by its embedded id: the active (configured) book, or the
// default, so a customised install can still read stock default messages.
auto FindBook(const QByteArray& id) -> const PasswordBook* {
  const auto& cfg = ConfiguredBook();
  if (id == cfg.id) return &cfg;
  const auto& def = DefaultBook();
  if (id == def.id) return &def;
  return nullptr;
}

// Derive a per-message keystream from the book (used as key material) and the
// random seed, via SHA-256 in counter mode. Because the book enters only inside
// the hash together with a fresh random seed on every message, the book's
// structure never appears in the output nor repeats across messages.
auto DeriveKeystream(const PasswordBook& book, const QByteArray& seed, int len)
    -> QByteArray {
  const QByteArray key(reinterpret_cast<const char*>(book.sub.data()),
                       static_cast<qsizetype>(book.sub.size()));
  QByteArray ks;
  ks.reserve(len + 32);
  for (quint32 counter = 0; ks.size() < len; ++counter) {
    QByteArray input = key;
    input.append(seed);
    input.append(static_cast<char>((counter >> 24) & 0xFFU));
    input.append(static_cast<char>((counter >> 16) & 0xFFU));
    input.append(static_cast<char>((counter >> 8) & 0xFFU));
    input.append(static_cast<char>(counter & 0xFFU));
    ks.append(QCryptographicHash::hash(input, QCryptographicHash::Sha256));
  }
  ks.truncate(len);
  return ks;
}

// True if the bytes begin like an encrypted OpenPGP message, i.e. with a
// public-key (PKESK, tag 1) or symmetric (SKESK, tag 3) session-key packet.
auto LooksLikeOpenPGPMessage(const QByteArray& b) -> bool {
  if (b.size() < kImMinBytes) return false;

  const auto c = static_cast<uint8_t>(b.at(0));
  if ((c & 0x80U) == 0) return false;  // every packet header has bit 7 set

  // New-format packets carry the 6-bit tag in the low bits; old-format packets
  // carry a 4-bit tag in bits 5..2.
  const auto tag =
      static_cast<int>((c & 0x40U) != 0 ? (c & 0x3FU) : ((c >> 2) & 0x0FU));
  return tag == 1 || tag == 3;
}

}  // namespace

auto InstantMessageOperator::Encode(const GFBuffer& binary_message) -> QString {
  const auto raw = binary_message.ConvertToQByteArray();
  if (raw.isEmpty()) return {};

  const PasswordBook& book = ConfiguredBook();

  // Fresh random seed per message: it randomises the keystream so the book is
  // never applied the same way twice and its structure stays hidden.
  QByteArray seed(kSeedLen, Qt::Uninitialized);
  for (int i = 0; i < kSeedLen; ++i) {
    seed[i] = static_cast<char>(QRandomGenerator::global()->generate() & 0xFFU);
  }

  const QByteArray ks =
      DeriveKeystream(book, seed, static_cast<int>(raw.size()));

  QByteArray frame;
  frame.reserve(kHeaderLen + raw.size());
  frame.append(static_cast<char>(kImVersion));
  frame.append(book.id);  // kBookIdLen bytes
  frame.append(seed);     // kSeedLen bytes
  for (int i = 0; i < raw.size(); ++i) {
    frame.append(static_cast<char>(raw[i] ^ ks[i]));
  }

  return Base58Encode(frame);
}

auto InstantMessageOperator::Inspect(
    const QString& text, GFBuffer& out,
    InstantMessageOperator::ImMessageInfo* info) -> DetectStatus {
  // A standard ASCII-armored block is handled by the normal decrypt path.
  if (text.contains(QLatin1String("-----BEGIN PGP"))) {
    return DetectStatus::kNOT_TOKEN;
  }

  // The token is the whole message; tolerate stray whitespace/line breaks a
  // messenger may have introduced. Base58Decode itself rejects any character
  // outside the alphabet, so no separate charset check is needed.
  QString compact = text;
  compact.remove(QRegularExpression(QStringLiteral("\\s")));
  if (compact.isEmpty()) return DetectStatus::kNOT_TOKEN;

  bool ok = false;
  const auto frame = Base58Decode(compact, ok);
  if (!ok || frame.size() < kHeaderLen + kImMinBytes) {
    return DetectStatus::kNOT_TOKEN;
  }

  // The version byte is the primary signature that this is meant to be one of
  // our tokens; without it, treat the input as ordinary text.
  if (static_cast<uint8_t>(frame[0]) != kImVersion) {
    return DetectStatus::kNOT_TOKEN;
  }

  const QByteArray book_id = frame.mid(1, kBookIdLen);
  if (info != nullptr) {
    info->version = static_cast<uint8_t>(frame[0]);
    info->book_id = book_id;
    info->encoded_length = static_cast<int>(compact.size());
    info->payload_size = static_cast<int>(frame.size() - kHeaderLen);
  }

  // Looks like our token, but we don't have the book it was encoded with — the
  // sender used a different Message Book Phrase.
  const PasswordBook* book = FindBook(book_id);
  if (book == nullptr) return DetectStatus::kBOOK_MISMATCH;

  const QByteArray seed = frame.mid(1 + kBookIdLen, kSeedLen);
  const QByteArray scrambled = frame.mid(kHeaderLen);

  // Re-derive the same keystream and XOR back to the raw OpenPGP message.
  const QByteArray ks =
      DeriveKeystream(*book, seed, static_cast<int>(scrambled.size()));
  QByteArray raw(scrambled.size(), Qt::Uninitialized);
  for (int i = 0; i < scrambled.size(); ++i) {
    raw[i] = static_cast<char>(scrambled[i] ^ ks[i]);
  }

  // Book matched, but the recovered bytes are not an OpenPGP message: the token
  // was truncated or altered in transit.
  if (!LooksLikeOpenPGPMessage(raw)) return DetectStatus::kMALFORMED;

  if (info != nullptr) {
    info->book_id = book->id;
    info->payload_size = static_cast<int>(raw.size());
  }
  out = GFBuffer(raw);
  return DetectStatus::kOK;
}

auto InstantMessageOperator::Detect(const QString& text, GFBuffer& out,
                                    InstantMessageOperator::ImMessageInfo* info)
    -> bool {
  return Inspect(text, out, info) == DetectStatus::kOK;
}

auto InstantMessageOperator::Contains(const QString& text) -> bool {
  GFBuffer tmp;
  return Detect(text, tmp);
}

auto InstantMessageOperator::FormatVersion() -> int { return kImVersion; }

auto InstantMessageOperator::ActiveBookId() -> QByteArray {
  return ConfiguredBook().id;
}

}  // namespace GpgFrontend

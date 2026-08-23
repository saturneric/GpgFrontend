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

#include "core/profile/ProfilePackageStream.h"

#include <sodium.h>

#include <QScopeGuard>
#include <cstring>

#include "core/utils/CommonUtils.h"

namespace GpgFrontend {

namespace {

constexpr size_t kSaltLen = crypto_pwhash_SALTBYTES;
constexpr size_t kStreamKeyLen = crypto_secretstream_xchacha20poly1305_KEYBYTES;
constexpr size_t kStreamHeaderLen =
    crypto_secretstream_xchacha20poly1305_HEADERBYTES;
constexpr size_t kChunkOverhead = crypto_secretstream_xchacha20poly1305_ABYTES;

/// The same parameters the one-shot container uses. Deliberately unchanged:
/// this is a new way of framing the same work, not a new opinion about how hard
/// a passphrase should be to grind.
constexpr unsigned long long kArgon2OpsLimit = 3;
constexpr size_t kArgon2MemLimit = 65536ULL * 1024ULL;  // 64 MiB

/// A chunk claiming more than this is a malformed or hostile file rather than
/// one this program wrote; the writer never emits more than the chunk size plus
/// the stream's own overhead.
constexpr qint64 kMaxChunkBytes =
    kProfilePackageChunkBytes + static_cast<qint64>(kChunkOverhead) + 64;

auto DeriveStreamKey(const GFBuffer &passphrase, const unsigned char *salt)
    -> GFBufferOrNone {
  if (!EnsureSodiumInit()) return {};

  GFBuffer key(kStreamKeyLen);
  const int rc = crypto_pwhash(
      reinterpret_cast<unsigned char *>(key.Data()),
      static_cast<unsigned long long>(key.Size()), passphrase.Data(),
      static_cast<unsigned long long>(passphrase.Size()), salt, kArgon2OpsLimit,
      kArgon2MemLimit, crypto_pwhash_ALG_ARGON2ID13);

  if (rc != 0) {
    LOG_E() << "crypto_pwhash failed while opening a package stream";
    return {};
  }
  return key;
}

void PutBigEndian32(char *out, quint32 value) {
  auto *bytes = reinterpret_cast<unsigned char *>(out);
  bytes[0] = static_cast<unsigned char>((value >> 24) & 0xFF);
  bytes[1] = static_cast<unsigned char>((value >> 16) & 0xFF);
  bytes[2] = static_cast<unsigned char>((value >> 8) & 0xFF);
  bytes[3] = static_cast<unsigned char>(value & 0xFF);
}

auto ReadBigEndian32(const char *in) -> quint32 {
  const auto *bytes = reinterpret_cast<const unsigned char *>(in);
  return (static_cast<quint32>(bytes[0]) << 24) |
         (static_cast<quint32>(bytes[1]) << 16) |
         (static_cast<quint32>(bytes[2]) << 8) | static_cast<quint32>(bytes[3]);
}

}  // namespace

struct ProfilePackageStreamWriter::Impl {
  std::function<bool(const char *, qint64)> sink;
  GFBuffer passphrase;

  bool is_protected_ = false;
  crypto_secretstream_xchacha20poly1305_state state{};

  /// Plaintext waiting to be sealed, in secure storage and allocated once to
  /// its full size. A growing QByteArray was neither: it held the profile's own
  /// key on the ordinary heap and left a copy of it in every block it outgrew.
  GFBuffer pending{static_cast<size_t>(kProfilePackageChunkBytes)};
  qint64 pending_size = 0;

  bool begun = false;
  bool finished = false;

  ~Impl() {
    pending.Zeroize();
    // Holds the key derived from the passphrase. Releasing it without wiping
    // leaves that key legible in freed heap, which undoes the care taken to
    // keep it in a GFBuffer on the way in.
    sodium_memzero(&state, sizeof(state));
  }
};

ProfilePackageStreamWriter::ProfilePackageStreamWriter(
    std::function<bool(const char *, qint64)> sink, GFBuffer passphrase,
    bool is_protected)
    : impl_(std::make_unique<Impl>()) {
  impl_->sink = std::move(sink);
  impl_->passphrase = std::move(passphrase);
  impl_->is_protected_ = is_protected;
}

ProfilePackageStreamWriter::~ProfilePackageStreamWriter() = default;

auto ProfilePackageStreamWriter::Begin() -> bool {
  if (impl_->begun) return false;
  impl_->begun = true;

  // Nothing to derive a key from, so the body is the payload. An unprotected
  // package is plaintext on disk by definition; framing it would cost without
  // protecting anything.
  if (!impl_->is_protected_) return true;

  if (impl_->passphrase.Empty()) {
    LOG_E() << "a protected package cannot be written without a passphrase";
    return false;
  }

  if (!EnsureSodiumInit()) return false;

  std::array<unsigned char, kSaltLen> salt{};
  randombytes_buf(salt.data(), salt.size());

  auto key = DeriveStreamKey(impl_->passphrase, salt.data());
  if (!key) return false;

  std::array<unsigned char, kStreamHeaderLen> header{};
  if (crypto_secretstream_xchacha20poly1305_init_push(
          &impl_->state, header.data(),
          reinterpret_cast<const unsigned char *>(key->Data())) != 0) {
    LOG_E() << "could not start a package stream";
    return false;
  }

  if (!impl_->sink(reinterpret_cast<const char *>(salt.data()),
                   static_cast<qint64>(salt.size()))) {
    return false;
  }
  return impl_->sink(reinterpret_cast<const char *>(header.data()),
                     static_cast<qint64>(header.size()));
}

auto ProfilePackageStreamWriter::FlushChunk(bool final_chunk) -> bool {
  if (impl_->pending_size == 0 && !final_chunk) return true;

  // Wipes the plaintext on the way out of every path below, including the
  // failing ones. A chunk that could not be sealed or could not be written is
  // still a chunk of somebody's profile sitting in memory.
  const auto forget_pending = qScopeGuard([this]() {
    sodium_memzero(impl_->pending.Data(),
                   static_cast<size_t>(impl_->pending_size));
    impl_->pending_size = 0;
  });

  QByteArray cipher(static_cast<qsizetype>(impl_->pending_size) +
                        static_cast<qsizetype>(kChunkOverhead),
                    Qt::Uninitialized);
  unsigned long long cipher_len = 0;

  const unsigned char tag =
      final_chunk ? crypto_secretstream_xchacha20poly1305_TAG_FINAL : 0;

  if (crypto_secretstream_xchacha20poly1305_push(
          &impl_->state, reinterpret_cast<unsigned char *>(cipher.data()),
          &cipher_len,
          reinterpret_cast<const unsigned char *>(impl_->pending.Data()),
          static_cast<unsigned long long>(impl_->pending_size), nullptr, 0,
          tag) != 0) {
    LOG_E() << "could not seal a package chunk";
    return false;
  }

  std::array<char, 4> length{};
  PutBigEndian32(length.data(), static_cast<quint32>(cipher_len));
  if (!impl_->sink(length.data(), static_cast<qint64>(length.size()))) {
    return false;
  }

  return impl_->sink(cipher.constData(), static_cast<qint64>(cipher_len));
}

auto ProfilePackageStreamWriter::Write(const char *data, qint64 length)
    -> bool {
  if (!impl_->begun || impl_->finished) return false;
  if (length <= 0) return true;

  if (!impl_->is_protected_) return impl_->sink(data, length);

  qint64 offset = 0;
  while (offset < length) {
    const auto room = kProfilePackageChunkBytes - impl_->pending_size;
    const auto take = std::min(room, length - offset);
    std::memcpy(impl_->pending.Data() + impl_->pending_size, data + offset,
                static_cast<size_t>(take));
    impl_->pending_size += take;
    offset += take;

    if (impl_->pending_size >= kProfilePackageChunkBytes) {
      if (!FlushChunk(false)) return false;
    }
  }
  return true;
}

auto ProfilePackageStreamWriter::Finish() -> bool {
  if (!impl_->begun || impl_->finished) return false;
  impl_->finished = true;

  if (!impl_->is_protected_) return true;

  // Always emitted, even for an empty payload: the final tag is what tells a
  // reader the file ended rather than stopped.
  return FlushChunk(true);
}

struct ProfilePackageStreamReader::Impl {
  std::function<qint64(char *, qint64)> source;
  GFBuffer passphrase;

  bool is_protected_ = false;
  crypto_secretstream_xchacha20poly1305_state state{};
  bool begun = false;
  bool complete = false;

  /// Same reason as the writer's: the state carries the key derived from the
  /// passphrase, and dropping it unwiped leaves that key in freed heap.
  ~Impl() { sodium_memzero(&state, sizeof(state)); }
};

ProfilePackageStreamReader::ProfilePackageStreamReader(
    std::function<qint64(char *, qint64)> source, GFBuffer passphrase,
    bool is_protected)
    : impl_(std::make_unique<Impl>()) {
  impl_->source = std::move(source);
  impl_->passphrase = std::move(passphrase);
  impl_->is_protected_ = is_protected;
}

ProfilePackageStreamReader::~ProfilePackageStreamReader() = default;

auto ProfilePackageStreamReader::Complete() const -> bool {
  return impl_->complete;
}

auto ProfilePackageStreamReader::Begin() -> bool {
  if (impl_->begun) return false;
  impl_->begun = true;

  if (!impl_->is_protected_) return true;

  // Refused here rather than mis-framed. The alternative was reading the
  // ciphertext as a plain body and handing it to the archive reader, which
  // reports a corrupt package to a user who only left the passphrase blank.
  if (impl_->passphrase.Empty()) {
    LOG_W() << "a protected package was opened without a passphrase";
    return false;
  }

  if (!EnsureSodiumInit()) return false;

  std::array<unsigned char, kSaltLen> salt{};
  if (impl_->source(reinterpret_cast<char *>(salt.data()),
                    static_cast<qint64>(salt.size())) !=
      static_cast<qint64>(salt.size())) {
    return false;
  }

  std::array<unsigned char, kStreamHeaderLen> header{};
  if (impl_->source(reinterpret_cast<char *>(header.data()),
                    static_cast<qint64>(header.size())) !=
      static_cast<qint64>(header.size())) {
    return false;
  }

  auto key = DeriveStreamKey(impl_->passphrase, salt.data());
  if (!key) return false;

  // Succeeds for any well-formed header, right passphrase or not: a wrong key
  // is caught by the first chunk failing to authenticate, which is what makes
  // that failure indistinguishable from tampering — and it should be.
  return crypto_secretstream_xchacha20poly1305_init_pull(
             &impl_->state, header.data(),
             reinterpret_cast<const unsigned char *>(key->Data())) == 0;
}

auto ProfilePackageStreamReader::Next(GFBuffer &out) -> bool {
  // The caller's previous chunk is wiped rather than merely dropped: it is
  // plaintext out of somebody's profile and nothing else is going to erase it.
  out.Zeroize();
  out = GFBuffer();

  if (!impl_->begun) return false;

  if (!impl_->is_protected_) {
    GFBuffer buffer(static_cast<size_t>(kProfilePackageChunkBytes));
    const auto read = impl_->source(buffer.Data(), kProfilePackageChunkBytes);

    // A source error is not a clean end. QFile::read() returns -1 on failure,
    // and calling that the end of the body reported a package that could not be
    // read off the disk as one that had been read in full.
    if (read < 0) {
      LOG_W() << "a package body could not be read";
      buffer.Zeroize();
      return false;
    }
    if (read == 0) {
      buffer.Zeroize();
      impl_->complete = true;
      return true;
    }

    // Wiped before the shrink rather than left to the allocator: the tail is
    // whatever the last read left there.
    sodium_memzero(buffer.Data() + read,
                   static_cast<size_t>(kProfilePackageChunkBytes - read));
    buffer.Resize(static_cast<ssize_t>(read));
    out = std::move(buffer);
    return true;
  }

  if (impl_->complete) return true;

  std::array<char, kProfilePackageChunkPrefixBytes> length_bytes{};
  const auto header_read = impl_->source(
      length_bytes.data(), static_cast<qint64>(length_bytes.size()));
  if (header_read == 0) {
    // Ran out with no final tag: the file was cut short. Reported as a failure
    // rather than as a clean end, since a length-prefixed stream cannot
    // otherwise tell a truncated package from a complete one.
    LOG_W() << "a package stream ended without its final chunk";
    return false;
  }
  if (header_read != static_cast<qint64>(length_bytes.size())) return false;

  const auto cipher_len =
      static_cast<qint64>(ReadBigEndian32(length_bytes.data()));
  if (cipher_len < static_cast<qint64>(kChunkOverhead) ||
      cipher_len > kMaxChunkBytes) {
    LOG_W() << "a package chunk claims an implausible size:" << cipher_len;
    return false;
  }

  QByteArray cipher(static_cast<qsizetype>(cipher_len), Qt::Uninitialized);
  if (impl_->source(cipher.data(), cipher_len) != cipher_len) return false;

  GFBuffer plain(static_cast<size_t>(cipher_len - kChunkOverhead));
  unsigned long long plain_len = 0;
  unsigned char tag = 0;

  if (crypto_secretstream_xchacha20poly1305_pull(
          &impl_->state, reinterpret_cast<unsigned char *>(plain.Data()),
          &plain_len, &tag,
          reinterpret_cast<const unsigned char *>(cipher.constData()),
          static_cast<unsigned long long>(cipher_len), nullptr, 0) != 0) {
    // The wrong passphrase and a damaged chunk are the same event here, and
    // the caller decides which to report from what else it knows. Whatever the
    // pull left behind is wiped either way.
    plain.Zeroize();
    return false;
  }

  if (plain.Size() > plain_len) {
    sodium_memzero(plain.Data() + plain_len, plain.Size() - plain_len);
  }
  plain.Resize(static_cast<ssize_t>(plain_len));
  out = std::move(plain);

  if (tag == crypto_secretstream_xchacha20poly1305_TAG_FINAL) {
    impl_->complete = true;
  }
  return true;
}

}  // namespace GpgFrontend

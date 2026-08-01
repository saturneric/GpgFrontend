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

#include "GpgFrontend.h"

#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)

#include <QLibrary>
#include <array>
#include <type_traits>

#include "core/function/GFBufferFactory.h"
#include "core/function/SystemSecretStore.h"
#include "platform/PlatformSecretStore.h"

namespace GpgFrontend {

namespace {

// libsecret is resolved at runtime so that neither the build nor the shipped
// bundle depends on it. Everything below therefore has to restate the parts of
// libsecret's ABI that we touch.

enum GFSecretSchemaAttributeType { kGF_SECRET_SCHEMA_ATTRIBUTE_STRING = 0 };

struct GFSecretSchemaAttribute {
  const char* name;
  int type;
};

/**
 * @brief Binary-compatible copy of libsecret's struct _SecretSchema.
 *
 * libsecret reads this by offset, so the attribute array must keep its fixed
 * length of 32 and the trailing reserved fields must stay: shortening either
 * would silently corrupt memory rather than fail to link.
 */
struct GFSecretSchema {
  const char* name;
  int flags;
  std::array<GFSecretSchemaAttribute, 32> attributes;
  int reserved;
  void* reserved1;
  void* reserved2;
  void* reserved3;
  void* reserved4;
  void* reserved5;
  void* reserved6;
  void* reserved7;
};

// std::array stands in for libsecret's plain C array only because it holds
// nothing else; assert that rather than assume it, since a mismatch here would
// hand libsecret a misaligned struct instead of failing to build.
static_assert(sizeof(std::array<GFSecretSchemaAttribute, 32>) ==
                  sizeof(GFSecretSchemaAttribute) * 32,
              "std::array must not add padding over a plain array");
static_assert(std::is_standard_layout_v<GFSecretSchema>,
              "the schema must stay layout-compatible with libsecret's");

// The trailing attributes are zeroed, which terminates the list.
GFSecretSchema g_schema = {"com.bktus.gpgfrontend.Secret",
                           0,
                           {{
                               {"service", kGF_SECRET_SCHEMA_ATTRIBUTE_STRING},
                               {"account", kGF_SECRET_SCHEMA_ATTRIBUTE_STRING},
                               {nullptr, 0},
                           }},
                           0,
                           nullptr,
                           nullptr,
                           nullptr,
                           nullptr,
                           nullptr,
                           nullptr,
                           nullptr};

/**
 * @brief Binary-compatible copy of glib's GError.
 *
 * Restated here for the same reason the schema above is: only the message is
 * wanted, and taking it must not turn glib into a build dependency. The layout
 * has been fixed since GLib 2.0 and is identical on 32- and 64-bit, since the
 * two leading 32-bit fields always pack ahead of a pointer.
 */
struct GFGError {
  quint32 domain;
  int code;
  char* message;
};

static_assert(offsetof(GFGError, message) == 8,
              "GError layout must match glib's");

// These must be declared variadic rather than with the trailing attribute
// arguments spelled out. On the x86-64 SysV ABI a variadic callee reads AL for
// the number of vector registers in use; calling one through a fixed-arity
// pointer leaves AL unset, which is undefined behaviour.
using StoreSyncFn = int (*)(const GFSecretSchema*, const char* collection,
                            const char* label, const char* password,
                            void* cancellable, GFGError** error, ...);
using LookupSyncFn = char* (*)(const GFSecretSchema*, void* cancellable,
                               GFGError** error, ...);
using ClearSyncFn = int (*)(const GFSecretSchema*, void* cancellable,
                            GFGError** error, ...);
using PasswordFreeFn = void (*)(char*);
using ErrorFreeFn = void (*)(GFGError*);

/// SONAME reported to the user. QLibrary builds the same name from
/// ("libsecret-1", 0).
constexpr auto kLibSecretSoname = "libsecret-1.so.0";

struct LibSecret {
  StoreSyncFn store = nullptr;
  LookupSyncFn lookup = nullptr;
  ClearSyncFn clear = nullptr;
  PasswordFreeFn free_password = nullptr;
  PasswordFreeFn wipe_password = nullptr;
  ErrorFreeFn free_error = nullptr;

  /// Why the library is unusable. Empty once it loaded.
  QString error;

  [[nodiscard]] auto Loaded() const -> bool {
    return store != nullptr && lookup != nullptr && clear != nullptr &&
           free_password != nullptr;
  }

  /**
   * @brief Consume a GError, returning its message.
   *
   * Every libsecret call here reports failure as a bare false or NULL, which
   * cannot distinguish a locked keyring from an absent daemon from a rejected
   * write. The message is the only thing that can, so it is always collected
   * rather than passed as NULL and discarded.
   *
   * @param error error to consume; may be null, in which case nothing is done
   * @return the message, or empty when there was no error or no message
   */
  [[nodiscard]] auto TakeError(GFGError* error) const -> QString {
    if (error == nullptr) return {};

    QString message = QString::fromUtf8(error->message);

    // g_error_free lives in glib, which libsecret pulls in: dlsym searches a
    // handle's dependencies, so it resolves without loading glib separately.
    // Should that ever fail, leaking one struct beats freeing it wrongly.
    if (free_error != nullptr) free_error(error);

    return message;
  }

  /// Release a string libsecret allocated, wiping it when possible.
  void Release(char* password) const {
    if (password == nullptr) return;
    if (wipe_password != nullptr) {
      wipe_password(password);
      return;
    }
    free_password(password);
  }
};

auto ResolveLibSecret() -> const LibSecret& {
  static const LibSecret kLib = []() -> LibSecret {
    LibSecret out;

    // Version 0 gives libsecret-1.so.0. The unversioned name only exists in
    // the -dev package and must not be relied on.
    QLibrary library(QStringLiteral("libsecret-1"), 0);
    if (!library.load()) {
      // errorString() is the whole point of this branch. A missing package and
      // a package that is present but cannot resolve its own dependencies --
      // the usual way a bundled build shadows the host's glib -- are the same
      // greyed-out menu item to the user, and only the loader can tell them
      // apart.
      out.error = QStringLiteral("%1 could not be loaded: %2")
                      .arg(QLatin1String(kLibSecretSoname),
                           library.errorString().trimmed());
      qWarning().noquote() << out.error;
      return out;
    }

    out.store = reinterpret_cast<StoreSyncFn>(
        library.resolve("secret_password_store_sync"));
    out.lookup = reinterpret_cast<LookupSyncFn>(
        library.resolve("secret_password_lookup_sync"));
    out.clear = reinterpret_cast<ClearSyncFn>(
        library.resolve("secret_password_clear_sync"));
    out.free_password = reinterpret_cast<PasswordFreeFn>(
        library.resolve("secret_password_free"));
    out.wipe_password = reinterpret_cast<PasswordFreeFn>(
        library.resolve("secret_password_wipe"));
    out.free_error =
        reinterpret_cast<ErrorFreeFn>(library.resolve("g_error_free"));

    if (!out.Loaded()) {
      LibSecret failed;
      failed.error =
          QStringLiteral(
              "%1 loaded from %2 but its symbols could not be "
              "resolved")
              .arg(QLatin1String(kLibSecretSoname), library.fileName());
      qWarning().noquote() << failed.error;
      return failed;
    }

    // Which copy answered matters when several are reachable, and it is the
    // first thing to ask for when the store misbehaves rather than vanishes.
    qDebug().noquote() << "libsecret loaded from" << library.fileName();

    return out;
  }();

  return kLib;
}

/**
 * @brief Secret Service backend, reached through libsecret.
 */
class LinuxSecretStore final : public SystemSecretStore {
 public:
  [[nodiscard]] auto Name() const -> QString override {
    return QStringLiteral("libsecret");
  }

  [[nodiscard]] auto IsAvailable() -> bool override {
    // A missing daemon and a missing entry both come back as NULL, so the only
    // honest check is a full round trip. Cached: it can prompt to unlock.
    static const bool kAvailable =
        ResolveLibSecret().Loaded() && ProbeSystemSecretStore(*this);
    return kAvailable;
  }

  auto Read(const QString& account) -> GFBufferOrNone override {
    const auto& lib = ResolveLibSecret();
    if (!lib.Loaded()) return {};

    const auto account_utf8 = account.toUtf8();
    GFGError* error = nullptr;
    char* raw =
        lib.lookup(&g_schema, nullptr, &error, "service", SystemSecretService(),
                   "account", account_utf8.constData(), nullptr);
    last_error_ = lib.TakeError(error);

    // A missing entry is not a failure and leaves the error unset, so only a
    // populated message means something actually went wrong.
    if (raw == nullptr) return {};

    auto decoded = GFBufferFactory::FromBase64(GFBuffer(QByteArray(raw)));
    lib.Release(raw);

    if (!decoded) {
      qWarning() << "stored secret is not valid base64, account:" << account;
      return {};
    }

    return decoded;
  }

  auto Write(const QString& account, const GFBuffer& secret) -> bool override {
    const auto& lib = ResolveLibSecret();
    if (!lib.Loaded()) return false;

    // The API takes a NUL-terminated string, so binary secrets travel base64.
    auto encoded = GFBufferFactory::ToBase64(secret);
    if (!encoded) return false;

    auto encoded_bytes = encoded->ConvertToQByteArray();
    const auto account_utf8 = account.toUtf8();
    const auto label = QStringLiteral("GpgFrontend: %1").arg(account).toUtf8();

    GFGError* error = nullptr;
    const int ok = lib.store(&g_schema, nullptr, label.constData(),
                             encoded_bytes.constData(), nullptr, &error,
                             "service", SystemSecretService(), "account",
                             account_utf8.constData(), nullptr);
    last_error_ = lib.TakeError(error);

    encoded_bytes.fill('\0');
    return ok != 0;
  }

  auto Remove(const QString& account) -> bool override {
    const auto& lib = ResolveLibSecret();
    if (!lib.Loaded()) return false;

    const auto account_utf8 = account.toUtf8();
    GFGError* error = nullptr;
    lib.clear(&g_schema, nullptr, &error, "service", SystemSecretService(),
              "account", account_utf8.constData(), nullptr);
    last_error_ = lib.TakeError(error);

    // clear_sync reports false when there was nothing to remove, which is the
    // state the caller wanted either way.
    return true;
  }

  [[nodiscard]] auto LastError() const -> QString override {
    return last_error_;
  }

 private:
  /// Message from the most recent call, empty when it did not fail.
  QString last_error_;
};

}  // namespace

void InstallPlatformSecretStore() {
  const auto& lib = ResolveLibSecret();
  if (!lib.Loaded()) {
    RegisterSystemSecretStoreUnavailable(lib.error);
    return;
  }

  RegisterSystemSecretStore(std::make_unique<LinuxSecretStore>());
}

}  // namespace GpgFrontend

#endif

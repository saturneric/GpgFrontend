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

#include <QDir>
#include <QFileInfo>
#include <QLibrary>
#include <array>
#include <type_traits>

#include "core/function/GFBufferFactory.h"
#include "core/function/SystemSecretStore.h"
#include "platform/LibSecretSearchPath.h"
#include "platform/PlatformSecretStore.h"

namespace GpgFrontend {

namespace {

// libsecret is resolved at runtime so that neither the build nor the shipped
// bundle depends on it. Everything below therefore has to restate the parts of
// libsecret's ABI that we touch.

enum GFSecretSchemaAttributeType { kGF_SECRET_SCHEMA_ATTRIBUTE_STRING = 0 };

/// SECRET_SERVICE_NONE: connect, but load neither collections nor a session.
constexpr int kSecretServiceNone = 0;

/// The alias every Secret Service maps to the collection secrets land in.
constexpr auto kDefaultCollectionAlias = "default";

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

// The unlock trio, by contrast, must be declared with their arguments spelled
// out: none of them takes attributes, so none is variadic, and declaring them
// so would introduce the very ABI mismatch the note above guards against.
//
// SecretService*, SecretCollection* and GCancellable* stay void*: they are only
// ever handed straight back to libsecret, so naming their types would mean
// restating three more GObject headers for nothing. SecretServiceFlags is a
// plain C enum and travels as int.
using ServiceGetSyncFn = void* (*)(int flags, void* cancellable,
                                   GFGError** error);
using ReadAliasPathSyncFn = char* (*)(void* service, const char* alias,
                                      void* cancellable, GFGError** error);
using UnlockPathsSyncFn = int (*)(void* service, const char** paths,
                                  void* cancellable, char*** unlocked,
                                  GFGError** error);
using MemFreeFn = void (*)(void*);
using StrvFreeFn = void (*)(char**);
using ObjectUnrefFn = void (*)(void*);

struct LibSecret {
  StoreSyncFn store = nullptr;
  LookupSyncFn lookup = nullptr;
  ClearSyncFn clear = nullptr;
  PasswordFreeFn free_password = nullptr;
  PasswordFreeFn wipe_password = nullptr;
  ErrorFreeFn free_error = nullptr;
  ServiceGetSyncFn service_get = nullptr;
  ReadAliasPathSyncFn read_alias_path = nullptr;
  UnlockPathsSyncFn unlock_paths = nullptr;
  MemFreeFn free_mem = nullptr;
  StrvFreeFn free_strv = nullptr;
  ObjectUnrefFn unref_object = nullptr;

  /// Why the library is unusable. Empty once it loaded.
  QString error;

  [[nodiscard]] auto Loaded() const -> bool {
    return store != nullptr && lookup != nullptr && clear != nullptr &&
           free_password != nullptr;
  }

  /**
   * @brief Whether the explicit-unlock path can be taken.
   *
   * Deliberately not folded into Loaded(): a libsecret too old to export these
   * still stores and reads perfectly well, and treating it as unloaded would
   * turn one missing capability into a credential store that greys out
   * entirely.
   *
   * @return true when every symbol Unlock() needs resolved
   */
  [[nodiscard]] auto CanUnlock() const -> bool {
    return service_get != nullptr && read_alias_path != nullptr &&
           unlock_paths != nullptr && free_mem != nullptr &&
           unref_object != nullptr;
  }

  /**
   * @brief Consume a GError, returning its message.
   *
   * Every libsecret call here reports failure as a bare false or NULL, which
   * cannot distinguish a locked keyring from an absent daemon from a rejected
   * write. The message is the only thing that can, so it is always collected
   * rather than passed as NULL and discarded.
   *
   * Cleared as well as freed, because glib refuses to report into a location
   * that is not null and a caller making more than one call in a row would
   * otherwise hand the next one a dangling pointer.
   *
   * @param error error to consume, reset to null; may already be null, in
   *              which case nothing is done
   * @return the message, or empty when there was no error or no message
   */
  [[nodiscard]] auto TakeError(GFGError*& error) const -> QString {
    if (error == nullptr) return {};

    QString message = QString::fromUtf8(error->message);

    // g_error_free lives in glib, which libsecret pulls in: dlsym searches a
    // handle's dependencies, so it resolves without loading glib separately.
    // Should that ever fail, leaking one struct beats freeing it wrongly.
    if (free_error != nullptr) free_error(error);
    error = nullptr;

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
    // Kept per candidate rather than only for the last one: "no such file" for
    // a bundled copy and "undefined symbol" for the host's are different bugs,
    // and one message can only ever describe one of them.
    QStringList failures;

    const auto candidates =
        LibSecretSearchPaths(qEnvironmentVariable("APPDIR"),
                             qEnvironmentVariable(kLibSecretPathEnv));

    for (const auto& candidate : candidates) {
      // Asked before QLibrary, because QLibrary answers a name it was never
      // given: having failed to open the path verbatim it retries with "lib"
      // and ".so" pasted on, and reports the error from that last attempt. A
      // missing bundled copy would be reported as
      // ".../liblibsecret-1.so.0.so: cannot open shared object file", which
      // sends whoever reads it looking for a file nobody ever asked for.
      if (QDir::isAbsolutePath(candidate) && !QFileInfo::exists(candidate)) {
        failures << QStringLiteral("%1 (no such file)").arg(candidate);
        continue;
      }

      QLibrary library(candidate);

      // RTLD_NOW, not QLibrary's lazy default. The failure this list exists
      // for -- a libsecret older or newer than the glib in front of it on the
      // search path -- is a missing function symbol, and lazy binding defers
      // that to the first call, where it takes the process down from inside a
      // settings page instead of greying out a menu item here.
      library.setLoadHints(QLibrary::ResolveAllSymbolsHint);

      if (!library.load()) {
        // errorString() is the whole point of this branch. A missing package
        // and a package that is present but cannot resolve its own
        // dependencies -- the usual way a bundled build shadows the host's
        // glib -- are the same greyed-out menu item to the user, and only the
        // loader can tell them apart.
        failures << QStringLiteral("%1 (%2)").arg(
            candidate, library.errorString().trimmed());
        continue;
      }

      // Declared here, not outside the loop: a candidate that resolves only
      // half its symbols must not leave those behind for the next one.
      LibSecret out;
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

      out.service_get = reinterpret_cast<ServiceGetSyncFn>(
          library.resolve("secret_service_get_sync"));
      out.read_alias_path = reinterpret_cast<ReadAliasPathSyncFn>(
          library.resolve("secret_service_read_alias_dbus_path_sync"));
      out.unlock_paths = reinterpret_cast<UnlockPathsSyncFn>(
          library.resolve("secret_service_unlock_dbus_paths_sync"));
      out.free_mem = reinterpret_cast<MemFreeFn>(library.resolve("g_free"));
      out.free_strv =
          reinterpret_cast<StrvFreeFn>(library.resolve("g_strfreev"));
      out.unref_object =
          reinterpret_cast<ObjectUnrefFn>(library.resolve("g_object_unref"));

      if (!out.Loaded()) {
        failures << QStringLiteral(
                        "%1 loaded but its symbols could not be "
                        "resolved")
                        .arg(library.fileName());
        continue;
      }

      // Which copy answered matters when several are reachable, and it is the
      // first thing to ask for when the store misbehaves rather than vanishes.
      qDebug().noquote() << "libsecret loaded from" << library.fileName();

      // Not a load failure, so this candidate still wins -- but it is worth
      // saying out loud, because the symptom it produces is the one this whole
      // path exists to remove: a locked keyring that never prompts.
      if (!out.CanUnlock()) {
        qWarning().noquote()
            << library.fileName()
            << "cannot unlock the keyring on demand: it does not export the "
               "secret service entry points";
      }

      return out;
    }

    LibSecret failed;
    failed.error = QStringLiteral("%1 could not be loaded; tried: %2")
                       .arg(QLatin1String(kLibSecretSoname),
                            failures.join(QStringLiteral("; ")));
    qWarning().noquote() << failed.error;
    return failed;
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

  /**
   * @brief Unlock the default collection, prompting the user if it is locked.
   *
   * secret_password_lookup_sync() looks like it already does this, and in the
   * common case it does: on finding the wanted item among the ones the daemon
   * reports as locked, it unlocks that item, which raises the prompt. The case
   * it does not cover is the one that matters here -- a collection that has not
   * been opened since boot, whose items the daemon does not report at all.
   * Search comes back with nothing in either list, so nothing is unlocked,
   * nothing is prompted, and the lookup returns NULL without even setting an
   * error. Unlocking the collection by name asks the same question with no
   * search in the way, so the answer no longer depends on what the daemon is
   * willing to enumerate while locked.
   *
   * @return true when the default collection is now unlocked
   */
  [[nodiscard]] auto Unlock() -> bool override {
    const auto& lib = ResolveLibSecret();
    if (!lib.Loaded() || !lib.CanUnlock()) return false;

    GFGError* error = nullptr;

    // The reference is ours to drop: libsecret only weak-refs the default
    // service, so releasing it below costs at most a reconnect next time.
    void* service = lib.service_get(kSecretServiceNone, nullptr, &error);
    last_error_ = lib.TakeError(error);
    if (service == nullptr) return false;

    bool unlocked = false;
    char* path =
        lib.read_alias_path(service, kDefaultCollectionAlias, nullptr, &error);
    last_error_ = lib.TakeError(error);

    if (path == nullptr || *path == '\0') {
      // Untranslated and deliberately ours: libsecret reports this as a bare
      // NULL, and an empty detail box is what sent the user here.
      if (last_error_.isEmpty()) {
        last_error_ =
            QStringLiteral("the secret service reports no default collection");
      }
    } else {
      // Zero-terminated, as the paths argument is read until NULL.
      std::array<const char*, 2> paths = {path, nullptr};

      char** unlocked_paths = nullptr;
      const int count = lib.unlock_paths(service, paths.data(), nullptr,
                                         &unlocked_paths, &error);
      last_error_ = lib.TakeError(error);

      // A no-op when the collection is already open: the service answers with
      // the path and no prompt object, so nobody is asked anything. That is
      // what makes this safe to call before knowing whether it was locked.
      unlocked = count > 0;

      if (!unlocked && last_error_.isEmpty()) {
        last_error_ = QStringLiteral(
            "the default keyring is still locked; the unlock prompt was "
            "dismissed or no prompter is available");
      }

      if (unlocked_paths != nullptr && lib.free_strv != nullptr) {
        lib.free_strv(unlocked_paths);
      }
    }

    if (path != nullptr) lib.free_mem(path);
    lib.unref_object(service);

    if (unlocked) last_error_.clear();
    return unlocked;
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

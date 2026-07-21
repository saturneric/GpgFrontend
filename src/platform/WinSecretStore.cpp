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

#ifdef Q_OS_WINDOWS

#include <windows.h>
//
#include <wincred.h>

#include "core/function/SystemSecretStore.h"
#include "platform/PlatformSecretStore.h"

namespace GpgFrontend {

namespace {

/// Credential Manager keys entries by a single target name.
auto TargetName(const QString& account) -> QString {
  return QStringLiteral("%1/%2").arg(QString::fromUtf8(kSystemSecretService),
                                     account);
}

auto AsWide(const QString& value) -> const wchar_t* {
  return reinterpret_cast<const wchar_t*>(value.utf16());
}

/**
 * @brief Credential Manager backend. Entries are protected by DPAPI.
 */
class WinSecretStore final : public SystemSecretStore {
 public:
  [[nodiscard]] auto Name() const -> QString override {
    return QStringLiteral("credential manager");
  }

  [[nodiscard]] auto IsAvailable() -> bool override {
    // Always present on Windows, but probe anyway so all backends behave the
    // same and a locked-down policy still reads as unavailable.
    static const bool available = ProbeSystemSecretStore(*this);
    return available;
  }

  auto Read(const QString& account) -> GFBufferOrNone override {
    const auto target = TargetName(account);

    PCREDENTIALW credential = nullptr;
    if (CredReadW(AsWide(target), CRED_TYPE_GENERIC, 0, &credential) == FALSE) {
      return {};
    }

    GFBuffer secret(
        QByteArray(reinterpret_cast<const char*>(credential->CredentialBlob),
                   static_cast<int>(credential->CredentialBlobSize)));

    SecureZeroMemory(credential->CredentialBlob,
                     credential->CredentialBlobSize);
    CredFree(credential);

    return secret;
  }

  auto Write(const QString& account, const GFBuffer& secret) -> bool override {
    if (secret.Size() > CRED_MAX_CREDENTIAL_BLOB_SIZE) {
      qWarning() << "secret too large for credential manager:" << secret.Size();
      return false;
    }

    const auto target = TargetName(account);

    CREDENTIALW credential = {};
    credential.Type = CRED_TYPE_GENERIC;
    credential.TargetName = const_cast<LPWSTR>(AsWide(target));
    credential.CredentialBlobSize = static_cast<DWORD>(secret.Size());
    credential.CredentialBlob =
        reinterpret_cast<LPBYTE>(const_cast<char*>(secret.Data()));
    credential.Persist = CRED_PERSIST_LOCAL_MACHINE;

    if (CredWriteW(&credential, 0) == FALSE) {
      qWarning() << "credential manager write failed, error:" << GetLastError();
      return false;
    }

    return true;
  }

  auto Remove(const QString& account) -> bool override {
    const auto target = TargetName(account);

    if (CredDeleteW(AsWide(target), CRED_TYPE_GENERIC, 0) == FALSE) {
      // Nothing to delete is the state the caller wanted.
      return GetLastError() == ERROR_NOT_FOUND;
    }

    return true;
  }
};

}  // namespace

void InstallPlatformSecretStore() {
  RegisterSystemSecretStore(std::make_unique<WinSecretStore>());
}

}  // namespace GpgFrontend

#endif

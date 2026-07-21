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

#ifdef Q_OS_MACOS

#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>

#include "core/function/SystemSecretStore.h"
#include "platform/PlatformSecretStore.h"

namespace GpgFrontend {

namespace {

/// Owns a CoreFoundation reference and releases it on scope exit.
template <typename T>
class ScopedCFRef {
 public:
  explicit ScopedCFRef(T ref) : ref_(ref) {}
  ~ScopedCFRef() {
    if (ref_ != nullptr) CFRelease(ref_);
  }

  ScopedCFRef(const ScopedCFRef&) = delete;
  auto operator=(const ScopedCFRef&) -> ScopedCFRef& = delete;
  ScopedCFRef(ScopedCFRef&&) = delete;
  auto operator=(ScopedCFRef&&) -> ScopedCFRef& = delete;

  [[nodiscard]] auto Get() const -> T { return ref_; }
  explicit operator bool() const { return ref_ != nullptr; }

 private:
  T ref_ = nullptr;
};

auto ToCFString(const QString& value) -> CFStringRef {
  const auto utf8 = value.toUtf8();
  return CFStringCreateWithBytes(
      kCFAllocatorDefault, reinterpret_cast<const UInt8*>(utf8.constData()),
      static_cast<CFIndex>(utf8.size()), kCFStringEncodingUTF8, false);
}

/// Build the query identifying one generic-password item.
auto MakeQuery(const QString& account) -> CFMutableDictionaryRef {
  auto* query = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
                                          &kCFTypeDictionaryKeyCallBacks,
                                          &kCFTypeDictionaryValueCallBacks);
  if (query == nullptr) return nullptr;

  ScopedCFRef<CFStringRef> service(
      ToCFString(QString::fromUtf8(kSystemSecretService)));
  ScopedCFRef<CFStringRef> account_ref(ToCFString(account));

  CFDictionarySetValue(query, kSecClass, kSecClassGenericPassword);
  CFDictionarySetValue(query, kSecAttrService, service.Get());
  CFDictionarySetValue(query, kSecAttrAccount, account_ref.Get());

  return query;
}

/**
 * @brief Keychain backend built on the Security framework.
 */
class MacSecretStore final : public SystemSecretStore {
 public:
  [[nodiscard]] auto Name() const -> QString override {
    return QStringLiteral("keychain");
  }

  [[nodiscard]] auto IsAvailable() -> bool override {
    // Sandboxed builds need a keychain-access-groups entitlement; without it
    // every call fails, and the round trip is what surfaces that.
    static const bool available = ProbeSystemSecretStore(*this);
    return available;
  }

  auto Read(const QString& account) -> GFBufferOrNone override {
    ScopedCFRef<CFMutableDictionaryRef> query(MakeQuery(account));
    if (!query) return {};

    CFDictionarySetValue(query.Get(), kSecReturnData, kCFBooleanTrue);
    CFDictionarySetValue(query.Get(), kSecMatchLimit, kSecMatchLimitOne);

    CFTypeRef result = nullptr;
    const auto status = SecItemCopyMatching(query.Get(), &result);
    if (status != errSecSuccess || result == nullptr) return {};

    ScopedCFRef<CFDataRef> data(static_cast<CFDataRef>(result));
    return GFBuffer(
        QByteArray(reinterpret_cast<const char*>(CFDataGetBytePtr(data.Get())),
                   static_cast<int>(CFDataGetLength(data.Get()))));
  }

  auto Write(const QString& account, const GFBuffer& secret) -> bool override {
    ScopedCFRef<CFMutableDictionaryRef> query(MakeQuery(account));
    if (!query) return false;

    ScopedCFRef<CFDataRef> data(CFDataCreate(
        kCFAllocatorDefault, reinterpret_cast<const UInt8*>(secret.Data()),
        static_cast<CFIndex>(secret.Size())));
    if (!data) return false;

    // Stays on this device and survives a reboot before the user logs in.
    CFDictionarySetValue(query.Get(), kSecAttrAccessible,
                         kSecAttrAccessibleAfterFirstUnlockThisDeviceOnly);
    CFDictionarySetValue(query.Get(), kSecValueData, data.Get());

    auto status = SecItemAdd(query.Get(), nullptr);

    if (status == errSecDuplicateItem) {
      // SecItemAdd refuses to replace, so update the existing item's value.
      ScopedCFRef<CFMutableDictionaryRef> match(MakeQuery(account));
      if (!match) return false;

      ScopedCFRef<CFMutableDictionaryRef> attrs(CFDictionaryCreateMutable(
          kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
          &kCFTypeDictionaryValueCallBacks));
      if (!attrs) return false;

      CFDictionarySetValue(attrs.Get(), kSecValueData, data.Get());
      status = SecItemUpdate(match.Get(), attrs.Get());
    }

    if (status != errSecSuccess) {
      qWarning() << "keychain write failed, status:" << status;
      return false;
    }

    return true;
  }

  auto Remove(const QString& account) -> bool override {
    ScopedCFRef<CFMutableDictionaryRef> query(MakeQuery(account));
    if (!query) return false;

    const auto status = SecItemDelete(query.Get());
    return status == errSecSuccess || status == errSecItemNotFound;
  }
};

}  // namespace

void InstallPlatformSecretStore() {
  RegisterSystemSecretStore(std::make_unique<MacSecretStore>());
}

}  // namespace GpgFrontend

#endif

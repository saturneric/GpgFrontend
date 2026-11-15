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

#ifdef Q_OS_ANDROID

#include <QJniEnvironment>
#include <QJniObject>
#include <mutex>
#include <optional>

#include "core/function/SystemSecretStore.h"
#include "core/utils/MemoryUtils.h"
#include "plat/common/PlatformSecretStore.h"

namespace GpgFrontend {

namespace {

constexpr auto kBridge = "com/bktus/gpgfrontend/SecretStoreBridge";

auto Context() -> QJniObject {
  return QJniObject(QNativeInterface::QAndroidApplication::context());
}

/// One answer from SecretStoreBridge: a status byte, then the secret for a
/// read that worked, otherwise the reason in UTF-8.
struct BridgeAnswer {
  SecretReadStatus status = SecretReadStatus::kUNAVAILABLE;
  GFBuffer payload;
};

/// Status bytes in SecretStoreBridge's order.
auto StatusFromByte(char byte) -> SecretReadStatus {
  switch (byte) {
    case 0:
      return SecretReadStatus::kOK;
    case 1:
      return SecretReadStatus::kABSENT;
    case 2:
      return SecretReadStatus::kCORRUPT;
    case 3:
      return SecretReadStatus::kLOCKED;
    default:
      // Anything unknown must not read as gone: that would offer a reset.
      return SecretReadStatus::kUNAVAILABLE;
  }
}

/// Copy a Java byte array out and zero it where it lies, so the secret is
/// not left behind in the Java heap.
auto TakeAnswer(const QJniObject& result) -> BridgeAnswer {
  QJniEnvironment env;
  if (env.checkAndClearExceptions() || !result.isValid()) {
    return {SecretReadStatus::kUNAVAILABLE,
            GFBuffer(u"the keystore bridge could not be called"_s)};
  }

  auto array = result.object<jbyteArray>();
  const auto length = env->GetArrayLength(array);
  if (length < 1) {
    return {SecretReadStatus::kUNAVAILABLE,
            GFBuffer(u"the keystore bridge gave no answer"_s)};
  }

  auto* bytes = env->GetPrimitiveArrayCritical(array, nullptr);
  if (bytes == nullptr) {
    return {SecretReadStatus::kUNAVAILABLE,
            GFBuffer(u"the keystore bridge answer could not be read"_s)};
  }

  const auto* data = static_cast<const char*>(bytes);
  BridgeAnswer answer{StatusFromByte(data[0]),
                      GFBuffer(data + 1, static_cast<size_t>(length - 1))};
  SecureWipeMemory(bytes, static_cast<size_t>(length));
  env->ReleasePrimitiveArrayCritical(array, bytes, 0);
  return answer;
}

/**
 * @brief The Android Keystore, through SecretStoreBridge.
 *
 * Each entry is a non-exportable AES-GCM key in the Keystore and the secret
 * sealed with it under the no-backup directory. What matters most here is that
 * a read says why it failed: a locked device or a Keystore that is not
 * answering must come back as temporary, because only an entry that is really
 * gone may lead the loader to offer a reset.
 */
class AndroidSecretStore final : public SystemSecretStore {
 public:
  [[nodiscard]] auto Name() const -> QString override {
    return u"Android Keystore"_s;
  }

  [[nodiscard]] auto IsAvailable() -> bool override {
    const std::lock_guard<std::mutex> lock(mutex_);
    if (available_) return *available_;

    const auto ok = ProbeSystemSecretStore(*this);

    // Only a definite answer is kept for the process. A probe that failed
    // because the device was locked, or the Keystore was not answering, says
    // nothing about the next attempt, and caching it would leave the store
    // "unavailable" until the application restarts.
    if (ok || !TemporaryStatus(last_status())) available_ = ok;
    return ok;
  }

  [[nodiscard]] auto LastError() const -> QString override {
    const std::lock_guard<std::mutex> lock(error_mutex_);
    return last_error_;
  }

  auto Read(const QString& account) -> GFBufferOrNone override {
    return ReadDetailed(account).secret;
  }

  auto ReadDetailed(const QString& account) -> SecretReadResult override {
    auto answer = TakeAnswer(QJniObject::callStaticMethod<jbyteArray>(
        kBridge, "read",
        "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)[B",
        Context().object(), Service().object<jstring>(),
        QJniObject::fromString(account).object<jstring>()));

    if (answer.status == SecretReadStatus::kOK) {
      record(answer.status, {});
      return {SecretReadStatus::kOK, std::move(answer.payload), {}};
    }

    const auto detail = answer.payload.ConvertToQByteArray();
    record(answer.status, QString::fromUtf8(detail));
    return {answer.status, std::nullopt, LastError()};
  }

  auto Write(const QString& account, const GFBuffer& secret) -> bool override {
    QJniEnvironment env;
    auto* array = env->NewByteArray(static_cast<jsize>(secret.Size()));
    if (array == nullptr) {
      env.checkAndClearExceptions();
      record(SecretReadStatus::kUNAVAILABLE, u"out of memory"_s);
      return false;
    }
    env->SetByteArrayRegion(array, 0, static_cast<jsize>(secret.Size()),
                            reinterpret_cast<const jbyte*>(secret.Data()));

    auto answer = TakeAnswer(QJniObject::callStaticMethod<jbyteArray>(
        kBridge, "write",
        "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;[B)[B",
        Context().object(), Service().object<jstring>(),
        QJniObject::fromString(account).object<jstring>(), array));

    // The secret's own copy in the Java heap, zeroed before it is let go.
    if (auto* bytes = env->GetPrimitiveArrayCritical(array, nullptr);
        bytes != nullptr) {
      SecureWipeMemory(bytes, secret.Size());
      env->ReleasePrimitiveArrayCritical(array, bytes, 0);
    }
    env->DeleteLocalRef(array);

    record(answer.status,
           QString::fromUtf8(answer.payload.ConvertToQByteArray()));
    return answer.status == SecretReadStatus::kOK;
  }

  auto Remove(const QString& account) -> bool override {
    auto answer = TakeAnswer(QJniObject::callStaticMethod<jbyteArray>(
        kBridge, "remove",
        "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)[B",
        Context().object(), Service().object<jstring>(),
        QJniObject::fromString(account).object<jstring>()));

    record(answer.status,
           QString::fromUtf8(answer.payload.ConvertToQByteArray()));
    return answer.status == SecretReadStatus::kOK;
  }

 private:
  std::mutex mutex_;  ///< guards available_
  std::optional<bool> available_;

  mutable std::mutex error_mutex_;  ///< guards last_status_ and last_error_
  SecretReadStatus last_status_ = SecretReadStatus::kOK;
  QString last_error_;

  static auto Service() -> QJniObject {
    return QJniObject::fromString(QString::fromUtf8(SystemSecretService()));
  }

  static auto TemporaryStatus(SecretReadStatus status) -> bool {
    return status == SecretReadStatus::kLOCKED ||
           status == SecretReadStatus::kUNAVAILABLE;
  }

  [[nodiscard]] auto last_status() const -> SecretReadStatus {
    const std::lock_guard<std::mutex> lock(error_mutex_);
    return last_status_;
  }

  void record(SecretReadStatus status, QString detail) {
    const std::lock_guard<std::mutex> lock(error_mutex_);
    last_status_ = status;
    last_error_ = std::move(detail);
  }
};

}  // namespace

void InstallPlatformSecretStore() {
  RegisterSystemSecretStore(std::make_unique<AndroidSecretStore>());
}

}  // namespace GpgFrontend

#endif  // Q_OS_ANDROID

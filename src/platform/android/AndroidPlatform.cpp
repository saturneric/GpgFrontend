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

#include <QCoreApplication>

#include "platform/common/PlatformClipboard.h"
#include "platform/common/PlatformPrivacy.h"
#include "platform/common/PlatformShare.h"

// The Android backends of the mobile platform interfaces, through
// com.bktus.gpgfrontend.PlatformBridge (src/platform/android/package).
#if defined(Q_OS_ANDROID)

#include <QJniEnvironment>
#include <array>
#include <QJniObject>

namespace GpgFrontend {

namespace {

constexpr auto kBridge = "com/bktus/gpgfrontend/PlatformBridge";

auto Activity() -> QJniObject {
  return QJniObject(QNativeInterface::QAndroidApplication::context());
}

void JNICALL OnShareTargetChosen(JNIEnv*, jclass, jint request) {
  ShareSession::Instance().TargetChosen(static_cast<int>(request));
}

void RegisterNatives() {
  static bool registered = false;
  if (registered) return;
  QJniEnvironment env;
  std::array<JNINativeMethod, 1> methods{{
      {"onShareTargetChosen", "(I)V",
       reinterpret_cast<void*>(OnShareTargetChosen)},
  }};
  registered = env.registerNativeMethods(kBridge, methods.data(),
                                         static_cast<int>(methods.size()));
}

}  // namespace

auto CanShare() -> bool { return true; }

void ShareText(const QString& text, const QString& title, ShareCallback done) {
  RegisterNatives();
  auto& session = ShareSession::Instance();
  const auto request = session.Begin(std::move(done));

  const auto shown = QJniObject::callStaticMethod<jboolean>(
      kBridge, "shareText",
      "(Landroid/app/Activity;Ljava/lang/String;Ljava/lang/String;I)Z",
      Activity().object(), QJniObject::fromString(text).object<jstring>(),
      QJniObject::fromString(title).object<jstring>(),
      static_cast<jint>(request));
  if (!shown) session.Failed(request, QStringLiteral("no share sheet"));
}

void ShareFile(const QString& path, const QString& mime_type,
               const QString& title, ShareCallback done) {
  RegisterNatives();
  auto& session = ShareSession::Instance();
  const auto request = session.Begin(std::move(done));

  const auto shown = QJniObject::callStaticMethod<jboolean>(
      kBridge, "shareFile",
      "(Landroid/app/Activity;Ljava/lang/String;Ljava/lang/String;"
      "Ljava/lang/String;I)Z",
      Activity().object(), QJniObject::fromString(path).object<jstring>(),
      QJniObject::fromString(mime_type).object<jstring>(),
      QJniObject::fromString(title).object<jstring>(),
      static_cast<jint>(request));
  if (!shown) session.Failed(request, QStringLiteral("no share sheet"));
}

void RevokeSharedFile(const QString& path) {
  QJniObject::callStaticMethod<void>(
      kBridge, "revokeFile", "(Landroid/content/Context;Ljava/lang/String;)V",
      Activity().object(), QJniObject::fromString(path).object<jstring>());
}

auto SetSensitiveClipboardText(const QString& text) -> bool {
  QJniObject::callStaticMethod<void>(
      kBridge, "setSensitiveClip",
      "(Landroid/content/Context;Ljava/lang/String;)V", Activity().object(),
      QJniObject::fromString(text).object<jstring>());
  return true;
}

auto CanSecureWindow() -> bool { return true; }

void SetWindowSecure(QWindow*, bool secure) {
  // One activity, one window: the flag is the activity's.
  QJniObject::callStaticMethod<void>(kBridge, "setSecure",
                                     "(Landroid/app/Activity;Z)V",
                                     Activity().object(),
                                     static_cast<jboolean>(secure));
}

}  // namespace GpgFrontend

#endif

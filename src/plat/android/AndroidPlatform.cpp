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

#include "plat/common/PlatformClipboard.h"
#include "plat/common/PlatformDisplay.h"
#include "plat/common/PlatformFolder.h"
#include "plat/common/PlatformInbox.h"
#include "plat/common/PlatformPrivacy.h"
#include "plat/common/PlatformRestart.h"
#include "plat/common/PlatformShare.h"

// The Android backends of the mobile platform interfaces, through
// com.bktus.gpgfrontend.PlatformBridge (src/plat/android/package).
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

auto SharedArrivedSlot() -> std::function<void()>& {
  static std::function<void()> arrived;
  return arrived;
}

void JNICALL OnShared(JNIEnv*, jclass) {
  // On Android's main thread: handed to the application's.
  auto* app = QCoreApplication::instance();
  if (app == nullptr) return;
  QMetaObject::invokeMethod(
      app,
      []() {
        if (const auto& arrived = SharedArrivedSlot(); arrived) arrived();
      },
      Qt::QueuedConnection);
}

void RegisterNatives() {
  static bool registered = false;
  if (registered) return;
  QJniEnvironment env;
  std::array<JNINativeMethod, 2> methods{{
      {"onShareTargetChosen", "(I)V",
       reinterpret_cast<void*>(OnShareTargetChosen)},
      {"onShared", "()V", reinterpret_cast<void*>(OnShared)},
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

void OpenFile(const QString& path, const QString& mime_type,
              const QString& title, ShareCallback done) {
  RegisterNatives();
  auto& session = ShareSession::Instance();
  const auto request = session.Begin(std::move(done));

  const auto shown = QJniObject::callStaticMethod<jboolean>(
      kBridge, "openFile",
      "(Landroid/app/Activity;Ljava/lang/String;Ljava/lang/String;"
      "Ljava/lang/String;I)Z",
      Activity().object(), QJniObject::fromString(path).object<jstring>(),
      QJniObject::fromString(mime_type).object<jstring>(),
      QJniObject::fromString(title).object<jstring>(),
      static_cast<jint>(request));
  if (!shown) session.Failed(request, QStringLiteral("no application"));
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

auto SystemFontScale() -> qreal {
  const auto configuration =
      Activity()
          .callObjectMethod("getResources", "()Landroid/content/res/Resources;")
          .callObjectMethod("getConfiguration",
                            "()Landroid/content/res/Configuration;");
  if (!configuration.isValid()) return 1.0;
  const auto scale = configuration.getField<jfloat>("fontScale");
  return scale > 0 ? static_cast<qreal>(scale) : 1.0;
}

void HideLaunchScreen() {
  // A short fade: the launch screen's logo and the loading page's are at
  // nearly, not exactly, the same place.
  QNativeInterface::QAndroidApplication::hideSplashScreen(250);
}

namespace {

auto Strings(const QJniObject& array) -> QStringList {
  QStringList strings;
  if (!array.isValid()) return strings;
  QJniEnvironment env;
  auto* items = array.object<jobjectArray>();
  const auto length = env->GetArrayLength(items);
  for (jsize i = 0; i < length; ++i) {
    strings << QJniObject::fromLocalRef(env->GetObjectArrayElement(items, i))
                   .toString();
  }
  return strings;
}

}  // namespace

auto TakeSharedItems() -> SharedItems {
  SharedItems items;
  const auto uris = Strings(QJniObject::callStaticObjectMethod(
      kBridge, "takeSharedUris", "()[Ljava/lang/String;"));
  for (const auto& uri : uris) {
    if (uri.startsWith(QStringLiteral("content:"))) {
      items.files << QUrl(uri);
    } else {
      ++items.refused;
    }
  }
  items.text = QJniObject::callStaticObjectMethod(kBridge, "takeSharedText",
                                                  "()Ljava/lang/String;")
                   .toString();
  return items;
}

void SetSharedItemsArrived(std::function<void()> arrived) {
  RegisterNatives();
  SharedArrivedSlot() = std::move(arrived);
}

auto FolderDocumentNames(const QUrl& folder) -> std::optional<QStringList> {
  const auto names = QJniObject::callStaticObjectMethod(
      kBridge, "childNames",
      "(Landroid/content/Context;Ljava/lang/String;)[Ljava/lang/String;",
      Activity().object(),
      QJniObject::fromString(folder.toString(QUrl::FullyEncoded))
          .object<jstring>());
  if (!names.isValid()) return std::nullopt;
  return Strings(names);
}

auto FolderFiles(const QUrl& folder) -> std::optional<FolderContents> {
  const auto found = QJniObject::callStaticObjectMethod(
      kBridge, "childDocuments",
      "(Landroid/content/Context;Ljava/lang/String;)[Ljava/lang/String;",
      Activity().object(),
      QJniObject::fromString(folder.toString(QUrl::FullyEncoded))
          .object<jstring>());
  if (!found.isValid()) return std::nullopt;
  FolderContents contents;
  for (const auto& entry : Strings(found)) {
    if (entry.startsWith(QStringLiteral("d:"))) {
      ++contents.folders;
    } else if (entry.startsWith(QStringLiteral("f:"))) {
      contents.files.append(QUrl(entry.mid(2)));
    }
  }
  return contents;
}

auto FindFolderDocument(const QUrl& folder, const QString& name) -> QUrl {
  const auto uri = QJniObject::callStaticObjectMethod(
      kBridge, "findChild",
      "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)"
      "Ljava/lang/String;",
      Activity().object(),
      QJniObject::fromString(folder.toString(QUrl::FullyEncoded))
          .object<jstring>(),
      QJniObject::fromString(name).object<jstring>());
  return uri.isValid() ? QUrl(uri.toString()) : QUrl();
}

auto CreateFolderDocument(const QUrl& folder, const QString& name,
                          const QString& mime_type) -> QUrl {
  const auto uri = QJniObject::callStaticObjectMethod(
      kBridge, "createChild",
      "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;"
      "Ljava/lang/String;)Ljava/lang/String;",
      Activity().object(),
      QJniObject::fromString(folder.toString(QUrl::FullyEncoded))
          .object<jstring>(),
      QJniObject::fromString(mime_type).object<jstring>(),
      QJniObject::fromString(name).object<jstring>());
  return uri.isValid() ? QUrl(uri.toString()) : QUrl();
}

auto CanSecureWindow() -> bool { return true; }

void SetWindowSecure(QWindow*, bool secure) {
  // One activity, one window: the flag is the activity's.
  QJniObject::callStaticMethod<void>(kBridge, "setSecure",
                                     "(Landroid/app/Activity;Z)V",
                                     Activity().object(),
                                     static_cast<jboolean>(secure));
}

auto RestartApplication() -> bool {
  return QJniObject::callStaticMethod<jboolean>(
             kBridge, "restart", "(Landroid/content/Context;)Z",
             Activity().object()) != JNI_FALSE;
}

}  // namespace GpgFrontend

#endif

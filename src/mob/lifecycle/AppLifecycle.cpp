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

#include "lifecycle/AppLifecycle.h"

#include <QGuiApplication>
#include <QJSEngine>

#include "core/function/rpgp/PasswordFetcher.h"
#include "crypto/PassphraseBroker.h"
#include "plat/common/PlatformClipboard.h"
#include "plat/common/PlatformDisplay.h"
#include "startup/InteractionBridge.h"

namespace GpgFrontend::Mobile {

namespace {

QPointer<AppLifecycle> g_instance;

}  // namespace

AppLifecycle::AppLifecycle(InteractionBridge* bridge,
                           PassphraseBroker* passphrases, QObject* parent)
    : QObject(parent), bridge_(bridge), passphrases_(passphrases) {
  if (auto* app = qobject_cast<QGuiApplication*>(QCoreApplication::instance());
      app != nullptr) {
    connect(app, &QGuiApplication::applicationStateChanged, this,
            &AppLifecycle::OnApplicationStateChanged);
  }
  read_font_scale();
}

auto AppLifecycle::FontScale() const -> qreal { return font_scale_; }

void AppLifecycle::read_font_scale() {
  const auto scale = SystemFontScale();
  if (qFuzzyCompare(scale, font_scale_)) return;
  font_scale_ = scale;
  emit SignalFontScaleChanged();
}

void AppLifecycle::SetInstance(AppLifecycle* instance) {
  g_instance = instance;
}

auto AppLifecycle::create(QQmlEngine*, QJSEngine*) -> AppLifecycle* {
  QJSEngine::setObjectOwnership(g_instance, QJSEngine::CppOwnership);
  return g_instance;
}

void AppLifecycle::OnApplicationStateChanged(Qt::ApplicationState state) {
  qInfo() << "application state:" << state;

  if (state == Qt::ApplicationSuspended) {
    if (suspended_) return;
    suspended_ = true;

    // Done before returning: Android may end a stopped process without
    // calling back again.
    if (passphrases_ != nullptr) passphrases_->CancelAll();
    ClearRpgpPasswordCache();
    bridge_->WipeSecretInputs();

    emit SignalSuspendedChanged();
    emit SignalSuspended();
    return;
  }

  if (state == Qt::ApplicationActive && suspended_) {
    suspended_ = false;
    // A sensitive copy's timer did not run in the background.
    ExpireSensitiveCopy();
    read_font_scale();
    emit SignalSuspendedChanged();
    emit SignalResumed();
  }
}

auto AppLifecycle::IsSuspended() const -> bool { return suspended_; }

}  // namespace GpgFrontend::Mobile

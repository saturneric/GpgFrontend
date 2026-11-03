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

#include "startup/MobileProfileLoaderDelegate.h"

#include "startup/InteractionBridge.h"

namespace GpgFrontend::Mobile {

namespace {

auto ResetReasonName(ProfileKeyResetReason reason) -> QString {
  switch (reason) {
    case ProfileKeyResetReason::kKEYCHAIN_SECRET_LOST:
      return QStringLiteral("keychain_secret_lost");
    case ProfileKeyResetReason::kPIN_FORGOTTEN:
      return QStringLiteral("pin_forgotten");
    case ProfileKeyResetReason::kKEY_FILE_MISSING:
      return QStringLiteral("key_file_missing");
  }
  return {};
}

}  // namespace

MobileProfileLoaderDelegate::MobileProfileLoaderDelegate(
    InteractionBridge* bridge)
    : bridge_(bridge) {}

auto MobileProfileLoaderDelegate::AskAppKeyPin(const AppKeyPinRequest& request)
    -> AppKeyPinAnswer {
  const auto reply = bridge_
                         ->Ask(InteractionBridge::Kind::kAPP_KEY_PIN,
                               {{QStringLiteral("creating"), request.creating},
                                {QStringLiteral("failures"), request.failures}})
                         .toMap();

  AppKeyPinAnswer answer;  // kQuit unless the person chose otherwise
  const auto action = reply.value(QStringLiteral("action")).toString();
  if (action == QStringLiteral("pin")) {
    answer.action = AppKeyPinAnswer::Action::kUsePin;
    answer.pin = GFBuffer(reply.value(QStringLiteral("pin")).toString());
  } else if (action == QStringLiteral("reset")) {
    // Confirmed here, as on the desktop: the loader takes a reset as decided.
    // Declined, it is one more wrong attempt, and the PIN is asked again.
    if (ConfirmKeyReset(ProfileKeyResetReason::kPIN_FORGOTTEN)) {
      answer.action = AppKeyPinAnswer::Action::kResetKey;
    } else {
      answer.action = AppKeyPinAnswer::Action::kUsePin;
    }
  }
  return answer;
}

auto MobileProfileLoaderDelegate::ConfirmForceUnlock(
    const ProfileLockResult& held) -> bool {
  // Always asked: a "yes" here is a force. A holder that has gone by then is
  // found by the loader itself, which tries once more before forcing.
  return bridge_
      ->Ask(InteractionBridge::Kind::kCONFIRM_FORCE_UNLOCK,
            {{QStringLiteral("pid"), held.pid},
             {QStringLiteral("host"), held.host},
             {QStringLiteral("application"), held.application}})
      .toBool();
}

auto MobileProfileLoaderDelegate::ConfirmKeyReset(ProfileKeyResetReason reason)
    -> bool {
  return bridge_
      ->Ask(InteractionBridge::Kind::kCONFIRM_KEY_RESET,
            {{QStringLiteral("reason"), ResetReasonName(reason)}})
      .toBool();
}

void MobileProfileLoaderDelegate::Report(const ProfileLoadError& error) {
  last_error_ = error;
  qCritical().noquote() << "profile load failed:" << error.subject << "-"
                        << error.detail;
}

auto MobileProfileLoaderDelegate::LastError() const
    -> std::optional<ProfileLoadError> {
  return last_error_;
}

auto MobileProfileLoaderDelegate::AskPackagePassphrase(const QString& package,
                                                       bool)
    -> std::optional<GFBuffer> {
  qWarning() << "no one to ask for the passphrase of" << package;
  return std::nullopt;
}

void MobileProfileLoaderDelegate::Note(ProfileLoadNotice,
                                       const QString& detail) {
  qInfo().noquote() << "profile note:" << detail;
}

}  // namespace GpgFrontend::Mobile

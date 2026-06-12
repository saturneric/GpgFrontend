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

#include "PassphraseService.h"

#include "core/function/CoreSignalStation.h"
#include "core/function/openpgp/AbstractKeyRepository.h"
#include "core/model/GpgPassphraseContext.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

PassphraseService::PassphraseService(int channel)
    : SingletonFunctionObject(channel) {}

auto PassphraseService::RequestPassphrase(const PassphraseState& state)
    -> GFBuffer {
  GpgAbstractKeyPtr key = nullptr;

  if (!state.ask_for_new && state.info.trimmed().isEmpty()) {
    LOG_W()
        << "Passphrase request with empty fingerprint and not asking for new "
           "passphrase. This may lead to incorrect key association.";
    return {};
  }

  auto t_state = state;  // Make a mutable copy of the state
  t_state.fpr = t_state.fpr.trimmed().toUpper();

  if (!t_state.fpr.isEmpty()) {
    key = AbstractKeyRepository::GetInstance(GetChannel()).GetKey(t_state.fpr);
    if (key == nullptr) {
      LOG_W()
          << "No key found for empty fingerprint. This may lead to incorrect "
             "passphrase association.";
    }
  }

  GFBuffer result_pwd;
  auto c = QSharedPointer<GpgPassphraseContext>::create(GetChannel(), key);
  c->SetPassphraseInfo(t_state.info);
  c->SetPrevWasBad(t_state.retry);
  c->SetAskForNew(t_state.ask_for_new);
  c->SetShouldConfirm(t_state.should_confirm);

  QEventLoop loop;
  QTimer timeout_timer;
  timeout_timer.setSingleShot(true);
  QObject::connect(&timeout_timer, &QTimer::timeout, &loop, &QEventLoop::quit);

  QObject::connect(
      CoreSignalStation::GetInstance(),
      &CoreSignalStation::SignalUserInputPassphraseReady, &loop,
      [&result_pwd,
       &loop](const QSharedPointer<GpgPassphraseContext>& ctx) -> void {
        result_pwd = ctx->GetPassphrase();
        loop.quit();
      });

  QTimer::singleShot(0, [c]() -> void {
    emit CoreSignalStation::GetInstance() -> SignalNeedUserInputPassphrase(c);
  });

  timeout_timer.start(60000);
  loop.exec();
  timeout_timer.stop();
  return result_pwd;
}
}  // namespace GpgFrontend
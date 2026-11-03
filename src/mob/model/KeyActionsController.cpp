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

#include "model/KeyActionsController.h"

#include <QPointer>

#include "core/function/KeyGenProfile.h"
#include "core/function/openpgp/ChannelHandle.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/function/openpgp/KeyGenerationOperation.h"
#include "core/function/openpgp/KeyImportExportOperation.h"
#include "core/function/openpgp/KeyManagementOperation.h"
#include "core/model/DataObject.h"
#include "core/model/GpgGenerateKeyResult.h"
#include "core/model/GpgKeyGenerateInfo.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/MemoryUtils.h"

namespace GpgFrontend::Mobile {

namespace {

auto Offered() -> QList<ResolvedKeyGenProfile> {
  // The profile's own, as the desktop made them, else the defaults.
  const auto stored = LoadStoredKeyGenProfiles();
  auto offered =
      OfferedKeyGenProfiles(UsableDefaultChannelHandle(), stored.profiles);
  // The one Quick Generate uses first, the rest in their usual order.
  const auto preferred =
      PickDefaultKeyGenProfile(offered, stored.default_profile);
  if (preferred > 0) offered.move(preferred, 0);
  return offered;
}

auto Failure(const GFError& err) -> QVariantMap {
  return {{QStringLiteral("error"), DescribeGFError(err).second}};
}

auto FindKey(int channel, const QString& fingerprint) -> GpgAbstractKeyPtr {
  return GpgKeyRepository::GetInstance(channel).GetKeyPtr(
      fingerprint.toUpper());
}

}  // namespace

KeyActionsController::KeyActionsController(QObject* parent)
    : QObject(parent) {}

auto KeyActionsController::Busy() const -> bool { return busy_; }

auto KeyActionsController::Profiles() const -> QVariantList {
  QVariantList list;
  for (const auto& resolved : Offered()) {
    list.append(QVariantMap{
        {QStringLiteral("name"), KeyGenProfileDisplayName(resolved.profile)},
        {QStringLiteral("description"),
         KeyGenProfileDisplayDescription(resolved.profile)},
    });
  }
  return list;
}

void KeyActionsController::generate(const QString& name, const QString& email,
                                    int profile, bool with_passphrase) {
  const auto offered = Offered();
  if (profile < 0 || profile >= offered.size()) {
    emit SignalFinished(QStringLiteral("generate"), false,
                        Failure(GFError::Application(GFErrorCode::kINVALID_VALUE)));
    return;
  }
  const auto resolved = offered.at(profile);
  // The name and comment become part of a mail address: what would break it
  // is refused here, as the desktop's wizard refuses it.
  const auto identity = CheckKeyGenIdentity(name, email, {});
  if (!identity.Ok()) {
    emit SignalFinished(
        QStringLiteral("generate"), false,
        {{QStringLiteral("error"),
          !identity.name.isEmpty() ? identity.name : identity.email}});
    return;
  }

  run(QStringLiteral("generate"), [=]() -> QVariantMap {
    auto key = SecureCreateSharedObject<KeyGenerateInfo>();
    QSharedPointer<KeyGenerateInfo> subkey;
    // Before the profile, which gives a subkey the same choice.
    key->SetNonPassPhrase(!with_passphrase);
    ApplyKeyGenProfile(resolved, QDateTime::currentDateTime(), *key, subkey);
    key->SetName(name.trimmed());
    key->SetEmail(email.trimmed());
    // Post-quantum algorithms only exist in the v6 format; anything else
    // keeps the interoperable v4 one, as the desktop does.
    const auto version = RequiredKeyVersion(*key, subkey);
    key->SetKeyVersion(version != 0 ? version : 4);

    const auto channel = UsableDefaultChannelHandle();
    auto& opera = KeyGenerationOperation::GetInstance(channel);
    auto [err, data] = subkey != nullptr
                           ? opera.GenerateKeyWithSubkeySync(key, subkey)
                           : opera.GenerateKeySync(key);
    if (!err.Ok() || data == nullptr ||
        data->GetObjectSize() == 0) {
      return Failure(err);
    }
    GpgKeyRepository::GetInstance(channel).FlushKeyCache();
    return {{QStringLiteral("fingerprint"),
             ExtractParams<GpgGenerateKeyResult>(data, 0).GetFingerprint()}};
  });
}

void KeyActionsController::exportPublic(const QString& fingerprint) {
  run(QStringLiteral("export"), [fingerprint]() -> QVariantMap {
    const auto channel = UsableDefaultChannelHandle();
    const auto key = FindKey(channel, fingerprint);
    if (key == nullptr) return Failure(GFError::Application(GFErrorCode::kNO_PUBLIC_KEY));
    auto [err, armored] = KeyImportExportOperation::GetInstance(channel)
                              .ExportKey(key, false, true, false);
    if (!err.Ok()) return Failure(err);
    return {{QStringLiteral("armored"),
             QString::fromUtf8(armored.ConvertToQByteArray())}};
  });
}

void KeyActionsController::remove(const QString& fingerprint) {
  run(QStringLiteral("delete"), [fingerprint]() -> QVariantMap {
    const auto channel = UsableDefaultChannelHandle();
    const auto key = FindKey(channel, fingerprint);
    if (key == nullptr) return Failure(GFError::Application(GFErrorCode::kNO_PUBLIC_KEY));
    KeyManagementOperation::GetInstance(channel).DeleteKey(key);
    auto& repository = GpgKeyRepository::GetInstance(channel);
    repository.FlushKeyCache();
    if (FindKey(channel, fingerprint) != nullptr) {
      return Failure(GFError::Application(GFErrorCode::kGENERAL));
    }
    return {};
  });
}

auto KeyActionsController::begin() -> bool {
  if (busy_) return false;
  busy_ = true;
  emit SignalBusyChanged();
  return true;
}

void KeyActionsController::run(const QString& action,
                               std::function<QVariantMap()> work) {
  if (!begin()) return;

  auto detail = QSharedPointer<QVariantMap>::create();
  QPointer<KeyActionsController> self(this);
  auto* task = new Thread::Task(
      [work = std::move(work), detail](const DataObjectPtr&) -> int {
        *detail = work();
        return 0;
      },
      "mobile_key_" + action, TransferParams(),
      [self, action, detail](int, const DataObjectPtr&) {
        if (self == nullptr) return;
        self->busy_ = false;
        emit self->SignalBusyChanged();
        emit self->SignalFinished(
            action, !detail->contains(QStringLiteral("error")), *detail);
      });

  Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_GPG)
      ->PostTask(task);
}

}  // namespace GpgFrontend::Mobile

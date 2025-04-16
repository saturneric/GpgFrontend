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

#include "core/model/GpgKeyGroup.h"

#include "core/function/gpg/GpgKeyGroupGetter.h"

namespace GpgFrontend {

GpgKeyGroup::GpgKeyGroup() = default;

GpgKeyGroup::GpgKeyGroup(const GpgKeyGroup &) = default;

GpgKeyGroup::~GpgKeyGroup() = default;

auto GpgKeyGroup::operator=(const GpgKeyGroup &) -> GpgKeyGroup & = default;

auto GpgKeyGroup::IsGood() const -> bool { return true; }

auto GpgKeyGroup::ID() const -> QString { return id_; }

auto GpgKeyGroup::Name() const -> QString { return name_; };

auto GpgKeyGroup::Email() const -> QString { return email_; }

auto GpgKeyGroup::Comment() const -> QString { return comment_; }

auto GpgKeyGroup::Fingerprint() const -> QString { return ID(); }

auto GpgKeyGroup::PublicKeyAlgo() const -> QString { return {}; }

auto GpgKeyGroup::Algo() const -> QString { return {}; }

auto GpgKeyGroup::ExpirationTime() const -> QDateTime {
  return QDateTime::fromSecsSinceEpoch(0);
};

auto GpgKeyGroup::CreationTime() const -> QDateTime { return creation_time_; };

auto GpgKeyGroup::IsHasEncrCap() const -> bool { return true; }

auto GpgKeyGroup::IsHasSignCap() const -> bool { return false; }

auto GpgKeyGroup::IsHasCertCap() const -> bool { return false; }

auto GpgKeyGroup::IsHasAuthCap() const -> bool { return false; }

auto GpgKeyGroup::IsPrivateKey() const -> bool { return false; }

auto GpgKeyGroup::IsExpired() const -> bool { return false; }

auto GpgKeyGroup::IsRevoked() const -> bool { return false; }

auto GpgKeyGroup::IsDisabled() const -> bool {
  return getter_ == nullptr ? true : getter_->IsKeyGroupDisabled(ID());
}

auto GpgKeyGroup::KeyType() const -> GpgAbstractKeyType {
  return GpgAbstractKeyType::kGPG_KEYGROUP;
}

GpgKeyGroup::GpgKeyGroup(QString name, QString email, QString comment,
                         QStringList key_ids)
    : id_("#&" + QUuid::createUuid().toRfc4122().toHex().left(14).toUpper()),
      name_(std::move(name)),
      email_(std::move(email)),
      comment_(std::move(comment)),
      key_ids_(std::move(key_ids)),
      creation_time_(QDateTime::currentDateTime()) {}

GpgKeyGroup::GpgKeyGroup(const KeyGroupCO &kg_co)
    : id_(kg_co.id),
      name_(kg_co.name),
      email_(kg_co.email),
      comment_(kg_co.comment),
      key_ids_(kg_co.key_ids),
      creation_time_(kg_co.creation_time) {}

auto GpgKeyGroup::ToCacheObject() const -> KeyGroupCO {
  KeyGroupCO co;
  co.id = id_;
  co.name = name_;
  co.email = email_;
  co.comment = comment_;
  co.key_ids = key_ids_;
  co.creation_time = creation_time_;
  return co;
}

auto GpgKeyGroup::KeyIds() const -> QStringList { return key_ids_; }

void GpgKeyGroup::SetKeyIds(QStringList key_ids) {
  key_ids_ = std::move(key_ids);
}

void GpgKeyGroup::SetKeyGroupGetter(GpgKeyGroupGetter *getter) {
  getter_ = getter;
}
}  // namespace GpgFrontend
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

#include "GpgAbstractKeyGetter.h"

#include "core/model/GpgKeyTableModel.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

GpgAbstractKeyGetter::GpgAbstractKeyGetter(int channel)
    : SingletonFunctionObject<GpgAbstractKeyGetter>(channel){};

auto GpgAbstractKeyGetter::Fetch() -> GpgAbstractKeyPtrList {
  auto ret = GpgAbstractKeyPtrList{};

  auto keys = key_.Fetch();
  for (const auto& key : keys) {
    ret.push_back(qSharedPointerCast<GpgAbstractKey>(key));
  }

  auto kgs = kg_.Fetch();
  for (const auto& kg : kgs) {
    ret.push_back(qSharedPointerCast<GpgAbstractKey>(kg));
  }

  return ret;
}

auto GpgAbstractKeyGetter::GetGpgKeyTableModel()
    -> QSharedPointer<GpgKeyTableModel> {
  return SecureCreateSharedObject<GpgKeyTableModel>(
      SingletonFunctionObject::GetChannel(), Fetch(), nullptr);
}

auto GpgAbstractKeyGetter::FlushCache() -> bool {
  return key_.FlushKeyCache() && kg_.FlushCache();
}

auto GpgAbstractKeyGetter::GetKey(const QString& key_id) -> GpgAbstractKeyPtr {
  if (IsKeyGroupID(key_id)) {
    return kg_.KeyGroup(key_id);
  }
  return key_.GetKeyORSubkeyPtr(key_id);
}

auto GpgAbstractKeyGetter::GetKeys(const QStringList& key_ids)
    -> GpgAbstractKeyPtrList {
  auto ret = GpgAbstractKeyPtrList{};
  for (const auto& key_id : key_ids) {
    auto key = GetKey(key_id);
    if (key == nullptr) continue;

    ret.push_back(key);
  }
  return ret;
}

GpgAbstractKeyGetter::~GpgAbstractKeyGetter() = default;
}  // namespace GpgFrontend
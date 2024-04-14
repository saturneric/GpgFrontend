/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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

#pragma once

#include "core/module/GlobalRegisterTable.h"

namespace GpgFrontend::Module {
class GPGFRONTEND_CORE_EXPORT GlobalRegisterTableTreeModel
    : public QAbstractItemModel {
 public:
  explicit GlobalRegisterTableTreeModel(GlobalRegisterTable *grt);

  [[nodiscard]] auto rowCount(const QModelIndex &parent) const -> int override;

  [[nodiscard]] auto columnCount(const QModelIndex &parent) const
      -> int override;

  [[nodiscard]] auto data(const QModelIndex &index, int role) const
      -> QVariant override;

  [[nodiscard]] auto index(int row, int column, const QModelIndex &parent) const
      -> QModelIndex override;

  [[nodiscard]] auto parent(const QModelIndex &index) const
      -> QModelIndex override;

  [[nodiscard]] auto headerData(int section, Qt::Orientation orientation,
                                int role) const -> QVariant override;

 private:
  class Impl;
  SecureUniquePtr<Impl> p_;
};

};  // namespace GpgFrontend::Module
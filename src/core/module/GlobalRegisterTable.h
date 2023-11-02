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

#include <any>
#include <functional>
#include <optional>

namespace GpgFrontend::Module {

using Namespace = std::string;
using Key = std::string;
using LPCallback = std::function<void(Namespace, Key, int, std::any)>;

class GlobalRegisterTable : public QObject {
  Q_OBJECT
 public:
  GlobalRegisterTable();

  ~GlobalRegisterTable() override;

  auto PublishKV(Namespace, Key, std::any) -> bool;

  auto LookupKV(Namespace, Key) -> std::optional<std::any>;

  auto ListenPublish(QObject *, Namespace, Key, LPCallback) -> bool;

 signals:
  void SignalPublish(Namespace, Key, int, std::any);

 private:
  class Impl;
  std::unique_ptr<Impl> p_;
};

}  // namespace GpgFrontend::Module
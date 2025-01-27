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

#pragma once

#include <cstddef>
#include <queue>

namespace GpgFrontend {

constexpr ssize_t kDataExchangerSize =
    static_cast<const ssize_t>(1024 * 1024 * 8);  // 8 MB

class GFDataExchanger {
 public:
  explicit GFDataExchanger(ssize_t size);

  auto Write(const std::byte* buffer, size_t size) -> ssize_t;

  auto Read(std::byte* buffer, size_t size) -> ssize_t;

  void CloseWrite();

 private:
  std::condition_variable not_full_, not_empty_;
  std::queue<std::byte> queue_;
  std::mutex mutex_;
  const ssize_t queue_max_size_;
  std::atomic_bool close_ = false;
};

inline auto CreateStandardGFDataExchanger() -> QSharedPointer<GFDataExchanger> {
  return QSharedPointer<GFDataExchanger>::create(kDataExchangerSize);
}

}  // namespace GpgFrontend
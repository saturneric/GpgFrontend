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

namespace GpgFrontend {

class GpgAbstractKey {
 public:
  [[nodiscard]] virtual auto ID() const -> QString = 0;
  [[nodiscard]] virtual auto Fingerprint() const -> QString = 0;
  [[nodiscard]] virtual auto IsSubKey() const -> bool = 0;

  [[nodiscard]] virtual auto IsHasEncrCap() const -> bool = 0;
  [[nodiscard]] virtual auto IsHasSignCap() const -> bool = 0;
  [[nodiscard]] virtual auto IsHasCertCap() const -> bool = 0;
  [[nodiscard]] virtual auto IsHasAuthCap() const -> bool = 0;
  [[nodiscard]] virtual auto IsRevoked() const -> bool = 0;
  [[nodiscard]] virtual auto IsDisabled() const -> bool = 0;
  [[nodiscard]] virtual auto IsExpired() const -> bool = 0;
  [[nodiscard]] virtual auto IsGood() const -> bool = 0;
  [[nodiscard]] virtual auto PublicKeyAlgo() const -> QString = 0;
  [[nodiscard]] virtual auto Algo() const -> QString = 0;
  [[nodiscard]] virtual auto CreationTime() const -> QDateTime = 0;
  [[nodiscard]] virtual auto ExpirationTime() const -> QDateTime = 0;

  //

  [[nodiscard]] auto IsPrimaryKey() const -> bool { return IsHasCertCap(); }

  auto operator==(const GpgAbstractKey& o) const -> bool {
    return ID() == o.ID();
  }

  auto operator<(const GpgAbstractKey& o) const -> bool {
    return ID() < o.ID();
  }

  virtual ~GpgAbstractKey() = default;
};

}  // namespace GpgFrontend


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

#include "core/function/openpgp/PassphraseService.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

class GF_CORE_EXPORT GpgPassphraseContext : public QObject {
  Q_OBJECT
 public:
  GpgPassphraseContext(int channel, GpgAbstractKeyPtr key);

  GpgPassphraseContext();

  ~GpgPassphraseContext();

  void SetPassphrase(const GFBuffer& passphrase);

  [[nodiscard]] auto GetChannel() const -> int;

  void SetChannel(int channel);

  [[nodiscard]] auto GetPassphrase() const -> GFBuffer;

  [[nodiscard]] auto GetKey() const -> GpgAbstractKeyPtr;

  [[nodiscard]] auto GetPassphraseInfo() const -> QString;

  void SetPassphraseInfo(const QString& info);

  [[nodiscard]] auto IsPreWasBad() const -> bool;

  void SetPrevWasBad(bool was_bad);

  [[nodiscard]] auto IsAskForNew() const -> bool;

  void SetAskForNew(bool ask_for_new);

  [[nodiscard]] auto ShouldConfirm() const -> bool;

  void SetShouldConfirm(bool should_confirm);

 private:
  int channel_;
  QString passphrase_info_;
  GpgAbstractKeyPtr key_;
  GFBuffer passphrase_;
  bool prev_was_bad_;
  bool ask_for_new_;
  bool should_confirm_;
};

}  // namespace GpgFrontend
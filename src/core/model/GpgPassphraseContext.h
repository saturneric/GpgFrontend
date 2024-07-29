

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

#include "core/GpgFrontendCoreExport.h"

namespace GpgFrontend {

class GPGFRONTEND_CORE_EXPORT GpgPassphraseContext : public QObject {
  Q_OBJECT
 public:
  GpgPassphraseContext(const QString& uids_info, const QString& passphrase_info,
                       bool prev_was_bad, bool ask_for_new);

  GpgPassphraseContext();

  void SetPassphrase(const QString& passphrase);

  [[nodiscard]] auto GetPassphrase() const -> QString;

  [[nodiscard]] auto GetUidsInfo() const -> QString;

  [[nodiscard]] auto GetPassphraseInfo() const -> QString;

  [[nodiscard]] auto IsPreWasBad() const -> bool;

  [[nodiscard]] auto IsAskForNew() const -> bool;

 private:
  QString passphrase_info_;
  QString uids_info_;
  QString passphrase_;
  bool prev_was_bad_;
  bool ask_for_new_;
};

}  // namespace GpgFrontend
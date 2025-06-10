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

#include "core/function/gpg/GpgContext.h"
#include "core/model/GpgKey.h"
#include "core/model/GpgKeyGenerateInfo.h"
#include "core/typedef/GpgTypedef.h"
#include "core/utils/MemoryUtils.h"
#include "ui/dialog/GeneralDialog.h"

class Ui_SubkeyGenDialog;

namespace GpgFrontend::UI {
/**
 * @brief
 *
 */
class SubkeyGenerateDialog : public GeneralDialog {
  Q_OBJECT

 public:
  /**
   * @brief Construct a new Subkey Generate Dialog object
   *
   * @param key_id
   * @param parent
   */
  explicit SubkeyGenerateDialog(int channel, GpgKeyPtr key, QWidget* parent);

 private slots:

  /**
   * @details check all line edits for false entries. Show error, when there is
   * one, otherwise generate the key
   */
  void slot_key_gen_accept();

 private:
  QSharedPointer<Ui_SubkeyGenDialog> ui_;  ///<
  int current_gpg_context_channel_;        ///<

  GpgKeyPtr key_;                                    ///<
  QSharedPointer<KeyGenerateInfo> gen_subkey_info_;  ///<
  QContainer<KeyAlgo> supported_subkey_algos_;       ///<

  /**
   * @brief Set the signal slot object
   *
   */
  void set_signal_slot_config();

  /**
   * @details Refresh widgets state by GenKeyInfo
   */
  void refresh_widgets_state();
};

}  // namespace GpgFrontend::UI

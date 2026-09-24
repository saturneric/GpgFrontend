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

#include <functional>

#include "core/model/GFBuffer.h"

namespace GpgFrontend::UI {

/**
 * @file CodecStage.h
 * @brief Running the codecs modules provide around the Host's own OpenPGP
 *        operations. See gf::cmd::host::CodecArgs for the contract.
 *
 * Every call goes through the command registry, so through each module's
 * entry gate: a module that is deactivating, or already gone, answers
 * GF_CMD_E_UNAVAILABLE, and the stage treats that exactly like a codec that
 * did not claim the input. Nothing here is GUI: a stage completes on the
 * thread that started it, through that thread's event loop.
 */

/// How a stage ended.
struct GF_UI_EXPORT CodecStageResult {
  enum class Kind {
    kNotHandled,  ///< no codec claimed the input: carry on without one
    kHandled,     ///< `output` replaces the input
    kFailed,      ///< a codec claimed the input and could not handle it
  };

  Kind kind = Kind::kNotHandled;
  GFBuffer output;
  QString error;
  QString cards;     ///< the codec's info-board card JSON, may be empty
  QString provider;  ///< the command that answered, when one did
};

class GF_UI_EXPORT CodecStage {
 public:
  using Done = std::function<void(CodecStageResult)>;

  /// How long one codec may take before the stage moves on without it.
  static constexpr int kCodecTimeoutMs = 10000;

  /**
   * @brief Offer @p input to every decoder, in id order.
   *
   * The first to answer kHandled or kFailed ends the stage with its answer.
   * One that answers kNotHandled, fails to answer, is unavailable or takes
   * longer than @p timeout_ms is passed over. @p done runs exactly once, on
   * the calling thread, from its event loop.
   */
  static void RunDecoders(GFBuffer input, Done done,
                          int timeout_ms = kCodecTimeoutMs);

  /**
   * @brief Have encoder @p id turn @p input into its output.
   *
   * The user asked for this encoder, so anything but kHandled -- including an
   * encoder that is gone -- ends as kFailed. @p done runs exactly once, on
   * the calling thread, from its event loop.
   */
  static void RunEncoder(const QString& id, GFBuffer input, Done done,
                         int timeout_ms = kCodecTimeoutMs);
};

}  // namespace GpgFrontend::UI

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

// Safe to include ahead of the Q_OBJECT class below only because this class has
// no translatable strings: lupdate drops the enclosing namespace from the tr()
// context of a Q_OBJECT class preceded by a typed enum.
#include "core/typedef/GFTypedef.h"

namespace GpgFrontend {

class GpgPassphraseContext;

/**
 * @brief
 *
 */
class GF_CORE_EXPORT CoreSignalStation : public QObject {
  Q_OBJECT
  static std::unique_ptr<CoreSignalStation> instance;

 public:
  /**
   * @brief Get the Instance object
   *
   * @return SignalStation*
   */
  static auto GetInstance() -> CoreSignalStation*;

 signals:

  /**
   * @brief
   *
   */
  void SignalNeedUserInputPassphrase(QSharedPointer<GpgPassphraseContext>);

  /**
   * @brief
   *
   */
  void SignalUserInputPassphraseReady(QSharedPointer<GpgPassphraseContext>);

  /**
   * @brief The requester has given up on this passphrase request (its deadline
   * elapsed), so the prompt opened for it must be dismissed.
   *
   * Without this a prompt outlives the request that opened it: nothing is
   * listening for its answer any more, yet it stays on screen and holds the
   * modal stack.
   *
   * @param ctx the request whose prompt should close; other prompts ignore it
   */
  void SignalCloseUserInputPassphrase(QSharedPointer<GpgPassphraseContext> ctx);

  /**
   * @brief
   *
   */
  /**
   * @brief The OpenPGP environment could not be brought up.
   *
   * @param reason what failed, so the UI can title it accurately
   * @param detail human-readable specifics for the message body
   */
  void SignalBadOpenPGPEnv(GpgFrontend::BadOpenPGPEnvReason reason,
                           QString detail);

  /**
   * @brief
   *
   */
  void SignalGoodGnupgEnv();

  /**
   * @brief
   *
   */
  void SignalCoreFullyLoaded();

  /**
   * @brief How far startup has got, for the dialog that is blocking on it.
   *
   * Emitted from whichever task runner thread did the work, so every delivery
   * is queued. The step is a code rather than a sentence on purpose: this
   * fires before InitUITranslations() has installed a translator, so a string
   * built here would reach the user untranslated. The UI owns the wording.
   *
   * @param percent 0 to 100, never decreasing within a run
   * @param step what is happening now
   * @param subject the key database or module name, where the step names one
   */
  void SignalCoreInitProgress(int percent, GpgFrontend::CoreInitStep step,
                              QString subject);
};

}  // namespace GpgFrontend

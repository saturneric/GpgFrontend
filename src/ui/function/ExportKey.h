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

#include "core/typedef/GpgTypedef.h"
#include "shell/rules/KeyExportRules.h"

namespace GpgFrontend::UI {

/**
 * @brief Export one key to a file the user picks.
 *
 * Was private to the key details dialog's Operations tab, which meant the only
 * way to save a key to a file was to open that dialog first. Lifted out so the
 * key list can offer it directly.
 *
 * @warning The export runs on a task runner and the callback drives the file
 * dialog, so an instance outlives the call that started it. It deletes itself
 * once done — do **not** deleteLater() it from the caller the way the
 * synchronous function widgets are used.
 */
class ExportKey : public QWidget {
  Q_OBJECT
 public:
  explicit ExportKey(QWidget* parent);

  /**
   * @brief Export the public half. No confirmation: a public key is meant to
   * be handed out.
   */
  void ExecPublic(int channel, const GpgKeyPtr& key);

  /**
   * @brief Export the full private key, after a loud confirmation.
   */
  void ExecPrivate(int channel, const GpgKeyPtr& key);

  /**
   * @brief Export a minimal private key, after a loud confirmation.
   */
  void ExecShortPrivate(int channel, const GpgKeyPtr& key);

  /**
   * @brief Export one private subkey of @p key. Asks nothing: the caller
   * confirms first.
   *
   * @param subkey_fpr fingerprint of the subkey to export
   * @param subkey_id its key id, for the suggested file name
   */
  void ExecSubkey(int channel, const GpgKeyPtr& key, const QString& subkey_fpr,
                  const QString& subkey_id);

 private:
  using ExportFunction = std::function<std::tuple<GFError, GFBuffer>()>;

  /**
   * @brief Export one kind of @p key.
   */
  void exec_export(int channel, const GpgKeyPtr& key,
                   Shell::KeyExportKind kind);

  /**
   * @brief Run @p export_function on the GPG task runner, then drive the save
   * dialog, offering @p file_name.
   *
   * The export may wait on a passphrase prompt, so it never runs on the GUI
   * thread.
   */
  void run_export(const ExportFunction& export_function,
                  const QString& file_name);
};

}  // namespace GpgFrontend::UI

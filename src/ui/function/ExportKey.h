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

namespace GpgFrontend::UI {

/**
 * @brief Build the default filename offered when saving an exported key.
 *
 * Pure so the platform template and the sanitising can be pinned down without
 * touching gpg or a file dialog.
 *
 * @param name key owner's name
 * @param email key owner's email address
 * @param id key id
 * @param type suffix distinguishing what was exported ("pub", "full_secret",
 *        "short_secret")
 * @return a filename safe to hand to QFileDialog on this platform
 */
auto GF_UI_EXPORT ExportKeyFileName(const QString& name, const QString& email,
                                    const QString& id, const QString& type)
    -> QString;

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

 private:
  /**
   * @brief Run the export and drive the save dialog.
   */
  void exec_export(int channel, const GpgKeyPtr& key, bool secret, bool ascii,
                   bool shortest, const QString& type);
};

}  // namespace GpgFrontend::UI

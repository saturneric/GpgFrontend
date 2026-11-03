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

namespace GpgFrontend::UI {

/// A tab with unsaved changes, as a close prompt names it.
struct GF_UI_EXPORT UnsavedTab {
  quint64 id = 0;
  QString name;
  QString path;
};

/// The answer for one unsaved tab.
enum class SaveChoice {
  kSAVE,
  kDISCARD,
  kCANCEL,
};

/**
 * @brief Every question closing tabs asks the user.
 *
 * The defaults show the dialogs. A test replaces them, so the close path can
 * be driven end to end without a modal loop.
 */
class GF_UI_EXPORT TabClosePrompter {
 public:
  virtual ~TabClosePrompter() = default;

  /// Save, discard or keep the one unsaved tab of a close.
  virtual auto AskSaveOne(QWidget* parent, const UnsavedTab& tab) -> SaveChoice;

  /**
   * @brief Which of several unsaved tabs to save; the rest are discarded.
   *
   * @return nullopt when the user cancelled the close
   */
  virtual auto AskSaveMany(QWidget* parent, const QList<UnsavedTab>& tabs)
      -> std::optional<QSet<quint64>>;

  /// Where to save a document; empty when the user cancelled.
  virtual auto ChooseSavePath(QWidget* parent, const QString& suggested,
                              const QString& filter) -> QString;

  /// Writing @p path failed.
  virtual void SaveFailed(QWidget* parent, const QString& path,
                          const QString& error);
};

}  // namespace GpgFrontend::UI

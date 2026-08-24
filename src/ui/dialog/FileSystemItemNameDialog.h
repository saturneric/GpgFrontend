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

#include <QDialog>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace GpgFrontend::UI {

/**
 * @brief Asks for the name of a file or folder, and refuses a bad one early.
 *
 * Creating and renaming ask the same question and apply the same rules, so
 * they ask it with the same dialog. The OK button stays disabled until the
 * name is usable and the hint label says what is wrong, rather than letting
 * the user commit and then arguing with them.
 */
class FileSystemItemNameDialog : public QDialog {
  Q_OBJECT

 public:
  enum class ItemType : uint8_t {
    kFILE,
    kFOLDER,
  };

  enum class Mode : uint8_t {
    kCREATE,
    kRENAME,
  };

  /**
   * @brief Name a new item in the given directory.
   *
   * @param item_type what is being created
   * @param target_dir where it will be created
   * @param parent the parent widget
   */
  explicit FileSystemItemNameDialog(ItemType item_type,
                                    const QString& target_dir,
                                    QWidget* parent = nullptr);

  /**
   * @brief Rename an existing item.
   *
   * The directory, the current name and the item type all come from the path,
   * so the caller has nothing left to get wrong.
   *
   * @param path the item to rename
   * @param parent the parent widget
   */
  explicit FileSystemItemNameDialog(const QString& path,
                                    QWidget* parent = nullptr);

  auto GetName() const -> QString;
  auto GetPath() const -> QString;

 private:
  void init_ui();
  void UpdateState();

  /**
   * @brief Whether something other than this item already sits at the target.
   *
   * On a case-insensitive filesystem an item collides with itself the moment
   * its name is only re-cased, which is a rename worth allowing.
   */
  [[nodiscard]] auto target_is_taken() const -> bool;

  Mode mode_;
  ItemType item_type_;
  QString target_dir_;
  QString original_name_;
  QString original_path_;

  QLabel* title_label_ = nullptr;
  QLabel* location_label_ = nullptr;
  QLabel* hint_label_ = nullptr;
  QLineEdit* name_edit_ = nullptr;
  QDialogButtonBox* button_box_ = nullptr;
};

}  // namespace GpgFrontend::UI

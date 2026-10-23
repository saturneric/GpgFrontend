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

/**
 * @brief A path shown on one line, elided in the middle to fit.
 *
 * The middle goes because the two ends are what identify a folder. The whole
 * path stays available: as the tooltip, as the accessible description, and on
 * the clipboard through the context menu or Ctrl+C once the label has focus.
 * It asks for little width, so a narrow window elides it instead of clipping
 * the page.
 */
class ElidedPathLabel : public QLabel {
 public:
  explicit ElidedPathLabel(QWidget* parent = nullptr) : QLabel(parent) {
    setFocusPolicy(Qt::StrongFocus);
    setContextMenuPolicy(Qt::DefaultContextMenu);
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    setMinimumWidth(fontMetrics().averageCharWidth() * 12);
    setTextFormat(Qt::PlainText);
  }

  /**
   * @brief The full path to show.
   */
  void SetPath(const QString& path) {
    path_ = path;
    setToolTip(path_);
    setAccessibleDescription(path_);
    update_elided();
  }

  [[nodiscard]] auto Path() const -> QString { return path_; }

  /**
   * @brief Put the full path on the clipboard.
   */
  void CopyPath() const {
    if (!path_.isEmpty()) QGuiApplication::clipboard()->setText(path_);
  }

 protected:
  void resizeEvent(QResizeEvent* event) override {
    QLabel::resizeEvent(event);
    update_elided();
  }

  void contextMenuEvent(QContextMenuEvent* event) override {
    QMenu menu(this);
    auto* copy = menu.addAction(QObject::tr("Copy"));
    copy->setEnabled(!path_.isEmpty());
    if (menu.exec(event->globalPos()) == copy) CopyPath();
  }

  void keyPressEvent(QKeyEvent* event) override {
    if (event->matches(QKeySequence::Copy)) {
      CopyPath();
      return;
    }
    QLabel::keyPressEvent(event);
  }

 private:
  QString path_;

  void update_elided() {
    setText(fontMetrics().elidedText(path_, Qt::ElideMiddle,
                                     std::max(0, contentsRect().width())));
  }
};

}  // namespace GpgFrontend::UI

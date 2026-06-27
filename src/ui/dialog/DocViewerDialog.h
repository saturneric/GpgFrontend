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

#include "ui/dialog/GeneralDialog.h"

namespace GpgFrontend::UI {

class ZoomGraphicsView;

/**
 * @brief A spacious, modeless dialog that presents a high-resolution render of
 * a generated report and lets the user zoom in/out freely to inspect details.
 */
class DocViewerDialog : public GeneralDialog {
  Q_OBJECT

 public:
  /**
   * @param doc the pre-rendered document pixmap to display.
   * @param source_scale the scale factor at which @p doc was rendered, so that
   *        a zoom factor of 1.0 maps to the document's natural on-screen size.
   * @param doc_id optional reference id shown in the window title.
   * @param parent the parent widget.
   */
  DocViewerDialog(const QPixmap& doc, qreal source_scale, const QString& doc_id,
                  QWidget* parent = nullptr);

 protected:
  void showEvent(QShowEvent* event) override;

 private:
  ZoomGraphicsView* view_{nullptr};
  bool first_show_{true};
};

}  // namespace GpgFrontend::UI

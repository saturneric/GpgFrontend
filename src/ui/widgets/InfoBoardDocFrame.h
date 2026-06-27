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
 * @brief QFrame subclass that paints a faint logo watermark behind its
 * children. Used as the backdrop of the generated report document.
 */
class DocFrame final : public QFrame {
 public:
  using QFrame::QFrame;

  void SetWatermark(const QString& /*text*/, const QColor& /*color*/) {
    watermark_active_ = true;
    if (watermark_pixmap_.isNull()) {
      watermark_pixmap_ =
          QPixmap(QStringLiteral(":/icons/gpgfrontend_logo.png"));
    }
    update();
  }

  void ClearWatermark() {
    watermark_active_ = false;
    update();
  }

 protected:
  void paintEvent(QPaintEvent* ev) override {
    QFrame::paintEvent(ev);
    if (!watermark_active_ || watermark_pixmap_.isNull()) return;

    constexpr int kWatermarkSize = 320;
    const QPixmap scaled =
        watermark_pixmap_.scaled(kWatermarkSize, kWatermarkSize,
                                 Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QPainter p(this);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.setOpacity(0.06);
    p.drawPixmap((width() - scaled.width()) / 2,
                 (height() - scaled.height()) / 2, scaled);
  }

 private:
  bool watermark_active_ = false;
  QPixmap watermark_pixmap_;
};

}  // namespace GpgFrontend::UI

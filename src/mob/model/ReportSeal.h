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

#include <QtQml/qqmlregistration.h>

#include <QColor>
#include <QQuickPaintedItem>

namespace GpgFrontend::Mobile {

/**
 * @brief The report's seal, as the desktop's Status Panel stamps it
 * (ui/widgets/InfoBoardDocFrame.h, ReportSeal): a postmark tilted a little,
 * its rings in a muted shade of the outcome's colour, "GPGFRONTEND" along the
 * top of the band and the outcome along the bottom, the outcome's symbol in
 * the middle, and cancellation waves running in from the left.
 *
 * Painted by the core (PaintReportSeal), as the desktop paints it; the colours
 * come from the page's theme rather than a widget palette. It is GpgFrontend's
 * own branding, not a signature.
 */
class ReportSeal : public QQuickPaintedItem {
  Q_OBJECT
  QML_ELEMENT

  /// "success", "warning", "cancelled", "failure" or "information".
  Q_PROPERTY(QString kind MEMBER kind_ NOTIFY SignalChanged)
  /// The outcome's colour.
  Q_PROPERTY(QColor accent MEMBER accent_ NOTIFY SignalChanged)
  /// The page's text colour and its quiet one.
  Q_PROPERTY(QColor text MEMBER text_ NOTIFY SignalChanged)
  Q_PROPERTY(QColor muted MEMBER muted_ NOTIFY SignalChanged)
  Q_PROPERTY(bool dark MEMBER dark_ NOTIFY SignalChanged)
  /// The outcome, written along the bottom of the band.
  Q_PROPERTY(QString inscription MEMBER inscription_ NOTIFY SignalChanged)

 public:
  explicit ReportSeal(QQuickItem* parent = nullptr);

  void paint(QPainter* painter) override;

 signals:
  void SignalChanged();

 private:
  QString kind_ = QStringLiteral("information");
  QColor accent_;
  QColor text_ = Qt::black;
  QColor muted_ = Qt::gray;
  bool dark_ = false;
  QString inscription_;
};

}  // namespace GpgFrontend::Mobile

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

#include <QColor>
#include <QFont>
#include <QRectF>

#include "GFShellExport.h"

class QPainter;

namespace GpgFrontend {

/// What a report's seal says happened.
enum class ReportSealKind : std::uint8_t {
  kSUCCESS,
  kWARNING,
  kCANCELLED,
  kFAILURE,
  kINFORMATION,
};

enum class InfoBoardHeadline : std::uint8_t;

/**
 * @brief The seal a report with @p headline is stamped with, on both
 * interfaces: a success only for one that succeeded, a failure for one that
 * failed even in part, the information mark for a report of facts, and the
 * warning mark for everything between.
 */
auto GF_SHELL_EXPORT ReportSealKindFor(InfoBoardHeadline headline)
    -> ReportSealKind;

/// @p kind as the phone's interface names it ("success", "warning",
/// "cancelled", "failure", "information").
auto GF_SHELL_EXPORT ReportSealId(ReportSealKind kind) -> QString;

/// The kind @p id names; the warning mark for one it does not know.
auto GF_SHELL_EXPORT ReportSealKindFromId(const QString& id) -> ReportSealKind;

/// The colours and lettering a seal is painted in: the page's, not its own.
struct ReportSealStyle {
  QColor accent;  ///< the outcome's colour
  QColor text;    ///< the page's text colour
  QColor muted;   ///< the page's quiet text colour
  bool dark = false;
  QFont font;  ///< the face of the lettering; its size is the seal's
};

/**
 * @brief @p a and @p b mixed @p wa to @p wb.
 */
auto GF_SHELL_EXPORT BlendReportColor(const QColor& a, const QColor& b, int wa,
                                      int wb) -> QColor;

/**
 * @brief Paint a report's seal, as the desktop's Status Panel and the phone's
 * report both stamp it: a postmark tilted a little, its rings in a muted shade
 * of the outcome's colour, "GPGFRONTEND" along the top of the band and
 * @p inscription along the bottom, the outcome's symbol in the middle, and
 * cancellation waves running in from the left.
 *
 * It is GpgFrontend's own branding, not a signature.
 *
 * @param p the painter, untransformed for the seal
 * @param bounds the seal is its right-hand square; the waves fill the rest
 * @param detail with lettering and ornament: they need real pixels
 */
void GF_SHELL_EXPORT PaintReportSeal(QPainter& p, const QRectF& bounds,
                                     ReportSealKind kind,
                                     const QString& inscription,
                                     const ReportSealStyle& style, bool detail);

}  // namespace GpgFrontend

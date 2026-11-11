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

#include "model/ErrorWording.h"

#include <QCoreApplication>

#include "core/utils/GpgUtils.h"

namespace GpgFrontend::Mobile {

auto DescribeActionError(const GFError& err) -> QVariantMap {
  // An engine that reports success but hands back nothing has still failed:
  // describing its success code would read as "Success".
  const auto failed =
      err.Ok() ? GFError::Application(GFErrorCode::kGENERAL) : err;
  const auto said = [](const char* reason, const QString& text) {
    return QVariantMap{{QStringLiteral("error"), text},
                       {QStringLiteral("reason"), QString::fromLatin1(reason)}};
  };

  if (failed.Canceled()) {
    return said("cancelled",
                QCoreApplication::translate("GpgFrontend::Mobile::ErrorWording",
                                            "Cancelled."));
  }
  switch (failed.code) {
    case GFErrorCode::kBAD_PASSPHRASE:
      return said("bad_passphrase",
                  QCoreApplication::translate(
                      "GpgFrontend::Mobile::ErrorWording",
                      "The passphrase was wrong. Try again with the "
                      "right one."));
    case GFErrorCode::kNO_PASSPHRASE:
      return said("no_passphrase",
                  QCoreApplication::translate(
                      "GpgFrontend::Mobile::ErrorWording",
                      "No passphrase was given, so the key could not be "
                      "used."));
    case GFErrorCode::kINVALID_VALUE:
    case GFErrorCode::kINVALID_ARGUMENT:
      return said("invalid", QCoreApplication::translate(
                                 "GpgFrontend::Mobile::ErrorWording",
                                 "This cannot be used. Check what you "
                                 "entered and try again."));
    default:
      return said("failed", DescribeGFError(failed).second);
  }
}

}  // namespace GpgFrontend::Mobile

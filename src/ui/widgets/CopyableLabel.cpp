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

#include "ui/widgets/CopyableLabel.h"

#include "core/utils/CommonUtils.h"

namespace GpgFrontend::UI {

namespace {

/// A v4 fingerprint (40 hex digits) is eight groups and fits on one line.
constexpr qsizetype kSingleLineGroups = 8;
constexpr int kCopiedNoticeMsecs = 1500;

}  // namespace

auto FingerprintDisplayText(const QString& fingerprint) -> QString {
  const auto groups =
      BeautifyFingerprint(fingerprint).split(' ', Qt::SkipEmptyParts);
  if (groups.size() <= kSingleLineGroups) return groups.join(' ');

  const auto first_line = (groups.size() + 1) / 2;
  return groups.mid(0, first_line).join(' ') + '\n' +
         groups.mid(first_line).join(' ');
}

CopyableLabel::CopyableLabel(QWidget* parent) : QLabel(parent) {
  setTextFormat(Qt::PlainText);
  setCursor(Qt::PointingHandCursor);
  setToolTip(tr("Click to copy"));
}

void CopyableLabel::SetValue(const QString& text, const QString& copy_text) {
  setText(text);
  copy_text_ = copy_text.isEmpty() ? text : copy_text;
}

void CopyableLabel::mouseReleaseEvent(QMouseEvent* event) {
  if (event->button() != Qt::LeftButton ||
      !rect().contains(event->position().toPoint()) || copy_text_.isEmpty()) {
    QLabel::mouseReleaseEvent(event);
    return;
  }

  // Accepted, or the release propagates to the parent and the tooltip's own
  // application-wide filter sees it there and hides the notice at once.
  event->accept();
  QApplication::clipboard()->setText(copy_text_);
  // Anchored at the label's own top: the style drops a tip a cursor's height
  // below its anchor, so it lands over the value rather than over whatever
  // sits under the label (the key details' expiry notice, say).
  QToolTip::showText(mapToGlobal(QPoint(0, 0)),
                     tr("Copied to clipboard"), this, rect(),
                     kCopiedNoticeMsecs);
  emit SignalCopied(copy_text_);
}

}  // namespace GpgFrontend::UI

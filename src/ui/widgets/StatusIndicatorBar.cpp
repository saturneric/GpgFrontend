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

#include "ui/widgets/StatusIndicatorBar.h"

#include "ui/UserInterfaceUtils.h"

namespace GpgFrontend::UI {

namespace {

/// space between two readings; they are told apart by this and by the
/// dim-caption contrast, never by a line we draw ourselves
constexpr int kSegmentSpacing = 14;

}  // namespace

StatusIndicatorSegment::StatusIndicatorSegment(QWidget* parent)
    : QLabel(parent) {
  setCursor(Qt::PointingHandCursor);
  setFocusPolicy(Qt::NoFocus);
  hide();
}

void StatusIndicatorSegment::SetInfo(const StatusIndicatorInfo& info) {
  info_ = info;
  setToolTip(info_.tooltip);
  render_info();
  setVisible(!info_.value.isEmpty());
}

void StatusIndicatorSegment::render_info() {
  if (info_.value.isEmpty()) {
    clear();
    return;
  }

  // An inline span rather than a stylesheet, so the label keeps the platform
  // font and the colour can follow the palette.
  const auto dim = MutedTextColor(palette());

  QString text;
  if (!info_.caption.isEmpty()) {
    text += QString(R"(<span style="color:%1;">%2</span>&nbsp;)")
                .arg(dim.name(QColor::HexRgb), info_.caption.toHtmlEscaped());
  }

  text += QString(R"(<span style="%1">%2</span>)")
              .arg(hovered_ ? "text-decoration:underline;" : "",
                   info_.value.toHtmlEscaped());

  setText(text);
}

void StatusIndicatorSegment::changeEvent(QEvent* e) {
  QLabel::changeEvent(e);

  // The dim caption is mixed from the palette, so a theme change has to be
  // painted rather than waited out.
  if (e->type() == QEvent::PaletteChange) render_info();
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void StatusIndicatorSegment::enterEvent(QEnterEvent* e) {
#else
void StatusIndicatorSegment::enterEvent(QEvent* e) {
#endif
  QLabel::enterEvent(e);
  hovered_ = true;
  render_info();
}

void StatusIndicatorSegment::leaveEvent(QEvent* e) {
  QLabel::leaveEvent(e);
  hovered_ = false;
  render_info();
}

void StatusIndicatorSegment::mouseReleaseEvent(QMouseEvent* e) {
  QLabel::mouseReleaseEvent(e);

  // Released outside means the click was taken back, which is what every other
  // button on screen does.
  if (e->button() == Qt::LeftButton && rect().contains(e->pos())) {
    emit SignalClicked();
  }
}

StatusIndicatorBar::StatusIndicatorBar(QWidget* parent)
    : QWidget(parent),
      profile_segment_(new StatusIndicatorSegment(this)),
      engine_segment_(new StatusIndicatorSegment(this)),
      deployment_segment_(new StatusIndicatorSegment(this)) {
  auto* layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(kSegmentSpacing);
  layout->addWidget(profile_segment_);
  layout->addWidget(engine_segment_);
  layout->addWidget(deployment_segment_);

  connect(profile_segment_, &StatusIndicatorSegment::SignalClicked, this,
          &StatusIndicatorBar::SignalProfileClicked);
  connect(engine_segment_, &StatusIndicatorSegment::SignalClicked, this,
          &StatusIndicatorBar::SignalEngineClicked);
  connect(deployment_segment_, &StatusIndicatorSegment::SignalClicked, this,
          &StatusIndicatorBar::SignalDeploymentClicked);
}

void StatusIndicatorBar::SetProfile(const StatusIndicatorInfo& info) {
  profile_segment_->SetInfo(info);
}

void StatusIndicatorBar::SetEngine(const StatusIndicatorInfo& info) {
  engine_segment_->SetInfo(info);
}

void StatusIndicatorBar::SetDeployment(const StatusIndicatorInfo& info) {
  deployment_segment_->SetInfo(info);
}

}  // namespace GpgFrontend::UI

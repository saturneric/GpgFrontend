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

#include "ui/widgets/ToggleSwitch.h"

#include "ui/function/UIStyle.h"

namespace GpgFrontend::UI {

namespace {

constexpr int kTrackWidth = 34;
constexpr int kTrackHeight = 18;
constexpr int kGap = 10;

}  // namespace

ToggleSwitch::ToggleSwitch(const QString& text, QWidget* parent)
    : QAbstractButton(parent), slide_(new QVariantAnimation(this)) {
  setText(text);
  setCheckable(true);
  setCursor(Qt::PointingHandCursor);
  setFocusPolicy(Qt::StrongFocus);
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

  slide_->setDuration(120);
  slide_->setEasingCurve(QEasingCurve::OutCubic);
  connect(slide_, &QVariantAnimation::valueChanged, this,
          [this](const QVariant& v) {
            position_ = v.toReal();
            update();
          });
  connect(this, &QAbstractButton::toggled, this, [this](bool on) {
    slide_->stop();
    // Set in code before the window is shown, the switch is simply there.
    if (!isVisible()) {
      position_ = on ? 1 : 0;
      update();
      return;
    }
    slide_->setStartValue(position_);
    slide_->setEndValue(on ? 1.0 : 0.0);
    slide_->start();
  });
}

auto ToggleSwitch::sizeHint() const -> QSize {
  const auto text_width = fontMetrics().horizontalAdvance(text());
  return {text_width + kGap + kTrackWidth + 4,
          std::max(kTrackHeight + 4, fontMetrics().height() + 4)};
}

auto ToggleSwitch::track_rect() const -> QRectF {
  return {static_cast<qreal>(width() - kTrackWidth - 2),
          (height() - kTrackHeight) / 2.0, kTrackWidth, kTrackHeight};
}

void ToggleSwitch::paintEvent(QPaintEvent*) {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);

  const auto pal = palette();
  const auto enabled = isEnabled();

  // The label, left; muted when the switch cannot be used.
  p.setPen(enabled ? pal.color(QPalette::WindowText) : MutedTextColor(pal));
  p.drawText(QRect(0, 0, width() - kTrackWidth - kGap, height()),
             Qt::AlignLeft | Qt::AlignVCenter, text());

  // The track: the accent colour when on, a hairline grey when off, blended
  // as the knob travels so the change reads as one motion.
  const auto track = track_rect();
  const auto on_color = pal.color(QPalette::Highlight);
  const auto off_color = BorderColor(pal);
  const auto mix = [this](const QColor& a, const QColor& b) {
    return QColor::fromRgbF(
        static_cast<float>(a.redF() + ((b.redF() - a.redF()) * position_)),
        static_cast<float>(a.greenF() + ((b.greenF() - a.greenF()) * position_)),
        static_cast<float>(a.blueF() + ((b.blueF() - a.blueF()) * position_)));
  };
  auto track_color = mix(off_color, on_color);
  if (!enabled) track_color.setAlphaF(0.4F);
  if (hovered_ && enabled) track_color = track_color.lighter(108);

  p.setPen(Qt::NoPen);
  p.setBrush(track_color);
  p.drawRoundedRect(track, kTrackHeight / 2.0, kTrackHeight / 2.0);

  // The knob.
  constexpr qreal kInset = 2;
  const auto knob = kTrackHeight - (2 * kInset);
  const auto x = track.left() + kInset +
                 ((track.width() - knob - (2 * kInset)) * position_);
  p.setBrush(enabled ? QColor(Qt::white) : pal.color(QPalette::Button));
  p.drawEllipse(QRectF(x, track.top() + kInset, knob, knob));

  if (hasFocus()) {
    auto focus = on_color;
    focus.setAlpha(110);
    p.setPen(QPen(focus, 1.5));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(track.adjusted(-2, -2, 2, 2), (kTrackHeight / 2.0) + 2,
                      (kTrackHeight / 2.0) + 2);
  }
}

void ToggleSwitch::enterEvent(QEnterEvent* event) {
  hovered_ = true;
  update();
  QAbstractButton::enterEvent(event);
}

void ToggleSwitch::leaveEvent(QEvent* event) {
  hovered_ = false;
  update();
  QAbstractButton::leaveEvent(event);
}

}  // namespace GpgFrontend::UI

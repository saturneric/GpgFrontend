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
#include "ui/widgets/EditorTabBar.h"

namespace GpgFrontend::UI {

namespace {

constexpr int kMinTabWidth = 96;
constexpr int kMaxTabWidth = 240;
constexpr int kCloseButtonSize = 16;
/// One wheel notch; a tab per notch.
constexpr int kWheelStep = 120;
/// A trackpad reports pixels; this many make a notch, so a slow swipe still
/// moves the strip, a tab at a time.
constexpr int kWheelUnitsPerPixel = 4;

/**
 * @brief A tab's close button that doubles as its unsaved-changes mark.
 *
 * Always the same size, so the title beside it never shifts: a dot while the
 * document is modified, a cross when the pointer is on it (or nothing is
 * unsaved).
 */
class TabCloseButton : public QAbstractButton {
 public:
  explicit TabCloseButton(QWidget* parent) : QAbstractButton(parent) {
    setFocusPolicy(Qt::NoFocus);
    setCursor(Qt::ArrowCursor);
    setFixedSize(kCloseButtonSize, kCloseButtonSize);
    setAttribute(Qt::WA_Hover);
  }

  void SetModified(bool modified) {
    if (modified_ == modified) return;
    modified_ = modified;
    update();
  }

  [[nodiscard]] auto Modified() const -> bool { return modified_; }

  [[nodiscard]] auto sizeHint() const -> QSize override {
    return {kCloseButtonSize, kCloseButtonSize};
  }

 protected:
  void paintEvent(QPaintEvent* /*event*/) override {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const auto r = QRectF(rect()).adjusted(1, 1, -1, -1);
    const bool hovered = underMouse();

    if (hovered) {
      auto bg = palette().color(QPalette::WindowText);
      bg.setAlphaF(isDown() ? 0.25F : 0.14F);
      p.setPen(Qt::NoPen);
      p.setBrush(bg);
      p.drawRoundedRect(r, 3, 3);
    }

    auto fg = palette().color(QPalette::WindowText);
    if (modified_ && !hovered) {
      p.setPen(Qt::NoPen);
      p.setBrush(fg);
      constexpr qreal kDot = 4.0;
      p.drawEllipse(r.center(), kDot, kDot);
      return;
    }

    if (!hovered) fg.setAlphaF(0.55F);
    p.setPen(QPen(fg, 1.4, Qt::SolidLine, Qt::RoundCap));
    const auto c = r.center();
    constexpr qreal kArm = 3.5;
    p.drawLine(QPointF(c.x() - kArm, c.y() - kArm),
               QPointF(c.x() + kArm, c.y() + kArm));
    p.drawLine(QPointF(c.x() + kArm, c.y() - kArm),
               QPointF(c.x() - kArm, c.y() + kArm));
  }

  void enterEvent(QEnterEvent* event) override {
    update();
    QAbstractButton::enterEvent(event);
  }

  void leaveEvent(QEvent* event) override {
    update();
    QAbstractButton::leaveEvent(event);
  }

 private:
  bool modified_ = false;
};

}  // namespace

EditorTabBar::EditorTabBar(QWidget* parent) : QTabBar(parent) {
  setMovable(true);
  setTabsClosable(false);
  setUsesScrollButtons(true);
  setElideMode(Qt::ElideRight);
  setExpanding(false);
  setDrawBase(false);
  setSelectionBehaviorOnRemove(QTabBar::SelectPreviousTab);
}

auto EditorTabBar::close_button_side() const -> QTabBar::ButtonPosition {
  return static_cast<QTabBar::ButtonPosition>(
      style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition, nullptr, this));
}

void EditorTabBar::tabInserted(int index) {
  QTabBar::tabInserted(index);

  auto* button = new TabCloseButton(this);
  button->setToolTip(tr("Close"));
  connect(button, &QAbstractButton::clicked, this, [this, button]() {
    const auto side = close_button_side();
    for (int i = 0; i < count(); ++i) {
      if (tabButton(i, side) == button) {
        emit tabCloseRequested(i);
        return;
      }
    }
  });
  setTabButton(index, close_button_side(), button);
}

void EditorTabBar::SetTabModified(int index, bool modified) {
  if (auto* b = dynamic_cast<TabCloseButton*>(
          tabButton(index, close_button_side()))) {
    b->SetModified(modified);
  }
}

auto EditorTabBar::TabModified(int index) const -> bool {
  const auto* b =
      dynamic_cast<TabCloseButton*>(tabButton(index, close_button_side()));
  return b != nullptr && b->Modified();
}

auto EditorTabBar::tabSizeHint(int index) const -> QSize {
  auto size = QTabBar::tabSizeHint(index);
  size.setWidth(std::clamp(size.width(), kMinTabWidth, kMaxTabWidth));
  return size;
}

auto EditorTabBar::minimumTabSizeHint(int index) const -> QSize {
  // Never squeezed below the minimum: the strip scrolls instead.
  return tabSizeHint(index);
}

auto EditorTabBar::scroll_button(bool back) const -> QAbstractButton* {
  // The bar's own scroll arrows, told apart by the arrow they show (public
  // API) rather than by Qt's private object names. In a right-to-left layout
  // the first tab is on the right.
  const bool rtl = layoutDirection() == Qt::RightToLeft;
  const auto wanted = back != rtl ? Qt::LeftArrow : Qt::RightArrow;
  for (auto* b : findChildren<QToolButton*>(Qt::FindDirectChildrenOnly)) {
    if (b->arrowType() == wanted && b->isVisible() && b->isEnabled()) return b;
  }
  return nullptr;
}

void EditorTabBar::ScrollBy(int steps) {
  for (int i = 0; i < std::abs(steps); ++i) {
    auto* b = scroll_button(steps > 0);
    if (b == nullptr) return;
    b->click();
  }
}

void EditorTabBar::wheelEvent(QWheelEvent* event) {
  // Whichever axis moved more; a trackpad's pixels are scaled to notches.
  const auto pixels = event->pixelDelta();
  const auto angle = event->angleDelta();
  int delta = 0;
  if (!pixels.isNull()) {
    delta = (std::abs(pixels.x()) > std::abs(pixels.y()) ? pixels.x()
                                                         : pixels.y()) *
            kWheelUnitsPerPixel;
  } else {
    delta = std::abs(angle.x()) > std::abs(angle.y()) ? angle.x() : angle.y();
  }
  event->accept();
  if (delta == 0) return;

  // A change of direction starts over, so a reversal answers at once.
  if ((delta > 0) != (wheel_rest_ > 0) && wheel_rest_ != 0) wheel_rest_ = 0;
  wheel_rest_ += delta;

  const int steps = wheel_rest_ / kWheelStep;
  if (steps == 0) return;
  wheel_rest_ -= steps * kWheelStep;
  ScrollBy(steps);
}

void EditorTabBar::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::MiddleButton) {
    middle_pressed_ = tabAt(event->position().toPoint());
    event->accept();
    return;
  }
  QTabBar::mousePressEvent(event);
}

void EditorTabBar::mouseReleaseEvent(QMouseEvent* event) {
  if (event->button() == Qt::MiddleButton) {
    // Pressed and released on the same tab: a click, not a drag off it.
    const int index = tabAt(event->position().toPoint());
    if (index >= 0 && index == middle_pressed_) emit tabCloseRequested(index);
    middle_pressed_ = -1;
    event->accept();
    return;
  }
  QTabBar::mouseReleaseEvent(event);
}

void EditorTabBar::mouseDoubleClickEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton &&
      tabAt(event->position().toPoint()) < 0) {
    emit SignalNewTabRequested();
    event->accept();
    return;
  }
  QTabBar::mouseDoubleClickEvent(event);
}

}  // namespace GpgFrontend::UI

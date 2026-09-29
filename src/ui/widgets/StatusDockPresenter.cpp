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

#include "ui/widgets/StatusDockPresenter.h"

#include "core/model/SettingsObject.h"
#include "ui/widgets/InfoBoardWidget.h"

namespace GpgFrontend::UI {

namespace {

constexpr int kDotSize = 10;
constexpr int kChevronSize = 16;

/// A chevron in the palette's button text colour, so it follows the theme
/// instead of a style's coloured arrow.
auto ChevronIcon(const QPalette& palette, bool up) -> QIcon {
  const qreal dpr = qApp != nullptr ? qApp->devicePixelRatio() : 1.0;
  QPixmap pixmap(QSize(kChevronSize, kChevronSize) * dpr);
  pixmap.setDevicePixelRatio(dpr);
  pixmap.fill(Qt::transparent);

  QPainter painter(&pixmap);
  painter.setRenderHint(QPainter::Antialiasing);
  QPen pen(palette.color(QPalette::ButtonText), 1.6);
  pen.setCapStyle(Qt::RoundCap);
  pen.setJoinStyle(Qt::RoundJoin);
  painter.setPen(pen);

  const qreal mid = kChevronSize / 2.0;
  const qreal half = 4.0;
  const qreal rise = up ? -2.0 : 2.0;
  const std::array<QPointF, 3> points = {QPointF(mid - half, mid - rise),
                                         QPointF(mid, mid + rise),
                                         QPointF(mid + half, mid - rise)};
  painter.drawPolyline(points.data(), static_cast<int>(points.size()));
  painter.end();
  return QIcon(pixmap);
}

}  // namespace

StatusDockPresenter::ExpandedForSaveGuard::ExpandedForSaveGuard(
    StatusDockPresenter* presenter)
    : presenter_(presenter) {
  if (presenter_ == nullptr) return;
  // A half-finished transition would save a height that is neither state.
  // Only a RUNNING one, though: finishing a panel at rest re-applies its
  // remembered height, which would undo whatever the user has dragged it to
  // since -- and save that instead.
  if (presenter_->IsTransitioning()) presenter_->finish_transition();
  presenter_->remember_expanded_height();
  // The content stays hidden: only the geometry that will be saved changes.
  if (presenter_->presentation_ == StatusDockPresentation::kCollapsed) {
    presenter_->apply_expanded_constraints();
  }
}

StatusDockPresenter::ExpandedForSaveGuard::~ExpandedForSaveGuard() {
  if (presenter_ != nullptr &&
      presenter_->presentation_ == StatusDockPresentation::kCollapsed) {
    presenter_->apply_collapsed_constraints();
  }
}

StatusDockPresenter::StatusDockPresenter(QMainWindow* window, QDockWidget* dock,
                                         InfoBoardWidget* board,
                                         QString settings_name)
    : QObject(window),
      window_(window),
      dock_(dock),
      board_(board),
      toggle_(new QAction(this)),
      // Not dock->height(): before the window is shown that is no height the
      // user ever saw, and remembering it would replace the restored one.
      expanded_min_height_(dock->minimumHeight()),
      settings_name_(std::move(settings_name)) {
  // What the last run saved the panel at. The restored layout carries it
  // too, but a tall tab in front at startup squeezes the dock before any of
  // this sees it, and that squeezed height must not become the panel's.
  if (!settings_name_.isEmpty()) {
    SettingsObject so(settings_name_);
    if (const auto v = so["expanded_height"];
        v.isDouble() && is_chosen_height(v.toInt())) {
      expanded_height_ = v.toInt();
      expanded_geometry_given_ = true;
    }
  }

  connect(toggle_, &QAction::triggered, this, [this]() {
    if (presentation_ == StatusDockPresentation::kCollapsed) {
      Expand();
    } else {
      Collapse();
    }
  });

  // Only the posting event: a result that is merely still on the board never
  // reopens a panel the user collapsed after reading it.
  connect(board, &InfoBoardWidget::SignalResultPosted, this,
          [this](InfoBoardStatus) { Expand(); });

  // The height a transition pins the dock at. Owned here, so it goes with
  // the presenter; the destructor stops it before anything it touches does.
  transition_ = new QVariantAnimation(this);
  transition_->setEasingCurve(QEasingCurve::OutCubic);
  connect(transition_, &QVariantAnimation::valueChanged, this,
          [this](const QVariant& value) {
            const int height = value.toInt();
            pin_height(height);
            // On the way up the bar stays until there is room for the board,
            // so its content is never shown squeezed into a sliver.
            if (presentation_ == StatusDockPresentation::kExpanded &&
                height >= reveal_at_) {
              show_full();
            }
          });
  connect(transition_, &QVariantAnimation::finished, this,
          [this]() { finish_transition(); });

  // The first show is when the restored layout becomes real geometry.
  window->installEventFilter(this);

  build_compact_bar();
  empty_content_ = new QWidget(dock_);
  empty_content_->setObjectName(QStringLiteral("statusDockCollapsedContent"));
  empty_content_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  empty_content_->setFixedHeight(0);
  empty_content_->hide();
  // The bar mirrors the board's indicator whenever it changes, results and
  // resets alike, so a collapsed panel never shows a stale colour.
  connect(board, &InfoBoardWidget::SignalStatusStyleChanged, this,
          [this](InfoBoardStatus) { refresh_compact_bar(); });

  update_toggle_action();
}

StatusDockPresenter::~StatusDockPresenter() {
  // Before the members it writes to: a frame delivered to a presenter that is
  // being torn down would touch a dock that may already be gone.
  transition_->stop();
}

void StatusDockPresenter::Expand() {
  transition_to(StatusDockPresentation::kExpanded, true);
}

void StatusDockPresenter::Collapse() {
  transition_to(StatusDockPresentation::kCollapsed, true);
}

auto StatusDockPresenter::IsTransitioning() const -> bool {
  return transition_->state() == QAbstractAnimation::Running;
}

void StatusDockPresenter::SetTransitionDuration(int ms) {
  duration_override_ = ms;
}

auto StatusDockPresenter::transition_ms() const -> int {
  if (duration_override_ >= 0) return duration_override_;
  // The application's own motion switch; with effects off, changes are
  // instant rather than merely quick.
  return QApplication::isEffectEnabled(Qt::UI_General) ? kTransitionMs : 0;
}

void StatusDockPresenter::transition_to(StatusDockPresentation target,
                                        bool animate) {
  if (presentation_ == target || dock_ == nullptr) return;

  // Before the window is on screen -- while MainWindow restores the saved
  // layout and then the tabs -- the dock has no real height yet, and pinning
  // one would overwrite the size restoreState() recorded: the panel would
  // come back at its minimum however tall it was left. So only the target is
  // recorded now; the rest waits for the window to be shown.
  if (window_ != nullptr && !window_->isVisible()) {
    transition_->stop();
    presentation_ = target;
    // Not even the content: the zero-height placeholder of the bar caps the
    // dock's maximum as soon as its layout runs, and QMainWindow then clamps
    // the restored size to it before the window is ever seen.
    geometry_deferred_ = true;
    update_toggle_action();
    emit SignalPresentationChanged(presentation_);
    return;
  }

  // The expanded height is only ever taken from a panel at rest: a Collapse
  // arriving halfway up would otherwise remember the halfway height.
  // Nor from a panel sitting at its minimum: that is where Qt pushes the dock
  // when the tab in front needs the room, not a height anyone chose.
  if (target == StatusDockPresentation::kCollapsed && !IsTransitioning()) {
    expanded_min_height_ = dock_->minimumHeight();
    if (is_chosen_height(dock_->height())) expanded_height_ = dock_->height();
  }
  const int from = dock_->height();

  presentation_ = target;
  update_toggle_action();
  emit SignalPresentationChanged(presentation_);

  // Nothing to watch move: straight to the final layout.
  const int ms = transition_ms();
  if (!animate || ms <= 0 || !dock_->isVisible() || dock_->isFloating()) {
    transition_->stop();
    if (target == StatusDockPresentation::kCollapsed) {
      show_compact();
      measure_bar();
    }
    finish_transition();
    return;
  }

  int to = 0;
  if (target == StatusDockPresentation::kCollapsed) {
    // The content goes first: shrinking the full board would show it
    // squeezed on every frame.
    show_compact();
    measure_bar();
    to = bar_height_;
  } else {
    to = expanded_target();
    reveal_at_ = qMax(from + ((to - from) / 2), expanded_min_height_);
  }

  // Redirected from wherever the last one had got to, never finished first:
  // a reversal runs from the current height towards the new target.
  transition_->stop();
  pin_height(from);
  transition_->setDuration(ms);
  transition_->setStartValue(from);
  transition_->setEndValue(to);
  transition_->start();
}

void StatusDockPresenter::finish_transition() {
  transition_->stop();
  if (dock_ == nullptr) return;
  if (presentation_ == StatusDockPresentation::kExpanded) {
    show_full();
    apply_expanded_constraints();
  } else {
    // Measured once, when the collapse began: measuring again here lets the
    // bar land a pixel off from where the transition was heading.
    show_compact();
    apply_collapsed_constraints();
  }
}

void StatusDockPresenter::show_compact() {
  if (dock_ == nullptr) return;
  // The bar replaces the title bar, and the board makes way for an empty,
  // full-width placeholder: one row, not a title bar over a strip. The board
  // keeps its result, hidden. (A dock whose content is merely hidden takes
  // its maximum size from the title bar alone, and shrinks to its width.)
  if (board_ != nullptr && dock_->widget() == board_) {
    dock_->setWidget(empty_content_);
    board_->setParent(dock_);
    board_->hide();
  }
  empty_content_->show();
  refresh_compact_bar();
  compact_bar_->show();
  if (dock_->titleBarWidget() != compact_bar_) {
    dock_->setTitleBarWidget(compact_bar_);
  }
}

void StatusDockPresenter::show_full() {
  if (dock_ == nullptr) return;
  // Native title bar back, the board back as the content; the bar and the
  // placeholder only hide.
  if (dock_->titleBarWidget() != nullptr) dock_->setTitleBarWidget(nullptr);
  compact_bar_->hide();
  if (board_ != nullptr && dock_->widget() != board_) {
    dock_->setWidget(board_);
    board_->show();
  }
  empty_content_->hide();
}

auto StatusDockPresenter::is_chosen_height(int height) const -> bool {
  const int floor = qMax(expanded_min_height_, 1);
  return height > floor + kMinimumSlack;
}

auto StatusDockPresenter::expanded_target() const -> int {
  if (is_chosen_height(expanded_height_)) return expanded_height_;
  // Nothing real is known: the default layout's share of the window.
  const int window_height = window_ != nullptr ? window_->height() : 0;
  return qMax(expanded_min_height_, window_height * kDefaultSharePercent / 100);
}

void StatusDockPresenter::heal_on_first_show() {
  if (healed_ || dock_ == nullptr || window_ == nullptr) return;
  healed_ = true;
  if (presentation_ != StatusDockPresentation::kExpanded || dock_->isHidden() ||
      dock_->isFloating()) {
    return;
  }
  // A panel that comes back at its very minimum was pushed there by an
  // earlier run (a tall tab in front when it saved), and would otherwise be
  // restored there on every start and saved there on every exit, for good.
  if (!is_chosen_height(dock_->height())) {
    window_->resizeDocks({dock_.data()}, {expanded_target()}, Qt::Vertical);
  }
}

void StatusDockPresenter::remember_expanded_height() {
  if (dock_ == nullptr) return;
  // What the user chose: the live height while expanded, unless it is only
  // the minimum the tab in front left it -- then the last real one.
  int height = presentation_ == StatusDockPresentation::kExpanded
                   ? dock_->height()
                   : expanded_height_;
  if (!is_chosen_height(height)) height = expanded_height_;
  if (!is_chosen_height(height)) return;
  // Kept here as well: a tall tab can push the dock to its minimum before
  // the next collapse, and this is then the height to come back to.
  expanded_height_ = height;
  if (settings_name_.isEmpty()) return;
  SettingsObject so(settings_name_);
  so["expanded_height"] = height;
  so.Store(so);
}

void StatusDockPresenter::apply_deferred_geometry() {
  if (!geometry_deferred_ || dock_ == nullptr) return;
  geometry_deferred_ = false;
  // An expanded panel needs nothing: the restored layout already is its
  // height. A collapsed one remembers that height now that it is real, then
  // becomes the bar.
  if (presentation_ != StatusDockPresentation::kCollapsed) {
    show_full();
    // Still pinned to the bar from a collapse made while it was on screen:
    // that one knows its real expanded height, so go back to it.
    if (dock_->minimumHeight() == dock_->maximumHeight()) {
      apply_expanded_constraints();
    }
    return;
  }
  if (!expanded_geometry_given_ && is_chosen_height(dock_->height())) {
    expanded_height_ = dock_->height();
    expanded_min_height_ = dock_->minimumHeight();
  }
  show_compact();
  measure_bar();
  apply_collapsed_constraints();
}

void StatusDockPresenter::pin_height(int height) {
  if (dock_ == nullptr) return;
  // A constraint, not a geometry: QMainWindow owns the dock's geometry and
  // follows its limits, one relayout per frame.
  dock_->setMinimumHeight(height);
  dock_->setMaximumHeight(height);
}

void StatusDockPresenter::SetCompactContext(bool compact) {
  if (compact == before_compact_.has_value()) return;

  if (compact) {
    before_compact_ = presentation_;
    // A panel the user closed stays closed; collapsing it would only change
    // what the View menu brings back.
    // Switching tabs is not something to watch move: instant.
    if (dock_ != nullptr && !dock_->isHidden()) {
      transition_to(StatusDockPresentation::kCollapsed, false);
    }
    return;
  }

  const auto restore = *before_compact_;
  before_compact_.reset();
  transition_to(restore, false);
}

void StatusDockPresenter::SetExpandedGeometry(int min_height, int height) {
  expanded_min_height_ = min_height;
  expanded_height_ = height;
  expanded_geometry_given_ = true;
  if (presentation_ != StatusDockPresentation::kExpanded) return;
  if (IsTransitioning()) {
    // On its way up already: aim at the new height instead.
    transition_->setEndValue(qMax(height, min_height));
    return;
  }
  apply_expanded_constraints();
}

void StatusDockPresenter::apply_expanded_constraints() {
  if (dock_ == nullptr) return;
  dock_->setMinimumHeight(expanded_min_height_);
  dock_->setMaximumHeight(QWIDGETSIZE_MAX);
  // Never back to a minimum the dock was merely pushed to.
  const int height = expanded_target();
  if (height <= 0) return;
  if (dock_->isFloating()) {
    dock_->resize(dock_->width(), height);
  } else if (window_ != nullptr) {
    window_->resizeDocks({dock_.data()}, {height}, Qt::Vertical);
  }
}

void StatusDockPresenter::measure_bar() {
  if (dock_ == nullptr) return;

  // The bar's height is whatever the dock needs around the bar, as the
  // current style lays it out -- asked of the layout rather than assumed,
  // since dock frames differ between styles and platforms. The expanded
  // minimum is still set when this runs, and the dock's hint would report it
  // back as if the dock needed it, so it is measured with that out of the
  // way.
  dock_->setMinimumHeight(0);
  dock_->setMaximumHeight(QWIDGETSIZE_MAX);
  if (board_ != nullptr && board_->layout() != nullptr) {
    board_->layout()->invalidate();
    board_->layout()->activate();
  }
  if (dock_->layout() != nullptr) {
    dock_->layout()->invalidate();
    dock_->layout()->activate();
  }
  bar_height_ = qMax(1, dock_->minimumSizeHint().height());
}

void StatusDockPresenter::apply_collapsed_constraints() {
  if (dock_ == nullptr) return;
  // Measured once per collapse: re-measuring on every re-apply lets the
  // bar drift by a pixel between states.
  if (bar_height_ <= 0) measure_bar();
  dock_->setMinimumHeight(bar_height_);
  dock_->setMaximumHeight(bar_height_);
  if (dock_->isFloating()) dock_->resize(dock_->width(), bar_height_);
}

void StatusDockPresenter::update_toggle_action() {
  const bool collapsed = presentation_ == StatusDockPresentation::kCollapsed;
  toggle_->setText(collapsed ? tr("Expand") : tr("Collapse"));
  toggle_->setToolTip(collapsed ? tr("Expand Status Panel")
                                : tr("Collapse Status Panel"));
  toggle_->setIcon(
      ChevronIcon(dock_ != nullptr ? dock_->palette() : QApplication::palette(),
                  collapsed));
}

void StatusDockPresenter::build_compact_bar() {
  // A frame, for the hairline that separates the row from the editor above
  // it the way the dock's own edge would.
  auto* bar = new QFrame(dock_);
  compact_bar_ = bar;
  compact_bar_->setObjectName(QStringLiteral("statusDockBar"));
  compact_bar_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  bar->setStyleSheet(QStringLiteral(
      "QFrame#statusDockBar { border-top: 1px solid palette(mid); }"));
  compact_bar_->setToolTip(tr("Expand Status Panel"));
  compact_bar_->installEventFilter(this);
  compact_bar_->hide();

  auto* row = new QHBoxLayout(compact_bar_);
  row->setContentsMargins(8, 3, 4, 3);
  row->setSpacing(6);

  bar_dot_ = new QLabel(compact_bar_);
  bar_dot_->setObjectName(QStringLiteral("statusDockBarIndicator"));
  bar_dot_->setFixedSize(kDotSize, kDotSize);

  bar_title_ = new QLabel(compact_bar_);
  bar_title_->setObjectName(QStringLiteral("statusDockBarTitle"));

  bar_status_ = new QLabel(compact_bar_);
  bar_status_->setObjectName(QStringLiteral("statusDockBarStatus"));
  auto status_font = bar_status_->font();
  status_font.setWeight(QFont::DemiBold);
  bar_status_->setFont(status_font);

  auto* expand = new QToolButton(compact_bar_);
  expand->setObjectName(QStringLiteral("statusDockBarExpand"));
  expand->setDefaultAction(toggle_);
  expand->setAutoRaise(true);
  expand->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  expand->setFocusPolicy(Qt::NoFocus);

  // The dock's own X, drawn the way the native title bar draws it.
  bar_close_ = new QToolButton(compact_bar_);
  bar_close_->setObjectName(QStringLiteral("statusDockBarClose"));
  bar_close_->setAutoRaise(true);
  bar_close_->setFocusPolicy(Qt::NoFocus);
  bar_close_->setIcon(dock_->style()->standardIcon(
      QStyle::SP_DockWidgetCloseButton, nullptr, dock_));
  bar_close_->setToolTip(tr("Close Status Panel"));
  connect(bar_close_, &QToolButton::clicked, dock_, &QDockWidget::close);

  row->addWidget(bar_dot_, 0, Qt::AlignVCenter);
  row->addWidget(bar_title_, 0, Qt::AlignVCenter);
  row->addWidget(bar_status_, 0, Qt::AlignVCenter);
  row->addStretch(1);
  row->addWidget(expand, 0, Qt::AlignVCenter);
  row->addWidget(bar_close_, 0, Qt::AlignVCenter);
}

void StatusDockPresenter::refresh_compact_bar() {
  if (compact_bar_ == nullptr || board_ == nullptr) return;
  const auto status = board_->CurrentStatus();

  bar_dot_->setStyleSheet(
      QStringLiteral("QLabel#statusDockBarIndicator { background-color: %1; "
                     "border-radius: %2px; }")
          .arg(board_->IndicatorColor(status).name())
          .arg(kDotSize / 2));
  bar_dot_->setToolTip(board_->StatusIndicatorToolTip(status));
  bar_title_->setText(dock_ != nullptr ? dock_->windowTitle() : QString());

  // An idle board has nothing to name: the grey dot says it.
  const bool idle = status == kINFO_ERROR_NEUTRAL;
  bar_status_->setVisible(!idle);
  if (!idle) {
    bar_status_->setText(board_->StatusTitle(status));
    auto pal = bar_status_->palette();
    pal.setColor(QPalette::WindowText, board_->StatusColor(status));
    bar_status_->setPalette(pal);
  }
}

auto StatusDockPresenter::eventFilter(QObject* watched, QEvent* event) -> bool {
  if (watched == window_ && event->type() == QEvent::Show) {
    // The layout has been activated by the time a window is told it is
    // shown, and nothing has been painted yet: the restored height is real
    // and there is no frame of the wrong presentation.
    apply_deferred_geometry();
    // Queued: QMainWindow applies the restored layout on its first layout
    // pass, after this, and would replace a size set now.
    if (!healed_) {
      QTimer::singleShot(0, this, [this]() { heal_on_first_show(); });
    }
  }
  if (watched == compact_bar_) {
    switch (event->type()) {
      case QEvent::MouseButtonDblClick:
        // The whole row is the target, as a title bar's double click is.
        Expand();
        return true;
      case QEvent::PaletteChange:
      case QEvent::StyleChange:
        update_toggle_action();
        refresh_compact_bar();
        break;
      default:
        break;
    }
  }
  return QObject::eventFilter(watched, event);
}

}  // namespace GpgFrontend::UI

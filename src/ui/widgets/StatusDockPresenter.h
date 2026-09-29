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

#include <optional>

class QDockWidget;
class QLabel;
class QMainWindow;
class QToolButton;
class QVariantAnimation;

namespace GpgFrontend::UI {

class InfoBoardWidget;

/**
 * @brief How a visible Status Panel presents itself.
 *
 * Deliberately separate from the dock's visibility: Hidden is the dock's own
 * state (its X, the View menu), and neither value here ever means it.
 */
enum class StatusDockPresentation : uint8_t {
  kExpanded,   ///< the full panel
  kCollapsed,  ///< one row in place of the title bar: status and Expand
};

/**
 * @brief Owns the Expanded / Collapsed presentation of one Status Panel dock.
 *
 * It never shows or hides the dock and never clears the board. What it does:
 *  - Collapse() / Expand(), also offered to the user as ToggleAction();
 *  - a new result posted to the board expands a collapsed panel;
 *  - SetCompactContext() collapses on entering a context that asked for a
 *    compact panel, keeps the user's choice while inside it, and restores
 *    what was there before on leaving it.
 */
class GF_UI_EXPORT StatusDockPresenter : public QObject {
  Q_OBJECT

 public:
  /**
   * @brief Keeps the dock at its expanded geometry while alive.
   *
   * Collapsed is a transient presentation, not a layout: a saveState() taken
   * while collapsed would record the bar's height as the panel's, and the
   * next start would open every ordinary tab with a bar-sized panel. Take one
   * around saving the window state; it also remembers the expanded height
   * under the presenter's settings name, for a next start whose front tab
   * leaves the restored panel no room. When it ends, a panel that is still
   * collapsed gets its bar back, so a close that is cancelled after the save
   * does not leave the live window expanded.
   */
  class GF_UI_EXPORT ExpandedForSaveGuard {
   public:
    explicit ExpandedForSaveGuard(StatusDockPresenter* presenter);
    ~ExpandedForSaveGuard();
    ExpandedForSaveGuard(const ExpandedForSaveGuard&) = delete;
    auto operator=(const ExpandedForSaveGuard&)
        -> ExpandedForSaveGuard& = delete;
    ExpandedForSaveGuard(ExpandedForSaveGuard&&) = delete;
    auto operator=(ExpandedForSaveGuard&&) -> ExpandedForSaveGuard& = delete;

   private:
    QPointer<StatusDockPresenter> presenter_;
  };

  /**
   * @param settings_name where the panel's expanded height is remembered
   *        between runs, or empty to remember nothing (see
   *        ExpandedForSaveGuard).
   */
  StatusDockPresenter(QMainWindow* window, QDockWidget* dock,
                      InfoBoardWidget* board, QString settings_name = {});
  ~StatusDockPresenter() override;

  [[nodiscard]] auto Presentation() const -> StatusDockPresentation {
    return presentation_;
  }
  [[nodiscard]] auto InCompactContext() const -> bool {
    return before_compact_.has_value();
  }

  /// Animated when the user or a new result asks; see SetTransitionDuration.
  void Expand();
  void Collapse();

  /// A height transition is running. Presentation() is already its target.
  [[nodiscard]] auto IsTransitioning() const -> bool;

  /**
   * @brief How long a transition takes, in milliseconds.
   *
   * By default kTransitionMs, or none at all when the application's effects
   * are switched off (Qt's own switch, which follows the desktop's animation
   * setting). 0 makes every change instant; a negative value goes back to the
   * default. The final layout is the same either way.
   */
  void SetTransitionDuration(int ms);

  static constexpr int kTransitionMs = 150;

  /**
   * @brief Tells the presenter whether the current page asked for compact.
   *
   * A transition, not a per-tab hook: ordinary -> compact collapses a visible
   * panel (a hidden one stays hidden, and is not turned into a collapsed one);
   * compact -> compact changes nothing; compact -> ordinary restores the
   * presentation from before the compact context.
   */
  void SetCompactContext(bool compact);

  /**
   * @brief The panel's expanded geometry, from a default layout.
   *
   * Applied now when expanded; only remembered when collapsed, so a layout
   * pass cannot stretch the bar.
   */
  void SetExpandedGeometry(int min_height, int height);

  [[nodiscard]] auto ToggleAction() const -> QAction* { return toggle_; }

  /**
   * @brief The row a collapsed dock shows in place of its title bar.
   *
   * Indicator, title, the result's status and Expand, then the dock's own
   * close button: one row instead of a title bar over a nearly empty strip.
   * Its close button is the dock's X -- QDockWidget::close() -- not collapse.
   */
  [[nodiscard]] auto CompactBar() const -> QWidget* { return compact_bar_; }

 protected:
  auto eventFilter(QObject* watched, QEvent* event) -> bool override;

 signals:
  /// Once per actual change of presentation, never for a no-op request.
  void SignalPresentationChanged(StatusDockPresentation presentation);

 private:
  QPointer<QMainWindow> window_;
  QPointer<QDockWidget> dock_;
  QPointer<InfoBoardWidget> board_;
  QAction* toggle_;

  QWidget* compact_bar_ = nullptr;
  QWidget* empty_content_ = nullptr;  ///< the dock's content while collapsed
  QLabel* bar_dot_ = nullptr;
  QLabel* bar_title_ = nullptr;
  QLabel* bar_status_ = nullptr;
  QToolButton* bar_close_ = nullptr;

  StatusDockPresentation presentation_ = StatusDockPresentation::kExpanded;
  /// Set exactly while inside a compact context: what to restore on leaving.
  std::optional<StatusDockPresentation> before_compact_;

  int expanded_height_ = 0;
  int expanded_min_height_ = 0;
  int bar_height_ = 0;  ///< measured on Collapse()

  /// The pinned height during a transition, and where the board comes back
  /// on the way up.
  QVariantAnimation* transition_ = nullptr;
  int reveal_at_ = 0;
  int duration_override_ = -1;

  /// A change made before the window had real geometry, whose height work
  /// waits for the window to be shown; see apply_deferred_geometry().
  bool geometry_deferred_ = false;
  /// The expanded geometry came from SetExpandedGeometry() or from the last
  /// run, not from the dock: at startup the dock's first height can be
  /// squeezed by whatever tab is in front, and is no height the user chose.
  bool expanded_geometry_given_ = false;
  QString settings_name_;
  bool healed_ = false;
  void remember_expanded_height();

  /// Within this of the dock's minimum, a height is where Qt pushed the dock,
  /// not one the user chose; such heights are never learned or remembered.
  static constexpr int kMinimumSlack = 8;
  /// With no real height known, the default layout's share of the window.
  static constexpr int kDefaultSharePercent = 30;
  [[nodiscard]] auto is_chosen_height(int height) const -> bool;
  [[nodiscard]] auto expanded_target() const -> int;
  void heal_on_first_show();
  void apply_deferred_geometry();

  void transition_to(StatusDockPresentation target, bool animate);
  void finish_transition();
  [[nodiscard]] auto transition_ms() const -> int;
  /// The dock's content and title bar for each presentation, idempotent.
  void show_compact();
  void show_full();
  void pin_height(int height);

  void apply_expanded_constraints();
  void measure_bar();
  void apply_collapsed_constraints();
  void update_toggle_action();
  void build_compact_bar();
  void refresh_compact_bar();
};

}  // namespace GpgFrontend::UI

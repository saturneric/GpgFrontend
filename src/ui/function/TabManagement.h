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
 * @brief Which tabs a "Close ..." command of the tab context menu means.
 *
 * Left and right are the tabs' visual order, which the user may have changed
 * by dragging.
 */
enum class TabCloseScope {
  kTHIS,        ///< the tab the menu was opened on
  kOTHERS,      ///< every tab but that one
  kRIGHT,       ///< the tabs to its right
  kLEFT,        ///< the tabs to its left
  kUNMODIFIED,  ///< every tab without unsaved changes
  kALL,         ///< every tab
};

/**
 * @brief The visual indices @p scope closes, in ascending order.
 *
 * @param anchor the tab the menu was opened on
 * @param modified one entry per tab, in visual order: has unsaved changes
 * @return empty when the command has nothing to close (or @p anchor is out of
 *         range for a scope that needs one)
 */
auto GF_UI_EXPORT TabsToClose(TabCloseScope scope, int anchor,
                              const QList<bool>& modified) -> QList<int>;

/**
 * @brief Tabs, most recently used first, by their stable tab id.
 */
class GF_UI_EXPORT TabMruOrder {
 public:
  /// @p id was just used: it moves to the front.
  void Touch(quint64 id);

  /// @p id is gone.
  void Remove(quint64 id);

  /// Keep only the ids in @p alive; ids in @p alive never used yet are not
  /// added.
  void Retain(const QList<quint64>& alive);

  [[nodiscard]] auto Order() const -> QList<quint64>;

 private:
  QList<quint64> order_;
};

/**
 * @brief The tab to show after closing @p closing.
 *
 * The active tab when it survives; otherwise the most recently used survivor;
 * otherwise (tabs never used, say) the visual neighbour: the first survivor
 * to the right of where the active tab was, else the nearest to its left.
 *
 * @param visual every tab before the close, in visual order
 * @return nullopt when nothing survives
 */
auto GF_UI_EXPORT TabToShowAfterClose(const QList<quint64>& mru,
                                      const QList<quint64>& visual,
                                      const QSet<quint64>& closing,
                                      quint64 active) -> std::optional<quint64>;

/**
 * @brief One Ctrl+Tab session: a walk over a snapshot of the MRU order.
 *
 * The snapshot is taken when the session begins, so the walk is not reordered
 * by the tabs it visits; position 0 is the tab that was current. Tabs closed
 * while it runs are stepped over.
 */
class GF_UI_EXPORT TabSwitchSession {
 public:
  void Begin(const QList<quint64>& mru);

  [[nodiscard]] auto Active() const -> bool;

  /**
   * @brief Move @p delta places, wrapping, over the tabs still @p alive.
   *
   * @return the tab now selected, or nullopt when there is none to go to
   */
  auto Step(int delta, const std::function<bool(quint64)>& alive)
      -> std::optional<quint64>;

  /// The session is over; the tab it ended on, if it moved at all.
  auto End() -> std::optional<quint64>;

 private:
  QList<quint64> snapshot_;
  qsizetype position_ = 0;
  bool active_ = false;
  bool moved_ = false;
};

/**
 * @brief An open tab, as the open-tabs list shows and searches it.
 */
struct GF_UI_EXPORT TabListEntry {
  quint64 id = 0;
  QString title;  ///< the document's name
  QString path;   ///< its file or folder, or empty
};

/**
 * @brief The entries matching @p query, best first.
 *
 * Case-insensitive. A title starting with the query ranks first, then a title
 * containing it, then a path containing it; within a rank the input order (the
 * MRU order) is kept. An empty query keeps every entry, in input order.
 */
auto GF_UI_EXPORT FilterTabEntries(const QList<TabListEntry>& entries,
                                   const QString& query) -> QList<TabListEntry>;

/**
 * @brief For tabs sharing a title, the shortest end of each one's folder that
 *        tells it apart.
 *
 * @param titles the tabs' titles
 * @param paths their files' paths, same length; empty for none
 * @return one entry per tab: empty when its title is unique (or it has no
 *         path), else its last folders joined with '/', e.g. "a/docs"
 */
auto GF_UI_EXPORT DisambiguatingSuffixes(const QStringList& titles,
                                         const QStringList& paths)
    -> QStringList;

/**
 * @brief A key two paths share exactly when they name the same file.
 *
 * Symbolic links and relative parts resolved for a file that exists; compared
 * case-insensitively where the file system usually is (Windows, macOS).
 */
auto GF_UI_EXPORT FileIdentityKey(const QString& path) -> QString;

/**
 * @brief The next "untitled" name not already taken by an open tab.
 *
 * @param counter the last number used; advanced past the name returned, so a
 *        name once given is never handed out again
 */
auto GF_UI_EXPORT NextUntitledTitle(int& counter, const QString& prefix,
                                    const QString& suffix,
                                    const QSet<QString>& taken) -> QString;

}  // namespace GpgFrontend::UI

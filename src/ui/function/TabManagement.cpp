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
#include "ui/function/TabManagement.h"

namespace GpgFrontend::UI {

auto TabsToClose(TabCloseScope scope, int anchor, const QList<bool>& modified)
    -> QList<int> {
  const auto count = static_cast<int>(modified.size());
  const bool anchored = anchor >= 0 && anchor < count;

  QList<int> indices;
  for (int i = 0; i < count; ++i) {
    bool take = false;
    switch (scope) {
      case TabCloseScope::kTHIS:
        take = anchored && i == anchor;
        break;
      case TabCloseScope::kOTHERS:
        take = anchored && i != anchor;
        break;
      case TabCloseScope::kRIGHT:
        take = anchored && i > anchor;
        break;
      case TabCloseScope::kLEFT:
        take = anchored && i < anchor;
        break;
      case TabCloseScope::kUNMODIFIED:
        take = !modified[i];
        break;
      case TabCloseScope::kALL:
        take = true;
        break;
    }
    if (take) indices.append(i);
  }
  return indices;
}

void TabMruOrder::Touch(quint64 id) {
  order_.removeAll(id);
  order_.prepend(id);
}

void TabMruOrder::Remove(quint64 id) { order_.removeAll(id); }

void TabMruOrder::Retain(const QList<quint64>& alive) {
  order_.removeIf([&](quint64 id) { return !alive.contains(id); });
}

auto TabMruOrder::Order() const -> QList<quint64> { return order_; }

auto TabToShowAfterClose(const QList<quint64>& mru,
                         const QList<quint64>& visual,
                         const QSet<quint64>& closing, quint64 active)
    -> std::optional<quint64> {
  const auto survives = [&](quint64 id) {
    return visual.contains(id) && !closing.contains(id);
  };

  if (survives(active)) return active;

  for (const auto id : mru) {
    if (survives(id)) return id;
  }

  const auto at = visual.indexOf(active);
  if (at >= 0) {
    for (auto i = at + 1; i < visual.size(); ++i) {
      if (survives(visual[i])) return visual[i];
    }
    for (auto i = at - 1; i >= 0; --i) {
      if (survives(visual[i])) return visual[i];
    }
  }

  for (const auto id : visual) {
    if (survives(id)) return id;
  }
  return std::nullopt;
}

void TabSwitchSession::Begin(const QList<quint64>& mru) {
  snapshot_ = mru;
  position_ = 0;
  active_ = true;
  moved_ = false;
}

auto TabSwitchSession::Active() const -> bool { return active_; }

auto TabSwitchSession::Step(int delta,
                            const std::function<bool(quint64)>& alive)
    -> std::optional<quint64> {
  const auto n = snapshot_.size();
  if (!active_ || n == 0 || delta == 0) return std::nullopt;

  const qsizetype step = delta > 0 ? 1 : -1;
  auto position = position_;
  // At most one lap: every other entry is looked at once.
  for (qsizetype tried = 0; tried < n; ++tried) {
    position = ((position + step) % n + n) % n;
    if (position == position_) break;
    if (alive(snapshot_[position])) {
      position_ = position;
      moved_ = true;
      return snapshot_[position];
    }
  }
  return std::nullopt;
}

auto TabSwitchSession::End() -> std::optional<quint64> {
  const bool was = active_ && moved_;
  active_ = false;
  moved_ = false;
  if (!was || position_ >= snapshot_.size()) return std::nullopt;
  return snapshot_[position_];
}

auto FilterTabEntries(const QList<TabListEntry>& entries, const QString& query)
    -> QList<TabListEntry> {
  const auto q = query.trimmed();
  if (q.isEmpty()) return entries;

  constexpr int kRanks = 3;
  std::array<QList<TabListEntry>, kRanks> ranked;
  for (const auto& e : entries) {
    if (e.title.startsWith(q, Qt::CaseInsensitive)) {
      ranked[0].append(e);
    } else if (e.title.contains(q, Qt::CaseInsensitive)) {
      ranked[1].append(e);
    } else if (e.path.contains(q, Qt::CaseInsensitive)) {
      ranked[2].append(e);
    }
  }

  QList<TabListEntry> out;
  for (const auto& r : ranked) out.append(r);
  return out;
}

namespace {

/// The folders holding @p path, innermost first.
auto ParentFolders(const QString& path) -> QStringList {
  if (path.isEmpty()) return {};
  const auto parent = QFileInfo(QDir::cleanPath(path)).path();
  auto parts =
      QDir::fromNativeSeparators(parent).split(u'/', Qt::SkipEmptyParts);
  std::reverse(parts.begin(), parts.end());
  return parts;
}

/// The last @p k of @p folders (innermost first), outermost first, as text.
auto FolderSuffix(const QStringList& folders, qsizetype k) -> QString {
  QStringList picked = folders.mid(0, k);
  std::reverse(picked.begin(), picked.end());
  return picked.join(u'/');
}

}  // namespace

auto DisambiguatingSuffixes(const QStringList& titles, const QStringList& paths)
    -> QStringList {
  QStringList out(titles.size());

  QHash<QString, QList<qsizetype>> by_title;
  for (qsizetype i = 0; i < titles.size(); ++i) by_title[titles[i]].append(i);

  for (const auto& group : std::as_const(by_title)) {
    if (group.size() < 2) continue;

    QHash<qsizetype, QStringList> folders;
    for (const auto i : group) {
      folders[i] = ParentFolders(i < paths.size() ? paths[i] : QString());
    }

    for (const auto i : group) {
      const auto& mine = folders[i];
      if (mine.isEmpty()) continue;

      QString suffix = FolderSuffix(mine, mine.size());
      for (qsizetype k = 1; k <= mine.size(); ++k) {
        const auto candidate = FolderSuffix(mine, k);
        const bool unique =
            std::none_of(group.cbegin(), group.cend(), [&](qsizetype j) {
              return j != i && !folders[j].isEmpty() &&
                     FolderSuffix(folders[j], k) == candidate;
            });
        if (unique) {
          suffix = candidate;
          break;
        }
      }
      out[i] = suffix;
    }
  }
  return out;
}

auto FileIdentityKey(const QString& path) -> QString {
  if (path.isEmpty()) return {};
  const QFileInfo info(path);
  auto key = info.canonicalFilePath();
  if (key.isEmpty()) key = QDir::cleanPath(info.absoluteFilePath());
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
  key = key.toCaseFolded();
#endif
  return key;
}

auto NextUntitledTitle(int& counter, const QString& prefix,
                       const QString& suffix, const QSet<QString>& taken)
    -> QString {
  const auto ext = suffix.trimmed();
  while (true) {
    auto title = prefix + QString::number(++counter);
    if (!ext.isEmpty()) title += u"."_s + ext;
    if (!taken.contains(title)) return title;
  }
}

}  // namespace GpgFrontend::UI

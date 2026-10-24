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

#include "ui/main_window/ToolBarHelper.h"

namespace GpgFrontend::UI {

void SetupToolBar(QToolBar* toolbar, Qt::ToolButtonStyle style, QSize size) {
  toolbar->setMovable(true);
  toolbar->setFloatable(false);
  toolbar->setToolButtonStyle(style);
  toolbar->setIconSize(size);
}

void SetupMenuToolButton(QToolButton* button, QMenu* menu, const QIcon& icon,
                         const QString& text, const QString& tooltip,
                         Qt::ToolButtonStyle style, QSize size) {
  button->setMenu(menu);
  button->setPopupMode(QToolButton::InstantPopup);
  button->setIcon(icon);
  button->setText(text);
  button->setToolTip(tooltip);
  button->setFocusPolicy(Qt::NoFocus);
  button->setAutoRaise(false);
  button->setToolButtonStyle(style);
  button->setIconSize(size);
}

auto GroupCryptoOperas(int mask, int defaults)
    -> std::vector<std::vector<GpgOperation>> {
  const std::array<std::vector<GpgOperation>, 4> families{{
      {kENCRYPT, kENCRYPT_SIGN, kSYMMETRIC_ENCRYPT},
      {kDECRYPT, kDECRYPT_VERIFY},
      {kSIGN},
      {kVERIFY},
  }};

  std::vector<std::vector<GpgOperation>> groups;
  for (const auto& family : families) {
    std::vector<GpgOperation> ticked;
    for (const auto opera : family) {
      if ((mask & opera) != 0) ticked.push_back(opera);
    }
    std::stable_partition(ticked.begin(), ticked.end(),
                          [defaults](GpgOperation o) {
                            return (defaults & ~kNeverDefaultOperas & o) != 0;
                          });
    if (!ticked.empty()) groups.push_back(std::move(ticked));
  }
  return groups;
}

auto FamilyFaceIndex(const std::vector<bool>& enabled, std::size_t faces)
    -> std::size_t {
  const auto end = enabled.cbegin() +
                   static_cast<std::ptrdiff_t>(std::min(faces, enabled.size()));
  const auto it = std::find(enabled.cbegin(), end, true);
  return it == end
             ? 0
             : static_cast<std::size_t>(std::distance(enabled.cbegin(), it));
}

void SetupActionFamilyToolButton(QToolButton* button,
                                 const QList<QAction*>& actions,
                                 qsizetype faces, Qt::ToolButtonStyle style,
                                 QSize size) {
  auto* menu = new QMenu(button);
  menu->addActions(actions);

  // DelayedPopup, not MenuButtonPopup: the latter reserves an arrow column
  // that pushes the icon off the centre its plain neighbours keep, while this
  // draws the same small indicator as the Open / Import buttons.
  button->setMenu(menu);
  button->setPopupMode(QToolButton::DelayedPopup);
  button->setContextMenuPolicy(Qt::CustomContextMenu);
  button->setFocusPolicy(Qt::NoFocus);
  button->setAutoRaise(true);
  button->setToolButtonStyle(style);
  button->setIconSize(size);

  const auto face = [actions, faces]() -> QAction* {
    std::vector<bool> enabled;
    for (const auto* act : actions) enabled.push_back(act->isEnabled());
    return actions.at(static_cast<qsizetype>(
        FamilyFaceIndex(enabled, static_cast<std::size_t>(faces))));
  };

  const auto sync = [button, face]() {
    auto* act = face();
    button->setIcon(act->icon());
    // iconText() has already resolved "&&" to "&", which a button would read
    // as a mnemonic and draw "Encrypt & Sign" as "Encrypt _Sign".
    button->setText(act->iconText().replace(u'&', u"&&"_s));
    button->setToolTip(act->toolTip());
    button->setEnabled(std::any_of(
        button->menu()->actions().cbegin(), button->menu()->actions().cend(),
        [](const QAction* a) { return a->isEnabled(); }));
  };

  for (auto* act : actions) {
    QObject::connect(act, &QAction::changed, button, sync);
  }
  // E.g. no key checked: Encrypt is off, Sym. Encrypt is in the list.
  QObject::connect(button, &QToolButton::clicked, button, [button, face]() {
    if (auto* act = face(); act->isEnabled()) {
      act->trigger();
    } else {
      button->showMenu();
    }
  });
  QObject::connect(button, &QWidget::customContextMenuRequested, button,
                   &QToolButton::showMenu);
  sync();
}

auto EvenButtonWidth(std::vector<int> widths) -> int {
  if (widths.empty()) return 0;
  const auto mid =
      widths.begin() + static_cast<std::ptrdiff_t>(widths.size() / 2);
  std::nth_element(widths.begin(), mid, widths.end());
  return *mid;
}

void EvenToolBarButtonWidths(QToolBar* toolbar) {
  QList<QWidget*> buttons;
  std::vector<int> widths;
  for (auto* act : toolbar->actions()) {
    auto* widget = toolbar->widgetForAction(act);
    if (qobject_cast<QToolButton*>(widget) == nullptr) continue;
    widget->setMinimumWidth(0);
    buttons.append(widget);
    widths.push_back(widget->sizeHint().width());
  }

  const auto width = EvenButtonWidth(std::move(widths));
  for (auto* widget : buttons) widget->setMinimumWidth(width);
}

}  // namespace GpgFrontend::UI

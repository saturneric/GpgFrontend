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

#include <QDesktopServices>
#include <QMessageBox>
#include <QThread>
#include <QVBoxLayout>
#include <memory>

#include "ui/lua/NativeInstances.h"

/**
 * @file EmbeddedViewCommon.h
 * @brief What the HTML view and the web editor services share: both put a
 * provider's widget into a slot of a module's native instance, and both hand
 * a link the user activated to the same question.
 */

namespace GpgFrontend::UI::Embedded {

inline auto OnGuiThread() -> bool {
  auto* app = QCoreApplication::instance();
  return app != nullptr && QThread::currentThread() == app->thread();
}

/// The texts of the link question, translated by the caller; each takes the
/// address as %1.
struct LinkPrompt {
  QString not_opened_title;
  QString not_opened;
  QString open_title;
  QString open;
};

/// The Host's question, with the address exactly as it would be opened.
/// Fully encoded, so an internationalised host name shows as its punycode
/// rather than as whatever it was chosen to look like.
inline void AskThenOpen(const QUrl& url, const LinkPrompt& prompt) {
  const auto scheme = url.scheme().toLower();
  const auto shown = url.toString(QUrl::FullyEncoded);
  auto* parent = QApplication::activeWindow();
  if (scheme != u"http"_s && scheme != u"https"_s && scheme != u"mailto"_s) {
    QMessageBox::information(parent, prompt.not_opened_title,
                             prompt.not_opened.arg(shown));
    return;
  }
  const auto answer = QMessageBox::question(
      parent, prompt.open_title, prompt.open.arg(shown),
      QMessageBox::Open | QMessageBox::Cancel, QMessageBox::Cancel);
  if (answer == QMessageBox::Open) QDesktopServices::openUrl(url);
}

/// @p slot, once it has been found by address in the tree of the live
/// instance @p instance that @p caller owns; nullptr when it is not there.
///
/// The slot is the caller's pointer, so it is not followed until it has been
/// found. Asking it for its ancestors would dereference whatever was passed.
inline auto FindSlot(const QString& caller, quint64 instance, const void* slot)
    -> QWidget* {
  // Who is asking is the grant's answer, never an argument's; and the
  // instance must be one the Host made for that module, still alive.
  const auto live = NativeInstances::Instance().Find(instance);
  if (slot == nullptr || !live.has_value() || live->owner != caller ||
      live->widget.isNull()) {
    return nullptr;
  }
  QWidget* root = live->widget.data();
  if (slot == root) return root;
  for (auto* child : root->findChildren<QWidget*>()) {
    if (child == slot) return child;
  }
  return nullptr;
}

/// A widget fills its slot, so the slot must be empty.
inline auto IsEmptySlot(const QWidget* slot) -> bool {
  return slot->layout() == nullptr || slot->layout()->count() == 0;
}

/// Puts @p widget into @p slot inside a margin-less frame named
/// @p frame_name, and returns the frame.
inline auto MountInSlot(QWidget* slot, QWidget* widget,
                        const QString& frame_name) -> QWidget* {
  auto* frame = new QWidget(slot);
  frame->setObjectName(frame_name);
  auto* frame_layout = new QVBoxLayout(frame);
  frame_layout->setContentsMargins(0, 0, 0, 0);
  frame_layout->setSpacing(0);
  widget->setParent(frame);
  frame_layout->addWidget(widget);
  auto* slot_layout = slot->layout();
  if (slot_layout == nullptr) {
    slot_layout = new QVBoxLayout(slot);
    slot_layout->setContentsMargins(0, 0, 0, 0);
    slot_layout->setSpacing(0);
  }
  slot_layout->addWidget(frame);
  return frame;
}

/// The highest-priority provider that is neither draining nor waiting for a
/// restart, among those @p accept takes; nullptr when there is none.
template <typename Provider, typename Accept>
auto PickProvider(const QList<std::shared_ptr<Provider>>& providers,
                  Accept accept) -> std::shared_ptr<Provider> {
  std::shared_ptr<Provider> best;
  for (const auto& p : providers) {
    if (p->draining || p->info.restart_required || !accept(*p)) continue;
    if (best == nullptr || p->info.priority > best->info.priority) best = p;
  }
  return best;
}

}  // namespace GpgFrontend::UI::Embedded

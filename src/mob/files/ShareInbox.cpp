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

#include "files/ShareInbox.h"

#include <QJSEngine>
#include <QPointer>

#include "files/FileWorkspace.h"

namespace GpgFrontend::Mobile {

namespace {

auto SourceSlot() -> ShareInbox::Source& {
  static ShareInbox::Source source = TakeSharedItems;
  return source;
}

}  // namespace

ShareInbox::ShareInbox(QObject* parent) : QObject(parent) {
  // The newest inbox hears of it: one per QML engine, and the one on screen
  // is the last made.
  QPointer<ShareInbox> self(this);
  SetSharedItemsArrived([self]() {
    if (self != nullptr) emit self->SignalArrived();
  });
}

auto ShareInbox::create(QQmlEngine*, QJSEngine*) -> ShareInbox* {
  return new ShareInbox();
}

void ShareInbox::SetSourceForTesting(Source source) {
  SourceSlot() = source ? std::move(source) : Source(TakeSharedItems);
}

auto ShareInbox::take() -> QVariantMap {
  const auto items = SourceSlot()();
  if (items.Empty()) return {};

  QVariantList ids;
  auto* files = FileWorkspace::Instance();
  auto refused = items.refused;
  if (files != nullptr) ids = files->receiveFiles(items.files);
  // Whatever was not taken in (no workspace, nowhere to stage them) is said
  // to be refused: the share is consumed either way.
  refused +=
      static_cast<int>(items.files.size()) - static_cast<int>(ids.size());
  return {
      {QStringLiteral("files"), ids},
      {QStringLiteral("text"), items.text},
      {QStringLiteral("refused"), refused},
  };
}

}  // namespace GpgFrontend::Mobile

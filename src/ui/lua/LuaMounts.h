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

#include <QCborMap>
#include <QDialog>
#include <QWidget>

#include "ui/lua/NativeInstances.h"

namespace GpgFrontend::UI::Lua {

/**
 * @file LuaMounts.h
 * @brief The Host containers module widgets are mounted in.
 *
 * The container is always the Host's: its window, its dialog frame, its
 * settings page. The module's widget sits inside and gets nothing from the
 * Host but an instance number.
 */

/// A dialog frame of the Host's around a module's dialog widget.
class GF_UI_EXPORT NativeDialog : public QDialog, public NativeContainer {
  Q_OBJECT

 public:
  NativeDialog(const QString& mount_id, const QString& widget_id,
               const QCborMap& args, QWidget* parent);
  ~NativeDialog() override;

  [[nodiscard]] auto Ok() const -> bool { return instance_ != 0; }
  [[nodiscard]] auto MountId() const -> const QString& { return mount_id_; }

  void OnClose() override;

 protected:
  void closeEvent(QCloseEvent* event) override;
  void reject() override;

 private:
  auto CloseAllowed() -> bool;

  QString mount_id_;
  quint64 instance_ = 0;
  bool closing_ = false;
};

/// A Settings dialog page of the Host's around a module's settings widget.
class GF_UI_EXPORT NativeSettingsPage : public QWidget, public NativeContainer {
  Q_OBJECT

 public:
  NativeSettingsPage(const QString& widget_id, QWidget* parent = nullptr);
  ~NativeSettingsPage() override;

  [[nodiscard]] auto Ok() const -> bool { return instance_ != 0; }
  void Load();
  auto Apply() -> bool;

  void OnRestartNeeded(int level) override;

 signals:
  void SignalRestartNeeded(int level);

 private:
  quint64 instance_ = 0;
};

/**
 * @brief Open one of @p module's dialog mounts.
 *
 * Only the module's own: a module cannot open a view another module mounted.
 * An already open one is raised rather than duplicated.
 *
 * @return a GF_CMD_* status
 */
auto GF_UI_EXPORT OpenDialogMount(const QString& module, const QString& view_id,
                                  const QCborMap& args, QWidget* parent) -> int;

/// A settings page per settings mount, with its section and search words.
struct GF_UI_EXPORT NativeSettingsPageInfo {
  NativeSettingsPage* page = nullptr;
  QString title;
  QString section;
  QStringList keywords;
};

auto GF_UI_EXPORT BuildNativeSettingsPages() -> QList<NativeSettingsPageInfo>;

/// Close every dialog @p module has open. For teardown.
void GF_UI_EXPORT CloseDialogsOf(const QString& module);

}  // namespace GpgFrontend::UI::Lua

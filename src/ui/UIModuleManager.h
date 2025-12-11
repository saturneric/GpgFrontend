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

#include "core/function/basic/GpgFunctionObject.h"
#include "core/module/Module.h"
#include "sdk/GFSDKBasicModel.h"
#include "ui/main_window/MainWindow.h"

namespace GpgFrontend::UI {

struct ModuleTranslatorInfo {
  GFTranslatorDataReader reader_;
};

class GF_UI_EXPORT UIModuleManager
    : public SingletonFunctionObject<UIModuleManager> {
 public:
  /**
   * @brief Construct a new UIModuleManager object
   *
   * @param channel
   */
  explicit UIModuleManager(int channel);

  /**
   * @brief Destroy the UIModuleManager object
   *
   */
  virtual ~UIModuleManager() override;

  /**
   * @brief
   *
   * @return auto
   */
  auto RegisterTranslatorDataReader(Module::ModuleIdentifier id,
                                    GFTranslatorDataReader reader) -> bool;

  /**
   * @brief
   *
   * @param id
   * @return auto
   */
  auto RegisterQObject(QObject*) -> QString;

  /**
   * @brief
   *
   * @param id
   * @return auto
   */
  auto GetQObject(const QString& id) -> QObject*;

  /**
   * @brief
   *
   * @param id
   * @return auto
   */
  auto MakeCapsule(std::any) -> QString;

  /**
   * @brief
   *
   * @param id
   * @return auto
   */
  auto GetCapsule(const QString& uuid) -> std::any;

  /**
   * @brief
   *
   */
  void RegisterAllModuleTranslators();

  /**
   * @brief Set the Main Window object
   *
   * @param main_window
   */
  void SetMainWindow(MainWindow* main_window);

  /**
   * @brief Get the Main Window object
   *
   * @return MainWindow*
   */
  auto GetMainWindow() -> MainWindow*;

  /**
   * @brief
   *
   * @return const QSettings*
   */
  [[nodiscard]] auto GetSettings() const -> const QSettings*;

 private:
  QMap<QString, ModuleTranslatorInfo> translator_data_readers_;
  QContainer<QTranslator*> registered_translators_;
  QContainer<QByteArray> read_translator_data_list_;
  QMap<QString, QPointer<QObject>> registered_qobjects_;
  QMap<QString, std::any> capsule_;
  QSettings settings_;
  QPointer<MainWindow> main_window_;
};

auto GF_UI_EXPORT RegisterQObject(QObject* p) -> QString;

}  // namespace GpgFrontend::UI
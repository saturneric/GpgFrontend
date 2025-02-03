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

#include "GpgFrontendUIExport.h"
#include "core/function/basic/GpgFunctionObject.h"
#include "core/module/Module.h"
#include "sdk/GFSDKBasic.h"
#include "sdk/GFSDKUI.h"
#include "ui/struct/UIMountPoint.h"

namespace GpgFrontend::UI {

struct MountedUIEntry {
  QString id_;
  QMap<QString, QString> meta_data_;
  QMap<QString, QString> meta_data_translated_;
  QObjectFactory factory_;

  MountedUIEntry() = default;

  [[nodiscard]] auto GetWidget() const -> QWidget*;

  [[nodiscard]] auto GetMetaDataByDefault(
      const QString& key, QString default_value) const -> QString;
};

struct ModuleTranslatorInfo {
  GFTranslatorDataReader reader_;
};

class GPGFRONTEND_UI_EXPORT UIModuleManager
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
   * @param id
   * @param entry_type
   * @param metadata_desc
   * @return true
   * @return false
   */
  auto DeclareMountPoint(const QString& id, const QString& entry_type,
                         QMap<QString, QVariant> meta_data_desc) -> bool;

  /**
   * @brief
   *
   * @param id
   * @param meta_data
   * @param entry
   * @return true
   * @return false
   */
  auto MountEntry(const QString& id, QMap<QString, QString> meta_data,
                  QObjectFactory factory) -> bool;

  /**
   * @brief
   *
   * @param id
   * @return QContainer<MountedUIEntry>
   */
  auto QueryMountedEntries(QString id) -> QContainer<MountedUIEntry>;

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
  auto RegisterQObject(const QString& id, QObject*) -> bool;

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
   * @brief
   *
   */
  void TranslateAllModulesParams();

 private:
  QMap<QString, UIMountPoint> mount_points_;
  QMap<QString, QContainer<MountedUIEntry>> mounted_entries_;
  QMap<QString, ModuleTranslatorInfo> translator_data_readers_;
  QContainer<QTranslator*> registered_translators_;
  QContainer<QByteArray> read_translator_data_list_;
  QMap<QString, QPointer<QObject>> registered_qobjects_;
  QMap<QString, std::any> capsule_;
};

}  // namespace GpgFrontend::UI
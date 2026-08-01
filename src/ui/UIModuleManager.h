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
#include "sdk/GFSDKUIModel.h"
#include "ui/main_window/MainWindow.h"

namespace GpgFrontend::UI {

struct ModuleTranslatorInfo {
  GFTranslatorDataReader reader_;
};

/**
 * @brief A settings page contributed by a module.
 *
 * Holds a factory rather than a widget: the Settings dialog is built and
 * destroyed on every open, so each dialog instance needs its own page.
 */
struct GF_UI_EXPORT SettingsPageRegistration {
  QString id;            ///< unique, module-namespaced
  QString section_id;    ///< canonical section key, see SettingsSectionOrder()
  QString title;         ///< untranslated source string, "GTrC" context
  QStringList keywords;  ///< untranslated source strings, "GTrC" context
  QObjectFactory factory{nullptr};  ///< runs on the main thread
  void* data{nullptr};              ///< passed to factory on every invocation
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
   * @param p
   * @return QString
   */
  auto RegisterQObject(const QString& id, QObject* p) -> QString;

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
   * @brief Register a module-owned page for the Settings dialog.
   *
   * A duplicate id is rejected rather than overwritten: a dialog that is
   * already open may hold a widget built by the previous factory.
   *
   * @param reg the registration; id, title and factory are required
   * @return true when the page was registered
   */
  auto RegisterSettingsPage(const SettingsPageRegistration& reg) -> bool;

  /**
   * @brief Drop a module-owned settings page registration.
   *
   * Modules must do this before unloading — a factory pointing into an
   * unloaded shared object would crash the next dialog build.
   *
   * @param id the identifier used to register
   * @return true when a registration was removed
   */
  auto UnregisterSettingsPage(const QString& id) -> bool;

  /**
   * @brief Every registered module settings page, in registration order.
   *
   * Order is preserved so it can act as the tiebreak between pages sharing a
   * section.
   *
   * @return const QList<SettingsPageRegistration>&
   */
  [[nodiscard]] auto ListSettingsPages() const
      -> const QList<SettingsPageRegistration>&;

  /**
   * @brief
   *
   * @return const QSettings*
   */
  [[nodiscard]] auto GetSettings() const -> const QSettings*;

  /**
   * @brief
   *
   * @param extension
   * @param event_prefix
   */
  auto RegisterFileExtensionHandleEvent(const QString& extension,
                                        const QString& event_prefix) -> bool;

  /**
   * @brief Get the File Extension Event Id object
   *
   * @param extension
   * @param operation
   * @return QString
   */
  auto GetFileExtensionEventId(const QString& extension,
                               const QString& operation) -> QString;

 private:
  QMap<QString, ModuleTranslatorInfo> translator_data_readers_;
  QContainer<QTranslator*> registered_translators_;
  QContainer<QByteArray> read_translator_data_list_;
  QMap<QString, QPointer<QObject>> registered_qobjects_;
  QMap<QString, std::any> capsule_;
  QMap<QString, QString> file_ext_event_prefix_map_;
  QList<SettingsPageRegistration> settings_pages_;
  /// Mutable so the const accessor can sync() it; syncing only reconciles with
  /// the backing store, it does not change what this object represents.
  mutable QSettings settings_;
};

auto GF_UI_EXPORT RegisterQObject(QObject* p) -> QString;

auto GF_UI_EXPORT RegisterNamedQObject(const QString& id, QObject* p)
    -> QString;

auto GF_UI_EXPORT FileExtensionEventId(const QString& extension,
                                       const QString& operation) -> QString;

}  // namespace GpgFrontend::UI
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

#include <QReadWriteLock>
#include <optional>

#include "core/function/basic/GpgFunctionObject.h"
#include "core/module/Module.h"
#include "sdk/GFSDKTypes.h"
#include "ui/main_window/MainWindow.h"

namespace GpgFrontend::UI {

struct ModuleTranslatorInfo {
  GFTranslatorDataReader reader_;
};

/**
 * @brief A module translator together with the QM bytes it reads from.
 *
 * QTranslator::load(const uchar*, int) does not copy its input: the buffer has
 * to outlive the translator, and the translator has to be uninstalled before
 * the buffer goes away. Keeping the two in one entry makes that impossible to
 * get wrong -- they are released together, in that order.
 */
struct InstalledModuleTranslator {
  QTranslator* translator{nullptr};
  QByteArray data;
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
   * @brief Drop a module's translator data reader.
   *
   * Modules must do this before unloading -- a reader pointing into an
   * unloaded shared object would crash the next language switch.
   *
   * @param id the identifier used to register
   * @return true when a reader was removed
   */
  auto UnregisterTranslatorDataReader(const Module::ModuleIdentifier& id)
      -> bool;

  /**
   * @brief Name a Host object for the Host's own later lookup.
   *
   * Host-internal only: nothing registered here is reachable from a module.
   * The entry drops itself when the object is destroyed; a null @p p clears
   * the name.
   *
   * @param id the name
   * @param p the object, or null
   * @return QString the name
   */
  auto RegisterQObject(const QString& id, QObject* p) -> QString;

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
   * @brief The translators installed by the last
   * RegisterAllModuleTranslators() call, in registration order.
   *
   * Handed out as guarded pointers so a caller can tell that a previous round
   * really was destroyed rather than merely forgotten.
   *
   * @return QContainer<QPointer<QTranslator>>
   */
  [[nodiscard]] auto InstalledTranslators() const
      -> QContainer<QPointer<QTranslator>>;

 private:
  /**
   * @brief Uninstall and destroy every installed module translator, then
   * release the QM bytes they were reading from.
   */
  void clear_installed_translators();

  QMap<QString, ModuleTranslatorInfo> translator_data_readers_;
  QContainer<InstalledModuleTranslator> installed_translators_;
  QMap<QString, QPointer<QObject>> registered_qobjects_;
  QMap<QString, std::any> capsule_;
};

auto GF_UI_EXPORT RegisterNamedQObject(const QString& id, QObject* p)
    -> QString;

/**
 * @brief The current editor tab's exact octets, for the SDK to hand modules.
 *
 * The bytes as the document holds them, line endings included, with any module
 * view flushed back first -- exactly what the application's own operations
 * act on. Empty when no tab is open, or when the current tab is not a text
 * tab.
 *
 * A typed function rather than a Q_INVOKABLE on TextEdit: reaching a widget
 * method by string name from across the module boundary makes a gf_ui
 * internal a de-facto ABI that nothing checks, and renaming it would break
 * modules at run time with no build error anywhere.
 *
 * Safe to call from any thread; the read is marshalled to the GUI thread.
 *
 * @return the bytes, or std::nullopt when no text tab is open
 */
auto GF_UI_EXPORT CurrentEditorContent() -> std::optional<QByteArray>;

/**
 * @brief What the current document is: `id`, `type`, `title`, `path`,
 *        `modified`. Never what it says.
 *
 * Marshalled to the GUI thread the same way as CurrentEditorContent().
 *
 * @return nullopt when no editor document is open
 */
auto GF_UI_EXPORT CurrentDocumentInfo() -> std::optional<QCborMap>;

}  // namespace GpgFrontend::UI
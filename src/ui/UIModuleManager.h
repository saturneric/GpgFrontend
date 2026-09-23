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

/**
 * @brief A tab page view contributed by a module.
 *
 * The host still builds the PlainTextEditorPage that owns the document; this
 * factory only supplies the widget mounted on top of it as the page's primary
 * view. Keeping the host page means save, crash recovery, the unsaved-changes
 * prompt and CurPlainText() all keep working untouched.
 *
 * Holds a factory rather than a widget: every tab of the type needs its own.
 */
struct GF_UI_EXPORT TabPageViewRegistration {
  QString tab_type;                 ///< upper-cased, e.g. "EMAIL"
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
   * Modules must do this before unloading -- a factory pointing into an
   * unloaded shared object would crash the next dialog build.
   *
   * @param id the identifier used to register
   * @return true when a registration was removed
   */
  auto UnregisterSettingsPage(const QString& id) -> bool;

  /**
   * @brief Register a module-owned primary view for a tab type.
   *
   * A duplicate tab type is rejected rather than overwritten, for the same
   * reason a duplicate settings page is: tabs already open hold a widget built
   * by the previous factory.
   *
   * @param reg the registration; tab_type and factory are required
   * @return true when the view was registered
   */
  auto RegisterTabPageView(const TabPageViewRegistration& reg) -> bool;

  /**
   * @brief Drop a module-owned tab page view registration.
   *
   * Modules must do this before unloading -- a factory pointing into an
   * unloaded shared object would crash the next time a tab of this type opens.
   *
   * @param tab_type the type used to register; matched case-insensitively
   * @return true when a registration was removed
   */
  auto UnregisterTabPageView(const QString& tab_type) -> bool;

  /**
   * @brief The view factory registered for a tab type, if any.
   *
   * @param tab_type matched case-insensitively
   * @return the registration, or nullopt when the type has no module view
   */
  [[nodiscard]] auto TabPageViewFor(const QString& tab_type) const
      -> std::optional<TabPageViewRegistration>;

  /**
   * @brief Every registered module settings page, in registration order.
   *
   * Order is preserved so it can act as the tiebreak between pages sharing a
   * section.
   *
   * @return const QList<SettingsPageRegistration>&
   */
  /// Returned BY VALUE, deliberately: a reference would outlive the lock that
  /// makes reading the list safe at all. See registry_lock_.
  [[nodiscard]] auto ListSettingsPages() const
      -> QList<SettingsPageRegistration>;

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
  /**
   * @brief Uninstall and destroy every installed module translator, then
   * release the QM bytes they were reading from.
   */
  void clear_installed_translators();

  QMap<QString, ModuleTranslatorInfo> translator_data_readers_;
  QContainer<InstalledModuleTranslator> installed_translators_;
  QMap<QString, QPointer<QObject>> registered_qobjects_;
  QMap<QString, std::any> capsule_;
  QMap<QString, QString> file_ext_event_prefix_map_;
  QList<SettingsPageRegistration> settings_pages_;
  QMap<QString, TabPageViewRegistration> tab_page_views_;
  /// Guards the two registries above.
  ///
  /// Modules register from GFRegisterModule and unregister from
  /// GFDeactivateModule, and BOTH of those run on the module task runner --
  /// ModuleManager posts them there -- while the GUI thread reads the same
  /// containers to build a Settings dialog or open a tab. Deactivating a
  /// module from the Module Controller while a .eml file is being opened was
  /// enough to have one thread erasing from a QMap another was walking.
  ///
  /// A lock rather than marshalling onto the GUI thread: the GUI thread can be
  /// sitting in a nested event loop waiting on the module runner (see
  /// GpgOperaHelper::WaitForOpera), so a blocking queued call in the other
  /// direction would deadlock.
  mutable QReadWriteLock registry_lock_;
  /// Mutable so the const accessor can sync() it; syncing only reconciles with
  /// the backing store, it does not change what this object represents.
  mutable QSettings settings_;
};

auto GF_UI_EXPORT RegisterQObject(QObject* p) -> QString;

auto GF_UI_EXPORT RegisterNamedQObject(const QString& id, QObject* p)
    -> QString;

auto GF_UI_EXPORT FileExtensionEventId(const QString& extension,
                                       const QString& operation) -> QString;

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
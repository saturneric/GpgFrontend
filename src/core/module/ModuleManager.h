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

#include <memory>
#include <optional>

#include "core/function/basic/GpgFunctionObject.h"
#include "core/module/Event.h"
#include "core/module/ModuleEntryBinding.h"
#include "core/module/ModuleHostPolicy.h"
#include "core/module/ModuleManifest.h"
#include "core/struct/settings_object/ModuleSO.h"
#include "core/utils/MemoryUtils.h"

namespace GpgFrontend::Module {

class Event;
class Module;
class ModuleManager;
class GlobalRegisterTable;

using EventReference = QSharedPointer<Event>;
using ModuleIdentifier = QString;
using ModulePtr = QSharedPointer<Module>;

/**
 * @brief Everything the module pipeline established about one module.
 *
 * FACTS ONLY. What was discovered, what was verified, and what the loaded
 * binary reports about itself. It carries no presentation state -- no display
 * strings, no icons, no policy-derived labels, no localized text, no ordering.
 * Those belong to the code that draws it. If a field would only ever be read
 * in order to render something, it does not belong here.
 *
 * It exists because three widgets used to assemble these same facts
 * independently, two of them from a different source than the third: the
 * dialog and the list read GetModuleMetaData(), while the metadata panel read
 * the manifest directly, and `packaged` was answered by IsPackaged() in one
 * place and by manifest.has_value() in another. They agreed only by accident
 * of a setter call.
 *
 * The trust distinction is preserved rather than flattened: @ref manifest
 * having a value is what makes the metadata a host-verified fact rather than
 * the module's own claim, and callers still key that difference off it.
 */
struct GF_CORE_EXPORT ModuleProvenance {
  QString identifier;  ///< the runtime identifier, always present
  QString version;     ///< what the loaded binary reports

  bool integrated = false;
  bool activated = false;

  /// Whether this came from a signed package the host verified.
  bool packaged = false;

  /// The `*.gfmodule` it was verified from, or the loose library's path.
  /// Never the ephemeral path an image was mapped through.
  QString source_package_path;

  /// The signed manifest, when there is one. Its presence is what divides a
  /// verified fact from a module's claim about itself.
  std::optional<ModuleManifest> manifest;

  int sdk_abi = 0;  ///< the module's own ABI generation, never the host's
  QString hash;     ///< digest of the bytes that were loaded

  /// Name / Description / Author. From the verified manifest when packaged,
  /// and from the module itself otherwise -- which is exactly the difference
  /// @ref packaged records.
  QMap<QString, QString> metadata;
};

using Namespace = QString;
using Key = QString;
using LPCallback = std::function<void(Namespace, Key, int, std::any)>;

/**
 * @brief A module that was found and not loaded, and why.
 *
 * Refusals used to be a counter. The reason string was written to the log and
 * dropped, and a refused module simply never appeared in any list -- so the
 * application could start with a module missing and have nothing to say about
 * it beyond a number. `--module-status` had the same hole, in the one place a
 * caller needs it most: its own help text says a caller wants to know WHICH
 * module failed when the command failed.
 *
 * Kept for every origin, not just external. A pending external module and a
 * broken integrated one are both things a person may need to see, and
 * building this for external modules alone would have left the older gap open
 * next to a new mechanism that closed it.
 */
struct GF_CORE_EXPORT ModuleRefusalRecord {
  QString descriptor_path;
  ModuleOrigin origin = ModuleOrigin::kINTEGRATED;

  /// Empty when the descriptor did not parse far enough to have one.
  QString module_id;

  /// What to show. Already a sentence.
  QString reason;

  /// The publisher key an external descriptor carried, when it got far enough
  /// to have one verified. Empty otherwise. Raw bytes; the fingerprint is
  /// derived for display, never stored.
  QByteArray publisher_key;

  /// Waiting on a person rather than broken: the publisher key is untrusted, or
  /// the module was never enabled. The Controller shows these differently,
  /// because one of them has an action attached and the other does not.
  bool pending_user_action = false;
};

/**
 * @brief A module the scan offered, and what preparing it established.
 *
 * Loading happens in two phases, and this is what passes between them. The
 * split is what lets the expensive half run concurrently while the half that
 * maps an image and runs its initialisers stays sequential -- see
 * ModuleManager::PrepareModule().
 */
struct GF_CORE_EXPORT ModuleLoadCandidate {
  QString source_path;  ///< the `*.gfmodule`

  /// Which trust boundary this module crossed, decided by the directory it
  /// was found in and never by anything the module says. See
  /// ModuleHostPolicy.h.
  ModuleOrigin origin = ModuleOrigin::kEXTERNAL;

  bool ok = false;  ///< preparation succeeded; phase 2 may proceed

  /// Where phase 2 maps from: the entry native the descriptor binds, verified
  /// and resolved inside that module's own namespace.
  QString library_path;

  /// The library's name as the signed manifest spells it. Taken from the
  /// manifest rather than from the path, so identity comes from what was signed
  /// rather than from what a directory happens to be called.
  QString library_name;

  std::optional<ModuleManifest> manifest;  ///< always set once @c ok

  /// The digest to record against this module's settings: the manifest's,
  /// already checked against the bytes.
  QString module_hash;

  /// The file @c library_path named while phase one verified it. Phase two
  /// re-checks it immediately before mapping and refuses a changed file.
  ModuleFileIdentity identity;
};

/// Told once a requested activation or deactivation has run, on the thread
/// that asked; @p ok says whether the module ended in the requested state.
using ModuleTransitionCallback = std::function<void(bool ok)>;

/**
 * @brief The module system: the load pipeline, every module's lifecycle
 *        state, event routing, and the runtime value store.
 *
 * ONE lock guards the module records, the event subscriptions and the
 * in-flight triggers, so any thread may ask about them. Module code is never
 * called with it held.
 *
 * Module code runs on the module task runner and only there: activation,
 * deactivation and event delivery are posted to it. A module's lifecycle is
 * Registered -> Activating -> Active -> Deactivating -> Inactive, with
 * Failed for an activation that did not succeed; only an Active (or
 * Activating) module may subscribe, and only an Active one receives events.
 */
class GF_CORE_EXPORT ModuleManager
    : public SingletonFunctionObject<ModuleManager> {
 public:
  explicit ModuleManager(int channel);

  ~ModuleManager() override;

  /**
   * @brief Everything discovered and not loaded, with reasons.
   *
   * Ordered as discovery found it. Safe to call at any time; empty before the
   * first scan.
   */
  auto ListModuleRefusals() -> QList<ModuleRefusalRecord>;

  /**
   * @brief Phase one: verify, without mapping anything.
   *
   * All the expensive work -- reading a package, checking its signature,
   * re-verifying the installed tree -- and none of the dangerous work. It maps
   * no image and runs no module code, so it is safe to run for several
   * modules at once.
   *
   * @param path the `*.gfmodule` the scan found
   * @param origin which namespace it was found in; a trust input, not a
   * label, and the only thing that can say so
   * @param integrated_ids ids of the integrated modules this scan found; an
   * external package may not claim one
   * @return what was established; @c ok is false when it was refused
   */
  auto PrepareModule(const QString& path, ModuleOrigin origin,
                     const QSet<QString>& integrated_ids = {})
      -> ModuleLoadCandidate;

  /**
   * @brief Phase two: map the library and register the module.
   *
   * Deliberately NOT safe to run concurrently with itself.
   * @c QLibrary::load() runs the module's own static initialisers, which are
   * third-party code whose thread-safety against *other modules'* initialisers
   * the host is in no position to establish.
   *
   * @param candidate a prepared candidate whose @c ok is true
   * @return true if the module was loaded and is valid
   */
  auto LoadPreparedModule(const ModuleLoadCandidate& candidate) -> bool;

  /**
   * @brief Register a module that is already loaded, then apply its stored
   *        auto-activation setting.
   *
   * What a test that builds a Module from a table of its own uses; phase two
   * does the same for what it loaded. Posted to the module runner. Not
   * counted towards IsAllModulesRegistered(), which is about the startup
   * scan alone.
   */
  void RegisterLoadedModule(ModulePtr module, bool integrated);

  /// Find a registered module; nullptr when there is none.
  auto SearchModule(ModuleIdentifier module_id) -> ModulePtr;

  /// How many modules the startup scan will try to register.
  void SetNeedRegisterModulesNum(int n);

  /// Every registered module id, sorted.
  auto ListAllRegisteredModuleID() -> QStringList;

  /// Forget every module, returning what was registered. Teardown only.
  auto TakeAllModules() -> QList<ModulePtr>;

  /// Whether every module the startup scan expected has been registered.
  auto IsAllModulesRegistered() -> bool;

  /// Whether the module is Active.
  auto IsModuleActivated(ModuleIdentifier module_id) -> bool;

  /// Whether the module came from the integrated namespace.
  auto IsIntegratedModule(ModuleIdentifier module_id) -> bool;

  /**
   * @brief Everything established about one module, in one place.
   *
   * @return the facts, or a default-constructed value if there is no such
   *         module (its @c identifier is then empty)
   */
  auto GetModuleProvenance(ModuleIdentifier module_id) -> ModuleProvenance;

  /**
   * @brief Subscribe a module to an event.
   *
   * Refused unless the host fires such an event, the module's signed manifest
   * declares it, and the module is Activating or Active.
   *
   * @return whether the subscription was made
   */
  auto ListenEvent(ModuleIdentifier module_id, EventIdentifier event_id)
      -> bool;

  /**
   * @brief Deliver an event to every Active module subscribed to it.
   *
   * Every listener it was delivered to owes exactly one answer, and the
   * event's callback hears each one. A listener that cannot answer -- it was
   * deactivated first, it refused, it failed -- is answered FOR, with
   * `ret` = -1, so a caller waiting on the callback is never left waiting. An
   * event nobody is subscribed to is answered once in the same way.
   */
  void TriggerEvent(EventReference event);

  /**
   * @brief A module's answer to an event it was delivered.
   *
   * Refused -- logged and dropped -- unless @p listener was delivered this
   * trigger and has not answered it yet.
   *
   * @return whether the answer was accepted
   */
  auto AnswerEvent(const EventTriggerIdentifier& trigger_id,
                   const ModuleIdentifier& listener,
                   const Event::Params& params) -> bool;

  /// Triggers still waiting on at least one answer. Diagnostic.
  auto PendingTriggerCount() -> int;

  /// The events a module is subscribed to.
  auto GetModuleListening(ModuleIdentifier module_id) -> QStringList;

  /// Activate a registered module, on the module runner.
  void ActiveModule(ModuleIdentifier module_id,
                    ModuleTransitionCallback done = nullptr);

  /// Deactivate an active module, on the module runner.
  void DeactivateModule(ModuleIdentifier module_id,
                        ModuleTransitionCallback done = nullptr);

  /**
   * @brief Deactivate every Active module, then run every module's final
   *        unregister hook -- in that order, and on the calling thread.
   *
   * Shutdown only. Must run on the module runner, which is what makes it the
   * only thing entering module code while it runs. Grants are kept: the
   * unregister hooks still log through them. The caller revokes them after.
   *
   * @return the ids of every registered module
   */
  auto DeactivateAndUnregisterAllForShutdown() -> QStringList;

  auto UpsertRTValue(Namespace ns, Key key, std::any value) -> bool;

  auto RetrieveRTValue(Namespace ns, Key key) -> std::optional<std::any>;

  /**
   * @brief Subscribe to change notifications for a namespace/key in the
   * register table.
   *
   * @param obj QObject whose lifetime bounds the subscription
   */
  auto ListenRTPublish(QObject* obj, Namespace ns, Key key, LPCallback callback)
      -> bool;

  auto ListRTChildKeys(const QString& ns, const QString& key)
      -> QContainer<Key>;

  /// The register table; never null after construction.
  auto GRT() -> GlobalRegisterTable*;

  /// Whether at least one Active module is subscribed to @p event_id.
  auto IsEventListening(const EventIdentifier& event_id) -> bool;

 private:
  class Impl;
  SecureUniquePtr<Impl> p_;
};

/**
 * @brief Create and dispatch an event via the singleton ModuleManager.
 *
 * @param event_id event type identifier
 * @param params key-value parameters (default: empty)
 * @param e_cb optional callback, told of every answer
 */
inline void TriggerEvent(const EventIdentifier& event_id,
                         const Event::Params& params = {},
                         Event::EventCallback e_cb = nullptr) {
  ModuleManager::GetInstance().TriggerEvent(
      MakeEvent(event_id, params, std::move(e_cb)));
}

/**
 * @brief Reconcile a module's stored settings with the module now loaded.
 *
 * THE policy, used by the loader and by the Module Controller alike. A changed
 * id or hash means a different build: the stored record is refreshed. What
 * the user decided explicitly -- whether it activates automatically -- is
 * kept across a rebuild or an upgrade; only a choice nobody made is reset to
 * the default, which is on for integrated modules and off otherwise.
 *
 * @return the reconciled settings, already stored
 */
auto GF_CORE_EXPORT ReconcileModuleSettings(const QString& module_id,
                                            const QString& module_hash,
                                            bool integrated) -> ModuleSO;

/**
 * @brief Return the directory that should be searched for a module's own
 * dependency libraries while that module is being loaded.
 *
 * A module may ship private shared libraries next to itself. The OS loader
 * only looks at the executable directory and the system directories, so the
 * module directory has to be handed to it explicitly.
 *
 * @param module_library_path absolute path of the module library
 * @return the module's directory in native separators, or an empty string if
 * the path is empty or its directory does not exist
 */
auto GF_CORE_EXPORT
ResolveModuleLibrarySearchPath(const QString& module_library_path) -> QString;

/**
 * @brief Return whether a file name is shaped like a module library.
 *
 * Shared by the module directory scan and by the pre-load gate, so the set of
 * files that are looked for and the set of files that may be loaded cannot
 * drift apart.
 *
 * @param file_name the bare file name, without any directory part
 * @return true if the name may belong to a module library
 */
auto GF_CORE_EXPORT IsModuleLibraryFileName(const QString& file_name) -> bool;

/**
 * @brief Whether a file name may belong to a module package.
 *
 * Separate from IsModuleLibraryFileName() because a package is named for the
 * module rather than for the library inside it: the `libgf_mod_` prefix
 * belongs to the binary, which lives one level down.
 *
 * @param file_name the bare file name, without any directory part
 * @return true if the name may belong to a module package
 */
auto GF_CORE_EXPORT IsModuleDescriptorFileName(const QString& file_name)
    -> bool;

/**
 * @brief Return whether a module with the given identifier is registered.
 *
 * @param module_id module identifier
 * @return true if the module exists in the registry
 */
auto GF_CORE_EXPORT IsModuleExists(ModuleIdentifier module_id) -> bool;

/**
 * @brief Insert or update a value in the singleton ModuleManager's register
 * table.
 *
 * @param namespace_ namespace string
 * @param key key string
 * @param value typed value to store
 * @return true on success
 */
auto GF_CORE_EXPORT UpsertRTValue(const QString& namespace_, const QString& key,
                                  const std::any& value) -> bool;

/**
 * @brief List child keys under a namespace/key node via the singleton
 * ModuleManager.
 *
 * @param namespace_ namespace string
 * @param key parent key string
 * @return list of child key strings
 */
auto GF_CORE_EXPORT ListRTChildKeys(const QString& namespace_,
                                    const QString& key) -> QContainer<Key>;

/**
 * @brief Retrieve a typed value from the singleton register table.
 *
 * @tparam T expected value type
 * @param namespace_ namespace string
 * @param key key string
 * @return the value cast to T, or empty if not found or type does not match
 */
template <typename T>
auto RetrieveRTValueTyped(const QString& namespace_, const QString& key)
    -> std::optional<T> {
  auto any_value =
      ModuleManager::GetInstance().RetrieveRTValue(namespace_, key);
  if (any_value && any_value->type() == typeid(T)) {
    return std::any_cast<T>(*any_value);
  }
  return std::nullopt;
}

/**
 * @brief Retrieve a typed value from the singleton register table, returning a
 * default if absent.
 *
 * @tparam T expected value type
 * @param namespace_ namespace string
 * @param key key string
 * @param defaultValue value to return if not found or type does not match
 * @return the stored value cast to T, or @p defaultValue
 */
template <typename T>
auto RetrieveRTValueTypedOrDefault(const QString& namespace_,
                                   const QString& key, const T& defaultValue)
    -> T {
  auto any_value =
      ModuleManager::GetInstance().RetrieveRTValue(namespace_, key);
  if (any_value && any_value->type() == typeid(T)) {
    return std::any_cast<T>(*any_value);
  }
  return defaultValue;
}

/**
 * @brief Return whether any module is listening for the given event trigger.
 *
 * @param trigger_id event trigger identifier
 * @return true if at least one module is subscribed
 */
auto GF_CORE_EXPORT IsEventListening(const EventTriggerIdentifier& trigger_id)
    -> bool;

}  // namespace GpgFrontend::Module

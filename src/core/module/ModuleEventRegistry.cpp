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

#include "ModuleEventRegistry.h"

#include <QRegularExpression>

namespace GpgFrontend::Module {

namespace {

constexpr auto kCore = ModuleEventLayer::kCORE;
constexpr auto kUi = ModuleEventLayer::kUI;
constexpr auto kObserve = ModuleEventSemantics::kOBSERVE;
constexpr auto kExtend = ModuleEventSemantics::kEXTEND;

/**
 * @brief The catalog.
 *
 * Seeded from what the Host actually fires, not from what would be tidy. The
 * honest finding while writing it down was that the extension points already
 * existed and had simply never been labelled -- the six `EDIT_TAB_TYPE_*`
 * operations hand their reply straight to the user's document. Those were
 * always extension points. Now they say so.
 *
 * Nothing here is a veto. No trigger site cancels an operation on a module's
 * word, TriggerEvent is asynchronous and returns void, and inventing one
 * would be designing a contract with no consumer.
 */
auto Catalog() -> const QList<ModuleEventSpec>& {
  static const QList<ModuleEventSpec> kCatalog = {
      // ---- lifecycle, core ------------------------------------------------
      {"APPLICATION_LOADED", kCore, kObserve, 0,
       "the application has finished starting; the main window exists"},
      {"KEY_DATABASE_REFRESH_DONE", kCore, kObserve, 0,
       "the key database finished reloading"},
      {"SETTINGS_CHANGED", kCore, kObserve, 0,
       "every settings page has been applied"},

      // Two hooks that were planned and deliberately NOT added.
      //
      // "KEYRING_CHANGED" would have duplicated KEY_DATABASE_REFRESH_DONE,
      // which already fires whenever the key database is reloaded after an
      // import, deletion or edit. Two names for one fact is how a module
      // comes to subscribe to the wrong one.
      //
      // "GPG_CONTEXT_CHANGED" has no single point that is true of it: the
      // active channel changes in several places and an engine change is a
      // restart. Declaring an event nothing fires is worse than not having
      // it -- a module would subscribe and wait forever, and the registry
      // would have said the subscription was fine.

      // ---- lifecycle, ui --------------------------------------------------
      {"MAIN_WINDOW_CLOSING", kUi, kObserve, 0,
       "the main window is about to close; not a veto"},
      {"SETTINGS_DIALOG_OPENED", kUi, kObserve, 0,
       "the settings dialog was opened"},
      {"SETTINGS_DIALOG_CLOSED", kUi, kObserve, 0,
       "the settings dialog was closed"},

      // ---- documents and tabs, ui ----------------------------------------
      {"TAB_CREATED", kUi, kObserve, 0, "an editor tab was created"},
      {"TAB_ACTIVATED", kUi, kObserve, 0,
       "an editor tab became the current one"},
      {"TAB_CLOSED", kUi, kObserve, 0, "an editor tab was closed"},
      {"DOCUMENT_OPENED", kUi, kObserve, 0,
       "a document was loaded into a tab from a file"},
      {"DOCUMENT_CHANGED", kUi, kObserve, 0,
       "a document's content was modified"},
      {"DOCUMENT_SAVED", kUi, kObserve, 0, "a document was written to a file"},

      // ---- no gui extension points ---------------------------------------
      //
      // The events that handed a module the Host's own menus, layouts, tabs
      // and settings pages are gone, and nothing replaces them here: a
      // module's UI is its script (anchors, actions, mounts) and its own
      // native widgets, and no Host object crosses to a module at all.

      // ---- key server requests --------------------------------------------
      {"REQUEST_GET_PUBLIC_KEY_BY_FINGERPRINT", kUi, kExtend,
       GF_EVENT_REPLY_CONSUMED | GF_EVENT_DEFERRABLE,
       "fetch a key; the reply's key_data is imported"},
      {"REQUEST_GET_PUBLIC_KEY_BY_KEY_ID", kUi, kExtend,
       GF_EVENT_REPLY_CONSUMED | GF_EVENT_DEFERRABLE,
       "fetch a key by key id; the reply's key_data is imported"},
      {"REQUEST_SEARCH_PUBLIC_KEY_BY_FINGERPRINT", kUi, kObserve,
       GF_EVENT_DEFERRABLE,
       "open your own key search UI; the reply is not read"},
      {"REQUEST_UPLOAD_PUBLIC_KEY", kUi, kExtend,
       GF_EVENT_REPLY_CONSUMED | GF_EVENT_DEFERRABLE,
       "upload a key; the reply's ret and error_msg are shown to the user"},
      {"REQUEST_GATHERING_ALL_GNUPG_INFO", kCore, kObserve, GF_EVENT_DEFERRABLE,
       "collect GnuPG environment information into the register table"},

      // ---- generated family -----------------------------------------------
      //
      // Built from a runtime value -- a document type -- so it cannot be
      // listed. The pattern is what a module subscribes against and what the
      // Host checks a subscription with. Data only: the document's bytes in,
      // the result's bytes back. Opening and saving a module's document type
      // is the Host's own work now (an editor mount, and its `extensions`).
      {"EDIT_TAB_TYPE_<TYPE>_OP_{ENCRYPT|DECRYPT|SIGN|VERIFY|ENCRYPT_SIGN|"
       "DECRYPT_VERIFY}",
       kUi, kExtend,
       GF_EVENT_PATTERN | GF_EVENT_REPLY_CONSUMED | GF_EVENT_DEFERRABLE,
       "a crypto operation on a module's document type; the reply replaces "
       "the document"},
  };
  return kCatalog;
}

/// Turn a pattern into a regular expression: `<...>` becomes one segment,
/// and `{A|B}` exactly one of the listed literals.
auto PatternToRegex(const QString& pattern) -> QRegularExpression {
  QString expr = "^";
  qsizetype i = 0;
  while (i < pattern.size()) {
    if (pattern[i] == '<') {
      const auto close = pattern.indexOf('>', i);
      if (close < 0) break;
      // A placeholder stands for one upper-case identifier segment, which is
      // the only shape the Host ever substitutes there.
      expr += "[A-Z][A-Z0-9_]*";
      i = close + 1;
      continue;
    }
    if (pattern[i] == '{') {
      const auto close = pattern.indexOf('}', i);
      if (close < 0) break;
      QStringList alternatives;
      for (const auto& literal :
           pattern.mid(i + 1, close - i - 1).split('|')) {
        alternatives << QRegularExpression::escape(literal);
      }
      expr += "(?:" + alternatives.join('|') + ")";
      i = close + 1;
      continue;
    }
    expr += QRegularExpression::escape(pattern.mid(i, 1));
    ++i;
  }
  expr += "$";
  return QRegularExpression(expr);
}

}  // namespace

auto ModuleEventCatalog() -> const QList<ModuleEventSpec>& { return Catalog(); }

auto FindModuleEventSpec(const QString& event_id) -> const ModuleEventSpec* {
  if (event_id.isEmpty()) return nullptr;

  // Exact matches first. A literal must win over a pattern it happens to fit,
  // because the literal is the one that carries the real contract.
  for (const auto& spec : Catalog()) {
    if ((spec.flags & GF_EVENT_PATTERN) != 0) continue;
    if (event_id == QLatin1String(spec.id)) return &spec;
  }

  for (const auto& spec : Catalog()) {
    if ((spec.flags & GF_EVENT_PATTERN) == 0) continue;
    // Compiled once per pattern: this runs on every subscription and, for the
    // generated families, on ids built per tab type.
    static QHash<QString, QRegularExpression> cache;
    const auto key = QString::fromLatin1(spec.id);
    auto it = cache.find(key);
    if (it == cache.end()) it = cache.insert(key, PatternToRegex(key));
    if (it->match(event_id).hasMatch()) return &spec;
  }

  return nullptr;
}

auto IsKnownModuleEvent(const QString& event_id) -> bool {
  return FindModuleEventSpec(event_id) != nullptr;
}

auto IsModuleEventExtensionPoint(const QString& event_id) -> bool {
  const auto* spec = FindModuleEventSpec(event_id);
  return spec != nullptr && spec->semantics == ModuleEventSemantics::kEXTEND;
}

}  // namespace GpgFrontend::Module

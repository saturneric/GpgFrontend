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

#include <QByteArray>
#include <QMap>
#include <QObject>
#include <QSharedPointer>
#include <QString>
#include <atomic>

/**
 * @file GFModuleEvent.h
 * @brief What a module receives when an event fires, and what it answers with.
 *
 * Replaces the CB/CB_SUCC/CB_ERR macros, which between them wrote the answer
 * transport into 83 call sites -- 64 of them passing the same error code, -1,
 * because the code never carried information. A handler now returns a value
 * and the runtime does the transport.
 */

class GFEventAnswer;
class GFEventFactory;
struct GFEventData;

/// Why a handler finished the way it did.
///
/// Finer than the wire, deliberately: the wire has only 0 and -1 and nothing
/// reads the difference, but a log line and a test both benefit from knowing
/// whether a handler refused a malformed request or tried and failed.
enum class GFEventStatus {
  kOK,           ///< handled
  kBAD_REQUEST,  ///< a parameter was missing or malformed
  kUNAVAILABLE,  ///< a host object or capability this hook needs is absent
  kFAILED,       ///< the work was attempted and did not succeed
  kDEFERRED,     ///< answered later through GFEventAnswer; do NOT auto-answer
};

/// A handler's verdict. The house shape: {ok, status, reason}.
struct GFEventResult {
  bool ok = false;
  GFEventStatus status = GFEventStatus::kFAILED;

  /// Becomes the "err" parameter of the answer. Ignored when @ref ok.
  QString reason;

  /// Extra parameters to send alongside "ret" (and "err").
  QMap<QString, QString> params;

  static auto Ok(QMap<QString, QString> params = {}) -> GFEventResult {
    return {true, GFEventStatus::kOK, {}, std::move(params)};
  }
  static auto Bad(QString reason) -> GFEventResult {
    return {false, GFEventStatus::kBAD_REQUEST, std::move(reason), {}};
  }
  static auto Unavailable(QString reason) -> GFEventResult {
    return {false, GFEventStatus::kUNAVAILABLE, std::move(reason), {}};
  }
  static auto Fail(QString reason) -> GFEventResult {
    return {false, GFEventStatus::kFAILED, std::move(reason), {}};
  }

  /// The handler has taken a GFEventAnswer and will answer later, possibly
  /// from another thread. The runtime sends nothing.
  static auto Deferred() -> GFEventResult {
    return {true, GFEventStatus::kDEFERRED, {}, {}};
  }
};

/**
 * @brief Answers one event after its handler has returned.
 *
 * Mandatory, not a convenience: handlers routinely start asynchronous work and
 * answer from its callback, long after returning. Copyable, safe to capture
 * into a lambda, safe to use from any thread, and idempotent -- a second
 * answer is dropped and logged, because the host frees the parameters it is
 * given and answering twice would be a double free.
 *
 * It holds the event and trigger ids only. It never holds the GFModuleEvent,
 * which the runtime has already reclaimed by the time a handler returns.
 */
class GFEventAnswer {
 public:
  void Ok(QMap<QString, QString> params = {}) const;
  void Fail(const QString& reason, QMap<QString, QString> params = {}) const;

  /// Send a result built elsewhere. A kDEFERRED result is refused and logged:
  /// deferring twice is a promise nobody kept.
  void Send(const GFEventResult& result) const;

  [[nodiscard]] auto Answered() const -> bool;

 private:
  friend class GFEvent;
  friend class GFEventFactory;

  struct State {
    QString event_id;
    QString trigger_id;
    /// Not atomic_flag: its test() is C++20, and this tree builds as C++17.
    std::atomic<bool> sent{false};
  };
  QSharedPointer<State> s_;
};

/**
 * @brief One delivered event.
 *
 * A value type. Cheap to copy and safe to outlive the handler, which is what
 * the deferred-answer case requires.
 *
 * Parameter values are held as the octets they arrived as. The old transport
 * flattened everything into QMap<QString,QString>, so binary payloads had to
 * be base64-encoded by the sender and decoded by the receiver; Bytes() removes
 * that round trip and with it a class of corruption that only shows up on
 * non-UTF-8 input.
 */
class GFEvent {
 public:
  [[nodiscard]] auto Id() const -> QString;
  [[nodiscard]] auto TriggerId() const -> QString;

  [[nodiscard]] auto Has(const QString& key) const -> bool;
  [[nodiscard]] auto Str(const QString& key) const -> QString;
  [[nodiscard]] auto Int(const QString& key, int fallback = 0) const -> int;

  /// The exact octets, with no text decoding anywhere on the path.
  [[nodiscard]] auto Bytes(const QString& key) const -> QByteArray;

  /// Every parameter, as text. For the rare handler that wants to iterate.
  [[nodiscard]] auto Params() const -> QMap<QString, QString>;

  /**
   * @brief Read a parameter that must be present and non-empty.
   *
   * @param key the parameter name
   * @param out set on success, untouched on failure
   * @return an OK result, or the failure to return from the handler
   */
  [[nodiscard]] auto Require(const QString& key, QString& out) const
      -> GFEventResult;

  /**
   * @brief Read a required GUI handle and cast it to the type it must be.
   *
   * Collapses the guard-cast-check-report sequence that appears three times
   * near-identically across the modules, and a dozen more times inside
   * m_email, into one line per object.
   */
  template <typename T>
  [[nodiscard]] auto RequireGui(const QString& key, T*& out) const
      -> GFEventResult {
    QObject* object = nullptr;
    if (auto r = require_gui_object(key, object); !r.ok) return r;
    out = qobject_cast<T*>(object);
    if (out == nullptr) return gui_object_wrong_type(key);
    return GFEventResult::Ok();
  }

  /// A handle that answers this event later, at most once.
  [[nodiscard]] auto Answer() const -> GFEventAnswer;

 private:
  friend class GFEventFactory;

  [[nodiscard]] auto require_gui_object(const QString& key, QObject*& out) const
      -> GFEventResult;
  [[nodiscard]] static auto gui_object_wrong_type(const QString& key)
      -> GFEventResult;

  QSharedPointer<const GFEventData> d_;
};

/// One event id bound to one function. Written in C++; never in JSON.
using GFEventHook = GFEventResult (*)(const GFEvent&);

struct GFEventBinding {
  const char* event_id;  ///< UPPER-CASE, must match the manifest's events[]
  GFEventHook handler;
};

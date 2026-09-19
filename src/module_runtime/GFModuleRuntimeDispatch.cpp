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

#include "GFModuleRuntimeDispatch.h"

#include <GFSDKBasic.h>
#include <GFSDKModule.h>

#include <GFSDKUI.h>

#include <atomic>

#include "GFModuleRuntimeBoot.h"
#include "include/GFModuleLog.h"
#include "include/GFModuleMemory.h"

namespace gf::runtime {

namespace {

/// Take a parameter list, freeing every node and buffer in it.
auto ConsumeParams(GFModuleEventParam* params) -> QMap<QString, QByteArray> {
  QMap<QString, QByteArray> out;

  GFModuleEventParam* current = params;
  while (current != nullptr) {
    // UDUPN keeps the octets; the value is NOT read as text anywhere here.
    const auto name = UDUP(current->name);
    auto value = current->value == nullptr
                     ? QByteArray()
                     : QByteArray(current->value,
                                  static_cast<qsizetype>(
                                      qstrlen(current->value)));
    GFSecFreeMemory(const_cast<char*>(current->value));

    if (!name.isEmpty()) out.insert(name, value);

    auto* done = current;
    current = current->next;
    GFFreeMemory(done);
  }

  return out;
}

}  // namespace

}  // namespace gf::runtime

auto GFEventFactory::Consume(GFModuleEvent* event) -> GFEvent {
  GFEvent wrapped;
  auto data = QSharedPointer<GFEventData>::create();

  if (event != nullptr) {
    data->id = UDUP(event->id);
    data->trigger_id = UDUP(event->trigger_id);
    data->params = gf::runtime::ConsumeParams(event->params);
    GFFreeMemory(event);
  }

  wrapped.d_ = data;
  return wrapped;
}

namespace gf::runtime {

auto ResultToParams(const GFEventResult& result) -> QMap<QString, QString> {
  auto params = result.params;

  if (result.ok && result.status == GFEventStatus::kOK) {
    params.insert("ret", "0");
    return params;
  }

  // One failure code on the wire, because that is all there has ever been and
  // nothing reads it. The status enum's extra resolution is for the log.
  params.insert("ret", "-1");
  params.insert("err", result.reason);
  return params;
}

void SendAnswer(const QString& event_id, const QString& trigger_id,
                const QMap<QString, QString>& params) {
  auto* reply =
      static_cast<GFModuleEvent*>(GFAllocateMemory(sizeof(GFModuleEvent)));
  reply->id = DUP(event_id.toUtf8());
  reply->trigger_id = DUP(trigger_id.toUtf8());
  reply->params = nullptr;

  GFModuleEventParam* head = nullptr;
  GFModuleEventParam* prev = nullptr;
  for (auto it = params.keyValueBegin(); it != params.keyValueEnd(); ++it) {
    auto* node = static_cast<GFModuleEventParam*>(
        GFAllocateMemory(sizeof(GFModuleEventParam)));
    node->name = DUP(it->first.toUtf8());
    node->value = SECDUP(it->second.toUtf8());
    node->next = nullptr;

    if (prev == nullptr) {
      head = node;
    } else {
      prev->next = node;
    }
    prev = node;
  }

  // The host frees all of it, on both the found and not-found paths.
  GFModuleTriggerModuleEventCallback(
      reply, Facts().id.toUtf8().constData(), head);
}

auto HookTable() -> QHash<QString, GFEventHook>& {
  static QHash<QString, GFEventHook> table;
  return table;
}

}  // namespace gf::runtime

// ----------------------------------------------------------------- GFEvent

auto GFEvent::Id() const -> QString { return d_ ? d_->id : QString(); }

auto GFEvent::TriggerId() const -> QString {
  return d_ ? d_->trigger_id : QString();
}

auto GFEvent::Has(const QString& key) const -> bool {
  return d_ && d_->params.contains(key);
}

auto GFEvent::Str(const QString& key) const -> QString {
  if (!d_) return {};
  const auto it = d_->params.constFind(key);
  return it == d_->params.constEnd() ? QString()
                                     : QString::fromUtf8(it.value());
}

auto GFEvent::Int(const QString& key, int fallback) const -> int {
  bool ok = false;
  const auto value = Str(key).toInt(&ok);
  return ok ? value : fallback;
}

auto GFEvent::Bytes(const QString& key) const -> QByteArray {
  if (!d_) return {};
  return d_->params.value(key);
}

auto GFEvent::Params() const -> QMap<QString, QString> {
  QMap<QString, QString> out;
  if (!d_) return out;
  for (auto it = d_->params.constBegin(); it != d_->params.constEnd(); ++it) {
    out.insert(it.key(), QString::fromUtf8(it.value()));
  }
  return out;
}

auto GFEvent::Require(const QString& key, QString& out) const
    -> GFEventResult {
  if (!Has(key)) {
    return GFEventResult::Bad(QString("no %1 in this event").arg(key));
  }
  auto value = Str(key);
  if (value.isEmpty()) {
    return GFEventResult::Bad(QString("%1 is empty").arg(key));
  }
  out = std::move(value);
  return GFEventResult::Ok();
}

auto GFEvent::require_gui_object(const QString& key, QObject*& out) const
    -> GFEventResult {
  QString handle;
  if (auto r = Require(key, handle); !r.ok) return r;

  out = static_cast<QObject*>(GFUIGetGUIObject(handle.toUtf8().constData()));
  if (out == nullptr) {
    return GFEventResult::Unavailable(
        QString("%1 handle is not a live object").arg(key));
  }
  return GFEventResult::Ok();
}

auto GFEvent::gui_object_wrong_type(const QString& key) -> GFEventResult {
  return GFEventResult::Unavailable(
      QString("%1 is not of the expected type").arg(key));
}

auto GFEvent::Answer() const -> GFEventAnswer {
  GFEventAnswer answer;
  answer.s_ = QSharedPointer<GFEventAnswer::State>::create();
  answer.s_->event_id = Id();
  answer.s_->trigger_id = TriggerId();
  return answer;
}

// ----------------------------------------------------------- GFEventAnswer

auto GFEventAnswer::Answered() const -> bool {
  return s_ && s_->sent.load();
}

void GFEventAnswer::Send(const GFEventResult& result) const {
  if (!s_) return;

  if (result.status == GFEventStatus::kDEFERRED) {
    LOG_ERROR("a deferred answer cannot itself be deferred; event: " +
              s_->event_id);
    return;
  }

  // At most once. The host frees the parameters it is handed, so a second
  // answer would be a second free of a trigger that is already gone.
  if (s_->sent.exchange(true)) {
    LOG_ERROR("event answered more than once, ignoring: " + s_->event_id);
    return;
  }

  gf::runtime::SendAnswer(s_->event_id, s_->trigger_id,
                          gf::runtime::ResultToParams(result));
}

void GFEventAnswer::Ok(QMap<QString, QString> params) const {
  Send(GFEventResult::Ok(std::move(params)));
}

void GFEventAnswer::Fail(const QString& reason,
                         QMap<QString, QString> params) const {
  auto result = GFEventResult::Fail(reason);
  result.params = std::move(params);
  Send(result);
}

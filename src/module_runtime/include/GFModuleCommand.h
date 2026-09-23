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

#include <QObject>
#include <QPointer>
#include <functional>
#include <optional>

#include "GFSDKCommand.h"
#include "GFSDKCommand.hpp"

/**
 * @file GFModuleCommand.h
 * @brief Invoking commands from a module, in C++.
 *
 * @code
 * // fire and forget
 * Commands().Invoke<gf::cmd::host::DocumentNew>({"email", "untitled.eml"});
 *
 * // with the typed result, delivered on `this`'s thread
 * Commands().Invoke<gf::cmd::host::DocumentOpen>(args, this,
 *     [](const gf::cmd::Outcome<gf::cmd::host::DocumentOpen::Result>& r) {
 *       if (!r.Ok()) LOG_WARN(r.error);
 *     });
 * @endcode
 *
 * Nothing here blocks. A result arrives later, on the receiver's thread, and
 * never after the receiver has been destroyed or the module deactivated.
 * The commands a module PROVIDES are listed in its hook table with
 * gf::cmd::Bind<>(); the runtime registers and withdraws them.
 */

namespace gf::cmd {

/// What Invoke() says straight away. The outcome comes later, if asked for.
struct CallTicket {
  int status = GF_CMD_OK;
  quint64 call_id = 0;

  [[nodiscard]] auto Ok() const -> bool { return status == GF_CMD_OK; }
};

/// Secret bytes to pass as a Blob argument. Copied once, into memory the
/// Host wipes; the caller's own copy is the caller's to wipe.
auto MakeBlob(const void* data, size_t size) -> Blob;

class CommandBus {
 public:
  using RawFn = std::function<void(const RawResult&)>;

  /// Fire and forget.
  template <typename C>
  auto Invoke(const typename C::Args& args) -> CallTicket {
    EncodeState st;
    auto m = EncodeMap(args, st);
    return InvokeRaw(C::kMeta.id, std::move(m), std::move(st.blobs), nullptr,
                     {});
  }

  /// With the typed outcome, delivered on @p receiver's thread.
  template <typename C, typename Fn>
  auto Invoke(const typename C::Args& args, QObject* receiver, Fn&& fn)
      -> CallTicket {
    using R = typename C::Result;
    EncodeState st;
    auto m = EncodeMap(args, st);
    return InvokeRaw(
        C::kMeta.id, std::move(m), std::move(st.blobs), receiver,
        [fn = std::forward<Fn>(fn)](const RawResult& raw) {
          if (raw.status != GF_CMD_OK) {
            fn(Outcome<R>::Failure(raw.status, raw.error));
            return;
          }
          R value{};
          QString error;
          if (!DecodeMap(raw.result, raw.blobs, value, &error)) {
            fn(Outcome<R>::Failure(GF_CMD_E_FAILED,
                                   QStringLiteral("malformed result: ") + error));
            return;
          }
          fn(Outcome<R>::Success(std::move(value)));
        });
  }

  /// By stable id, for commands this module knows only by name.
  auto InvokeDynamic(const QString& id, const QCborMap& args,
                     QObject* receiver = nullptr, RawFn fn = {})
      -> CallTicket;

  auto Cancel(quint64 call_id) -> int;
  auto Describe(const QString& id) -> std::optional<QCborMap>;
  auto List(const QString& prefix = {}) -> QStringList;

  /// The raw form everything above ends in.
  auto InvokeRaw(const char* id, QCborMap args, std::vector<Blob> blobs,
                 QObject* receiver, RawFn fn) -> CallTicket;
};

}  // namespace gf::cmd

/// The module's command bus.
auto Commands() -> gf::cmd::CommandBus&;

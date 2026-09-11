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

#include "core/typedef/CoreTypedef.h"
#include "core/typedef/GpgTypedef.h"
#include "ui/struct/GpgOperaResult.h"

namespace GpgFrontend::UI {

using OperaWaitingHd = std::function<void()>;
using OperaWaitingCb = std::function<void(OperaWaitingHd)>;

struct GpgOperaCategory {
  QStringList paths;
  QStringList o_paths;
  QContainer<GFBuffer> buffers;
};

struct GpgOperaContext;

struct GF_UI_EXPORT GpgOperaContextBasement {
  // Each opera captures the GpgOperaContext it was built from, and that context
  // holds a strong reference back to this basement — so this container owns a
  // cycle. Whoever runs the operas must clear it once they are done, or the
  // basement (with its keys and buffers) is never released.
  QContainer<OperaWaitingCb> operas;
  QContainer<GpgOperaResult> opera_results;
  GpgAbstractKeyPtrList keys;
  GpgAbstractKeyPtrList singer_keys;
  QStringList unknown_fprs;
  bool ascii;

  QMap<int, GpgOperaCategory> categories;

  auto GetContextPath(int category) -> QStringList&;

  auto GetContextOutPath(int category) -> QStringList&;

  auto GetContextBuffer(int category) -> QContainer<GFBuffer>&;

  auto GetAllPath() -> QStringList;

  auto GetAllOutPath() -> QStringList;
};

struct GpgOperaContext {
  QSharedPointer<GpgOperaContextBasement> base;

  QStringList paths;
  QStringList o_paths;
  QContainer<GFBuffer> buffers;

  explicit GpgOperaContext(QSharedPointer<GpgOperaContextBasement> base);
};

auto GF_UI_EXPORT GetGpgOperaContextFromBasement(
    const QSharedPointer<GpgOperaContextBasement>& base, int category)
    -> QSharedPointer<GpgOperaContext>;

/**
 * @brief Owns a basement and guarantees its opera cycle is broken.
 *
 * The cycle described on GpgOperaContextBasement has to be cut by hand, and
 * that worked only as long as every path out of a slot remembered to do it —
 * one did not, and every instant message decrypted in that session stayed in
 * memory. This makes the release a property of the scope instead, so a return
 * added above the wait, or a wait that ends without a clear, still frees the
 * keys and buffers below.
 *
 * Safe while operations are still running. The starters queued by
 * WaitForMultipleOperas capture by value, so each holds its own copy of the
 * callback and therefore its own strong reference; clearing the container here
 * cannot strand a callback that is still in flight. The basement simply lives
 * until the last queued copy is gone.
 *
 * Deliberately does not Zeroize the buffers: GFBuffer::Zeroize() wipes through
 * every copy-on-write share, and results are handed on to the editor and the
 * info board. Breaking the cycle lets the last share die, and GFBuffer erases
 * its storage when it does.
 */
class GF_UI_EXPORT GpgOperaContextHolder {
 public:
  GpgOperaContextHolder();
  ~GpgOperaContextHolder();

  GpgOperaContextHolder(const GpgOperaContextHolder&) = delete;
  auto operator=(const GpgOperaContextHolder&)
      -> GpgOperaContextHolder& = delete;
  GpgOperaContextHolder(GpgOperaContextHolder&&) = delete;
  auto operator=(GpgOperaContextHolder&&) -> GpgOperaContextHolder& = delete;

  auto operator->() const -> GpgOperaContextBasement*;

  /**
   * @brief The owned basement, for the helpers that take it by shared pointer.
   */
  [[nodiscard]] auto Base() const
      -> const QSharedPointer<GpgOperaContextBasement>&;

 private:
  QSharedPointer<GpgOperaContextBasement> base_;
};

}  // namespace GpgFrontend::UI
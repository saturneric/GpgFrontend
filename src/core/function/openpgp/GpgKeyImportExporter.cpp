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

#include "GpgKeyImportExporter.h"

#include "core/function/openpgp/Async.h"
#include "core/function/openpgp/traits/KeyImportExportTraits.h"
#include "core/model/GpgImportInformation.h"

namespace GpgFrontend {

GpgKeyImportExporter::GpgKeyImportExporter(int channel)
    : SingletonFunctionObject<GpgKeyImportExporter>(channel),
      ctx_(GpgContext::GetInstance(SingletonFunctionObject::GetChannel())) {}

auto GpgKeyImportExporter::ImportKey(const GFBuffer& in_buffer)
    -> QSharedPointer<GpgImportInformation> {
  return RunRegisteredForward<ImportKeyOpTag>(ctx_, in_buffer);
}

auto GpgKeyImportExporter::ImportRevCert(const GFBuffer& in_buffer)
    -> QSharedPointer<GpgImportInformation> {
  return RunRegisteredForward<ImportRevCertOpTag>(ctx_, in_buffer);
}

auto GpgKeyImportExporter::ExportKey(const GpgAbstractKeyPtr& key, bool secret,
                                     bool ascii, bool shortest,
                                     bool ssh_mode) const
    -> std::tuple<GpgError, GFBuffer> {
  return RunRegisteredForward<ExportKeysOpTag>(
      ctx_, GpgAbstractKeyPtrList{key}, secret, ascii, shortest, ssh_mode);
}

void GpgKeyImportExporter::ExportKeys(const GpgAbstractKeyPtrList& keys,
                                      bool secret, bool ascii, bool shortest,
                                      bool ssh_mode,
                                      const GpgOperationCallback& cb) const {
  RunRegisteredAsync<ExportKeysAsyncOpTag>(ctx_, cb, keys, secret, ascii,
                                           shortest, ssh_mode);
}

void GpgKeyImportExporter::ExportAllKeys(const GpgAbstractKeyPtrList& keys,
                                         bool secret, bool ascii,
                                         const GpgOperationCallback& cb) const {
  RunRegisteredAsync<ExportAllKeysOpTag>(ctx_, cb, keys, secret, ascii);
}

auto GpgKeyImportExporter::ExportSubkey(const QString& fpr, bool ascii) const
    -> std::tuple<GpgError, GFBuffer> {
  return RunRegisteredForward<ExportSubkeyOpTag>(ctx_, fpr, ascii);
}
}  // namespace GpgFrontend

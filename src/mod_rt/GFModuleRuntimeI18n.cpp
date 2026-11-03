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

#include "GFModuleRuntimeI18n.h"

#include <GFSDKBuffer.h>

#include <QFile>
#include <QString>

#include "GFModuleRuntimeBoot.h"
#include "include/GFModuleLog.h"
#include "include/GFModuleMemory.h"

namespace gf::runtime {

namespace {

/**
 * @brief Hand the host this module's compiled translations for one locale.
 *
 * The host calls this once per installed locale. It borrows @p locale and
 * takes ownership of the buffer written to @p data, copying it and freeing
 * ours immediately -- so this allocates from the SDK arena and forgets about
 * it, which is the contract.
 *
 * @return the byte count, which the host reads as the size, not as a status
 */
auto ReadTranslation(const char* locale, char** data) -> int {
  const auto requested = QString::fromUtf8(locale == nullptr ? "" : locale);
  const auto& context = Facts().translation_context;

  if (data == nullptr) return 0;
  *data = nullptr;

  if (context.isEmpty()) {
    LOG_WARN("no translation context for this module; translations disabled");
    return 0;
  }

  QFile file(QString(":/i18n/%1.%2.qm").arg(context, requested));
  if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
    // Routine: a module ships a subset of locales and the host asks for
    // whatever the user has. Not an error, and not worth a warning each time.
    LOG_DEBUG("no translations for locale " + requested + " in " +
              file.fileName());
    return 0;
  }

  const auto bytes = file.readAll();
  *data = AllocBufferAndCopy(bytes);
  return static_cast<int>(bytes.size());
}

}  // namespace

auto RegisterTranslations() -> bool {
  const auto& facts = Facts();
  if (facts.id.isEmpty()) return false;

  // Straight to the bootstrap primitive: installing a module's translations
  // is runtime plumbing, not an application capability, so it is not part of
  // the public SDK a module author sees.
  auto* ctx = gf::runtime::SdkContext();
  if (ctx == nullptr || ctx->host == nullptr ||
      ctx->host->bootstrap == nullptr) {
    return false;
  }
  return ctx->host->bootstrap->register_translator_reader(
             ctx->host->context, facts.id.toUtf8().constData(),
             &ReadTranslation) == 0;
}

}  // namespace gf::runtime

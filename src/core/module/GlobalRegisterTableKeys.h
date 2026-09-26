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

/**
 * @file GlobalRegisterTableKeys.h
 * @brief The keys the host itself keeps up to date in the register table.
 *
 * Everything here lives in the "core" namespace, which only the host writes.
 * Keys are lower case throughout: the SDK lower-cases every module lookup, so
 * a key with a capital in it could be published but never read back.
 *
 *   engine.<gnupg|rpgp>.supported      bool     engine usable this session
 *   engine.<gnupg|rpgp>.version        text     engine version
 *   engine.default                     text     engine new work goes to
 *   modules.<seg>.state                text     lifecycle state name
 *   modules.<seg>.integrated           bool     shipped with the application
 *   modules.<seg>.listening            int      events it listens to
 *   modules.<seg>.version              text     what the loaded binary reports
 *   modules.<seg>.packaged             bool     from a signed package
 *   modules.<seg>.refusal              text     why loading it was refused
 *   commands.<seg>.owner               text     module (or "host") providing it
 *   commands.<seg>.title               text     untranslated descriptor title
 *   commands.<seg>.flags               int      GF command flags
 *   commands.<seg>.caps                text     capabilities it requires
 *   stats.events.<event>.fired         int      times triggered
 *   stats.events.<event>.unheard       int      times no active module listened
 *   stats.commands.<seg>.<outcome>     int      invoked/refused/finished/
 *                                               failed/cancelled
 *   stats.operations.<op>.run          int      OpenPGP operations started
 *   stats.operations.<op>.failed       int      ...and those that errored
 *   stats.module_load.<field>          int      loaded/refused/hashed_bytes/
 *                                               peak_native
 *
 * A module or command id holds dots, and the table splits keys on dots, so an
 * id is folded into ONE path segment (<seg>) with GRTSegment(). Without that,
 * "a.b" and "a.b.c" would land as parent and child of each other.
 */

namespace GpgFrontend::Module {

/// The namespace the host publishes into; modules may read it, never write.
inline const QString kGRTCoreNamespace = QStringLiteral("core");

/**
 * @brief Fold an id into one lower-case register table path segment.
 *
 * Dots become '/', which the table does not split on.
 */
inline auto GRTSegment(const QString& id) -> QString {
  return id.toLower().replace('.', '/');
}

inline auto GRTEngineKey(const QString& engine, const QString& field)
    -> QString {
  return QStringLiteral("engine.%1.%2").arg(GRTSegment(engine), field);
}

inline auto GRTDefaultEngineKey() -> QString {
  return QStringLiteral("engine.default");
}

inline auto GRTModuleKey(const QString& module_id) -> QString {
  return QStringLiteral("modules.%1").arg(GRTSegment(module_id));
}

inline auto GRTModuleKey(const QString& module_id, const QString& field)
    -> QString {
  return GRTModuleKey(module_id) + "." + field;
}

inline auto GRTCommandKey(const QString& command_id) -> QString {
  return QStringLiteral("commands.%1").arg(GRTSegment(command_id));
}

inline auto GRTCommandKey(const QString& command_id, const QString& field)
    -> QString {
  return GRTCommandKey(command_id) + "." + field;
}

inline auto GRTEventStatKey(const QString& event_id, const QString& field)
    -> QString {
  return QStringLiteral("stats.events.%1.%2").arg(GRTSegment(event_id), field);
}

inline auto GRTCommandStatKey(const QString& command_id, const QString& field)
    -> QString {
  return QStringLiteral("stats.commands.%1.%2")
      .arg(GRTSegment(command_id), field);
}

inline auto GRTOperationStatKey(const QString& operation, const QString& field)
    -> QString {
  return QStringLiteral("stats.operations.%1.%2")
      .arg(GRTSegment(operation), field);
}

inline auto GRTModuleLoadStatKey(const QString& field) -> QString {
  return QStringLiteral("stats.module_load.%1").arg(field);
}

}  // namespace GpgFrontend::Module

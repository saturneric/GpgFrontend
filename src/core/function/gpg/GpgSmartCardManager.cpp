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

#include "GpgSmartCardManager.h"

#include "core/function/gpg/GpgAutomatonHandler.h"

namespace GpgFrontend {

GpgSmartCardManager::GpgSmartCardManager(int channel)
    : SingletonFunctionObject<GpgSmartCardManager>(channel) {}

auto GpgSmartCardManager::Fetch(const QString& serial_number) -> bool {
  GpgAutomatonHandler::AutomatonNextStateHandler next_state_handler =
      [=](AutomatonState state, QString status, QString args) {
        auto tokens = args.split(' ');

        switch (state) {
          case GpgAutomatonHandler::kAS_START:
            if (status == "GET_LINE" && args == "cardedit.prompt") {
              return GpgAutomatonHandler::kAS_COMMAND;
            }
            return GpgAutomatonHandler::kAS_ERROR;
          case GpgAutomatonHandler::kAS_COMMAND:
            if (status == "GET_LINE" && args == "cardedit.prompt") {
              return GpgAutomatonHandler::kAS_QUIT;
            }
            return GpgAutomatonHandler::kAS_ERROR;
          case GpgAutomatonHandler::kAS_QUIT:
          case GpgAutomatonHandler::kAS_ERROR:
            if (status == "GET_LINE" && args == "keyedit.prompt") {
              return GpgAutomatonHandler::kAS_QUIT;
            }
            return GpgAutomatonHandler::kAS_ERROR;
          default:
            return GpgAutomatonHandler::kAS_ERROR;
        };
      };

  AutomatonActionHandler action_handler = [](AutomatonHandelStruct& handler,
                                             AutomatonState state) {
    switch (state) {
      case GpgAutomatonHandler::kAS_COMMAND:
        return QString("fetch");
      case GpgAutomatonHandler::kAS_QUIT:
        return QString("quit");
      case GpgAutomatonHandler::kAS_START:
      case GpgAutomatonHandler::kAS_ERROR:
      default:
        return QString("");
    }
    return QString("");
  };

  return GpgAutomatonHandler::GetInstance(GetChannel())
      .DoCardInteract(serial_number, next_state_handler, action_handler);
}

auto GpgSmartCardManager::GetSerialNumbers() -> QStringList {
  auto [r, s] = assuan_.SendStatusCommand(GpgComponentType::kGPG_AGENT,
                                          "SCD SERIALNO --all");
  if (!r) {
    cached_scd_serialno_status_hash_.clear();
    cache_scd_card_serial_numbers_.clear();
    return {};
  }

  auto hash =
      QCryptographicHash::hash(s.join(' ').toUtf8(), QCryptographicHash::Sha1)
          .toHex();
  // check and skip
  if (cached_scd_serialno_status_hash_ == hash) {
    return cache_scd_card_serial_numbers_;
  }

  cached_scd_serialno_status_hash_.clear();
  cache_scd_card_serial_numbers_.clear();
  auto [ret, status] = assuan_.SendStatusCommand(GpgComponentType::kGPG_AGENT,
                                                 "SCD GETINFO all_active_apps");
  if (!ret || status.empty()) {
    LOG_D() << "command SCD GETINFO all_active_apps failed, resetting...";
    return {};
  }

  for (const auto& line : status) {
    auto tokens = line.split(' ');

    if (tokens.size() < 2 || tokens[0] != "SERIALNO") {
      LOG_E() << "invalid response of command GETINFO all_active_apps: "
              << line;
      continue;
    }

    auto serial_number = tokens[1];
    if (!line.contains("openpgp")) {
      LOG_W() << "smart card: " << serial_number << "doesn't support openpgp.";
      continue;
    }

    cache_scd_card_serial_numbers_.append(serial_number);
  }

  cached_scd_serialno_status_hash_ = hash;
  return cache_scd_card_serial_numbers_;
}

auto GpgSmartCardManager::SelectCardBySerialNumber(const QString& serial_number)
    -> std::tuple<bool, QString> {
  if (serial_number.isEmpty()) return {false, "Serial Number is empty."};

  auto [ret, status] = assuan_.SendStatusCommand(
      GpgComponentType::kGPG_AGENT,
      QString("SCD SERIALNO --demand=%1 openpgp").arg(serial_number));
  if (!ret || status.isEmpty()) {
    return {false, status.join(' ')};
  }

  auto line = status.front();
  auto token = line.split(' ');

  if (token.size() != 2) {
    LOG_E() << "invalid response of command SERIALNO: " << line;
    return {false, line};
  }

  LOG_D() << "selected smart card by serial number: " << serial_number;

  return {true, {}};
}

auto GpgSmartCardManager::FetchCardInfoBySerialNumber(
    const QString& serial_number) -> QSharedPointer<GpgOpenPGPCard> {
  if (serial_number.trimmed().isEmpty()) return nullptr;

  auto [ret, status] = assuan_.SendStatusCommand(
      GpgComponentType::kGPG_AGENT, "SCD LEARN --force " + serial_number);
  if (!ret || status.isEmpty()) {
    LOG_E() << "scd learn failed: " << status;
    return nullptr;
  }

  auto card_info = GpgOpenPGPCard(status);
  if (!card_info.good) {
    return nullptr;
  }

  return QSharedPointer<GpgOpenPGPCard>::create(card_info);
}

auto PercentDataEscape(const QByteArray& data, bool plus_escape = false,
                       const QString& prefix = QString()) -> QString {
  QString result;

  if (!prefix.isEmpty()) {
    for (QChar ch : prefix) {
      if (ch == '%' || ch.unicode() < 0x20) {
        result +=
            QString("%%%1")
                .arg(static_cast<int>(ch.unicode()), 2, 16, QLatin1Char('0'))
                .toUpper();
      } else {
        result += ch;
      }
    }
  }

  for (unsigned char ch : data) {
    if (ch == '\0') {
      result += "%00";
    } else if (ch == '%') {
      result += "%25";
    } else if (plus_escape && ch == ' ') {
      result += '+';
    } else if (plus_escape && (ch < 0x20 || ch == '+')) {
      result += QString("%%%1").arg(ch, 2, 16, QLatin1Char('0')).toUpper();
    } else {
      result += QLatin1Char(ch);
    }
  }

  return result;
}

auto GpgSmartCardManager::ModifyAttr(const QString& attr, const QString& value)
    -> std::tuple<bool, QString> {
  if (attr.trimmed().isEmpty() || value.trimmed().isEmpty()) {
    return {false, "ATTR or Value is empty"};
  }

  const auto command = QString("SCD SETATTR %1 ").arg(attr);
  const auto escaped_command =
      PercentDataEscape(value.trimmed().toUtf8(), true, command);

  auto [r, s] =
      assuan_.SendStatusCommand(GpgComponentType::kGPG_AGENT, escaped_command);

  if (!r) {
    LOG_E() << "SCD SETATTR command failed for attr" << attr;
    return {false, s.join(' ')};
  }

  return {true, {}};
}

auto GpgSmartCardManager::ModifyPin(const QString& pin_ref)
    -> std::tuple<bool, QString> {
  if (pin_ref.trimmed().isEmpty()) return {false, "PIN Reference is empty"};

  QString command;
  if (pin_ref == "OPENPGP.1") {
    command = "SCD PASSWD OPENPGP.1";
  } else if (pin_ref == "OPENPGP.3") {
    command = "SCD PASSWD OPENPGP.3";
  } else if (pin_ref == "OPENPGP.2") {
    command = "SCD PASSWD --reset OPENPGP.2";
  } else {
    command = QString("SCD PASSWD %1").arg(pin_ref);
  }

  auto [success, status] =
      assuan_.SendStatusCommand(GpgComponentType::kGPG_AGENT, command);

  if (!success) {
    LOG_E() << "modify pin of smart failed: " << status;
    return {false, status.join(' ')};
  }

  return {true, {}};
}

auto GpgSmartCardManager::GenerateKey(
    const QString& serial_number, const QString& name, const QString& email,
    const QString& comment, const QDateTime& expire,
    bool non_expire) -> std::tuple<bool, QString> {
  if (name.isEmpty() || email.isEmpty()) {
    return {false, "name or email is empty"};
  }

  qint64 days_before_expire = 0;
  if (!non_expire) {
    days_before_expire = QDateTime::currentDateTime().daysTo(expire);
  }

  GpgAutomatonHandler::AutomatonNextStateHandler next_state_handler =
      [=](AutomatonState state, const QString& status, const QString& args) {
        auto tokens = args.split(' ');

        switch (state) {
          case GpgAutomatonHandler::kAS_START:
            if (status == "GET_LINE" && args == "cardedit.prompt") {
              return GpgAutomatonHandler::kAS_ADMIN;
            }
            return GpgAutomatonHandler::kAS_ERROR;
          case GpgAutomatonHandler::kAS_ADMIN:
            if (status == "GET_LINE" && args == "cardedit.prompt") {
              return GpgAutomatonHandler::kAS_COMMAND;
            }
            return GpgAutomatonHandler::kAS_ERROR;
          case GpgAutomatonHandler::kAS_COMMAND:
            if (status == "GET_LINE" && args == "cardedit.genkeys.backup_enc") {
              return GpgAutomatonHandler::kAS_COMMAND;
            }
            if (status == "GET_BOOL" &&
                args == "cardedit.genkeys.replace_keys") {
              return GpgAutomatonHandler::kAS_COMMAND;
            }
            if (status == "GET_LINE" && args == "keygen.valid") {
              return GpgAutomatonHandler::kAS_COMMAND;
            }
            if (status == "GET_LINE" && args == "keygen.name") {
              return GpgAutomatonHandler::kAS_COMMAND;
            }
            if (status == "GET_LINE" && args == "keygen.email") {
              return GpgAutomatonHandler::kAS_COMMAND;
            }
            if (status == "GET_LINE" && args == "keygen.comment") {
              return GpgAutomatonHandler::kAS_COMMAND;
            }
            if (status == "PINENTRY_LAUNCHED" ||
                status == "BACKUP_KEY_CREATED" || status == "KEY_CONSIDERED" ||
                status == "KEY_CREATED") {
              return GpgAutomatonHandler::kAS_INFO;
            }
            return GpgAutomatonHandler::kAS_ERROR;
          case GpgAutomatonHandler::kAS_INFO:
            if (status == "PINENTRY_LAUNCHED" ||
                status == "BACKUP_KEY_CREATED" || status == "KEY_CONSIDERED" ||
                status == "KEY_CREATED") {
              return GpgAutomatonHandler::kAS_INFO;
            }
            if (status == "GET_LINE" && args == "cardedit.prompt") {
              return GpgAutomatonHandler::kAS_QUIT;
            }
            return GpgAutomatonHandler::kAS_ERROR;
          case GpgAutomatonHandler::kAS_QUIT:
          case GpgAutomatonHandler::kAS_ERROR:
            if (status == "GET_LINE" && args == "keyedit.prompt") {
              return GpgAutomatonHandler::kAS_QUIT;
            }
            return GpgAutomatonHandler::kAS_ERROR;
          default:
            return GpgAutomatonHandler::kAS_ERROR;
        };
      };

  AutomatonActionHandler action_handler = [=](AutomatonHandelStruct& handler,
                                              AutomatonState state) {
    switch (state) {
      case GpgAutomatonHandler::kAS_ADMIN:
        return QString("admin");
      case GpgAutomatonHandler::kAS_COMMAND: {
        auto [status, args] = handler.PromptStatus();
        if (args == "cardedit.prompt") {
          return QString("generate");
        }
        if (args == "cardedit.genkeys.backup_enc") {
          return QString("y");
        }
        if (args == "cardedit.genkeys.replace_keys") {
          return QString("y");
        }
        if (args == "keygen.valid") {
          return QString::number(days_before_expire);
        }
        if (args == "keygen.name") {
          return name;
        }
        if (args == "keygen.email") {
          return email;
        }
        if (args == "keygen.comment") {
          return comment;
        }
        return QString{};
      }
      case GpgAutomatonHandler::kAS_QUIT:
        return QString("quit");
      case GpgAutomatonHandler::kAS_INFO:
      case GpgAutomatonHandler::kAS_START:
      case GpgAutomatonHandler::kAS_ERROR:
      default:
        return QString{};
    }

    return QString{};
  };

  return {
      GpgAutomatonHandler::GetInstance(GetChannel())
          .DoCardInteract(serial_number, next_state_handler, action_handler),
      "unknown error"};
}
}  // namespace GpgFrontend
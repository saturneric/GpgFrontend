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

#include "ui/main_window/EncryptBlockReason.h"

#include "core/model/GpgKey.h"
#include "core/model/GpgSubKey.h"

namespace GpgFrontend::UI {

namespace {

auto Tr(const char* text) -> QString {
  return QCoreApplication::translate("GpgFrontend::UI::EncryptBlockReason",
                                     text);
}

// ISO dates read the same in every locale; "03-08-2026" does not.
auto FormatDate(const QDateTime& time) -> QString {
  return time.date().toString(Qt::ISODate);
}

auto Label(const GpgAbstractKey& key) -> QString {
  if (key.Email().isEmpty()) return key.Name();
  return QString("%1 <%2>").arg(key.Name(), key.Email());
}

auto FactsOf(const GpgSubKey& s_key) -> EncryptSubkeyFacts {
  EncryptSubkeyFacts facts;
  facts.can_encrypt = s_key.IsHasEncrCap();
  facts.revoked = s_key.IsRevoked();
  facts.disabled = s_key.IsDisabled();
  facts.expired = s_key.IsExpired();
  facts.expires = s_key.ExpirationTime();
  return facts;
}

}  // namespace

auto DescribeEncryptBlock(const EncryptKeyFacts& facts) -> QString {
  if (facts.usable) return {};

  if (facts.revoked) {
    return Tr("The key of %1 has been revoked.").arg(facts.label);
  }

  if (facts.expired) {
    return Tr("The key of %1 expired on %2.")
        .arg(facts.label, FormatDate(facts.expires));
  }

  bool any_encrypt = false;
  const EncryptSubkeyFacts* latest_expired = nullptr;
  for (const auto& s_key : facts.subkeys) {
    if (!s_key.can_encrypt) continue;
    any_encrypt = true;

    if (s_key.revoked || s_key.disabled || !s_key.expired) continue;
    if (latest_expired == nullptr || s_key.expires > latest_expired->expires) {
      latest_expired = &s_key;
    }
  }

  if (!any_encrypt) {
    return Tr("The key of %1 has no subkey that can encrypt.").arg(facts.label);
  }

  // The most recent expiry is the one worth naming: it is the subkey the
  // owner would extend.
  if (latest_expired != nullptr) {
    return Tr("The encryption subkey of %1 expired on %2.")
        .arg(facts.label, FormatDate(latest_expired->expires));
  }

  return Tr("The encryption subkey of %1 has been revoked or disabled.")
      .arg(facts.label);
}

auto CollectEncryptKeyFacts(const GpgAbstractKeyPtr& key) -> EncryptKeyFacts {
  EncryptKeyFacts facts;
  if (key == nullptr) return facts;

  facts.label = Label(*key);
  facts.usable = key->IsHasEncrCap();
  facts.revoked = key->IsRevoked();
  facts.expired = key->IsExpired();
  facts.expires = key->ExpirationTime();

  if (auto g_key = qSharedPointerDynamicCast<GpgKey>(key); g_key != nullptr) {
    for (const auto& s_key : g_key->SubKeys()) {
      facts.subkeys.push_back(FactsOf(s_key));
    }
  } else if (auto s_key = qSharedPointerDynamicCast<GpgSubKey>(key);
             s_key != nullptr) {
    facts.subkeys.push_back(FactsOf(*s_key));
  }

  return facts;
}

auto DescribeEncryptBlock(const GpgAbstractKeyPtr& key) -> QString {
  return DescribeEncryptBlock(CollectEncryptKeyFacts(key));
}

}  // namespace GpgFrontend::UI

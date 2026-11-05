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

#include "shell/rules/EncryptBlockReason.h"

#include "core/model/GpgKey.h"
#include "core/model/GpgKeyGroup.h"
#include "core/model/GpgSubKey.h"

namespace GpgFrontend::Shell {

// Every translate() below spells its context out: lupdate is a static parser
// and reads no helper that hides it. The short notes keep the context the
// phone's QML gave them ("Wording"), which keeps their translations.

namespace {

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

auto IsGroup(const GpgAbstractKey& key) -> bool {
  return key.KeyType() == GpgAbstractKeyType::kGPG_KEYGROUP;
}

}  // namespace

auto DescribeEncryptBlock(const EncryptKeyFacts& facts) -> QString {
  if (facts.empty_group) {
    return QCoreApplication::translate("GpgFrontend::Shell::EncryptBlockReason",
                                       "The key group %1 has no keys in it.")
        .arg(facts.label);
  }
  if (facts.usable) return {};

  if (facts.revoked) {
    return QCoreApplication::translate("GpgFrontend::Shell::EncryptBlockReason",
                                       "The key of %1 has been revoked.")
        .arg(facts.label);
  }

  if (facts.expired) {
    return QCoreApplication::translate("GpgFrontend::Shell::EncryptBlockReason",
                                       "The key of %1 expired on %2.")
        .arg(facts.label, FormatDate(facts.expires));
  }

  if (facts.disabled) {
    return QCoreApplication::translate("GpgFrontend::Shell::EncryptBlockReason",
                                       "The key of %1 is disabled.")
        .arg(facts.label);
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
    return QCoreApplication::translate(
               "GpgFrontend::Shell::EncryptBlockReason",
               "The key of %1 has no subkey that can encrypt.")
        .arg(facts.label);
  }

  // The most recent expiry is the one worth naming: it is the subkey the
  // owner would extend.
  if (latest_expired != nullptr) {
    return QCoreApplication::translate(
               "GpgFrontend::Shell::EncryptBlockReason",
               "The encryption subkey of %1 expired on %2.")
        .arg(facts.label, FormatDate(latest_expired->expires));
  }

  return QCoreApplication::translate(
             "GpgFrontend::Shell::EncryptBlockReason",
             "The encryption subkey of %1 has been revoked or disabled.")
      .arg(facts.label);
}

auto EncryptBlockNote(const EncryptKeyFacts& facts) -> QString {
  if (facts.empty_group) {
    return QCoreApplication::translate("Wording", "Has no keys");
  }
  if (facts.usable) return {};
  if (facts.revoked) return QCoreApplication::translate("Wording", "Revoked");
  if (facts.expired) return QCoreApplication::translate("Wording", "Expired");
  if (facts.disabled) return QCoreApplication::translate("Wording", "Disabled");
  return QCoreApplication::translate("Wording", "Cannot be encrypted to");
}

auto CollectEncryptKeyFacts(const GpgAbstractKey* key) -> EncryptKeyFacts {
  EncryptKeyFacts facts;
  if (key == nullptr) return facts;

  facts.label = Label(*key);
  if (IsGroup(*key)) {
    const auto* group = dynamic_cast<const GpgKeyGroup*>(key);
    facts.usable = true;
    facts.empty_group = group == nullptr || group->KeyIds().isEmpty();
    return facts;
  }

  facts.revoked = key->IsRevoked();
  facts.expired = key->IsExpired();
  facts.disabled = key->IsDisabled();
  // What the key says it can do, and only while it may be used at all.
  facts.usable = key->IsHasEncrCap() && !facts.revoked && !facts.expired &&
                 !facts.disabled;
  facts.expires = key->ExpirationTime();

  if (const auto* g_key = dynamic_cast<const GpgKey*>(key); g_key != nullptr) {
    for (const auto& s_key : g_key->SubKeys()) {
      facts.subkeys.push_back(FactsOf(s_key));
    }
  } else if (const auto* s_key = dynamic_cast<const GpgSubKey*>(key);
             s_key != nullptr) {
    facts.subkeys.push_back(FactsOf(*s_key));
  }

  return facts;
}

auto CollectEncryptKeyFacts(const GpgAbstractKeyPtr& key) -> EncryptKeyFacts {
  return CollectEncryptKeyFacts(key.get());
}

auto DescribeEncryptBlock(const GpgAbstractKeyPtr& key) -> QString {
  return DescribeEncryptBlock(CollectEncryptKeyFacts(key));
}

auto CollectSignKeyFacts(const GpgAbstractKey* key) -> SignKeyFacts {
  SignKeyFacts facts;
  if (key == nullptr) return facts;

  facts.label = Label(*key);
  facts.group = IsGroup(*key);
  if (facts.group) return facts;
  facts.secret = key->IsPrivateKey();
  facts.revoked = key->IsRevoked();
  facts.expired = key->IsExpired();
  facts.disabled = key->IsDisabled();
  facts.usable = key->IsHasSignCap() && !facts.revoked && !facts.expired &&
                 !facts.disabled;
  facts.expires = key->ExpirationTime();
  return facts;
}

auto DescribeSignBlock(const SignKeyFacts& facts) -> QString {
  if (facts.usable && !facts.group) return {};
  if (facts.group) {
    return QCoreApplication::translate("GpgFrontend::Shell::EncryptBlockReason",
                                       "The key group %1 cannot sign.")
        .arg(facts.label);
  }
  if (!facts.secret) {
    return QCoreApplication::translate(
               "GpgFrontend::Shell::EncryptBlockReason",
               "%1 is not a key of your own: its secret key is not here.")
        .arg(facts.label);
  }
  if (facts.revoked) {
    return QCoreApplication::translate("GpgFrontend::Shell::EncryptBlockReason",
                                       "The key of %1 has been revoked.")
        .arg(facts.label);
  }
  if (facts.expired) {
    return QCoreApplication::translate("GpgFrontend::Shell::EncryptBlockReason",
                                       "The key of %1 expired on %2.")
        .arg(facts.label, FormatDate(facts.expires));
  }
  if (facts.disabled) {
    return QCoreApplication::translate("GpgFrontend::Shell::EncryptBlockReason",
                                       "The key of %1 is disabled.")
        .arg(facts.label);
  }
  return QCoreApplication::translate(
             "GpgFrontend::Shell::EncryptBlockReason",
             "The key of %1 has no subkey here that can sign.")
      .arg(facts.label);
}

auto SignBlockNote(const SignKeyFacts& facts) -> QString {
  if (facts.usable && !facts.group) return {};
  if (facts.group) return QCoreApplication::translate("Wording", "Cannot sign");
  if (!facts.secret) {
    return QCoreApplication::translate("Wording", "Not a key of your own");
  }
  if (facts.revoked) return QCoreApplication::translate("Wording", "Revoked");
  if (facts.expired) return QCoreApplication::translate("Wording", "Expired");
  if (facts.disabled) return QCoreApplication::translate("Wording", "Disabled");
  return QCoreApplication::translate("Wording", "Cannot sign");
}

}  // namespace GpgFrontend::Shell

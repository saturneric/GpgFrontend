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

#include "GpgKeyGenerateInfo.h"

#include <cassert>

#include "core/function/openpgp/helper/Async.h"
#include "core/function/openpgp/traits/KeyGenerationTraits.h"

namespace GpgFrontend {

namespace {

auto FindAlgoByIdAndType(const QContainer<KeyAlgo> &algos, const QString &id,
                         const QString &type) -> KeyAlgo {
  const auto it =
      std::find_if(algos.cbegin(), algos.cend(), [&](const KeyAlgo &algo) {
        return algo.Id() == id && algo.Type() == type;
      });

  assert(it != algos.cend());
  return it != algos.cend() ? *it : KeyAlgo{};
}

auto FindSubAlgo(const QString &id, const QString &type) -> KeyAlgo {
  return FindAlgoByIdAndType(KeyGenerateInfo::kSubKeyAlgos, id, type);
}

auto FindPrimaryAlgo(const QString &id, const QString &type) -> KeyAlgo {
  return FindAlgoByIdAndType(KeyGenerateInfo::kPrimaryKeyAlgos, id, type);
}

}  // namespace

const KeyAlgo KeyGenerateInfo::kNoneAlgo = {
    "none",
    KeyGenerateInfo::tr("None"),
    "None",
    0,
    0,
    {{OpenPGPEngine::kGNUPG, "0.0.0"}, {OpenPGPEngine::kRPGP, "0.0.0"}}};

const QContainer<KeyAlgo> KeyGenerateInfo::kPrimaryKeyAlgos = {
    kNoneAlgo,

    /**
     * Algorithm (DSA) as a government standard for digital signatures.
     * Originally, it supported key lengths between 512 and 1024 bits.
     * Recently, NIST has declared 512-bit keys obsolete:
     * now, DSA is available in 1024, 2048 and 3072-bit lengths.
     */
    {
        "rsa1024",
        "RSA",
        "RSA",
        1024,
        kENCRYPT | kSIGN | kAUTH | kCERT,
        {{OpenPGPEngine::kGNUPG, "2.2.0"}, {OpenPGPEngine::kRPGP, "0.1.0"}},
    },
    {
        "rsa2048",
        "RSA",
        "RSA",
        2048,
        kENCRYPT | kSIGN | kAUTH | kCERT,
        {{OpenPGPEngine::kGNUPG, "2.2.0"}, {OpenPGPEngine::kRPGP, "0.1.0"}},
    },
    {
        "rsa3072",
        "RSA",
        "RSA",
        3072,
        kENCRYPT | kSIGN | kAUTH | kCERT,
        {{OpenPGPEngine::kGNUPG, "2.2.0"}, {OpenPGPEngine::kRPGP, "0.1.0"}},
    },
    {
        "rsa4096",
        "RSA",
        "RSA",
        4096,
        kENCRYPT | kSIGN | kAUTH | kCERT,
        {{OpenPGPEngine::kGNUPG, "2.2.0"}, {OpenPGPEngine::kRPGP, "0.1.0"}},
    },
    {
        "dsa1024",
        "DSA",
        "DSA",
        1024,
        kSIGN | kAUTH | kCERT,
        {{OpenPGPEngine::kGNUPG, "2.2.0"}, {OpenPGPEngine::kRPGP, "0.1.0"}},
    },
    {
        "dsa2048",
        "DSA",
        "DSA",
        2048,
        kSIGN | kAUTH | kCERT,
        {{OpenPGPEngine::kGNUPG, "2.2.0"}, {OpenPGPEngine::kRPGP, "0.1.0"}},
    },
    {
        "dsa3072",
        "DSA",
        "DSA",
        3072,
        kSIGN | kAUTH | kCERT,
        {{OpenPGPEngine::kGNUPG, "2.2.0"}, {OpenPGPEngine::kRPGP, "0.1.0"}},
    },
    {
        "ed25519",
        "ED25519",
        "EdDSA",
        255,
        kSIGN | kAUTH | kCERT,
        {{OpenPGPEngine::kGNUPG, "2.2.0"}, {OpenPGPEngine::kRPGP, "0.1.0"}},
    },
    {
        "nistp256",
        "NIST",
        "ECDSA",
        256,
        kSIGN | kAUTH | kCERT,
        {{OpenPGPEngine::kGNUPG, "2.2.0"}, {OpenPGPEngine::kRPGP, "0.1.0"}},
    },
    {
        "nistp384",
        "NIST",
        "ECDSA",
        384,
        kSIGN | kAUTH | kCERT,
        {{OpenPGPEngine::kGNUPG, "2.2.0"}, {OpenPGPEngine::kRPGP, "0.1.0"}},
    },
    {
        "nistp521",
        "NIST",
        "ECDSA",
        521,
        kSIGN | kAUTH | kCERT,
        {{OpenPGPEngine::kGNUPG, "2.2.0"}, {OpenPGPEngine::kRPGP, "0.1.0"}},
    },
    {
        "brainpoolp256r1",
        "BrainPooL",
        "ECDSA",
        256,
        kSIGN | kAUTH | kCERT,
        {{OpenPGPEngine::kGNUPG, "2.3.0"}},
    },
    {
        "brainpoolp384r1",
        "BrainPooL",
        "ECDSA",
        384,
        kSIGN | kAUTH | kCERT,
        {{OpenPGPEngine::kGNUPG, "2.3.0"}},
    },
    {
        "brainpoolp512r1",
        "BrainPooL",
        "ECDSA",
        512,
        kSIGN | kAUTH | kCERT,
        {{OpenPGPEngine::kGNUPG, "2.3.0"}},
    },
    {
        "ed448",
        "ED448",
        "EdDSA",
        448,
        kSIGN | kAUTH | kCERT,
        {{OpenPGPEngine::kGNUPG, "2.3.0"}, {OpenPGPEngine::kRPGP, "0.1.0"}},
    },
    {
        "secp256k1",
        "SECP256K1",
        "EdDSA",
        256,
        kSIGN | kAUTH | kCERT,
        {{OpenPGPEngine::kGNUPG, "2.3.0"}, {OpenPGPEngine::kRPGP, "0.1.0"}},
    },

    {
        "slhdsashake128s",
        "SLH-DSA-SHAKE-128S",
        "SLH-DSA",
        128,
        kSIGN | kAUTH,
        {{OpenPGPEngine::kRPGP, "0.1.2"}},
    },
    {
        "slhdsashake128f",
        "SLH-DSA-SHAKE-128F",
        "SLH-DSA",
        128,
        kSIGN | kAUTH,
        {{OpenPGPEngine::kRPGP, "0.1.2"}},
    },
    {
        "slhdsashake256s",
        "SLH-DSA-SHAKE-256S",
        "SLH-DSA",
        256,
        kSIGN | kAUTH,
        {{OpenPGPEngine::kRPGP, "0.1.2"}},
    },
};

const QContainer<KeyAlgo> KeyGenerateInfo::kHybridPrimaryKeyAlgo = {
    {"mldsa65",
     "ML-DSA",
     "HYBRID-SIGN",
     65,
     kSIGN | kAUTH,
     {{OpenPGPEngine::kRPGP, "0.1.2"}},
     {
         {
             FindPrimaryAlgo("ed25519", "EdDSA"),  // ed25519
             {
                 {OpenPGPEngine::kRPGP, "0.1.2"},
             },
         },
     }},
    {"mldsa87",
     "ML-DSA",
     "HYBRID-SIGN",
     87,
     kSIGN | kAUTH,
     {{OpenPGPEngine::kRPGP, "0.1.2"}},
     {
         {
             FindPrimaryAlgo("ed448", "EdDSA"),  // ed448
             {
                 {OpenPGPEngine::kRPGP, "0.1.2"},
             },
         },
     }},
};

const QContainer<KeyAlgo> KeyGenerateInfo::kSubKeyAlgos = {
    kNoneAlgo,

    {
        "rsa1024",
        "RSA",
        "RSA",
        1024,
        kENCRYPT | kSIGN | kAUTH,
        {{OpenPGPEngine::kGNUPG, "2.2.0"}, {OpenPGPEngine::kRPGP, "0.1.0"}},
    },
    {
        "rsa2048",
        "RSA",
        "RSA",
        2048,
        kENCRYPT | kSIGN | kAUTH,
        {{OpenPGPEngine::kGNUPG, "2.2.0"}, {OpenPGPEngine::kRPGP, "0.1.0"}},
    },
    {
        "rsa3072",
        "RSA",
        "RSA",
        3072,
        kENCRYPT | kSIGN | kAUTH,
        {{OpenPGPEngine::kGNUPG, "2.2.0"}, {OpenPGPEngine::kRPGP, "0.1.0"}},
    },
    {
        "rsa4096",
        "RSA",
        "RSA",
        4096,
        kENCRYPT | kSIGN | kAUTH,
        {{OpenPGPEngine::kGNUPG, "2.2.0"}, {OpenPGPEngine::kRPGP, "0.1.0"}},
    },
    {
        "dsa1024",
        "DSA",
        "DSA",
        1024,
        kSIGN | kAUTH,
        {{OpenPGPEngine::kGNUPG, "2.2.0"}, {OpenPGPEngine::kRPGP, "0.1.0"}},
    },
    {
        "dsa2048",
        "DSA",
        "DSA",
        2048,
        kSIGN | kAUTH,
        {{OpenPGPEngine::kGNUPG, "2.2.0"}, {OpenPGPEngine::kRPGP, "0.1.0"}},
    },
    {
        "dsa3072",
        "DSA",
        "DSA",
        3072,
        kSIGN | kAUTH,
        {{OpenPGPEngine::kGNUPG, "2.2.0"}, {OpenPGPEngine::kRPGP, "0.1.0"}},
    },
    {
        "ed25519",
        "ED25519",
        "EdDSA",
        255,
        kSIGN | kAUTH,
        {{OpenPGPEngine::kGNUPG, "2.2.0"}, {OpenPGPEngine::kRPGP, "0.1.0"}},
    },
    {
        "cv25519",
        "CV25519",
        "ECDH",
        255,
        kENCRYPT,
        {{OpenPGPEngine::kGNUPG, "2.2.0"}, {OpenPGPEngine::kRPGP, "0.1.0"}},
    },
    {
        "nistp256",
        "NIST",
        "ECDH",
        256,
        kENCRYPT,
        {{OpenPGPEngine::kGNUPG, "2.2.0"}, {OpenPGPEngine::kRPGP, "0.1.0"}},
    },
    {
        "nistp384",
        "NIST",
        "ECDH",
        384,
        kENCRYPT,
        {{OpenPGPEngine::kGNUPG, "2.2.0"}, {OpenPGPEngine::kRPGP, "0.1.0"}},
    },
    {
        "nistp521",
        "NIST",
        "ECDH",
        521,
        kENCRYPT,
        {{OpenPGPEngine::kGNUPG, "2.2.0"}, {OpenPGPEngine::kRPGP, "0.1.0"}},
    },
    {
        "nistp256",
        "NIST",
        "ECDSA",
        256,
        kSIGN | kAUTH,
        {{OpenPGPEngine::kGNUPG, "2.2.0"}, {OpenPGPEngine::kRPGP, "0.1.0"}},
    },
    {
        "nistp384",
        "NIST",
        "ECDSA",
        384,
        kSIGN | kAUTH,
        {{OpenPGPEngine::kGNUPG, "2.2.0"}, {OpenPGPEngine::kRPGP, "0.1.0"}},
    },
    {
        "nistp521",
        "NIST",
        "ECDSA",
        521,
        kSIGN | kAUTH,
        {{OpenPGPEngine::kGNUPG, "2.2.0"}, {OpenPGPEngine::kRPGP, "0.1.0"}},
    },
    {
        "brainpoolp256r1",
        "BrainPooL",
        "ECDH",
        256,
        kENCRYPT,
        {{OpenPGPEngine::kGNUPG, "2.3.0"}},
    },
    {
        "brainpoolp384r1",
        "BrainPooL",
        "ECDH",
        384,
        kENCRYPT,
        {{OpenPGPEngine::kGNUPG, "2.3.0"}},
    },
    {
        "brainpoolp512r1",
        "BrainPooL",
        "ECDH",
        512,
        kENCRYPT,
        {{OpenPGPEngine::kGNUPG, "2.3.0"}},
    },
    {
        "brainpoolp256r1",
        "BrainPooL",
        "ECDSA",
        256,
        kSIGN | kAUTH,
        {{OpenPGPEngine::kGNUPG, "2.3.0"}},
    },
    {
        "brainpoolp384r1",
        "BrainPooL",
        "ECDSA",
        384,
        kSIGN | kAUTH,
        {{OpenPGPEngine::kGNUPG, "2.3.0"}},
    },
    {
        "brainpoolp512r1",
        "BrainPooL",
        "ECDSA",
        512,
        kSIGN | kAUTH,
        {{OpenPGPEngine::kGNUPG, "2.3.0"}},
    },
    {
        "x448",
        "X448",
        "ECDH",
        448,
        kENCRYPT,
        {{OpenPGPEngine::kGNUPG, "2.3.0"}, {OpenPGPEngine::kRPGP, "0.1.0"}},
    },
    {
        "secp256k1",
        "SECP256K1",
        "ECDH",
        256,
        kENCRYPT,
        {{OpenPGPEngine::kGNUPG, "2.3.0"}},
    },

    /**
     * GnuPG supports the Elgamal asymmetric encryption algorithm in key lengths
     * ranging from 1024 to 4096 bits.
     */
    {
        "elg1024",
        "ELG-E",
        "ELG-E",
        1024,
        kENCRYPT,
        {{OpenPGPEngine::kGNUPG, "2.2.0"}},
    },
    {
        "elg2048",
        "ELG-E",
        "ELG-E",
        2048,
        kENCRYPT,
        {{OpenPGPEngine::kGNUPG, "2.2.0"}},
    },
    {
        "elg3072",
        "ELG-E",
        "ELG-E",
        3072,
        kENCRYPT,
        {{OpenPGPEngine::kGNUPG, "2.2.0"}},
    },
    {
        "elg4096",
        "ELG-E",
        "ELG-E",
        4096,
        kENCRYPT,
        {{OpenPGPEngine::kGNUPG, "2.2.0"}},
    },

    {
        "ed448",
        "ED448",
        "EdDSA",
        448,
        kSIGN | kAUTH,
        {{OpenPGPEngine::kRPGP, "0.1.2"}},
    },

    {
        "slhdsashake128s",
        "SLH-DSA-SHAKE-128S",
        "SLH-DSA",
        128,
        kSIGN | kAUTH,
        {{OpenPGPEngine::kRPGP, "0.1.2"}},
    },
    {
        "slhdsashake128f",
        "SLH-DSA-SHAKE-128F",
        "SLH-DSA",
        128,
        kSIGN | kAUTH,
        {{OpenPGPEngine::kRPGP, "0.1.2"}},
    },
    {
        "slhdsashake256s",
        "SLH-DSA-SHAKE-256S",
        "SLH-DSA",
        256,
        kSIGN | kAUTH,
        {{OpenPGPEngine::kRPGP, "0.1.2"}},
    },

};

// Refer: https://lists.gnupg.org/pipermail/gnupg-devel/2024-May/035537.html
const QContainer<KeyAlgo> KeyGenerateInfo::kHybridSubKeyAlgos = {
    {"ky768",
     "Kyber",
     "HYBRID-KEM",
     768,
     kENCRYPT,
     {{OpenPGPEngine::kGNUPG, "2.5.0"}, {OpenPGPEngine::kRPGP, "0.1.2"}},
     {
         {
             FindSubAlgo("cv25519", "ECDH"),  // cv25519
             {
                 {OpenPGPEngine::kGNUPG, "2.5.0"},
                 {OpenPGPEngine::kRPGP, "0.1.2"},
             },
         },
         {
             FindSubAlgo("nistp256", "ECDH"),  // nistp256
             {
                 {OpenPGPEngine::kGNUPG, "2.5.0"},
             },
         },
         {
             FindSubAlgo("nistp384", "ECDH"),  // nistp384
             {
                 {OpenPGPEngine::kGNUPG, "2.5.0"},
             },
         },
         {
             FindSubAlgo("nistp521", "ECDH"),  // nistp521
             {
                 {OpenPGPEngine::kGNUPG, "2.5.0"},
             },
         },
         {
             FindSubAlgo("brainpoolp256r1", "ECDH"),  // brainpoolp256r1
             {
                 {OpenPGPEngine::kGNUPG, "2.5.0"},
             },
         },
         {
             FindSubAlgo("brainpoolp384r1", "ECDH"),  // brainpoolp384r1
             {
                 {OpenPGPEngine::kGNUPG, "2.5.0"},
             },
         },
         {
             FindSubAlgo("brainpoolp512r1", "ECDH"),  // brainpoolp512r1
             {
                 {OpenPGPEngine::kGNUPG, "2.5.0"},
             },
         },
         {
             FindSubAlgo("x448", "ECDH"),  // x448
             {{OpenPGPEngine::kGNUPG, "2.5.0"}},
         },
     }},
    {"kyber1024",
     "Kyber",
     "HYBRID-KEM",
     1024,
     kENCRYPT,
     {{OpenPGPEngine::kGNUPG, "2.5.0"}, {OpenPGPEngine::kRPGP, "0.1.2"}},
     {
         {
             FindSubAlgo("cv25519", "ECDH"),  // cv25519
             {
                 {OpenPGPEngine::kGNUPG, "2.5.0"},
             },
         },
         {
             FindSubAlgo("nistp256", "ECDH"),  // nistp256
             {
                 {OpenPGPEngine::kGNUPG, "2.5.0"},
             },
         },
         {
             FindSubAlgo("nistp384", "ECDH"),  // nistp384
             {
                 {OpenPGPEngine::kGNUPG, "2.5.0"},
             },
         },
         {
             FindSubAlgo("nistp521", "ECDH"),  // nistp521
             {
                 {OpenPGPEngine::kGNUPG, "2.5.0"},
             },
         },
         {
             FindSubAlgo("brainpoolp256r1", "ECDH"),  // brainpoolp256r1
             {
                 {OpenPGPEngine::kGNUPG, "2.5.0"},
             },
         },
         {
             FindSubAlgo("brainpoolp384r1", "ECDH"),  // brainpoolp384r1
             {
                 {OpenPGPEngine::kGNUPG, "2.5.0"},
             },
         },
         {
             FindSubAlgo("brainpoolp512r1", "ECDH"),  // brainpoolp512r1
             {
                 {OpenPGPEngine::kGNUPG, "2.5.0"},
             },
         },
         {
             FindSubAlgo("x448", "ECDH"),  // x448
             {
                 {OpenPGPEngine::kGNUPG, "2.5.0"},
                 {OpenPGPEngine::kRPGP, "0.1.2"},
             },
         },
     }},
    {"mldsa65",
     "ML-DSA",
     "HYBRID-SIGN",
     65,
     kSIGN | kAUTH,
     {{OpenPGPEngine::kRPGP, "0.1.2"}},
     {
         {
             FindSubAlgo("ed25519", "EdDSA"),  // ed25519
             {
                 {OpenPGPEngine::kRPGP, "0.1.2"},
             },
         },
     }},
    {"mldsa87",
     "ML-DSA",
     "HYBRID-SIGN",
     87,
     kSIGN | kAUTH,
     {{OpenPGPEngine::kRPGP, "0.1.2"}},
     {
         {
             FindSubAlgo("ed448", "EdDSA"),  // ed448
             {
                 {OpenPGPEngine::kRPGP, "0.1.2"},
             },
         },
     }},
};

auto KeyGenerateInfo::GetSupportedKeyAlgo(int channel) -> QContainer<KeyAlgo> {
  QContainer<KeyAlgo> algos;

  for (const auto &algo : kPrimaryKeyAlgos) {
    if (!GpgContextSupportIf(channel, algo.SupportedVersion())) continue;

    algos.append(algo);
  }

  // Add hybrid primary key algos if supported since we only have few hybrid
  // primary key algos, we can directly add them as a normal algo temperately.
  // Add hybrid subkey algos if supported
  for (const auto &algo : kHybridPrimaryKeyAlgo) {
    if (!GpgContextSupportIf(channel, algo.SupportedVersion())) continue;

    algos.append(algo);
  }

  std::sort(algos.begin(), algos.end(), [](const KeyAlgo &a, const KeyAlgo &b) {
    if (a.Name() != b.Name()) return a.Name() < b.Name();
    if (a.KeyLength() != b.KeyLength()) return a.KeyLength() < b.KeyLength();
    if (a.Type() != b.Type()) return a.Type() < b.Type();
    return a.Id() < b.Id();
  });

  return algos;
}

auto KeyGenerateInfo::GetSupportedSubkeyAlgo(int channel)
    -> QContainer<KeyAlgo> {
  QContainer<KeyAlgo> algos;

  for (const auto &algo : kSubKeyAlgos) {
    if (!GpgContextSupportIf(channel, algo.SupportedVersion())) continue;

    algos.append(algo);
  }

  // Add hybrid subkey algos if supported
  for (const auto &algo : kHybridSubKeyAlgos) {
    if (!GpgContextSupportIf(channel, algo.SupportedVersion())) continue;

    algos.append(algo);
  }

  std::sort(algos.begin(), algos.end(), [](const KeyAlgo &a, const KeyAlgo &b) {
    if (a.Name() != b.Name()) return a.Name() < b.Name();
    if (a.KeyLength() != b.KeyLength()) return a.KeyLength() < b.KeyLength();
    if (a.Type() != b.Type()) return a.Type() < b.Type();
    return a.Id() < b.Id();
  });

  return algos;
}

auto KeyGenerateInfo::GetSupportedSubkeyAlgo(int channel, const GpgKey &key)
    -> QContainer<KeyAlgo> {
  auto algos = GetSupportedSubkeyAlgo(channel);
  auto &ctx = OpenPGPContext::GetInstance(channel);
  return RunRegisteredForward<FilterKeyAlgoByKeyTag>(ctx, key, algos);
}

KeyGenerateInfo::KeyGenerateInfo(bool is_subkey)
    : subkey_(is_subkey),
      algo_(kNoneAlgo),
      expired_(QDateTime::currentDateTime().toLocalTime().addYears(2)) {}

auto KeyGenerateInfo::SearchPrimaryKeyAlgo(const QString &algo_id)
    -> std::tuple<bool, KeyAlgo> {
  auto it =
      std::find_if(kPrimaryKeyAlgos.cbegin(), kPrimaryKeyAlgos.cend(),
                   [=](const KeyAlgo &algo) { return algo.Id() == algo_id; });

  if (it != kPrimaryKeyAlgos.cend()) {
    return {true, *it};
  }

  return {false, KeyAlgo{}};
}

auto KeyGenerateInfo::SearchSubKeyAlgo(const QString &algo_id)
    -> std::tuple<bool, KeyAlgo> {
  const auto all_sub_algos = GetSupportedSubkeyAlgo(0);

  auto it = std::find_if(
      all_sub_algos.cbegin(), all_sub_algos.cend(),
      [=](const KeyAlgo &algo) -> bool { return algo.Id() == algo_id; });

  if (it != all_sub_algos.cend()) {
    return {true, *it};
  }

  return {false, KeyAlgo{}};
}

void KeyGenerateInfo::SetAlgo(const KeyAlgo &algo) {
  // reset all options
  reset_options();

  this->SetAllowCert(algo.CanCert());
  this->allow_change_certification_ = false;

  SetAllowEncr(algo.CanEncrypt());
  allow_change_encryption_ = algo.CanEncrypt();

  SetAllowSign(algo.CanSign());
  allow_change_signing_ = algo.CanSign();

  SetAllowAuth(algo.CanAuth());
  allow_change_authentication_ = algo.CanAuth();

  this->algo_ = algo;
}

void KeyGenerateInfo::reset_options() {
  allow_change_encryption_ = true;
  SetAllowEncr(true);

  allow_change_certification_ = true;
  SetAllowCert(true);

  allow_change_signing_ = true;
  SetAllowSign(true);

  allow_change_authentication_ = true;
  SetAllowAuth(true);
}

void KeyGenerateInfo::SetExpireTime(const QDateTime &m_expired) {
  KeyGenerateInfo::expired_ = m_expired;
}

void KeyGenerateInfo::SetNonExpired(bool m_non_expired) {
  if (!m_non_expired) this->expired_ = QDateTime::fromSecsSinceEpoch(0);
  KeyGenerateInfo::non_expired_ = m_non_expired;
}

void KeyGenerateInfo::SetAllowEncr(bool m_allow_encryption) {
  if (allow_change_encryption_) {
    KeyGenerateInfo::allow_encryption_ = m_allow_encryption;
  }
}

void KeyGenerateInfo::SetAllowCert(bool m_allow_certification) {
  if (allow_change_certification_) {
    KeyGenerateInfo::allow_certification_ = m_allow_certification;
  }
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
[[nodiscard]] auto KeyGenerateInfo::IsSubKey() const -> bool { return subkey_; }

/**
 * @brief Set the Is Sub Key object
 *
 * @param m_sub_key
 */
void KeyGenerateInfo::SetIsSubKey(bool m_sub_key) {
  KeyGenerateInfo::subkey_ = m_sub_key;
}

/**
 * @brief Get the Userid object
 *
 * @return QString
 */
[[nodiscard]] auto KeyGenerateInfo::GetUserid() const -> QString {
  return QString("%1(%2)<%3>").arg(name_).arg(comment_).arg(email_);
}

/**
 * @brief Set the Name object
 *
 * @param m_name
 */
void KeyGenerateInfo::SetName(const QString &m_name) { this->name_ = m_name; }

/**
 * @brief Set the Email object
 *
 * @param m_email
 */
void KeyGenerateInfo::SetEmail(const QString &m_email) {
  this->email_ = m_email;
}

/**
 * @brief Set the Comment object
 *
 * @param m_comment
 */
void KeyGenerateInfo::SetComment(const QString &m_comment) {
  this->comment_ = m_comment;
}

/**
 * @brief Get the Name object
 *
 * @return QString
 */
[[nodiscard]] auto KeyGenerateInfo::GetName() const -> QString { return name_; }

/**
 * @brief Get the Email object
 *
 * @return QString
 */
[[nodiscard]] auto KeyGenerateInfo::GetEmail() const -> QString {
  return email_;
}

/**
 * @brief Get the Comment object
 *
 * @return QString
 */
[[nodiscard]] auto KeyGenerateInfo::GetComment() const -> QString {
  return comment_;
}

/**
 * @brief Get the Algo object
 *
 * @return const QString&
 */
[[nodiscard]] auto KeyGenerateInfo::GetAlgo() const -> const KeyAlgo & {
  return algo_;
}

/**
 * @brief Get the Key Size object
 *
 * @return int
 */
[[nodiscard]] auto KeyGenerateInfo::GetKeyLength() const -> int {
  return algo_.KeyLength();
}

/**
 * @brief Get the Expired object
 *
 * @return const QDateTime&
 */
[[nodiscard]] auto KeyGenerateInfo::GetExpireTime() const -> const QDateTime & {
  return expired_;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
[[nodiscard]] auto KeyGenerateInfo::IsNonExpired() const -> bool {
  return non_expired_;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
[[nodiscard]] auto KeyGenerateInfo::IsNoPassPhrase() const -> bool {
  return this->no_passphrase_;
}

/**
 * @brief Set the Non Pass Phrase object
 *
 * @param m_non_pass_phrase
 */
void KeyGenerateInfo::SetNonPassPhrase(bool m_non_pass_phrase) {
  KeyGenerateInfo::no_passphrase_ = m_non_pass_phrase;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
[[nodiscard]] auto KeyGenerateInfo::IsAllowSign() const -> bool {
  return allow_signing_;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
[[nodiscard]] auto KeyGenerateInfo::IsAllowNoPassPhrase() const -> bool {
  return allow_no_pass_phrase_;
}

/**
 * @brief Set the Allow Signing object
 *
 * @param m_allow_signing
 */
void KeyGenerateInfo::SetAllowSign(bool m_allow_signing) {
  if (allow_change_signing_) KeyGenerateInfo::allow_signing_ = m_allow_signing;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
[[nodiscard]] auto KeyGenerateInfo::IsAllowEncr() const -> bool {
  return allow_encryption_;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
[[nodiscard]] auto KeyGenerateInfo::IsAllowCert() const -> bool {
  return allow_certification_;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
[[nodiscard]] auto KeyGenerateInfo::IsAllowAuth() const -> bool {
  return allow_authentication_;
}

/**
 * @brief Set the Allow Authentication object
 *
 * @param m_allow_authentication
 */
void KeyGenerateInfo::SetAllowAuth(bool m_allow_authentication) {
  if (allow_change_authentication_) {
    KeyGenerateInfo::allow_authentication_ = m_allow_authentication;
  }
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
[[nodiscard]] auto KeyGenerateInfo::IsAllowModifySign() const -> bool {
  return allow_change_signing_;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
[[nodiscard]] auto KeyGenerateInfo::IsAllowModifyEncr() const -> bool {
  return allow_change_encryption_;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
[[nodiscard]] auto KeyGenerateInfo::IsAllowModifyCert() const -> bool {
  return allow_change_certification_;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
[[nodiscard]] auto KeyGenerateInfo::IsAllowModifyAuth() const -> bool {
  return allow_change_authentication_;
}

KeyAlgo::KeyAlgo(QString id, QString name, QString type, int length, int opera,
                 EngineSupportList support_ifs,
                 QContainer<QPair<KeyAlgo, EngineSupportList>> sub_algos)
    : id_(std::move(id)),
      name_(std::move(name)),
      type_(std::move(type)),
      length_(length),
      support_ifs_(std::move(support_ifs)),
      sub_algos_(std::move(sub_algos)) {
  encrypt_ = ((opera & kENCRYPT) != 0);
  sign_ = ((opera & kSIGN) != 0);
  auth_ = ((opera & kAUTH) != 0);
  cert_ = ((opera & kCERT) != 0);
};

auto KeyAlgo::Id() const -> QString { return id_; }

auto KeyAlgo::Name() const -> QString { return name_; }

auto KeyAlgo::KeyLength() const -> int { return length_; }

auto KeyAlgo::Type() const -> QString { return type_; }

auto KeyAlgo::CanEncrypt() const -> bool { return encrypt_; }

auto KeyAlgo::CanSign() const -> bool { return sign_; }

auto KeyAlgo::CanAuth() const -> bool { return auth_; }

auto KeyAlgo::CanCert() const -> bool { return cert_; }

auto KeyAlgo::SupportedVersion() const -> QContainer<EngineSupportIf> {
  return support_ifs_;
}

auto KeyAlgo::operator==(const KeyAlgo &o) const -> bool {
  return id_ == o.id_ && type_ == o.type_ && length_ == o.length_;
}

auto KeyAlgo::operator!=(const KeyAlgo &o) const -> bool {
  return !(*this == o);
}

[[nodiscard]] auto KeyAlgo::SubAlgos(int channel) const -> QContainer<KeyAlgo> {
  auto result = QContainer<KeyAlgo>();
  for (const auto &sub_algo : sub_algos_) {
    if (GpgContextSupportIf(channel, sub_algo.second)) {
      result.append(sub_algo.first);
    }
  }
  return result;
}

[[nodiscard]] auto KeyGenerateInfo::SubAlgo() const -> const KeyAlgo & {
  return sub_algo_;
}

void KeyGenerateInfo::SetSubAlgo(const KeyAlgo &sub_algo) {
  sub_algo_ = sub_algo;
}
}  // namespace GpgFrontend

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

#include <gtest/gtest.h>

#include "GpgFrontendTest.h"
#include "ui/dialog/key_generate/KeyGenerateSummary.h"
#include "ui/function/KeyGenSettings.h"
#include "ui/function/KeyGenerateHelper.h"

namespace GpgFrontend::Test {

namespace {

auto PrimaryAlgos() -> QList<KeyAlgo> {
  return KeyGenerateInfo::kPrimaryKeyAlgos +
         KeyGenerateInfo::kHybridPrimaryKeyAlgo;
}

auto SubkeyAlgos() -> QList<KeyAlgo> {
  return KeyGenerateInfo::kSubKeyAlgos + KeyGenerateInfo::kHybridSubKeyAlgos;
}

auto Find(const QString& id, const QString& type, const QList<KeyAlgo>& in)
    -> KeyAlgo {
  auto [found, algo] = GetAlgoByIdType(id, type, in);
  return found ? algo : KeyGenerateInfo::kNoneAlgo;
}

/// The pairings an engine with every algorithm would allow; no channel, so
/// nothing here touches a key database.
auto AllPairings(const KeyAlgo& algo) -> QList<KeyAlgo> {
  if (algo.Id() == u"ky768"_s) {
    return {Find(u"cv25519"_s, u"ECDH"_s, SubkeyAlgos())};
  }
  if (algo.Id() == u"kyber1024"_s) {
    return {Find(u"x448"_s, u"ECDH"_s, SubkeyAlgos())};
  }
  if (algo.Id() == u"mldsa65"_s) {
    return {Find(u"ed25519"_s, u"EdDSA"_s, PrimaryAlgos())};
  }
  if (algo.Id() == u"mldsa87"_s) {
    return {Find(u"ed448"_s, u"EdDSA"_s, PrimaryAlgos())};
  }
  return {};
}

auto NoPairings(const KeyAlgo&) -> QList<KeyAlgo> { return {}; }

}  // namespace

TEST(KeyGenerateSummaryTest, EveryStepHasATitle) {
  for (int i = 0; i < UI::kKeyGeneratePageCount; ++i) {
    EXPECT_FALSE(UI::KeyGenerateStepTitle(static_cast<UI::KeyGenerateStep>(i))
                     .isEmpty());
  }
}

TEST(KeyGenerateSummaryTest, ExpertModeSwapsTheSecondStepOnly) {
  using UI::KeyGenerateStep;
  using UI::NextKeyGenerateStep;
  EXPECT_EQ(NextKeyGenerateStep(KeyGenerateStep::kIdentity, false),
            KeyGenerateStep::kKeyType);
  EXPECT_EQ(NextKeyGenerateStep(KeyGenerateStep::kIdentity, true),
            KeyGenerateStep::kKeyDetails);
  // Both lead on to the same steps, whatever the switch says by then.
  for (const auto expert : {false, true}) {
    EXPECT_EQ(NextKeyGenerateStep(KeyGenerateStep::kKeyType, expert),
              KeyGenerateStep::kProtection);
    EXPECT_EQ(NextKeyGenerateStep(KeyGenerateStep::kKeyDetails, expert),
              KeyGenerateStep::kProtection);
    EXPECT_EQ(NextKeyGenerateStep(KeyGenerateStep::kProtection, expert),
              KeyGenerateStep::kReview);
    EXPECT_FALSE(NextKeyGenerateStep(KeyGenerateStep::kReview, expert));
  }

  // One step, two pages: both are "Step 2 of 4".
  EXPECT_EQ(UI::KeyGenerateStepNumber(KeyGenerateStep::kKeyDetails),
            UI::KeyGenerateStepNumber(KeyGenerateStep::kKeyType));
  EXPECT_EQ(UI::KeyGenerateStepNumber(KeyGenerateStep::kKeyType), 1);
  EXPECT_FALSE(
      UI::KeyGenerateStepTitle(KeyGenerateStep::kKeyDetails).isEmpty());
  EXPECT_TRUE(UI::CanAdvanceKeyGenerate(
      KeyGenerateStep::kKeyDetails,
      {u"Alex Doe"_s, {}, {}, /*algorithms_complete=*/true}));
}

TEST(KeyGenerateSummaryTest, TheIdentityNeedsANameAndAValidAddress) {
  EXPECT_FALSE(UI::CheckKeyGenIdentity({}, {}, {}).Ok());
  EXPECT_TRUE(UI::CheckKeyGenIdentity(u"Alex Doe"_s, {}, {}).Ok());
  EXPECT_TRUE(
      UI::CheckKeyGenIdentity(u"Alex Doe"_s, u"alex@example.com"_s, {}).Ok());
  EXPECT_FALSE(
      UI::CheckKeyGenIdentity(u"Alex Doe"_s, u"not an address"_s, {}).Ok());
  // The delimiters of a mail name-addr would break the user id apart.
  const auto check = UI::CheckKeyGenIdentity(u"Alex <x>"_s, {}, u"(work)"_s);
  EXPECT_FALSE(check.name.isEmpty());
  EXPECT_FALSE(check.comment.isEmpty());
}

TEST(KeyGenerateSummaryTest, NoStepPassesWhatAnEarlierOneRefused) {
  UI::KeyGenerateState state;
  state.algorithms_complete = true;
  for (int i = 0; i < UI::kKeyGenerateStepCount; ++i) {
    EXPECT_FALSE(
        UI::CanAdvanceKeyGenerate(static_cast<UI::KeyGenerateStep>(i), state));
  }

  state.name = u"Alex Doe"_s;
  for (int i = 0; i < UI::kKeyGenerateStepCount; ++i) {
    EXPECT_TRUE(
        UI::CanAdvanceKeyGenerate(static_cast<UI::KeyGenerateStep>(i), state));
  }

  // Half a hybrid holds up every step after the identity.
  state.algorithms_complete = false;
  EXPECT_TRUE(UI::CanAdvanceKeyGenerate(UI::KeyGenerateStep::kIdentity, state));
  EXPECT_FALSE(UI::CanAdvanceKeyGenerate(UI::KeyGenerateStep::kReview, state));
}

TEST(KeyGenerateSummaryTest, EachTileIsTheProfileItsTitleNames) {
  using UI::KeyGenTileRole;
  const auto offered = ResolveKeyGenProfiles(
      MakeDefaultKeyGenProfiles(), PrimaryAlgos(), SubkeyAlgos(), AllPairings);
  const auto featured = UI::FeaturedKeyGenProfiles(offered);
  ASSERT_EQ(featured.size(), 3);

  EXPECT_EQ(featured.at(0).role, KeyGenTileRole::kModern);
  const auto& modern = offered.at(featured.at(0).index);
  EXPECT_EQ(modern.primary.Id(), u"ed25519"_s);
  EXPECT_FALSE(modern.post_quantum);

  EXPECT_EQ(featured.at(1).role, KeyGenTileRole::kCompatibility);
  EXPECT_EQ(offered.at(featured.at(1).index).primary.Id(), u"rsa4096"_s);

  // Classical signing, post-quantum encryption; never the profiles that sign
  // post-quantum too.
  EXPECT_EQ(featured.at(2).role, KeyGenTileRole::kHybridEncryption);
  const auto& hybrid = offered.at(featured.at(2).index);
  EXPECT_EQ(hybrid.primary.Id(), u"ed25519"_s);
  EXPECT_TRUE(hybrid.subkey.IsPostQuantum());

  for (const auto& f : featured) {
    EXPECT_FALSE(UI::KeyGenTileTitle(f.role).isEmpty());
    EXPECT_FALSE(UI::KeyGenTileDescription(f.role).isEmpty());
  }
  EXPECT_TRUE(UI::FeaturedKeyGenProfiles({}).isEmpty());
}

TEST(KeyGenerateSummaryTest, AnEngineWithoutARoleShowsNoTileForIt) {
  // Without hybrids, there is nothing to call hybrid encryption.
  const auto classical = ResolveKeyGenProfiles(
      MakeDefaultKeyGenProfiles(), KeyGenerateInfo::kPrimaryKeyAlgos,
      KeyGenerateInfo::kSubKeyAlgos, NoPairings);
  for (const auto& f : UI::FeaturedKeyGenProfiles(classical)) {
    EXPECT_NE(f.role, UI::KeyGenTileRole::kHybridEncryption);
  }
}

TEST(KeyGenerateSummaryTest, AProfileIsDescribedWhateverTheEngine) {
  const auto profiles = MakeDefaultKeyGenProfiles();
  const auto& modern = profiles.at(0);  // Ed25519 / X25519
  const auto& rsa = profiles.at(3);     // RSA-4096, no subkey
  const auto& hybrid = profiles.at(6);  // Ed25519 / ML-KEM-768 + X25519

  const auto all = UI::DescribeKeyGenProfileFacts(modern, PrimaryAlgos(),
                                                  SubkeyAlgos(), AllPairings);
  EXPECT_EQ(all.signing, u"Ed25519"_s);
  EXPECT_EQ(all.encryption, u"X25519"_s);
  EXPECT_EQ(all.algorithms, u"Ed25519 \u00b7 X25519"_s);
  EXPECT_FALSE(all.validity.isEmpty());
  EXPECT_TRUE(all.missing.isEmpty());

  // A primary key that encrypts says so, rather than "None".
  const auto rsa_facts = UI::DescribeKeyGenProfileFacts(
      rsa, PrimaryAlgos(), SubkeyAlgos(), AllPairings);
  EXPECT_TRUE(rsa_facts.encryption.contains(u"RSA-4096"_s))
      << rsa_facts.encryption.toStdString();

  // An engine without hybrids names exactly what it lacks, and still says
  // what the profile is.
  const auto lacking =
      UI::DescribeKeyGenProfileFacts(hybrid, KeyGenerateInfo::kPrimaryKeyAlgos,
                                     KeyGenerateInfo::kSubKeyAlgos, NoPairings);
  EXPECT_EQ(lacking.encryption, u"ML-KEM-768 + X25519"_s);
  EXPECT_EQ(lacking.missing, QStringList{u"ML-KEM-768"_s});

  // Lifetimes that differ are both given.
  auto split = modern;
  split.s_key_validity = u"1y"_s;
  EXPECT_NE(UI::DescribeKeyGenProfileFacts(split, PrimaryAlgos(), SubkeyAlgos(),
                                           AllPairings)
                .validity,
            all.validity);
}

TEST(KeyGenerateSummaryTest, AlgorithmsGoByTheirOwnNames) {
  const auto none = KeyGenerateInfo::kNoneAlgo;
  EXPECT_EQ(
      UI::DescribeKeyAlgo(Find(u"ed25519"_s, u"EdDSA"_s, PrimaryAlgos()), none),
      u"Ed25519"_s);
  EXPECT_EQ(
      UI::DescribeKeyAlgo(Find(u"cv25519"_s, u"ECDH"_s, SubkeyAlgos()), none),
      u"X25519"_s);
  EXPECT_EQ(
      UI::DescribeKeyAlgo(Find(u"rsa4096"_s, u"RSA"_s, PrimaryAlgos()), none),
      u"RSA-4096"_s);
  EXPECT_EQ(UI::DescribeKeyAlgo(
                Find(u"brainpoolp256r1"_s, u"ECDSA"_s, PrimaryAlgos()), none),
            u"Brainpool P-256"_s);
  EXPECT_EQ(UI::DescribeKeyAlgo(Find(u"ky768"_s, {}, SubkeyAlgos()),
                                Find(u"cv25519"_s, u"ECDH"_s, SubkeyAlgos())),
            u"ML-KEM-768 + X25519"_s);
  EXPECT_EQ(UI::DescribeKeyAlgo(none, none), QObject::tr("None"));
}

TEST(KeyGenerateSummaryTest, TheReviewNamesWhatIsAboutToBeMade) {
  KeyGenerateInfo key;
  key.SetName(u"Alex Doe"_s);
  key.SetAlgo(Find(u"ed25519"_s, u"EdDSA"_s, PrimaryAlgos()));
  key.SetNonPassPhrase(true);

  const auto rows = UI::KeyGenerateReviewRows(key, nullptr, {}, false, true);
  QStringList values;
  bool warned = false;
  for (const auto& row : rows) {
    values << row.value;
    if (row.danger) warned = true;
  }
  EXPECT_TRUE(values.contains(key.GetUserid()));
  EXPECT_TRUE(warned) << "a key without a passphrase must be called out";
}

TEST(KeyGenerateHelperTest, AlgorithmListsNameEachEntryOnce) {
  // Widgets belong to the GUI thread; the test body does not run there.
  QStringList labels;
  RunOnMainThread([&labels]() {
    QComboBox combo;
    UI::PopulateAlgoComboBox(&combo, SubkeyAlgos());
    const auto* model = qobject_cast<QStandardItemModel*>(combo.model());
    for (int i = 0; i < combo.count(); ++i) {
      const auto* item = model != nullptr ? model->item(i) : nullptr;
      if (item != nullptr && !item->flags().testFlag(Qt::ItemIsSelectable)) {
        continue;  // a section header
      }
      labels << combo.itemText(i);
    }
  });

  ASSERT_FALSE(labels.isEmpty());
  EXPECT_EQ(QSet<QString>(labels.cbegin(), labels.cend()).size(), labels.size())
      << labels.join(u", "_s).toStdString();
  // The names they go by, without the family label said twice.
  EXPECT_TRUE(labels.contains(u"X25519"_s));
  EXPECT_TRUE(labels.contains(u"Ed25519"_s));
  EXPECT_TRUE(labels.contains(u"RSA"_s));
  EXPECT_FALSE(labels.contains(u"RSA (RSA)"_s));
}

TEST(KeyGenerateHelperTest, EngineAlgorithmNamesBecomeGenerationIds) {
  EXPECT_EQ(UI::NormalizeKeyAlgoId(u"ED25519"_s), u"ed25519"_s);
  EXPECT_EQ(UI::NormalizeKeyAlgoId(u"RSA 3072"_s), u"rsa3072"_s);
  EXPECT_EQ(UI::NormalizeKeyAlgoId(u"RSA3072"_s), u"rsa3072"_s);
  EXPECT_EQ(UI::NormalizeKeyAlgoId(u"NIST P-256"_s), u"nistp256"_s);
  EXPECT_EQ(UI::NormalizeKeyAlgoId(u"Brainpool P-384"_s), u"brainpoolp384r1"_s);
  EXPECT_EQ(UI::NormalizeKeyAlgoId(u"BRAINPOOLP256R1"_s), u"brainpoolp256r1"_s);
  EXPECT_EQ(UI::NormalizeKeyAlgoId(u"ML-DSA-65_ED25519"_s), u"mldsa65"_s);
}

TEST(KeyGenerateHelperTest, ASubkeyFollowsThePrimaryKeysFamily) {
  using UI::SubkeyPurpose;
  const auto subkeys = SubkeyAlgos();
  const auto id = [&subkeys](SubkeyPurpose purpose, const QString& primary) {
    return UI::SuggestSubkeyAlgo(purpose, primary, subkeys).Id();
  };

  EXPECT_EQ(id(SubkeyPurpose::kEncrypt, u"ed25519"_s), u"cv25519"_s);
  EXPECT_EQ(id(SubkeyPurpose::kSign, u"ed25519"_s), u"ed25519"_s);
  EXPECT_EQ(id(SubkeyPurpose::kEncrypt, u"ed448"_s), u"x448"_s);
  EXPECT_EQ(id(SubkeyPurpose::kEncrypt, u"rsa3072"_s), u"rsa3072"_s);
  EXPECT_EQ(id(SubkeyPurpose::kEncrypt, u"mldsa65"_s), u"ky768"_s);
  EXPECT_EQ(id(SubkeyPurpose::kEncrypt, u"dsa2048"_s), u"elg2048"_s);

  // A curve keeps its id for both jobs; the job decides the type.
  const auto nist_encrypt =
      UI::SuggestSubkeyAlgo(SubkeyPurpose::kEncrypt, u"nistp256"_s, subkeys);
  EXPECT_EQ(nist_encrypt.Id(), u"nistp256"_s);
  EXPECT_TRUE(nist_encrypt.CanEncrypt());
  EXPECT_TRUE(
      UI::SuggestSubkeyAlgo(SubkeyPurpose::kSign, u"nistp256"_s, subkeys)
          .CanSign());
}

TEST(KeyGenerateHelperTest, ASubkeyFallsBackToAnythingThatDoesTheJob) {
  // A v4 key under an engine without its family's encryption algorithm.
  const QList<KeyAlgo> only_rsa{Find(u"rsa2048"_s, u"RSA"_s, SubkeyAlgos())};
  EXPECT_EQ(
      UI::SuggestSubkeyAlgo(UI::SubkeyPurpose::kEncrypt, u"ed25519"_s, only_rsa)
          .Id(),
      u"rsa2048"_s);
  EXPECT_EQ(
      UI::SuggestSubkeyAlgo(UI::SubkeyPurpose::kEncrypt, u"ed25519"_s, {}),
      KeyGenerateInfo::kNoneAlgo);
}

}  // namespace GpgFrontend::Test

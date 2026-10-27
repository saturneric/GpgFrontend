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
#include "ui/function/KeyGenProfile.h"
#include "ui/function/KeyGenSettings.h"
#include "ui/function/KeyGenerateHelper.h"

namespace GpgFrontend::Test {

namespace {

using UI::KeyGenProfile;
using UI::ResolvedKeyGenProfile;

auto PrimaryAlgos() -> QList<KeyAlgo> {
  return KeyGenerateInfo::kPrimaryKeyAlgos +
         KeyGenerateInfo::kHybridPrimaryKeyAlgo;
}

auto SubkeyAlgos() -> QList<KeyAlgo> {
  return KeyGenerateInfo::kSubKeyAlgos + KeyGenerateInfo::kHybridSubKeyAlgos;
}

auto Find(const QString& id, const QString& type, const QList<KeyAlgo>& in)
    -> KeyAlgo {
  auto [found, algo] = UI::GetAlgoByIdType(id, type, in);
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

auto Now() -> QDateTime {
  return QDateTime(QDate(2026, 1, 15), QTime(12, 0), QTimeZone::UTC);
}

}  // namespace

TEST(KeyGenProfileTest, EveryDefaultSurvivesItsStoredForm) {
  for (const auto& p : UI::MakeDefaultKeyGenProfiles()) {
    const auto back = UI::KeyGenProfileFromJson(UI::KeyGenProfileToJson(p));
    ASSERT_TRUE(back.has_value()) << p.name.toStdString();
    EXPECT_EQ(*back, p) << p.name.toStdString();
  }
}

TEST(KeyGenProfileTest, EveryDefaultNameIsAValidName) {
  // Editing a built-in profile must not be refused for the name it came with,
  // nor a copy of it for the " (2)" it gets.
  const auto defaults = UI::MakeDefaultKeyGenProfiles();
  for (qsizetype i = 0; i < defaults.size(); ++i) {
    EXPECT_EQ(UI::CheckKeyGenProfileName(defaults.at(i).name, defaults,
                                         static_cast<int>(i)),
              UI::KeyGenProfileNameProblem::kNone)
        << defaults.at(i).name.toStdString();
    EXPECT_LE(defaults.at(i).name.size() + 4, UI::kKeyGenProfileNameMaxLen);
  }
}

TEST(KeyGenProfileTest, EveryDefaultHasADescription) {
  for (const auto& p : UI::MakeDefaultKeyGenProfiles()) {
    EXPECT_FALSE(p.description.isEmpty()) << p.name.toStdString();
    EXPECT_LE(p.description.size(), UI::kKeyGenProfileDescriptionMaxLen);
  }
}

TEST(KeyGenProfileTest, AProfileTheOldDialogCachedStillReads) {
  // Written by the tabbed dialog: no description, no subkey validity echo,
  // a hybrid's type left out.
  const auto json = QJsonDocument::fromJson(R"({
      "name": "Mine", "hidden": false,
      "primary": {"algo": "ED25519", "type": "EdDSA", "validity": "1y"},
      "subkey": {"algo": "ky768", "type": "HYBRID-KEM", "validity": "6m",
                 "sub_algo": "cv25519"}})")
                        .object();
  const auto p = UI::KeyGenProfileFromJson(json);
  ASSERT_TRUE(p.has_value());
  EXPECT_EQ(p->name, u"Mine"_s);
  EXPECT_TRUE(p->description.isEmpty());
  EXPECT_EQ(p->key_algo, u"ed25519"_s);  // ids are compared lower-case
  EXPECT_EQ(p->key_validity, u"1y"_s);
  ASSERT_TRUE(p->has_s_key);
  EXPECT_EQ(p->s_key_validity, u"6m"_s);
  EXPECT_EQ(p->s_key_sub_algo, u"cv25519"_s);
  EXPECT_FALSE(p->s_key_sub_algo_type.isEmpty());  // looked up from the id
}

TEST(KeyGenProfileTest, WhatIsNotAProfileIsSkipped) {
  QJsonArray array{
      QJsonObject{}, QJsonObject{{"name", "No primary"}},
      UI::KeyGenProfileToJson(UI::MakeDefaultKeyGenProfiles().front())};
  EXPECT_EQ(UI::KeyGenProfilesFromJson(array).size(), 1);
}

TEST(KeyGenProfileTest, TheFingerprintIsWhatAProfileMakes) {
  const auto base = UI::MakeDefaultKeyGenProfiles().front();
  const auto fpr = UI::KeyGenProfileFingerprint(base);

  // Renamed, reworded, hidden or spelled in another case: the same keys.
  auto same = base;
  same.name = u"Mine"_s;
  same.description = u"for work"_s;
  same.hidden = true;
  same.key_algo = base.key_algo.toUpper();
  EXPECT_EQ(UI::KeyGenProfileFingerprint(same), fpr);

  // Another validity or another subkey: other keys.
  auto longer = base;
  longer.key_validity = u"5y"_s;
  EXPECT_NE(UI::KeyGenProfileFingerprint(longer), fpr);
  auto no_subkey = base;
  no_subkey.has_s_key = false;
  EXPECT_NE(UI::KeyGenProfileFingerprint(no_subkey), fpr);

  // Every built-in makes something of its own.
  QSet<QString> seen;
  for (const auto& p : UI::MakeDefaultKeyGenProfiles()) {
    EXPECT_FALSE(seen.contains(UI::KeyGenProfileFingerprint(p)))
        << p.name.toStdString();
    seen.insert(UI::KeyGenProfileFingerprint(p));
  }
}

TEST(KeyGenProfileTest, RestoringAddsOnlyWhatIsMissing) {
  const auto defaults = UI::MakeDefaultKeyGenProfiles();

  // Nothing missing: nothing added, nothing changed.
  EXPECT_EQ(UI::RestoreDefaultKeyGenProfiles(defaults), defaults);

  // A renamed built-in and one of the user's own stay as they are; the one
  // deleted comes back, once.
  auto current = defaults;
  current[0].name = u"Everyday"_s;
  const auto deleted = current.takeAt(1);
  KeyGenProfile own;
  own.name = u"Own"_s;
  own.key_algo = u"rsa3072"_s;
  own.key_algo_type = u"RSA"_s;
  own.key_validity = u"1y"_s;
  current.append(own);

  const auto restored = UI::RestoreDefaultKeyGenProfiles(current);
  ASSERT_EQ(restored.size(), current.size() + 1);
  EXPECT_EQ(restored.mid(0, current.size()), current);
  EXPECT_EQ(restored.last(), deleted);

  // A built-in whose name was taken by something else comes back renamed.
  auto taken = defaults;
  const auto gone = taken.takeAt(1);
  taken[0].name = gone.name;
  const auto back = UI::RestoreDefaultKeyGenProfiles(taken);
  ASSERT_EQ(back.size(), taken.size() + 1);
  EXPECT_EQ(UI::KeyGenProfileFingerprint(back.last()),
            UI::KeyGenProfileFingerprint(gone));
  EXPECT_NE(back.last().name, gone.name);
}

TEST(KeyGenProfileTest, ValidityStringsMeanWhatTheySay) {
  const auto now = Now();
  EXPECT_EQ(UI::ParseKeyGenValidity(u"2y"_s, now), now.addYears(2));
  EXPECT_EQ(UI::ParseKeyGenValidity(u"6m"_s, now), now.addMonths(6));
  EXPECT_EQ(UI::ParseKeyGenValidity(u"90d"_s, now), now.addDays(90));
  EXPECT_EQ(UI::ParseKeyGenValidity(u"3600t"_s, now), now.addSecs(3600));
  // Nonsense is the two-year default rather than an expired key.
  EXPECT_EQ(UI::ParseKeyGenValidity(u"soon"_s, now), now.addYears(2));
  EXPECT_EQ(UI::ParseKeyGenValidity(u"-3y"_s, now), now.addYears(2));
  EXPECT_TRUE(UI::IsNeverExpiringValidity(u"forever"_s));
  EXPECT_TRUE(UI::IsNeverExpiringValidity(u"none"_s));
  EXPECT_FALSE(UI::IsNeverExpiringValidity(u"10y"_s));
}

TEST(KeyGenProfileTest, AnExpiryIsDescribedByThePresetItMatches) {
  const auto now = Now();
  for (const auto& preset : UI::KeyGenValidityPresets()) {
    const auto never = UI::IsNeverExpiringValidity(preset);
    EXPECT_EQ(
        UI::KeyGenValidityFor(never, UI::ParseKeyGenValidity(preset, now), now),
        preset)
        << preset.toStdString();
  }
  // Anything else is counted in whole days.
  EXPECT_EQ(UI::KeyGenValidityFor(false, now.addDays(45), now), u"45d"_s);
}

TEST(KeyGenProfileTest, ValidityHasAName) {
  for (const auto& preset : UI::KeyGenValidityPresets()) {
    EXPECT_FALSE(UI::KeyGenValidityDisplayName(preset).isEmpty());
  }
  EXPECT_FALSE(UI::KeyGenValidityDisplayName(u"45d"_s).isEmpty());
}

TEST(KeyGenProfileTest, NamesAreCheckedAgainstTheOthers) {
  const auto profiles = UI::MakeDefaultKeyGenProfiles();
  using P = UI::KeyGenProfileNameProblem;
  EXPECT_EQ(UI::CheckKeyGenProfileName(u"  "_s, profiles), P::kEmpty);
  EXPECT_EQ(UI::CheckKeyGenProfileName(
                QString(UI::kKeyGenProfileNameMaxLen + 1, u'x'), profiles),
            P::kTooLong);
  EXPECT_EQ(UI::CheckKeyGenProfileName(u"custom"_s, profiles), P::kReserved);
  EXPECT_EQ(
      UI::CheckKeyGenProfileName(profiles.front().name.toUpper(), profiles),
      P::kTaken);
  // Renaming a profile to its own name is not taking somebody else's.
  EXPECT_EQ(UI::CheckKeyGenProfileName(profiles.front().name, profiles, 0),
            P::kNone);
  EXPECT_EQ(UI::CheckKeyGenProfileName(u"Mine"_s, profiles), P::kNone);
  EXPECT_FALSE(UI::DescribeKeyGenProfileNameProblem(P::kTaken).isEmpty());
  EXPECT_TRUE(UI::DescribeKeyGenProfileNameProblem(P::kNone).isEmpty());
}

TEST(KeyGenProfileTest, WhatTheEngineCannotMakeIsNotOffered) {
  const auto profiles = UI::MakeDefaultKeyGenProfiles();

  const auto all = UI::ResolveKeyGenProfiles(profiles, PrimaryAlgos(),
                                             SubkeyAlgos(), AllPairings);
  // Everything but the hidden DSA profile.
  EXPECT_EQ(all.size(), profiles.size() - 1);
  for (const auto& r : all) EXPECT_FALSE(r.profile.hidden);

  // Without hybrids, none of the post-quantum profiles can be made.
  const auto classical =
      UI::ResolveKeyGenProfiles(profiles, KeyGenerateInfo::kPrimaryKeyAlgos,
                                KeyGenerateInfo::kSubKeyAlgos, NoPairings);
  for (const auto& r : classical) {
    EXPECT_FALSE(r.post_quantum) << r.profile.name.toStdString();
  }
  EXPECT_LT(classical.size(), all.size());
}

TEST(KeyGenProfileTest, AHybridComesWithBothHalves) {
  const auto offered =
      UI::ResolveKeyGenProfiles(UI::MakeDefaultKeyGenProfiles(), PrimaryAlgos(),
                                SubkeyAlgos(), AllPairings);
  bool saw_hybrid = false;
  for (const auto& r : offered) {
    if (!r.post_quantum) continue;
    saw_hybrid = true;
    EXPECT_NE(r.subkey_sub, KeyGenerateInfo::kNoneAlgo)
        << r.profile.name.toStdString();
  }
  EXPECT_TRUE(saw_hybrid);
}

TEST(KeyGenProfileTest, TheDefaultIsTheNamedOneThenEd25519) {
  const auto offered =
      UI::ResolveKeyGenProfiles(UI::MakeDefaultKeyGenProfiles(), PrimaryAlgos(),
                                SubkeyAlgos(), AllPairings);
  const auto ed = UI::PickDefaultKeyGenProfile(offered, {});
  ASSERT_GE(ed, 0);
  EXPECT_EQ(offered.at(ed).primary.Id(), u"ed25519"_s);

  const auto rsa =
      UI::PickDefaultKeyGenProfile(offered, u"rsa-4096 (high compatibility)"_s);
  ASSERT_GE(rsa, 0);
  EXPECT_EQ(offered.at(rsa).primary.Id(), u"rsa4096"_s);

  // A name nobody has falls back rather than leaving nothing chosen.
  EXPECT_EQ(UI::PickDefaultKeyGenProfile(offered, u"Gone"_s), ed);
  EXPECT_EQ(UI::PickDefaultKeyGenProfile({}, {}), -1);
}

TEST(KeyGenProfileTest, ApplyingAProfileSetsUpBothKeys) {
  auto profile = UI::MakeDefaultKeyGenProfiles().front();
  profile.key_validity = u"1y"_s;
  profile.s_key_validity = u"forever"_s;
  const auto r = UI::ResolveKeyGenProfile(profile, PrimaryAlgos(),
                                          SubkeyAlgos(), AllPairings);
  ASSERT_TRUE(r.has_value());

  KeyGenerateInfo key;
  key.SetNonPassPhrase(true);
  QSharedPointer<KeyGenerateInfo> subkey;
  UI::ApplyKeyGenProfile(*r, Now(), key, subkey);

  EXPECT_EQ(key.GetAlgo().Id(), u"ed25519"_s);
  EXPECT_FALSE(key.IsNonExpired());
  EXPECT_EQ(key.GetExpireTime(), Now().addYears(1));
  ASSERT_NE(subkey, nullptr);
  EXPECT_EQ(subkey->GetAlgo().Id(), u"cv25519"_s);
  EXPECT_TRUE(subkey->IsNonExpired());
  EXPECT_TRUE(subkey->IsNoPassPhrase());  // one choice for both keys

  // A profile without a subkey takes one away.
  const auto rsa =
      UI::ResolveKeyGenProfile(UI::MakeDefaultKeyGenProfiles().at(3),
                               PrimaryAlgos(), SubkeyAlgos(), AllPairings);
  ASSERT_TRUE(rsa.has_value());
  UI::ApplyKeyGenProfile(*rsa, Now(), key, subkey);
  EXPECT_EQ(subkey, nullptr);
}

TEST(KeyGenProfileTest, SavingWhatWasAppliedGivesTheSameProfile) {
  for (const auto& r :
       UI::ResolveKeyGenProfiles(UI::MakeDefaultKeyGenProfiles(),
                                 PrimaryAlgos(), SubkeyAlgos(), AllPairings)) {
    KeyGenerateInfo key;
    QSharedPointer<KeyGenerateInfo> subkey;
    UI::ApplyKeyGenProfile(r, Now(), key, subkey);

    auto saved = UI::KeyGenProfileFromInfo(key, subkey, Now());
    saved.name = r.profile.name;
    saved.description = r.profile.description;
    auto expected = r.profile;
    // The pairing a hybrid was given is written down, whether or not the
    // profile named it.
    expected.key_sub_algo = saved.key_sub_algo;
    expected.key_sub_algo_type = saved.key_sub_algo_type;
    EXPECT_EQ(saved, expected) << r.profile.name.toStdString();
  }
}

TEST(KeyGenProfileTest, PostQuantumNeedsTheV6Format) {
  KeyGenerateInfo key;
  key.SetAlgo(Find(u"ed25519"_s, u"EdDSA"_s, PrimaryAlgos()));
  EXPECT_EQ(UI::RequiredKeyVersion(key, nullptr), 0);

  auto subkey = QSharedPointer<KeyGenerateInfo>::create(true);
  subkey->SetAlgo(Find(u"ky768"_s, {}, SubkeyAlgos()));
  EXPECT_EQ(UI::RequiredKeyVersion(key, subkey), 6);
}

TEST(KeyGenSettingsTest, FirstUseMovesTheOldProfilesOver) {
  QJsonArray legacy{QJsonObject{
      {"name", "Mine"},
      {"primary", QJsonObject{{"algo", "rsa3072"}, {"validity", "2y"}}}}};
  const auto moved = UI::KeyGenSettingsFromStored({}, legacy);
  EXPECT_FALSE(moved.profiles_stored);  // still to be written
  ASSERT_EQ(moved.profiles.size(), 1);
  EXPECT_EQ(moved.profiles.front().name, u"Mine"_s);

  // With nothing to move, the defaults.
  const auto fresh = UI::KeyGenSettingsFromStored({}, {});
  EXPECT_EQ(fresh.profiles, UI::MakeDefaultKeyGenProfiles());
}

TEST(KeyGenSettingsTest, StoredSettingsWinOverTheOldCache) {
  UI::KeyGenSO stored(QJsonObject{});
  stored.default_profile = u"Mine"_s;
  stored.uid_pattern.name = u"test-{token:12}"_s;
  stored.profiles = {};  // deleting every profile is a choice too
  const auto json = stored.ToJson();

  QJsonArray legacy{
      UI::KeyGenProfileToJson(UI::MakeDefaultKeyGenProfiles().front())};
  const auto read = UI::KeyGenSettingsFromStored(json, legacy);
  EXPECT_TRUE(read.profiles_stored);
  EXPECT_TRUE(read.profiles.isEmpty());
  EXPECT_EQ(read.default_profile, u"Mine"_s);
  EXPECT_EQ(read.uid_pattern.name, u"test-{token:12}"_s);
}

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
  const auto offered =
      UI::ResolveKeyGenProfiles(UI::MakeDefaultKeyGenProfiles(), PrimaryAlgos(),
                                SubkeyAlgos(), AllPairings);
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
  const auto classical = UI::ResolveKeyGenProfiles(
      UI::MakeDefaultKeyGenProfiles(), KeyGenerateInfo::kPrimaryKeyAlgos,
      KeyGenerateInfo::kSubKeyAlgos, NoPairings);
  for (const auto& f : UI::FeaturedKeyGenProfiles(classical)) {
    EXPECT_NE(f.role, UI::KeyGenTileRole::kHybridEncryption);
  }
}

TEST(KeyGenerateSummaryTest, AProfileIsDescribedWhateverTheEngine) {
  const auto profiles = UI::MakeDefaultKeyGenProfiles();
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

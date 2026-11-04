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
#include "core/function/KeyGenProfile.h"

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

auto Now() -> QDateTime {
  return QDateTime(QDate(2026, 1, 15), QTime(12, 0), QTimeZone::UTC);
}

}  // namespace
TEST(KeyGenProfileTest, EveryDefaultSurvivesItsStoredForm) {
  for (const auto& p : MakeDefaultKeyGenProfiles()) {
    const auto back = KeyGenProfileFromJson(KeyGenProfileToJson(p));
    ASSERT_TRUE(back.has_value()) << p.name.toStdString();
    EXPECT_EQ(*back, p) << p.name.toStdString();
  }
}

TEST(KeyGenProfileTest, EveryDefaultNameIsAValidName) {
  // Editing a built-in profile must not be refused for the name it came with,
  // nor a copy of it for the " (2)" it gets.
  const auto defaults = MakeDefaultKeyGenProfiles();
  for (qsizetype i = 0; i < defaults.size(); ++i) {
    EXPECT_EQ(CheckKeyGenProfileName(defaults.at(i).name, defaults,
                                         static_cast<int>(i)),
              KeyGenProfileNameProblem::kNone)
        << defaults.at(i).name.toStdString();
    EXPECT_LE(defaults.at(i).name.size() + 4, kKeyGenProfileNameMaxLen);
  }
}

TEST(KeyGenProfileTest, EveryDefaultHasADescription) {
  for (const auto& p : MakeDefaultKeyGenProfiles()) {
    EXPECT_FALSE(p.description.isEmpty()) << p.name.toStdString();
    EXPECT_LE(p.description.size(), kKeyGenProfileDescriptionMaxLen);
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
  const auto p = KeyGenProfileFromJson(json);
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
      KeyGenProfileToJson(MakeDefaultKeyGenProfiles().front())};
  EXPECT_EQ(KeyGenProfilesFromJson(array).size(), 1);
}

TEST(KeyGenProfileTest, TheFingerprintIsWhatAProfileMakes) {
  const auto base = MakeDefaultKeyGenProfiles().front();
  const auto fpr = KeyGenProfileFingerprint(base);

  // Renamed, reworded, hidden or spelled in another case: the same keys.
  auto same = base;
  same.name = u"Mine"_s;
  same.description = u"for work"_s;
  same.hidden = true;
  same.key_algo = base.key_algo.toUpper();
  EXPECT_EQ(KeyGenProfileFingerprint(same), fpr);

  // Another validity or another subkey: other keys.
  auto longer = base;
  longer.key_validity = u"5y"_s;
  EXPECT_NE(KeyGenProfileFingerprint(longer), fpr);
  auto no_subkey = base;
  no_subkey.has_s_key = false;
  EXPECT_NE(KeyGenProfileFingerprint(no_subkey), fpr);

  // Every built-in makes something of its own.
  QSet<QString> seen;
  for (const auto& p : MakeDefaultKeyGenProfiles()) {
    EXPECT_FALSE(seen.contains(KeyGenProfileFingerprint(p)))
        << p.name.toStdString();
    seen.insert(KeyGenProfileFingerprint(p));
  }
}

TEST(KeyGenProfileTest, RestoringAddsOnlyWhatIsMissing) {
  const auto defaults = MakeDefaultKeyGenProfiles();

  // Nothing missing: nothing added, nothing changed.
  EXPECT_EQ(RestoreDefaultKeyGenProfiles(defaults), defaults);

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

  const auto restored = RestoreDefaultKeyGenProfiles(current);
  ASSERT_EQ(restored.size(), current.size() + 1);
  EXPECT_EQ(restored.mid(0, current.size()), current);
  EXPECT_EQ(restored.last(), deleted);

  // A built-in whose name was taken by something else comes back renamed.
  auto taken = defaults;
  const auto gone = taken.takeAt(1);
  taken[0].name = gone.name;
  const auto back = RestoreDefaultKeyGenProfiles(taken);
  ASSERT_EQ(back.size(), taken.size() + 1);
  EXPECT_EQ(KeyGenProfileFingerprint(back.last()),
            KeyGenProfileFingerprint(gone));
  EXPECT_NE(back.last().name, gone.name);
}

TEST(KeyGenProfileTest, ValidityStringsMeanWhatTheySay) {
  const auto now = Now();
  EXPECT_EQ(ParseKeyGenValidity(u"2y"_s, now), now.addYears(2));
  EXPECT_EQ(ParseKeyGenValidity(u"6m"_s, now), now.addMonths(6));
  EXPECT_EQ(ParseKeyGenValidity(u"90d"_s, now), now.addDays(90));
  EXPECT_EQ(ParseKeyGenValidity(u"3600t"_s, now), now.addSecs(3600));
  // Nonsense is the two-year default rather than an expired key.
  EXPECT_EQ(ParseKeyGenValidity(u"soon"_s, now), now.addYears(2));
  EXPECT_EQ(ParseKeyGenValidity(u"-3y"_s, now), now.addYears(2));
  EXPECT_TRUE(IsNeverExpiringValidity(u"forever"_s));
  EXPECT_TRUE(IsNeverExpiringValidity(u"none"_s));
  EXPECT_FALSE(IsNeverExpiringValidity(u"10y"_s));
}

TEST(KeyGenProfileTest, AnExpiryIsDescribedByThePresetItMatches) {
  const auto now = Now();
  for (const auto& preset : KeyGenValidityPresets()) {
    const auto never = IsNeverExpiringValidity(preset);
    EXPECT_EQ(
        KeyGenValidityFor(never, ParseKeyGenValidity(preset, now), now),
        preset)
        << preset.toStdString();
  }
  // Anything else is counted in whole days.
  EXPECT_EQ(KeyGenValidityFor(false, now.addDays(45), now), u"45d"_s);
}

TEST(KeyGenProfileTest, ValidityHasAName) {
  for (const auto& preset : KeyGenValidityPresets()) {
    EXPECT_FALSE(KeyGenValidityDisplayName(preset).isEmpty());
  }
  EXPECT_FALSE(KeyGenValidityDisplayName(u"45d"_s).isEmpty());
}

TEST(KeyGenProfileTest, NamesAreCheckedAgainstTheOthers) {
  const auto profiles = MakeDefaultKeyGenProfiles();
  using P = KeyGenProfileNameProblem;
  EXPECT_EQ(CheckKeyGenProfileName(u"  "_s, profiles), P::kEmpty);
  EXPECT_EQ(CheckKeyGenProfileName(
                QString(kKeyGenProfileNameMaxLen + 1, u'x'), profiles),
            P::kTooLong);
  EXPECT_EQ(CheckKeyGenProfileName(u"custom"_s, profiles), P::kReserved);
  EXPECT_EQ(
      CheckKeyGenProfileName(profiles.front().name.toUpper(), profiles),
      P::kTaken);
  // Renaming a profile to its own name is not taking somebody else's.
  EXPECT_EQ(CheckKeyGenProfileName(profiles.front().name, profiles, 0),
            P::kNone);
  EXPECT_EQ(CheckKeyGenProfileName(u"Mine"_s, profiles), P::kNone);
  EXPECT_FALSE(DescribeKeyGenProfileNameProblem(P::kTaken).isEmpty());
  EXPECT_TRUE(DescribeKeyGenProfileNameProblem(P::kNone).isEmpty());
}

TEST(KeyGenProfileTest, WhatTheEngineCannotMakeIsNotOffered) {
  const auto profiles = MakeDefaultKeyGenProfiles();

  const auto all = ResolveKeyGenProfiles(profiles, PrimaryAlgos(),
                                             SubkeyAlgos(), AllPairings);
  // Everything but the hidden DSA profile.
  EXPECT_EQ(all.size(), profiles.size() - 1);
  for (const auto& r : all) EXPECT_FALSE(r.profile.hidden);

  // Without hybrids, none of the post-quantum profiles can be made.
  const auto classical =
      ResolveKeyGenProfiles(profiles, KeyGenerateInfo::kPrimaryKeyAlgos,
                                KeyGenerateInfo::kSubKeyAlgos, NoPairings);
  for (const auto& r : classical) {
    EXPECT_FALSE(r.post_quantum) << r.profile.name.toStdString();
  }
  EXPECT_LT(classical.size(), all.size());
}

TEST(KeyGenProfileTest, AHybridComesWithBothHalves) {
  const auto offered =
      ResolveKeyGenProfiles(MakeDefaultKeyGenProfiles(), PrimaryAlgos(),
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
      ResolveKeyGenProfiles(MakeDefaultKeyGenProfiles(), PrimaryAlgos(),
                                SubkeyAlgos(), AllPairings);
  const auto ed = PickDefaultKeyGenProfile(offered, {});
  ASSERT_GE(ed, 0);
  EXPECT_EQ(offered.at(ed).primary.Id(), u"ed25519"_s);

  const auto rsa =
      PickDefaultKeyGenProfile(offered, u"rsa-4096 (high compatibility)"_s);
  ASSERT_GE(rsa, 0);
  EXPECT_EQ(offered.at(rsa).primary.Id(), u"rsa4096"_s);

  // A name nobody has falls back rather than leaving nothing chosen.
  EXPECT_EQ(PickDefaultKeyGenProfile(offered, u"Gone"_s), ed);
  EXPECT_EQ(PickDefaultKeyGenProfile({}, {}), -1);
}

TEST(KeyGenProfileTest, ApplyingAProfileSetsUpBothKeys) {
  auto profile = MakeDefaultKeyGenProfiles().front();
  profile.key_validity = u"1y"_s;
  profile.s_key_validity = u"forever"_s;
  const auto r = ResolveKeyGenProfile(profile, PrimaryAlgos(),
                                          SubkeyAlgos(), AllPairings);
  ASSERT_TRUE(r.has_value());

  KeyGenerateInfo key;
  key.SetNonPassPhrase(true);
  QSharedPointer<KeyGenerateInfo> subkey;
  ApplyKeyGenProfile(*r, Now(), key, subkey);

  EXPECT_EQ(key.GetAlgo().Id(), u"ed25519"_s);
  EXPECT_FALSE(key.IsNonExpired());
  EXPECT_EQ(key.GetExpireTime(), Now().addYears(1));
  ASSERT_NE(subkey, nullptr);
  EXPECT_EQ(subkey->GetAlgo().Id(), u"cv25519"_s);
  EXPECT_TRUE(subkey->IsNonExpired());
  EXPECT_TRUE(subkey->IsNoPassPhrase());  // one choice for both keys

  // A profile without a subkey takes one away.
  const auto rsa =
      ResolveKeyGenProfile(MakeDefaultKeyGenProfiles().at(3),
                               PrimaryAlgos(), SubkeyAlgos(), AllPairings);
  ASSERT_TRUE(rsa.has_value());
  ApplyKeyGenProfile(*rsa, Now(), key, subkey);
  EXPECT_EQ(subkey, nullptr);
}

TEST(KeyGenProfileTest, SavingWhatWasAppliedGivesTheSameProfile) {
  for (const auto& r :
       ResolveKeyGenProfiles(MakeDefaultKeyGenProfiles(),
                                 PrimaryAlgos(), SubkeyAlgos(), AllPairings)) {
    KeyGenerateInfo key;
    QSharedPointer<KeyGenerateInfo> subkey;
    ApplyKeyGenProfile(r, Now(), key, subkey);

    auto saved = KeyGenProfileFromInfo(key, subkey, Now());
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
  EXPECT_EQ(RequiredKeyVersion(key, nullptr), 0);

  auto subkey = QSharedPointer<KeyGenerateInfo>::create(true);
  subkey->SetAlgo(Find(u"ky768"_s, {}, SubkeyAlgos()));
  EXPECT_EQ(RequiredKeyVersion(key, subkey), 6);
}

TEST(KeyGenSettingsTest, FirstUseMovesTheOldProfilesOver) {
  QJsonArray legacy{QJsonObject{
      {"name", "Mine"},
      {"primary", QJsonObject{{"algo", "rsa3072"}, {"validity", "2y"}}}}};
  const auto moved = KeyGenSettingsFromStored({}, legacy);
  EXPECT_FALSE(moved.profiles_stored);  // still to be written
  ASSERT_EQ(moved.profiles.size(), 1);
  EXPECT_EQ(moved.profiles.front().name, u"Mine"_s);

  // With nothing to move, the defaults.
  const auto fresh = KeyGenSettingsFromStored({}, {});
  EXPECT_EQ(fresh.profiles, MakeDefaultKeyGenProfiles());
}

TEST(KeyGenSettingsTest, StoredSettingsWinOverTheOldCache) {
  KeyGenSO stored(QJsonObject{});
  stored.default_profile = u"Mine"_s;
  stored.uid_pattern.name = u"test-{token:12}"_s;
  stored.profiles = {};  // deleting every profile is a choice too
  const auto json = stored.ToJson();

  QJsonArray legacy{KeyGenProfileToJson(MakeDefaultKeyGenProfiles().front())};
  const auto read = KeyGenSettingsFromStored(json, legacy);
  EXPECT_TRUE(read.profiles_stored);
  EXPECT_TRUE(read.profiles.isEmpty());
  EXPECT_EQ(read.default_profile, u"Mine"_s);
  EXPECT_EQ(read.uid_pattern.name, u"test-{token:12}"_s);
}

TEST(KeyGenerateHelperTest, EngineAlgorithmNamesBecomeGenerationIds) {
  EXPECT_EQ(NormalizeKeyAlgoId(u"ED25519"_s), u"ed25519"_s);
  EXPECT_EQ(NormalizeKeyAlgoId(u"RSA 3072"_s), u"rsa3072"_s);
  EXPECT_EQ(NormalizeKeyAlgoId(u"RSA3072"_s), u"rsa3072"_s);
  EXPECT_EQ(NormalizeKeyAlgoId(u"NIST P-256"_s), u"nistp256"_s);
  EXPECT_EQ(NormalizeKeyAlgoId(u"Brainpool P-384"_s), u"brainpoolp384r1"_s);
  EXPECT_EQ(NormalizeKeyAlgoId(u"BRAINPOOLP256R1"_s), u"brainpoolp256r1"_s);
  EXPECT_EQ(NormalizeKeyAlgoId(u"ML-DSA-65_ED25519"_s), u"mldsa65"_s);
}

TEST(KeyGenerateHelperTest, ASubkeyFollowsThePrimaryKeysFamily) {
  const auto subkeys = SubkeyAlgos();
  const auto id = [&subkeys](SubkeyPurpose purpose, const QString& primary) {
    return SuggestSubkeyAlgo(purpose, primary, subkeys).Id();
  };

  EXPECT_EQ(id(SubkeyPurpose::kEncrypt, u"ed25519"_s), u"cv25519"_s);
  EXPECT_EQ(id(SubkeyPurpose::kSign, u"ed25519"_s), u"ed25519"_s);
  EXPECT_EQ(id(SubkeyPurpose::kEncrypt, u"ed448"_s), u"x448"_s);
  EXPECT_EQ(id(SubkeyPurpose::kEncrypt, u"rsa3072"_s), u"rsa3072"_s);
  EXPECT_EQ(id(SubkeyPurpose::kEncrypt, u"mldsa65"_s), u"ky768"_s);
  EXPECT_EQ(id(SubkeyPurpose::kEncrypt, u"dsa2048"_s), u"elg2048"_s);

  // A curve keeps its id for both jobs; the job decides the type.
  const auto nist_encrypt =
      SuggestSubkeyAlgo(SubkeyPurpose::kEncrypt, u"nistp256"_s, subkeys);
  EXPECT_EQ(nist_encrypt.Id(), u"nistp256"_s);
  EXPECT_TRUE(nist_encrypt.CanEncrypt());
  EXPECT_TRUE(SuggestSubkeyAlgo(SubkeyPurpose::kSign, u"nistp256"_s, subkeys)
                  .CanSign());
}

TEST(KeyGenerateHelperTest, ASubkeyFallsBackToAnythingThatDoesTheJob) {
  // A v4 key under an engine without its family's encryption algorithm.
  const QList<KeyAlgo> only_rsa{Find(u"rsa2048"_s, u"RSA"_s, SubkeyAlgos())};
  EXPECT_EQ(
      SuggestSubkeyAlgo(SubkeyPurpose::kEncrypt, u"ed25519"_s, only_rsa).Id(),
      u"rsa2048"_s);
  EXPECT_EQ(SuggestSubkeyAlgo(SubkeyPurpose::kEncrypt, u"ed25519"_s, {}),
            KeyGenerateInfo::kNoneAlgo);
}

TEST(KeyGenerateHelperTest, APurposeIsReadByItsName) {
  EXPECT_EQ(ParseSubkeyPurpose(u"encrypt"_s), SubkeyPurpose::kEncrypt);
  EXPECT_EQ(ParseSubkeyPurpose(u"sign"_s), SubkeyPurpose::kSign);
  EXPECT_EQ(ParseSubkeyPurpose(u"authenticate"_s),
            SubkeyPurpose::kAuthenticate);
  EXPECT_FALSE(ParseSubkeyPurpose(u"launch"_s).has_value());
  EXPECT_FALSE(ParseSubkeyPurpose({}).has_value());
}

TEST(KeyGenerateHelperTest, ASubkeyIsMadeForOneJobOnly) {
  // Ed25519 could also authenticate; made to sign, it only signs.
  const auto ed25519 = Find(u"ed25519"_s, u"EdDSA"_s, SubkeyAlgos());
  KeyGenerateInfo info(true);
  ApplySubkeyPurpose(info, SubkeyPurpose::kSign, ed25519);
  EXPECT_EQ(info.GetAlgo(), ed25519);
  EXPECT_EQ(info.SubAlgo(), KeyGenerateInfo::kNoneAlgo);
  EXPECT_TRUE(info.IsAllowSign());
  EXPECT_FALSE(info.IsAllowEncr());
  EXPECT_FALSE(info.IsAllowAuth());
}

TEST(KeyGenerateHelperTest, AKeyIsV4UnlessItsAlgorithmsNeedV6) {
  KeyGenerateInfo key;
  key.SetAlgo(Find(u"ed25519"_s, u"EdDSA"_s, PrimaryAlgos()));
  ApplyDefaultKeyVersion(key, nullptr);
  EXPECT_EQ(key.GetKeyVersion(), 4);

  KeyGenerateInfo pqc;
  pqc.SetAlgo(Find(u"mldsa65"_s, {}, PrimaryAlgos()));
  ASSERT_TRUE(pqc.GetAlgo().IsPostQuantum());
  ApplyDefaultKeyVersion(pqc, nullptr);
  EXPECT_EQ(pqc.GetKeyVersion(), 6);
}

}  // namespace GpgFrontend::Test

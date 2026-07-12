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

#include "core/function/ImSessionStore.h"

#include "core/function/DataObjectOperator.h"
#include "core/function/SecureRandomGenerator.h"

namespace GpgFrontend {

namespace {

constexpr auto kIdentityKey = "im/identity";
constexpr auto kSessionIndexKey = "im/session_index";
constexpr auto kPfsConfirmedKey = "im/pfs_confirmed";
constexpr uint8_t kIdentityVersion = 0x01;
constexpr int kKeyLen = X25519::kPubKeyLen;

auto SessionKey(const QString& fpr) -> QString {
  return QStringLiteral("im/session/") + fpr;
}
auto BundleKey(const QString& fpr) -> QString {
  return QStringLiteral("im/bundle/") + fpr;
}

void AppendU32BE(QByteArray& out, quint32 v) {
  out.append(static_cast<char>((v >> 24) & 0xFF));
  out.append(static_cast<char>((v >> 16) & 0xFF));
  out.append(static_cast<char>((v >> 8) & 0xFF));
  out.append(static_cast<char>(v & 0xFF));
}
auto ReadU32BE(const QByteArray& in, int off) -> quint32 {
  return (static_cast<quint32>(static_cast<uint8_t>(in[off])) << 24) |
         (static_cast<quint32>(static_cast<uint8_t>(in[off + 1])) << 16) |
         (static_cast<quint32>(static_cast<uint8_t>(in[off + 2])) << 8) |
         static_cast<quint32>(static_cast<uint8_t>(in[off + 3]));
}

auto SerializeIdentity(const ImLocalIdentity& id) -> GFBuffer {
  QByteArray out;
  out.append(static_cast<char>(kIdentityVersion));
  const auto put = [&out](const X25519KeyPair& kp) {
    out.append(kp.priv.ConvertToQByteArray());
    out.append(kp.pub.ConvertToQByteArray());
  };
  put(id.ik);
  AppendU32BE(out, id.spk_id);
  put(id.spk);
  AppendU32BE(out, static_cast<quint32>(id.opks.size()));
  for (const auto& opk : id.opks) {
    AppendU32BE(out, opk.id);
    put(opk.kp);
  }
  return GFBuffer(out);
}

auto DeserializeIdentity(const GFBuffer& buf) -> std::optional<ImLocalIdentity> {
  const QByteArray in = buf.ConvertToQByteArray();
  int off = 0;
  const auto avail = [&](int n) { return off + n <= in.size(); };
  if (!avail(1) || static_cast<uint8_t>(in[off]) != kIdentityVersion) return {};
  off += 1;

  const auto get = [&](X25519KeyPair& kp) -> bool {
    if (!avail(2 * kKeyLen)) return false;
    kp.priv = GFBuffer(in.mid(off, kKeyLen));
    kp.pub = GFBuffer(in.mid(off + kKeyLen, kKeyLen));
    off += 2 * kKeyLen;
    return true;
  };

  ImLocalIdentity id;
  if (!get(id.ik)) return {};
  if (!avail(4)) return {};
  id.spk_id = ReadU32BE(in, off);
  off += 4;
  if (!get(id.spk)) return {};
  if (!avail(4)) return {};
  const quint32 n = ReadU32BE(in, off);
  off += 4;
  for (quint32 i = 0; i < n; ++i) {
    if (!avail(4)) return {};
    ImLocalPrekey opk;
    opk.id = ReadU32BE(in, off);
    off += 4;
    if (!get(opk.kp)) return {};
    id.opks.push_back(opk);
  }
  return id;
}

}  // namespace

ImSessionStore::ImSessionStore(int channel)
    : SingletonFunctionObject<ImSessionStore>(channel) {}

auto ImSessionStore::CreateIdentity() -> ImLocalIdentity {
  ImLocalIdentity id;
  auto ik = X25519::Generate();
  auto spk = X25519::Generate();
  if (ik) id.ik = *ik;
  if (spk) id.spk = *spk;

  auto rnd = SecureRandomGenerator::Generate(4);
  id.spk_id = rnd ? ReadU32BE(rnd->ConvertToQByteArray(), 0) : 1;

  for (int i = 0; i < kOneTimePrekeyPool; ++i) {
    auto kp = X25519::Generate();
    if (!kp) continue;
    id.opks.push_back({static_cast<quint32>(i + 1), *kp});
  }
  return id;
}

auto ImSessionStore::PersistIdentity(const ImLocalIdentity& id) -> bool {
  return !DataObjectOperator::GetInstance()
              .StoreSecDataObj(kIdentityKey, SerializeIdentity(id))
              .isEmpty();
}

auto ImSessionStore::LoadOrCreateIdentity() -> ImLocalIdentity {
  QMutexLocker locker(&mutex_);
  auto stored = DataObjectOperator::GetInstance().GetSecDataObject(kIdentityKey);
  if (stored) {
    if (auto id = DeserializeIdentity(*stored)) return *id;
    LOG_W() << "im identity blob corrupt; regenerating";
  }
  auto id = CreateIdentity();
  PersistIdentity(id);
  return id;
}

auto ImSessionStore::PublicBundleKeys() -> ImContactBundle {
  const auto id = LoadOrCreateIdentity();
  ImContactBundle b;
  b.ik_pub = id.ik.pub;
  b.spk_id = id.spk_id;
  b.spk_pub = id.spk.pub;
  for (const auto& opk : id.opks) b.opks.push_back({opk.id, opk.kp.pub});
  return b;
}

auto ImSessionStore::GetSignedPrekeyById(quint32 spk_id)
    -> std::optional<X25519KeyPair> {
  QMutexLocker locker(&mutex_);
  const auto id = LoadOrCreateIdentity();
  if (id.spk_id == spk_id) return id.spk;
  return {};
}

auto ImSessionStore::ConsumeOneTimePrekey(quint32 opk_id)
    -> std::optional<X25519KeyPair> {
  QMutexLocker locker(&mutex_);
  auto id = LoadOrCreateIdentity();
  for (int i = 0; i < id.opks.size(); ++i) {
    if (id.opks[i].id == opk_id) {
      auto kp = id.opks[i].kp;
      id.opks.remove(i);
      PersistIdentity(id);
      return kp;
    }
  }
  return {};
}

auto ImSessionStore::LoadSession(const QString& peer_fpr)
    -> std::optional<RatchetState> {
  QMutexLocker locker(&mutex_);
  auto blob = DataObjectOperator::GetInstance().GetSecDataObject(
      SessionKey(peer_fpr));
  if (!blob) return {};
  return DoubleRatchet::DeserializeState(*blob);
}

auto ImSessionStore::SaveSession(const QString& peer_fpr, const RatchetState& st)
    -> bool {
  QMutexLocker locker(&mutex_);
  return !DataObjectOperator::GetInstance()
              .StoreSecDataObj(SessionKey(peer_fpr),
                               DoubleRatchet::SerializeState(st))
              .isEmpty();
}

auto ImSessionStore::LoadLoopbackSession(const QString& peer_fpr)
    -> std::optional<RatchetState> {
  QMutexLocker locker(&mutex_);
  auto blob = DataObjectOperator::GetInstance().GetSecDataObject(
      QStringLiteral("im/session_lb/") + peer_fpr);
  if (!blob || blob->Empty()) return {};
  return DoubleRatchet::DeserializeState(*blob);
}

auto ImSessionStore::SaveLoopbackSession(const QString& peer_fpr,
                                         const RatchetState& st) -> bool {
  QMutexLocker locker(&mutex_);
  return !DataObjectOperator::GetInstance()
              .StoreSecDataObj(QStringLiteral("im/session_lb/") + peer_fpr,
                               DoubleRatchet::SerializeState(st))
              .isEmpty();
}

void ImSessionStore::DeleteSession(const QString& peer_fpr) {
  QMutexLocker locker(&mutex_);
  // Overwrite with a 1-byte tombstone (not an empty buffer — the store may not
  // persist an empty object, which would leave the stale session in place). The
  // byte fails the state version check on load, so the ratchet state (and all
  // past message keys) become unrecoverable — the intended reset semantics.
  const GFBuffer tombstone(QByteArray(1, '\0'));
  auto& op = DataObjectOperator::GetInstance();
  op.StoreSecDataObj(SessionKey(peer_fpr), tombstone);
  op.StoreSecDataObj(QStringLiteral("im/session_lb/") + peer_fpr, tombstone);
}

auto ImSessionStore::SavePeerBundle(const ImContactBundle& bundle) -> bool {
  if (bundle.pgp_fpr.isEmpty()) return false;
  QMutexLocker locker(&mutex_);
  return !DataObjectOperator::GetInstance()
              .StoreSecDataObj(BundleKey(bundle.pgp_fpr), bundle.Serialize())
              .isEmpty();
}

auto ImSessionStore::LoadPeerBundle(const QString& peer_fpr)
    -> std::optional<ImContactBundle> {
  QMutexLocker locker(&mutex_);
  auto blob =
      DataObjectOperator::GetInstance().GetSecDataObject(BundleKey(peer_fpr));
  if (!blob || blob->Empty()) return {};
  return ImContactBundle::Parse(*blob);
}

void ImSessionStore::SetSessionPeer(const QByteArray& session_id,
                                    const QString& peer_fpr) {
  QMutexLocker locker(&mutex_);
  auto& op = DataObjectOperator::GetInstance();
  auto doc = op.GetDataObject(kSessionIndexKey);
  QJsonObject obj = doc ? doc->object() : QJsonObject{};
  obj[QString::fromLatin1(session_id.toHex())] = peer_fpr;
  op.StoreDataObj(kSessionIndexKey, QJsonDocument(obj));
}

auto ImSessionStore::SessionIdToPeer(const QByteArray& session_id)
    -> std::optional<QString> {
  QMutexLocker locker(&mutex_);
  auto doc = DataObjectOperator::GetInstance().GetDataObject(kSessionIndexKey);
  if (!doc) return {};
  const auto v = doc->object().value(QString::fromLatin1(session_id.toHex()));
  if (!v.isString()) return {};
  return v.toString();
}

void ImSessionStore::SavePeerSessionId(const QString& peer_fpr,
                                       const QByteArray& session_id) {
  QMutexLocker locker(&mutex_);
  DataObjectOperator::GetInstance().StoreSecDataObj(
      QStringLiteral("im/sid/") + peer_fpr, GFBuffer(session_id));
}

auto ImSessionStore::GetPeerSessionId(const QString& peer_fpr)
    -> std::optional<QByteArray> {
  QMutexLocker locker(&mutex_);
  auto blob = DataObjectOperator::GetInstance().GetSecDataObject(
      QStringLiteral("im/sid/") + peer_fpr);
  if (!blob || blob->Empty()) return {};
  return blob->ConvertToQByteArray();
}

void ImSessionStore::MarkPfsConfirmed(const QString& peer_fpr) {
  QMutexLocker locker(&mutex_);
  auto& op = DataObjectOperator::GetInstance();
  auto doc = op.GetDataObject(kPfsConfirmedKey);
  QJsonArray arr = doc ? doc->array() : QJsonArray{};
  if (!arr.contains(peer_fpr)) {
    arr.append(peer_fpr);
    op.StoreDataObj(kPfsConfirmedKey, QJsonDocument(arr));
  }
}

void ImSessionStore::ClearPfsConfirmed(const QString& peer_fpr) {
  QMutexLocker locker(&mutex_);
  auto& op = DataObjectOperator::GetInstance();
  auto doc = op.GetDataObject(kPfsConfirmedKey);
  if (!doc) return;
  QJsonArray arr = doc->array();
  for (int i = arr.size() - 1; i >= 0; --i) {
    if (arr.at(i).toString() == peer_fpr) arr.removeAt(i);
  }
  op.StoreDataObj(kPfsConfirmedKey, QJsonDocument(arr));
}

auto ImSessionStore::IsPfsConfirmed(const QString& peer_fpr) -> bool {
  QMutexLocker locker(&mutex_);
  auto doc = DataObjectOperator::GetInstance().GetDataObject(kPfsConfirmedKey);
  if (!doc) return false;
  return doc->array().contains(peer_fpr);
}

}  // namespace GpgFrontend

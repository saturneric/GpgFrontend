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

#include "core/function/ImHandshake.h"

#include <gpg-error.h>
#include <sodium.h>

#include "core/function/DoubleRatchet.h"
#include "core/function/openpgp/MessageCryptoOperation.h"
#include "core/model/DataObject.h"
#include "core/model/GpgSignature.h"
#include "core/model/GpgVerifyResult.h"

namespace GpgFrontend {

namespace {

constexpr std::string_view kX3dhDomain = "GF-X3DH-v1";

// SK = BLAKE2b(domain || book || DH1 || DH2 || DH3 [|| DH4]). Mixing the shared
// password book in makes it a genuine additional handshake secret.
auto ComputeSharedSecret(const GFBuffer& book, const QVector<GFBuffer>& dhs)
    -> GFBufferOrNone {
  QByteArray in(kX3dhDomain.data(), static_cast<qsizetype>(kX3dhDomain.size()));
  in.append(book.ConvertToQByteArray());
  for (const auto& dh : dhs) {
    if (dh.Empty()) return {};
    in.append(dh.ConvertToQByteArray());
  }

  GFBuffer sk(32);
  if (crypto_generichash(reinterpret_cast<unsigned char*>(sk.Data()), sk.Size(),
                         reinterpret_cast<const unsigned char*>(in.constData()),
                         static_cast<unsigned long long>(in.size()), nullptr,
                         0) != 0) {
    return {};
  }
  return sk;
}

}  // namespace

auto ImHandshake::SignDetached(int channel, const GpgKeyPtr& signer,
                               const GFBuffer& data)
    -> std::optional<QByteArray> {
  if (signer == nullptr) return {};

  GpgAbstractKeyPtrList signers;
  signers.push_back(signer);

  auto [err, obj] = MessageCryptoOperation::GetInstance(channel).SignSync(
      signers, data, GPGME_SIG_MODE_DETACH, false);
  if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) return {};

  auto sig = ExtractParams<GFBuffer>(obj, 1);
  if (sig.Empty()) return {};
  return sig.ConvertToQByteArray();
}

auto ImHandshake::VerifyDetached(int channel, const GFBuffer& data,
                                 const QByteArray& sig)
    -> std::optional<QString> {
  if (sig.isEmpty()) return {};

  auto [err, obj] = MessageCryptoOperation::GetInstance(channel).VerifySync(
      data, GFBuffer(sig));
  if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) return {};

  auto result = ExtractParams<GpgVerifyResult>(obj, 0);
  const auto sigs = result.GetSignature();
  if (sigs.isEmpty()) return {};

  // We require a cryptographically good signature (the data was signed by this
  // key). Web-of-trust validity is a separate, higher-level concern.
  const auto& s = sigs.first();
  if (gpgme_err_code(s.GetStatus()) != GPG_ERR_NO_ERROR) return {};
  const auto fpr = s.GetFingerprint();
  if (fpr.isEmpty()) return {};
  return fpr;
}

auto ImHandshake::Initiate(const ImLocalIdentity& own,
                           const ImContactBundle& peer, const GFBuffer& book,
                           InitPayload& payload) -> std::optional<RatchetState> {
  auto ek = X25519::Generate();
  if (!ek) return {};

  // SPK-only X3DH (no one-time prekey). Without a server to hand out and retire
  // one-time prekeys, a static bundle would re-serve a consumed OPK on every
  // re-initiation and the responder could never reconstruct it — breaking
  // recovery. This is exactly Signal's documented fallback when OPKs are
  // unavailable; the Double Ratchet still provides forward secrecy from the
  // first reply onward.
  QVector<GFBuffer> dhs;
  auto dh1 = X25519::DH(own.ik.priv, peer.spk_pub);
  auto dh2 = X25519::DH(ek->priv, peer.ik_pub);
  auto dh3 = X25519::DH(ek->priv, peer.spk_pub);
  if (!dh1 || !dh2 || !dh3) return {};
  dhs = {*dh1, *dh2, *dh3};

  auto sk = ComputeSharedSecret(book, dhs);
  if (!sk) return {};

  auto state = DoubleRatchet::InitSender(*sk, peer.spk_pub);
  if (!state) return {};

  payload.ik_pub = own.ik.pub;
  payload.ek_pub = ek->pub;
  payload.spk_id = peer.spk_id;
  payload.opk_id = 0;
  return state;
}

auto ImHandshake::Accept(const ImLocalIdentity& own,
                         const X25519KeyPair& own_spk,
                         const std::optional<X25519KeyPair>& own_opk,
                         const InitPayload& payload, const GFBuffer& book)
    -> std::optional<RatchetState> {
  QVector<GFBuffer> dhs;
  auto dh1 = X25519::DH(own_spk.priv, payload.ik_pub);
  auto dh2 = X25519::DH(own.ik.priv, payload.ek_pub);
  auto dh3 = X25519::DH(own_spk.priv, payload.ek_pub);
  if (!dh1 || !dh2 || !dh3) return {};
  dhs = {*dh1, *dh2, *dh3};

  if (payload.opk_id != 0) {
    if (!own_opk) return {};  // referenced OPK is gone; cannot complete
    auto dh4 = X25519::DH(own_opk->priv, payload.ek_pub);
    if (!dh4) return {};
    dhs.append(*dh4);
  }

  auto sk = ComputeSharedSecret(book, dhs);
  if (!sk) return {};

  return DoubleRatchet::InitReceiver(*sk, own_spk);
}

}  // namespace GpgFrontend

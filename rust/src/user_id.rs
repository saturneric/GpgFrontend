/*
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

use pgp::{
    composed::{ArmorOptions, Deserializable, SignedPublicKey, SignedSecretKey},
    crypto::hash::HashAlgorithm,
    packet::{SignatureConfig, SignatureType, Subpacket, SubpacketData},
    types::{KeyDetails, PacketHeaderVersion, Password, SecretParams, Tag},
};

use crate::{
    err::IntoGfrResult,
    types::{GfrFreeCb, GfrPasswordFetchCb, GfrStatus},
    utils::fetch_password_internal,
};

pub fn delete_user_id_internal(key_block: &str, target_uid: &str) -> Result<String, GfrStatus> {
    if let Ok((mut skey, _)) = SignedSecretKey::from_string(key_block) {
        let initial_len = skey.details.users.len();
        skey.details
            .users
            .retain(|u| String::from_utf8_lossy(u.id.id()) != target_uid);

        if skey.details.users.len() == initial_len {
            return Err(GfrStatus::ErrorInvalidInput);
        }
        return skey.to_armored_string(ArmorOptions::default()).into_gfr();
    }

    if let Ok((mut pkey, _)) = SignedPublicKey::from_string(key_block) {
        let initial_len = pkey.details.users.len();
        pkey.details
            .users
            .retain(|u| String::from_utf8_lossy(u.id.id()) != target_uid);

        if pkey.details.users.len() == initial_len {
            return Err(GfrStatus::ErrorInvalidInput);
        }
        return pkey.to_armored_string(ArmorOptions::default()).into_gfr();
    }

    Err(GfrStatus::ErrorInvalidData)
}

pub fn add_user_id_internal(
    channel: i32,
    secret_key_block: &str,
    new_uid_str: &str,
    fetch_cb: Option<GfrPasswordFetchCb>,
    free_cb: Option<GfrFreeCb>,
) -> Result<String, GfrStatus> {
    let (mut skey, _) = SignedSecretKey::from_string(secret_key_block).into_gfr()?;
    let fpr = skey.primary_key.fingerprint().to_string();

    let is_enc = matches!(skey.primary_key.secret_params(), SecretParams::Encrypted(_));
    let pwd_bytes = if is_enc {
        fetch_password_internal(channel, &fpr, "Add User ID", fetch_cb, free_cb)?
    } else {
        Vec::new()
    };
    let pwd = Password::from(pwd_bytes.as_slice());

    let new_uid = pgp::packet::UserId::from_str(PacketHeaderVersion::New, new_uid_str)
        .map_err(|_| GfrStatus::ErrorInternal)?;

    let mut rng = rand::thread_rng();

    let pk = skey.primary_key.public_key();
    let signed_user = new_uid
        .sign(&mut rng, &skey.primary_key, &pk, &pwd)
        .into_gfr()?;

    skey.details.users.push(signed_user);

    skey.to_armored_string(ArmorOptions::default()).into_gfr()
}

pub fn update_user_id_internal(
    channel: i32,
    secret_key_block: &str,
    old_uid: &str,
    new_uid: &str,
    fetch_cb: Option<GfrPasswordFetchCb>,
    free_cb: Option<GfrFreeCb>,
) -> Result<String, GfrStatus> {
    let block_with_new =
        add_user_id_internal(channel, secret_key_block, new_uid, fetch_cb, free_cb)?;
    delete_user_id_internal(&block_with_new, old_uid)
}

pub fn set_primary_user_id_internal(
    channel: i32,
    secret_key_block: &str,
    target_uid_str: &str,
    fetch_cb: Option<GfrPasswordFetchCb>,
    free_cb: Option<GfrFreeCb>,
) -> Result<String, GfrStatus> {
    let (mut skey, _) = SignedSecretKey::from_string(secret_key_block).into_gfr()?;

    // 1. 查找目标 UID 索引
    let target_idx = skey
        .details
        .users
        .iter()
        .position(|u| String::from_utf8_lossy(u.id.id()) == target_uid_str)
        .ok_or(GfrStatus::ErrorInvalidInput)?;

    let fpr = skey.primary_key.fingerprint().to_string();

    // 2. 获取密码
    let is_enc = matches!(skey.primary_key.secret_params(), SecretParams::Encrypted(_));
    let pwd_bytes = if is_enc {
        fetch_password_internal(channel, &fpr, "Set Primary User ID", fetch_cb, free_cb)?
    } else {
        Vec::new()
    };
    let pwd = Password::from(pwd_bytes.as_slice());
    let pk = skey.primary_key.public_key();

    let primary_fpr_bytes = skey.primary_key.fingerprint().as_bytes().to_vec();

    // 3. 遍历所有 User ID，对 user 可变引用进行操作
    for (i, user) in skey.details.users.iter_mut().enumerate() {
        let is_target = i == target_idx;

        // 收集该 UID 身上所有由当前主密钥签发的自签名 (Self-Signatures)
        let self_sigs: Vec<&pgp::packet::Signature> = user
            .signatures
            .iter()
            .filter(|sig| {
                sig.issuer_fingerprint()
                    .iter()
                    .any(|f| f.as_bytes() == primary_fpr_bytes)
            })
            .collect();

        // 检查历史签名中是否在任何地方残留了 IsPrimary(true) 标记
        let has_any_primary = self_sigs.iter().any(|sig| {
            sig.config()
                .map(|c| {
                    c.hashed_subpackets
                        .iter()
                        .chain(c.unhashed_subpackets.iter())
                        .any(|s| matches!(s.data, SubpacketData::IsPrimary(true)))
                })
                .unwrap_or(false)
        });

        log::info!(
            "Processing UID '{}', is_target: {}, has_self_sigs: {}, has_any_primary: {}",
            String::from_utf8_lossy(user.id.id()),
            is_target,
            !self_sigs.is_empty(),
            has_any_primary
        );

        // 只有两种情况我们需要重写它的签名：
        // 1. 它是目标 UID（必须确保提权，加上 IsPrimary 标记并置换为最新时间戳）
        // 2. 它不是目标 UID，但身上却残留了 Primary 标记（必须强行洗掉标记进行降权！）
        if !is_target && !has_any_primary {
            continue;
        }

        // 找到时间戳最新的那个自签名，用来继承它的偏好配置 (KeyFlags, Algorithms 等)
        let latest_self_sig = self_sigs.into_iter().last();

        let mut cfg = if let Some(sig) = latest_self_sig {
            let mut c = sig.config().cloned().unwrap_or_else(|| {
                SignatureConfig::v4(
                    SignatureType::CertPositive,
                    skey.primary_key.algorithm(),
                    HashAlgorithm::Sha512,
                )
            });

            let filter_subpackets = |subpackets: &mut Vec<Subpacket>| {
                subpackets.retain(|s| {
                    !matches!(
                        s.data,
                        SubpacketData::SignatureCreationTime(_)
                            | SubpacketData::IssuerFingerprint(_)
                            | SubpacketData::IssuerKeyId(_)
                            | SubpacketData::IsPrimary(_)
                    )
                });
            };

            filter_subpackets(&mut c.hashed_subpackets);
            filter_subpackets(&mut c.unhashed_subpackets);
            c
        } else {
            SignatureConfig::v4(
                SignatureType::CertPositive,
                skey.primary_key.algorithm(),
                HashAlgorithm::Sha512,
            )
        };

        // 注入新的时间戳和签发者指纹
        cfg.hashed_subpackets.push(
            Subpacket::regular(SubpacketData::IssuerFingerprint(
                skey.primary_key.fingerprint(),
            ))
            .map_err(|_| GfrStatus::ErrorInternal)?,
        );
        cfg.hashed_subpackets.push(
            Subpacket::regular(SubpacketData::SignatureCreationTime(
                pgp::types::Timestamp::now(),
            ))
            .map_err(|_| GfrStatus::ErrorInternal)?,
        );

        // 【核心逻辑】：只有目标 UID 才会注入 IsPrimary(true)
        if is_target {
            cfg.hashed_subpackets.push(
                Subpacket::regular(SubpacketData::IsPrimary(true))
                    .map_err(|_| GfrStatus::ErrorInternal)?,
            );
        }

        // 重新生成规范的单认证签名
        let new_sig = cfg
            .sign_certification(&skey.primary_key, &pk, &pwd, Tag::UserId, &user.id)
            .into_gfr()?;

        // 【终极清理】：无情剔除这个 UID 身上所有的旧自签名！只保留别人的认证签名
        user.signatures.retain(|sig| {
            !sig.issuer_fingerprint()
                .iter()
                .any(|f| f.as_bytes() == primary_fpr_bytes)
        });

        // 填入这唯一的一个最新合规自签名
        user.signatures.push(new_sig);
    }

    // 4. 将新指定的 Primary UID 移动到数组 Index 0 的位置
    if target_idx != 0 {
        let primary_user = skey.details.users.remove(target_idx);
        skey.details.users.insert(0, primary_user);
    }

    // 5. 导出 Armor
    skey.to_armored_string(ArmorOptions::default()).into_gfr()
}

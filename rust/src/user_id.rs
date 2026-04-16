use pgp::{
    composed::{ArmorOptions, Deserializable, SignedPublicKey, SignedSecretKey},
    types::{KeyDetails, PacketHeaderVersion, Password, SecretParams},
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

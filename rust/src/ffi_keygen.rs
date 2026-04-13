use crate::keygen::{GeneratedKeys, add_subkey_internal, create_key_internal};
use crate::types::{GfrFreeCb, GfrKeyConfig, GfrKeyGenerateResult, GfrPasswordFetchCb, GfrStatus};

use std::{
    ffi::{CStr, CString, c_char},
    panic::catch_unwind,
};

#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_generate_key(
    user_id: *const c_char,
    key_config: GfrKeyConfig,
    s_key_configs: *const GfrKeyConfig,
    s_key_count: usize,
    fetch_pwd_cb: GfrPasswordFetchCb,
    free_cb: GfrFreeCb,
    o_result: *mut GfrKeyGenerateResult,
) -> GfrStatus {
    let result = catch_unwind(|| -> Result<GeneratedKeys, GfrStatus> {
        if user_id.is_null() || o_result.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let user_id_str = unsafe { CStr::from_ptr(user_id) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

        if s_key_configs.is_null() && s_key_count > 0 {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let mut configs: &[GfrKeyConfig] = &[];
        if s_key_count > 0 {
            configs = unsafe { std::slice::from_raw_parts(s_key_configs, s_key_count) };
        }

        let keys = create_key_internal(
            user_id_str,
            key_config,
            configs,
            Some(fetch_pwd_cb),
            Some(free_cb),
        )?;

        Ok(keys)
    });

    match result {
        Ok(inner_result) => match inner_result {
            Ok(keys) => {
                let Ok(c_s) = CString::new(keys.secret) else {
                    return GfrStatus::ErrorInternal;
                };
                let Ok(c_p) = CString::new(keys.public) else {
                    return GfrStatus::ErrorInternal;
                };
                let Ok(c_f) = CString::new(keys.fingerprint) else {
                    return GfrStatus::ErrorInternal;
                };

                unsafe {
                    *o_result = GfrKeyGenerateResult {
                        secret_key: c_s.into_raw(),
                        public_key: c_p.into_raw(),
                        fingerprint: c_f.into_raw(),
                    };
                }
                GfrStatus::Success
            }

            Err(status) => status,
        },
        Err(_) => GfrStatus::ErrorPanic,
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn gfr_crypto_add_subkey(
    channel: i32,
    key_block: *const c_char,
    config: GfrKeyConfig,
    fetch_pwd_cb: GfrPasswordFetchCb,
    free_cb: GfrFreeCb,
    o_result: *mut GfrKeyGenerateResult,
) -> GfrStatus {
    let result = catch_unwind(|| -> Result<GeneratedKeys, GfrStatus> {
        if key_block.is_null() || o_result.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        let key_block_str = unsafe { CStr::from_ptr(key_block) }
            .to_str()
            .map_err(|_| GfrStatus::ErrorInvalidInput)?;

        // Call the internal implementation
        let keys = add_subkey_internal(
            channel,
            key_block_str,
            &config,
            Some(fetch_pwd_cb),
            Some(free_cb),
        )?;

        Ok(keys)
    });

    match result {
        Ok(inner_result) => match inner_result {
            Ok(keys) => {
                // Convert Rust Strings to CStrings, handling internal null byte errors
                let Ok(c_s) = CString::new(keys.secret) else {
                    return GfrStatus::ErrorInternal;
                };
                let Ok(c_p) = CString::new(keys.public) else {
                    return GfrStatus::ErrorInternal;
                };
                let Ok(c_f) = CString::new(keys.fingerprint) else {
                    return GfrStatus::ErrorInternal;
                };

                // Transfer ownership of the pointers to the out parameter
                unsafe {
                    *o_result = GfrKeyGenerateResult {
                        secret_key: c_s.into_raw(),
                        public_key: c_p.into_raw(),
                        fingerprint: c_f.into_raw(),
                    };
                }
                GfrStatus::Success
            }
            Err(status) => status,
        },
        Err(_) => GfrStatus::ErrorPanic,
    }
}

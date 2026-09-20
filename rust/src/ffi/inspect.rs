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

//! The OpenPGP structure inspector's FFI boundary.

use std::{
    ffi::{CString, c_char},
    panic::catch_unwind,
};

use crate::err::clear_last_error;
use crate::types::{GfrBuffer, GfrStatus};

/// Describe the structure of an OpenPGP blob as a JSON document.
///
/// Accepts anything: armored or binary, message, detached signature,
/// certificate or cleartext-signed text. Encrypted payloads are never
/// decrypted; compressed payloads are recursed into. Input that is not
/// OpenPGP at all still succeeds, returning a document whose `errors` say so.
///
/// On success `*out_json` is set to a heap-allocated UTF-8 JSON string. Free
/// it with `gfr_crypto_free_string`.
///
/// # Safety
/// `out_json` must be non-null, and `in_data` must describe a readable buffer.
#[unsafe(no_mangle)]
pub extern "C" fn gfr_pgp_inspect(in_data: GfrBuffer, out_json: *mut *mut c_char) -> GfrStatus {
    clear_last_error();
    let result = catch_unwind(|| -> Result<(), GfrStatus> {
        if out_json.is_null() {
            return Err(GfrStatus::ErrorInvalidInput);
        }

        // An empty input is not an error: it inspects to an empty document.
        let data = unsafe { in_data.as_slice() }.unwrap_or(&[]);
        let document = crate::inspect::inspect(data);

        let json = serde_json::to_string(&document).map_err(|_| GfrStatus::ErrorInternal)?;
        let c_str = CString::new(json).map_err(|_| GfrStatus::ErrorInternal)?;
        unsafe {
            *out_json = c_str.into_raw();
        }
        Ok(())
    });

    match result {
        Ok(Ok(_)) => GfrStatus::Success,
        Ok(Err(e)) => e,
        Err(_) => GfrStatus::ErrorPanic,
    }
}

#[cfg(test)]
mod ffi_inspect_tests {
    //! The inspector's boundary: null handling, ownership, and the promise
    //! that no input makes it fail.

    use super::*;
    use crate::testutil::corpus;
    use std::ffi::CStr;

    fn buf(data: &[u8]) -> GfrBuffer {
        GfrBuffer {
            data: data.as_ptr(),
            len: data.len(),
        }
    }

    fn inspect_json(data: &[u8]) -> serde_json::Value {
        let mut out: *mut c_char = std::ptr::null_mut();
        assert_eq!(gfr_pgp_inspect(buf(data), &mut out), GfrStatus::Success);
        assert!(!out.is_null());
        let json = unsafe { CStr::from_ptr(out) }
            .to_string_lossy()
            .into_owned();
        crate::ffi::mem::gfr_crypto_free_string(out);
        serde_json::from_str(&json).expect("the inspector must emit valid JSON")
    }

    #[test]
    fn a_null_out_pointer_is_rejected_before_anything_else() {
        assert_eq!(
            gfr_pgp_inspect(buf(corpus::SIG_GOOD_DETACHED), std::ptr::null_mut()),
            GfrStatus::ErrorInvalidInput
        );
    }

    #[test]
    fn a_null_input_buffer_inspects_to_an_empty_document() {
        // Not an error: "nothing to inspect" is a finding, not a failure.
        let doc = inspect_json(&[]);
        assert_eq!(doc["size"], 0);
        assert!(doc["blocks"].as_array().expect("blocks").is_empty());
    }

    #[test]
    fn a_detached_signature_inspects_to_one_signature_packet() {
        let doc = inspect_json(corpus::SIG_GOOD_DETACHED);
        let packets = doc["blocks"][0]["packets"].as_array().expect("packets");
        assert_eq!(packets.len(), 1);
        assert_eq!(packets[0]["tag"], 2);
    }

    #[test]
    fn the_returned_string_is_a_fresh_allocation_each_call() {
        // Each caller frees what it got, so a shared pointer would be a
        // double free waiting to happen.
        let mut a: *mut c_char = std::ptr::null_mut();
        let mut b: *mut c_char = std::ptr::null_mut();
        let data = corpus::SIG_GOOD_DETACHED;
        assert_eq!(gfr_pgp_inspect(buf(data), &mut a), GfrStatus::Success);
        assert_eq!(gfr_pgp_inspect(buf(data), &mut b), GfrStatus::Success);
        assert_ne!(a, b);
        crate::ffi::mem::gfr_crypto_free_string(a);
        crate::ffi::mem::gfr_crypto_free_string(b);
    }

    #[test]
    fn no_corpus_vector_makes_the_boundary_fail() {
        // The contract the dialog relies on: every input yields a document.
        for data in [
            corpus::ENC_V1SEIPD_MDC,
            corpus::ENC_V2SEIPD_OCB,
            corpus::ENC_MULTI_RECIPIENT,
            corpus::ENC_SYMMETRIC_V1,
            corpus::ENC_SED_TAG9,
            corpus::SIG_GOOD_DETACHED,
            corpus::SIG_V6_DETACHED,
            corpus::SIG_INLINE_COMPRESSED,
            corpus::TWO_SIGNER,
            corpus::PKESK_NO_SEIPD,
            corpus::GARBAGE,
            corpus::EMPTY,
            corpus::SIG_GOOD_CLEARTEXT.as_bytes(),
            corpus::TRUNCATED_ARMOR.as_bytes(),
            corpus::CORRUPT_CRC.as_bytes(),
            corpus::AUX_GOOD.as_bytes(),
        ] {
            let _ = inspect_json(data);
        }
    }
}

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
//! Clear-text signed messages (RFC 9580 §7) read as a stream.
//!
//! rPGP's `CleartextSignedMessage` holds the whole text in one `String`, and
//! its signed form in another. Here the text is read a line at a time and
//! its signed form, dash-unescaped, trimmed and with CRLF line endings, is
//! written to an anonymous temporary file that the signatures are checked
//! against; only the header lines and the signature block, both small, are
//! held. They are handed to rPGP's own parser, with the text left out, so a
//! message is accepted or refused exactly as the in-memory path does.

use std::io::BufRead;

use super::*;

/// The longest line of signed text that is read. A longer one is refused
/// rather than held.
const MAX_LINE: usize = 16 << 20;

/// The most the header lines, or the signature block, may take up.
const MAX_FRAME: usize = 16 << 20;

const BEGIN_SIGNED: &str = "-----BEGIN PGP SIGNED MESSAGE-----";
const BEGIN_SIGNATURE: &str = "-----BEGIN PGP SIGNATURE-----";

/// Why a clear-text message cannot be read.
fn invalid(what: &str) -> GfrStatus {
    set_last_error(what);
    GfrStatus::ErrorInvalidData
}

/// Read the next line of `input`, with its `\n`, into `line`, replacing what
/// it held. False at the end of the input. A line longer than `limit` is
/// refused.
fn next_line<B: BufRead>(input: &mut B, line: &mut Vec<u8>, limit: usize) -> std::io::Result<bool> {
    line.clear();
    loop {
        let available = input.fill_buf()?;
        if available.is_empty() {
            return Ok(!line.is_empty());
        }
        let newline = available.iter().position(|&b| b == b'\n');
        let take = newline.map_or(available.len(), |at| at + 1);
        line.extend_from_slice(&available[..take]);
        input.consume(take);
        if line.len() > limit {
            return Err(std::io::Error::new(
                std::io::ErrorKind::InvalidData,
                "a line of the clear-text message is too long",
            ));
        }
        if newline.is_some() {
            return Ok(true);
        }
    }
}

/// A line of signed text as it is signed (RFC 9580 §7.1): without its line
/// ending, its dash escape or trailing spaces and tabs, as rPGP's
/// `dash_unescape_and_trim` makes it.
fn signed_form(line: &str) -> &str {
    let content = line
        .strip_suffix("\r\n")
        .or_else(|| line.strip_suffix('\n'))
        .unwrap_or(line);
    content
        .strip_prefix("- ")
        .unwrap_or(content)
        .trim_end_matches([' ', '\t'])
}

/// Whether `line` ends the signed text: rPGP ends it at the first line that
/// starts with five dashes, and GF's whitespace normalization trims the
/// signature block's first line before rPGP sees it.
fn ends_text(line: &str) -> bool {
    line.starts_with("-----") || line.trim().starts_with(BEGIN_SIGNATURE)
}

/// Verify a clear-text signed message read from `input`, writing its signed
/// text (CRLF line endings, as [`verify_internal`] gives it) to `output`.
///
/// Bounded memory for a text of any size: see the module documentation. The
/// signed text is staged in an anonymous temporary file in `spool_dir`, or
/// the OS temporary directory when it is `None`, and reaches `output` only
/// once every signature has been checked, so a damaged message puts nothing
/// into it. Attribution is that of [`verify_internal`]'s clear-text mode.
pub fn verify_cleartext_stream_internal<R, W>(
    channel: i32,
    input: R,
    mut output: W,
    spool_dir: Option<&Path>,
    fetch_pubkey_cb: Option<GfrPublicKeyFetchCb>,
    user_data: *mut c_void,
) -> Result<VerifyStreamResultInternal, GfrStatus>
where
    R: Read,
    W: Write,
{
    let mut input = BufReader::new(crate::cancel::CancellableReader::new(channel, input));
    let read_error = |e: std::io::Error| {
        if crate::cancel::is_cancelled(channel) {
            return GfrStatus::ErrorCanceled;
        }
        set_last_error(&format!("cannot read the clear-text message: {e}"));
        if e.kind() == std::io::ErrorKind::InvalidData {
            GfrStatus::ErrorInvalidData
        } else {
            GfrStatus::ErrorIo
        }
    };
    let mut line = Vec::new();
    let next = |input: &mut BufReader<_>, line: &mut Vec<u8>, limit| -> Result<bool, GfrStatus> {
        next_line(input, line, limit).map_err(read_error)
    };
    let text_of = |line: &[u8]| -> Result<String, GfrStatus> {
        String::from_utf8(line.to_vec()).map_err(|_| {
            set_last_error("the clear-text message is not UTF-8 text");
            GfrStatus::ErrorInvalidInput
        })
    };

    // 1. The armor header line, after any blank lines, and the header lines
    //    up to the blank line that ends them.
    let mut frame = String::new();
    loop {
        if !next(&mut input, &mut line, MAX_LINE)? {
            return Err(invalid("the input holds no clear-text signed message"));
        }
        let text = text_of(&line)?;
        if text.trim().is_empty() {
            continue;
        }
        if !text.trim().starts_with(BEGIN_SIGNED) {
            return Err(invalid("text comes before the clear-text signed message"));
        }
        frame.push_str(&text);
        break;
    }
    loop {
        if !next(&mut input, &mut line, MAX_LINE)? {
            return Err(invalid("the clear-text message ends in its header"));
        }
        let text = text_of(&line)?;
        frame.push_str(&text);
        if frame.len() > MAX_FRAME {
            return Err(invalid("the clear-text message's header is too long"));
        }
        if text.trim().is_empty() {
            break;
        }
    }

    // 2. The signed text, a line at a time, into the spool in its signed
    //    form. A line is written when the next one shows it was not the
    //    last: the last line's line ending is not signed.
    let mut spool = match spool_dir {
        Some(dir) => tempfile::tempfile_in(dir),
        None => tempfile::tempfile(),
    }
    .map_err(|e| {
        set_last_error(&format!(
            "cannot create a temporary file for the signed text: {e}"
        ));
        GfrStatus::ErrorIo
    })?;
    let mut spool_writer = std::io::BufWriter::new(&mut spool);
    let spool_error = |e: std::io::Error| {
        set_last_error(&format!(
            "cannot write the signed text to a temporary file: {e}"
        ));
        GfrStatus::ErrorIo
    };
    let mut first = true;
    loop {
        if !next(&mut input, &mut line, MAX_LINE)? {
            return Err(invalid("the clear-text message has no signature"));
        }
        let text = text_of(&line)?;
        if ends_text(&text) {
            frame.push_str(&text);
            break;
        }
        if !first {
            spool_writer.write_all(b"\r\n").map_err(spool_error)?;
        }
        spool_writer
            .write_all(signed_form(&text).as_bytes())
            .map_err(spool_error)?;
        first = false;
    }
    spool_writer.flush().map_err(spool_error)?;
    drop(spool_writer);

    // 3. The signature block, and anything after it, which rPGP refuses
    //    unless it is whitespace.
    loop {
        if !next(&mut input, &mut line, MAX_FRAME)? {
            break;
        }
        frame.push_str(&text_of(&line)?);
        if frame.len() > MAX_FRAME {
            return Err(invalid(
                "the clear-text message's signature block is too long",
            ));
        }
    }

    // 4. rPGP reads the header and the signatures from the frame, its text
    //    left out, as `verify_internal` has it read the whole message.
    let frame = crate::utils::strip_armor_whitespace(frame.trim_start());
    let (message, _) = CleartextSignedMessage::from_string(&frame).into_gfr()?;
    let sig_packets = message.signatures();
    let mut signatures: Vec<SignatureResultInternal> = sig_packets
        .iter()
        .map(|sig| sig_entry_from_packet(sig, GfrSignMode::ClearText))
        .collect();
    let certs = fetch_certs_for_signatures(&signatures, fetch_pubkey_cb, user_data);
    let mut is_verified = false;
    // Per-index attribution over the spooled text, rewound for every attempt
    // as the detached stream verifier does it.
    attribute_entries(&mut signatures, &certs, &mut is_verified, |i, cert, t| {
        signature_verdict_stream(cert, &sig_packets[i], &mut spool, channel, t)
    });
    if crate::cancel::is_cancelled(channel) {
        return Err(GfrStatus::ErrorCanceled);
    }

    // 5. The signed text, now that the whole message has been read.
    spool
        .seek(SeekFrom::Start(0))
        .and_then(|_| std::io::copy(&mut spool, &mut output))
        .and_then(|_| output.flush())
        .map_err(|e| {
            set_last_error(&format!("cannot write the verified output: {e}"));
            GfrStatus::ErrorIo
        })?;

    Ok(VerifyStreamResultInternal {
        is_verified,
        signatures,
    })
}

#[cfg(test)]
mod cleartext_stream_tests {
    //! The clear-text stream verifier. Every clear-text case of the verify
    //! and conformance suites is also checked against the in-memory verifier;
    //! these cover the framing a line-at-a-time reader can get wrong.

    use super::*;
    use crate::testutil::{cb, keys, verify_stream};

    fn sign(text: &[u8], key: &keys::Fixture) -> Vec<u8> {
        crate::crypto::sign_internal(
            0,
            "",
            text,
            &[&key.secret_armored],
            None,
            GfrSignMode::ClearText,
            true,
        )
        .expect("sign")
        .data
    }

    /// Verify `message` in memory and as a stream, which must agree; the
    /// stream's result and the text it wrote.
    fn verify(
        message: &[u8],
        key: &keys::Fixture,
    ) -> (Result<VerifyStreamResultInternal, GfrStatus>, Vec<u8>) {
        cb::set_pubkey_answer(&key.public_armored);
        let memory = crate::crypto::verify_internal(
            message,
            &[],
            GfrSignMode::ClearText,
            Some(cb::pubkey_fetch),
            std::ptr::null_mut(),
        );
        verify_stream::agrees(message, GfrSignMode::ClearText, &memory);
        let mut out = Vec::new();
        let res = verify_cleartext_stream_internal(
            0,
            Cursor::new(message),
            &mut out,
            None,
            Some(cb::pubkey_fetch),
            std::ptr::null_mut(),
        );
        (res, out)
    }

    fn assert_valid(res: Result<VerifyStreamResultInternal, GfrStatus>, what: &str) {
        let res = res.unwrap_or_else(|e| panic!("{what}: {e:?}"));
        assert!(res.is_verified, "{what}");
        assert_eq!(res.signatures.len(), 1, "{what}");
        assert_eq!(
            res.signatures[0].status,
            GfrSignatureStatus::Valid,
            "{what}"
        );
    }

    #[test]
    fn texts_a_line_reader_could_get_wrong_verify() {
        for text in [
            &b""[..],
            b"no final line ending",
            b"crlf\r\nlines\r\n",
            b"- a dash\n-----five dashes\nFrom a mail line\n",
            b"trailing \t \nspaces  \n",
            b"\n\n\n",
            b"a lone\rcarriage return\n",
            b"two carriage returns\r\r\n",
        ] {
            for key in [&*keys::V4_SIGN, &*keys::V6_SIGN] {
                let (res, _) = verify(&sign(text, key), key);
                assert_valid(res, &format!("{:?}", String::from_utf8_lossy(text)));
            }
        }
    }

    #[test]
    fn the_signed_text_is_written_out_with_crlf() {
        let (res, out) = verify(
            &sign(b"one\ntwo  \n- three\n", &keys::V4_SIGN),
            &keys::V4_SIGN,
        );
        assert_valid(res, "text");
        // The signer keeps the text's last line ending as signed text.
        assert_eq!(out, b"one\r\ntwo\r\n- three\r\n");
    }

    #[test]
    fn a_message_with_crlf_framing_verifies() {
        let signed = String::from_utf8(sign(b"text\nlines\n", &keys::V4_SIGN)).unwrap();
        let crlf = signed.replace('\n', "\r\n");
        let (res, out) = verify(crlf.as_bytes(), &keys::V4_SIGN);
        assert_valid(res, "CRLF framing");
        assert_eq!(out, b"text\r\nlines\r\n");
    }

    #[test]
    fn a_changed_text_is_a_bad_signature() {
        let signed = String::from_utf8(sign(b"original\n", &keys::V4_SIGN)).unwrap();
        let changed = signed.replace("original", "changed!");
        let (res, _) = verify(changed.as_bytes(), &keys::V4_SIGN);
        let res = res.expect("a bad signature is a result");
        assert!(!res.is_verified);
        assert_ne!(res.signatures[0].status, GfrSignatureStatus::Valid);
    }

    #[test]
    fn broken_framing_is_refused_and_writes_nothing() {
        let signed = String::from_utf8(sign(b"framed text\n", &keys::V4_SIGN)).unwrap();
        let sig_at = signed.find(BEGIN_SIGNATURE).unwrap();
        for (what, message) in [
            ("text before it", format!("leading text\n{signed}")),
            ("data after it", format!("{signed}trailing data\n")),
            ("no signature", signed[..sig_at].to_string()),
            ("a cut signature", signed[..sig_at + 40].to_string()),
            (
                "no header end",
                "-----BEGIN PGP SIGNED MESSAGE-----\nHash: SHA256\n".to_string(),
            ),
            ("nothing", "\n \n".to_string()),
        ] {
            let (res, out) = verify(message.as_bytes(), &keys::V4_SIGN);
            assert!(res.is_err(), "{what}");
            assert!(out.is_empty(), "{what}: {} bytes written", out.len());
        }
    }

    #[test]
    fn a_line_past_the_limit_is_refused_without_holding_it() {
        let mut message = format!("{BEGIN_SIGNED}\nHash: SHA256\n\n").into_bytes();
        message.resize(message.len() + MAX_LINE + 1, b'a');
        message.extend_from_slice(b"\n");
        let mut out = Vec::new();
        let res = verify_cleartext_stream_internal(
            0,
            Cursor::new(&message),
            &mut out,
            None,
            None,
            std::ptr::null_mut(),
        );
        assert_eq!(res.err(), Some(GfrStatus::ErrorInvalidData));
        assert!(out.is_empty());
    }

    #[test]
    fn text_that_is_not_utf8_is_refused() {
        let signed = sign(b"valid text\n", &keys::V4_SIGN);
        let at = signed.windows(5).position(|w| w == b"valid").unwrap();
        let mut broken = signed.clone();
        broken[at] = 0xFF;
        let (res, out) = verify(&broken, &keys::V4_SIGN);
        assert_eq!(res.err(), Some(GfrStatus::ErrorInvalidInput));
        assert!(out.is_empty());
    }

    #[test]
    fn cancellation_aborts_a_cleartext_verify_stream() {
        const CH: i32 = 0x0C_92;
        let signed = sign(b"cancelled\n", &keys::V4_SIGN);
        cb::set_pubkey_answer(&keys::V4_SIGN.public_armored);
        crate::cancel::set_cancelled(CH, true);
        let mut out = Vec::new();
        let res = verify_cleartext_stream_internal(
            CH,
            Cursor::new(&signed),
            &mut out,
            None,
            Some(cb::pubkey_fetch),
            std::ptr::null_mut(),
        );
        crate::cancel::set_cancelled(CH, false);
        assert_eq!(res.err(), Some(GfrStatus::ErrorCanceled));
        assert!(out.is_empty());
    }
}

// SPDX-License-Identifier: GPL-3.0-or-later

pragma Singleton

import QtQuick
import GpgFrontend.Mobile

// The words every page uses for the same things: operations and how they
// went, what is wrong with a key, the three facts of a signature. One place,
// so the status line, the sheet and the lists never say a thing two ways.
QtObject {
    // A key as a person knows it: the name, else the address, else its ID.
    function keyName(key) {
        if (!key) return ""
        return key.name || key.email || key.keyId
               || (key.fingerprint || "").slice(-16)
    }

    // "Alice", "Alice, Bob", "Alice, Bob +2".
    // Up to two of @p keys by name, and how many more of @p total there are.
    function names(keys, total) {
        const shown = keys.slice(0, 2).map(k => keyName(k))
        const more = (total === undefined ? keys.length : total) - shown.length
        return more > 0 ? qsTr("%1 +%2").arg(shown.join(", ")).arg(more)
                        : shown.join(", ")
    }

    // The keys summary's first line: who an encryption goes to.
    function recipientsLine(selection) {
        if (selection.passphraseMode) return qsTr("With a passphrase")
        if (selection.recipientCount === 0) return qsTr("Choose recipients")
        return qsTr("To: %1").arg(names(selection.shownRecipientKeys,
                                        selection.recipientCount))
    }

    // The word under the desktop's stamp, for the same report.
    function reportWord(report) {
        if (report.cancelled === true) return qsTr("Information")
        switch (report.severity) {
        case "ok": return qsTr("Success")
        case "warn": return qsTr("Warning")
        default: return qsTr("Error")
        }
    }

    // The quick path's line: the last operation and its keys, done again in
    // one tap ("Encrypt to Alice, Bob +1 · signed as Me"); empty for none.
    function lastOperation(last) {
        if (!last || !last.op) return ""
        if (last.op === "symmetric") return qsTr("Encrypt with a passphrase")
        if (last.op === "sign")
            return qsTr("Sign as %1").arg(keyName(last.signerKey))
        const to = qsTr("Encrypt to %1").arg(names(last.recipientKeys,
                                                   last.recipientCount))
        return last.signs ? qsTr("%1 · signed as %2").arg(to)
                                                     .arg(keyName(last.signerKey))
                          : to
    }

    // Its second line: whether, and by whom, encryptions are signed.
    function signerLine(selection) {
        if (!selection.signsEncryption) return qsTr("Not signed")
        return qsTr("Signed as %1").arg(keyName(selection.signerKey))
    }

    // A key in a list: its name, else its address, else its key ID.
    function keyTitle(name, email, keyId) {
        return name.length > 0 ? name : email.length > 0 ? email : keyId
    }

    // What of a key's secret is here, as GnuPG's listing marks it
    // (KeyRoleModel's secretState): pub, sec, sec# (secret subkeys only),
    // sec> (the primary on a card). Not words: GnuPG's own short marks.
    function keyTag(secretState) {
        switch (secretState) {
        case "full": return "sec"
        case "partial": return "sec#"
        case "card": return "sec>"
        case "none": return "pub"
        default: return ""
        }
    }

    // The same, read out.
    function keyTagDescription(secretState) {
        switch (secretState) {
        case "full": return qsTr("Public and secret key")
        case "partial": return qsTr("Public key, secret subkeys only")
        case "card": return qsTr("Public key, secret key on a smart card")
        case "none": return qsTr("Public key only")
        default: return ""
        }
    }

    // What a key can do in GnuPG's letters (KeyRoleModel's capabilities:
    // "CESA", "CSE", "ES"), in words: "Certify, Encrypt, Sign", in the
    // letters' order.
    function capabilityWords(letters) {
        const words = {"C": qsTr("Certify"), "E": qsTr("Encrypt"),
                       "S": qsTr("Sign"), "A": qsTr("Authenticate")}
        return (letters || "").split("").filter(l => words[l] !== undefined)
                              .map(l => words[l]).join(", ")
    }

    // A subkey's mark: ssb with its secret, ssb> on a card, else sub.
    function subkeyTag(secret, card) {
        return card ? "ssb>" : secret ? "ssb" : "sub"
    }

    // How many keys are chosen: "1 key selected", "3 keys selected"; a
    // plural, as memberCount's: languages count more forms than two.
    function selectedCount(count) {
        return qsTr("%n key(s) selected", "", count)
    }

    // How many keys a group holds: "3 keys", "1 key", "No keys yet".
    function memberCount(count) {
        if (count === 0) return qsTr("No keys yet")
        return qsTr("%n key(s)", "", count)
    }

    // Why Encrypt or Sign cannot run, as the toolbox heads its answer.
    function block(reason) {
        switch (reason) {
        case "no_recipients": return qsTr("Choose who can read it.")
        case "recipient_issue": return qsTr("Some chosen keys cannot be used. Remove them or choose others.")
        case "no_signer": return qsTr("Choose a key to sign with.")
        case "signer_issue": return qsTr("The signing key cannot be used. Choose another.")
        default: return ""
        }
    }

    function running(op) {
        switch (op) {
        case "encrypt": return qsTr("Encrypting…")
        case "decrypt": return qsTr("Decrypting…")
        case "sign": return qsTr("Signing…")
        default: return qsTr("Verifying…")
        }
    }

    // An operation as a menu or button names it: "Encrypt & Sign".
    function operationLabel(op) {
        switch (op) {
        case "encrypt": return qsTr("Encrypt")
        case "encrypt_sign": return qsTr("Encrypt & Sign")
        case "symmetric": return qsTr("Encrypt with a passphrase")
        case "decrypt": return qsTr("Decrypt")
        case "decrypt_verify": return qsTr("Decrypt & Verify")
        case "sign": return qsTr("Sign")
        case "verify": return qsTr("Verify")
        default: return ""
        }
    }

    // @p text for an Action, a menu item's or a button's: an ampersand there
    // marks a shortcut unless doubled.
    function actionText(text) {
        return text.replace(/&/g, "&&")
    }

    // The operation by name, as the desktop's report heads it.
    function operationName(op) {
        switch (op) {
        case "encrypt": return qsTr("Encrypt Operation")
        case "decrypt": return qsTr("Decrypt Operation")
        case "sign": return qsTr("Sign Operation")
        default: return qsTr("Verify Operation")
        }
    }

    function done(op) {
        switch (op) {
        case "encrypt": return qsTr("Encrypted")
        case "decrypt": return qsTr("Decrypted")
        case "sign": return qsTr("Signed")
        default: return qsTr("Verified")
        }
    }

    function failed(op) {
        switch (op) {
        case "encrypt": return qsTr("Not encrypted")
        case "decrypt": return qsTr("Not decrypted")
        case "sign": return qsTr("Not signed")
        default: return qsTr("Not verified")
        }
    }

    // A record's headline: what happened, in a word or two.
    function title(record) {
        if (!record || record.operation === undefined) return ""
        switch (record.phase) {
        case "queued": return qsTr("Waiting")
        case "running": return running(record.operation)
        case "waiting_passphrase": return qsTr("Waiting for the passphrase")
        case "cancelled": return qsTr("Cancelled")
        case "failed": return failed(record.operation)
        default:
            // A verification that ran is "Verified" only if a signature holds.
            if (record.operation === "verify") {
                const sigs = (record.report || {}).signatures || []
                if (sigs.length === 0) return qsTr("No signature found")
                return sigs[0].signature === "valid" ? qsTr("Signature checked")
                                                     : qsTr("Signature problem")
            }
            return done(record.operation)
        }
    }

    // Why it ended without an engine saying so.
    function reason(word) {
        switch (word) {
        case "background": return qsTr("Stopped when the app went to the background.")
        case "stopped": return qsTr("Stopped before it started.")
        case "removed": return qsTr("Its file was removed from the list.")
        case "closed": return qsTr("Stopped when the app closed it.")
        default: return ""
        }
    }

    // Why an operation failed, in a few words, by the kind ResultPresenter
    // names (report.reason); empty for a kind with no words of its own.
    function failureReason(kind) {
        switch (kind) {
        case "no_secret_key": return qsTr("No matching secret key")
        case "no_public_key": return qsTr("Public key not found")
        case "no_key": return qsTr("Key not found")
        case "bad_passphrase": return qsTr("Wrong passphrase")
        case "no_passphrase": return qsTr("No passphrase given")
        case "not_openpgp": return qsTr("Not OpenPGP data")
        case "damaged": return qsTr("Damaged data")
        case "decrypt_failed": return qsTr("Could not be decrypted")
        case "bad_signature": return qsTr("Bad signature")
        case "key_expired": return qsTr("Key expired")
        case "key_revoked": return qsTr("Key revoked")
        case "unsupported_algorithm": return qsTr("Unsupported algorithm")
        default: return ""
        }
    }

    // The same, as a sentence that says what it means.
    function failureExplanation(kind) {
        switch (kind) {
        case "no_secret_key":
            return qsTr("This message cannot be decrypted because no matching "
                        + "secret key is available.")
        case "no_public_key":
            return qsTr("The public key this needs is not in your keys.")
        case "no_key":
            return qsTr("The key this needs is not in your keys.")
        case "bad_passphrase":
            return qsTr("The passphrase was wrong. Try again with the right one.")
        case "no_passphrase":
            return qsTr("No passphrase was given, so the key could not be used.")
        case "not_openpgp":
            return qsTr("This is not OpenPGP data, or not complete.")
        case "damaged":
            return qsTr("The OpenPGP data is damaged and cannot be read.")
        case "decrypt_failed":
            return qsTr("The message could not be decrypted. It may be damaged "
                        + "or made for another key.")
        case "bad_signature":
            return qsTr("The signature does not match the data.")
        case "key_expired":
            return qsTr("The key has expired and can no longer be used for this.")
        case "key_revoked":
            return qsTr("The key was revoked and can no longer be used for this.")
        case "unsupported_algorithm":
            return qsTr("This uses an algorithm this app does not support.")
        default: return ""
        }
    }

    // What a record was for: "the text", or the file's name.
    function target(record) {
        const targets = (record && record.targets) || []
        if (targets.length === 0) return ""
        if (targets[0].fileId < 0) return qsTr("Text")
        if (targets.length > 1)
            return qsTr("%1 with %2").arg(targets[0].name).arg(targets[1].name)
        return targets[0].name
    }

    // The three facts of a signature, apart.
    function signatureFact(word) {
        switch (word) {
        case "valid": return qsTr("Valid")
        case "invalid": return qsTr("Invalid: the content or signature was changed")
        case "expired": return qsTr("Valid, but the signature has expired")
        case "rejected": return qsTr("Not valid: the key had expired when it signed")
        default: return qsTr("Could not be checked")
        }
    }

    function keyStatusFact(word) {
        switch (word) {
        case "ok": return qsTr("OK")
        case "missing": return qsTr("Not in your keys")
        case "expired": return qsTr("Expired")
        case "revoked": return qsTr("Revoked")
        case "not_authorized": return qsTr("Not allowed to sign")
        default: return qsTr("Has problems")
        }
    }

    function identityFact(word) {
        switch (word) {
        case "trusted": return qsTr("Trusted")
        case "not_trusted": return qsTr("Not fully trusted")
        default: return qsTr("Not assessed")
        }
    }

    // What a key or subkey can do (KeyActionsController's usage map), as
    // the desktop lists it: "Certify, Sign".
    function usage(u) {
        if (!u) return ""
        return [u.certify === true ? qsTr("Certify") : "",
                u.sign === true ? qsTr("Sign") : "",
                u.encrypt === true ? qsTr("Encrypt") : "",
                u.authenticate === true ? qsTr("Authenticate") : ""]
               .filter(t => t.length > 0).join(", ")
    }

    // A key's or subkey's state in a few words: revoked, expired, or until
    // when it is valid.
    function keyState(k) {
        if (!k) return ""
        if (k.revoked === true) return qsTr("Revoked")
        if (k.disabled === true) return qsTr("Disabled")
        const when = Theme.date(k.expires)
        if (k.expired === true)
            return when.length > 0 ? qsTr("Expired on %1").arg(when) : qsTr("Expired")
        return when.length > 0 ? qsTr("Valid until %1").arg(when)
                               : qsTr("Does not expire")
    }

    // Why a subkey is revoked, as OpenPGP's reason codes say it (0 to 3).
    function revocationReasons() {
        return [{"code": 0, "text": qsTr("No reason given")},
                {"code": 1, "text": qsTr("It may have been compromised")},
                {"code": 2, "text": qsTr("A newer subkey replaces it")},
                {"code": 3, "text": qsTr("It is no longer used")}]
    }

    // Why a user ID is revoked: OpenPGP gives a user ID only these two
    // (RFC 9580 §5.2.3.31), as the desktop offers them; the core refuses
    // any other.
    function uidRevocationReasons() {
        return [{"code": 0, "text": qsTr("No reason given")},
                {"code": 4, "text": qsTr("The user ID is no longer valid")}]
    }
}

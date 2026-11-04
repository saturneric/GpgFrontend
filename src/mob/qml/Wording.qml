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
        if (report.status > 0) return qsTr("Success")
        return report.status === 0 ? qsTr("Warning") : qsTr("Error")
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

    // Under keyTitle(): the address with the short key ID, which tells two
    // keys of one address apart (its end, never elided), or the key ID alone
    // when the address is already on top or missing.
    function keySubtitle(name, email, keyId) {
        return name.length > 0 && email.length > 0
            ? qsTr("%1 · %2").arg(email).arg(keyId.slice(-8)) : keyId
    }

    // How many keys a group holds: "3 keys", "1 key", "No keys yet".
    function memberCount(count) {
        if (count === 0) return qsTr("No keys yet")
        return qsTr("%n key(s)", "", count)
    }

    function keyIssue(issue) {
        switch (issue) {
        case "empty_group": return qsTr("Has no keys")
        case "missing": return qsTr("No longer in your keys")
        case "expired": return qsTr("Expired")
        case "revoked": return qsTr("Revoked")
        case "disabled": return qsTr("Disabled")
        case "cannot_encrypt": return qsTr("Cannot be encrypted to")
        case "cannot_sign": return qsTr("Cannot sign")
        case "no_secret": return qsTr("Not a key of your own")
        default: return ""
        }
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

    // The same, short, as a bar button names it: the desktop's toolbar
    // says "Sym. Encrypt".
    function buttonLabel(op) {
        return op === "symmetric" ? qsTr("Sym. Encrypt") : operationLabel(op)
    }

    // @p text for an Action, a menu item's or a button's: an ampersand there
    // marks a shortcut unless doubled.
    function actionText(text) {
        return text.replace(/&/g, "&&")
    }

    // Whether More offers @p op: an operation of the two kinds on a button
    // is there when the button does another (@p defaultEncrypt,
    // @p defaultDecrypt); one besides the four once turned on (@p extra).
    function inMore(op, extra, defaultEncrypt, defaultDecrypt) {
        if (op === defaultEncrypt || op === defaultDecrypt) return false
        if (op === "encrypt" || op === "decrypt") return true
        return extra.includes(op)
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

    // What the Text page holds, by ContentKind.
    function textKind(kind) {
        switch (kind) {
        case "message": return qsTr("Encrypted message")
        case "signed": return qsTr("Signed text")
        case "signature": return qsTr("Signature")
        case "public_key": return qsTr("Public key")
        case "private_key": return qsTr("Private key")
        case "plain": return qsTr("Plain text")
        default: return ""
        }
    }

    // What a file is by its name (FileWorkspace's content role, as the
    // desktop tells it), said where it helps: OpenPGP data. Other files go
    // unlabelled.
    function fileContent(kind) {
        switch (kind) {
        case "message": return qsTr("Encrypted or signed data")
        case "signed": return qsTr("Signed text")
        case "signature": return qsTr("Signature")
        case "public_key": return qsTr("Public key")
        case "private_key": return qsTr("Private key")
        default: return ""
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
        return [qsTr("No reason given"),
                qsTr("It may have been compromised"),
                qsTr("A newer subkey replaces it"),
                qsTr("It is no longer used")]
    }
}

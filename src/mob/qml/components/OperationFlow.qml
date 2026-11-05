// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import GpgFrontend.Mobile

// How a page's Encrypt, Decrypt, Sign and Verify are carried out, the same on
// the Text page and the Files page: encrypting and signing first ask with
// which keys, in the key toolbox, which then runs the operation on what it
// was asked for; the quick path repeats the last operation with its keys.
// The page says how an operation starts and whether what it was asked for is
// still there.
QtObject {
    id: flow

    // Start @p op on @p targets: true when it started.
    required property var start
    // What of @p targets is still there to run on once the keys are chosen:
    // {ok: true, targets} to go on with, or {ok: false, message} saying why
    // not. An explicit result: a page's targets may themselves be text.
    property var recheck: function(targets) { return {"ok": true, "targets": targets} }
    // The page's notice, for what cannot start.
    required property var notice
    // The sheet host, when there is one: the key toolbox is opened in it.
    property var sheets: null
    // Asks one question before an encryption goes ahead: called with
    // ({title, text, detail, accept}, goOn); goOn() when the person agrees.
    // Without it nothing is asked.
    property var ask: null

    // @p op on @p targets.
    function request(op, targets) {
        // To a passphrase: no keys to choose, it starts now.
        if (op === "symmetric") {
            WorkSelection.passphraseMode = true
            run("encrypt", targets)
            return
        }
        // Whether an encryption is signed is the operation's: Encrypt or
        // Encrypt & Sign, chosen in the menu.
        const signs = op === "encrypt_sign"
        if (op === "encrypt" || signs) {
            WorkSelection.passphraseMode = false
            WorkSelection.signingEnabled = signs
            op = "encrypt"
        }
        if (op !== "encrypt" && op !== "sign") {
            run(op, targets)
            return
        }
        if (!sheets) {
            const block = op === "encrypt" ? WorkSelection.encryptBlock
                                           : WorkSelection.signBlock
            if (block.length > 0) notice.show(Wording.block(block))
            else run(op, targets)
            return
        }
        sheets.openKeys(op === "sign" ? "sign" : "encrypt", {
            "op": op,
            "signs": signs,
            "label": signs ? qsTr("Encrypt and sign now")
                   : op === "encrypt" ? qsTr("Encrypt now") : qsTr("Sign now"),
            "run": function() {
                const still = flow.recheck(targets)
                if (!still.ok) {
                    flow.notice.show(still.message)
                    return
                }
                flow.run(op, still.targets)
            }
        })
    }

    // The quick path: the last operation, with its keys, on @p targets at
    // once; what has become of a key since is asked for as on the standard
    // path.
    function repeatLast(targets) {
        const op = WorkSelection.restoreLast()
        if (op.length === 0) return
        if (op === "symmetric") {
            request("symmetric", targets)
            return
        }
        const block = op === "sign" ? WorkSelection.signBlock : WorkSelection.encryptBlock
        if (block.length === 0) run(op, targets)
        // An encryption is asked for again as it was: signed or not.
        else request(op === "encrypt" && WorkSelection.signingEnabled ? "encrypt_sign" : op,
                     targets)
    }

    // Start @p op, and keep what was done with keys to be done again in one
    // tap. An encryption to keys first asks what the desktop asks
    // (WorkSelection.encryptQuestions), one question after the other.
    function run(op, targets) {
        if (op === "encrypt" && flow.ask)
            askThen(WorkSelection.encryptQuestions(), function() { flow.go(op, targets) })
        else
            go(op, targets)
    }

    function askThen(questions, goOn) {
        if (questions.length === 0) {
            goOn()
            return
        }
        flow.ask(questions[0], function() { flow.askThen(questions.slice(1), goOn) })
    }

    function go(op, targets) {
        if (!flow.start(op, targets)) return
        if (op === "encrypt")
            WorkSelection.rememberOperation(WorkSelection.passphraseMode ? "symmetric"
                                                                         : "encrypt")
        else if (op === "sign")
            WorkSelection.rememberOperation("sign")
    }
}

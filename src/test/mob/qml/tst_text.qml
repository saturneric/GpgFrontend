import QtQuick
import QtTest
import QtQuick.Controls
import GpgFrontend.Mobile

// The text page and its sheets, with no core behind them: what they show,
// not what an operation does (gf_mob_tests covers that).
Item {
    width: 400
    height: 800

    TextPage {
        id: text
        anchors.fill: parent
    }

    PassphraseSheet {
        id: passphrase
    }

    TestCase {
        name: "TextPage"
        when: windowShown

        function findByName(parent, name) {
            for (var i = 0; i < parent.children.length; ++i) {
                var child = parent.children[i]
                if (child.objectName === name)
                    return child
                var found = findByName(child, name)
                if (found)
                    return found
            }
            return null
        }

        function test_nothing_to_do_without_text() {
            var input = findByName(text, "textInput")
            const actions = [text.encryptAction, text.decryptAction,
                             text.signAction, text.verifyAction]
            for (const action of actions) verify(!action.enabled, action.text)
            // Nothing there: the ways to start, instead of an empty page.
            verify(findByName(text, "emptyText").visible)
            input.text = "hello"
            for (const action of actions) verify(action.enabled, action.text)
            verify(!findByName(text, "emptyText").visible)
            input.text = ""
        }

        function test_the_bar_offers_what_suits_the_text() {
            var input = findByName(text, "textInput")
            input.text = "Dear Bob"
            compare(text.kind, "plain")
            compare(text.primaryAction, text.encryptAction)
            input.text = "-----BEGIN PGP MESSAGE-----\nhQ==\n"
            compare(text.kind, "message")
            compare(text.primaryAction, text.decryptAction)
            compare(findByName(text, "textKind").text, "Encrypted message")
            input.text = "-----BEGIN PGP SIGNED MESSAGE-----\nHash: SHA512\n"
            compare(text.primaryAction, text.verifyAction)
            input.text = "-----BEGIN PGP PUBLIC KEY BLOCK-----\n"
            compare(text.primaryAction, text.importAction)
            // Every operation stays offered whatever the text is.
            verify(text.encryptAction.enabled && text.signAction.enabled)
            input.text = ""
        }

        function test_the_operations_are_labelled_for_everyone() {
            var input = findByName(text, "textInput")
            input.text = "hello"
            // Each operation its own button, named; the one that fits the
            // text marked. No keys in the bar: they are asked for later.
            for (const name of ["encryptButton", "decryptButton",
                                "signButton", "verifyButton"]) {
                var button = findByName(text, name)
                verify(button !== null && button.visible, name)
                verify(button.height >= 40)
                verify(button.Accessible.name.length > 0)
            }
            compare(findByName(text, "encryptButton").text, "Encrypt")
            // The one that fits is said, not drawn as chosen: action
            // buttons are not tabs.
            for (const name of ["encryptButton", "decryptButton",
                                "signButton", "verifyButton"])
                verify(!findByName(text, name).highlighted, name)
            verify(findByName(text, "encryptButton").Accessible.description.length > 0)
            compare(findByName(text, "decryptButton").Accessible.description, "")
            // More is the fifth action, as wide as the others.
            const more = findByName(text, "operationMore")
            compare(more.width, findByName(text, "verifyButton").width)
            verify(more.height >= 48)
            compare(findByName(text, "keysButton"), null)
            input.text = ""
        }

        function test_the_editor_s_tools_come_only_with_the_keyboard() {
            verify(!findByName(text, "editTools").visible)
            verify(findByName(text, "actionDock").visible)
        }

        function test_no_result_no_status() {
            verify(!text.resultState)
            verify(!findByName(text, "resultViews").visible)
            verify(!findByName(text, "statusLine").visible)
        }

        function test_a_result_and_its_original_in_turn() {
            var input = findByName(text, "textInput")
            text.resultInput = "the message"
            text.resultOutput = "-----BEGIN PGP MESSAGE-----"
            text.resultState = true
            text.setText(text.resultOutput)
            verify(findByName(text, "resultViews").visible)
            verify(input.readOnly)
            compare(text.primaryAction, text.copyAction)

            text.views.currentIndex = 0
            compare(input.text, "the message")
            text.views.currentIndex = 1
            compare(input.text, "-----BEGIN PGP MESSAGE-----")

            // Back leaves it, keeping what is shown as the text to work on,
            // with an undo history of its own.
            verify(text.back())
            verify(!text.resultState)
            verify(!input.readOnly)
            compare(input.text, "-----BEGIN PGP MESSAGE-----")
            verify(!input.canUndo)
            verify(!text.back())
            input.text = ""
        }

        function test_clear_is_taken_back_by_undo() {
            var input = findByName(text, "textInput")
            verify(!text.clearAction.enabled)
            input.text = "keep me"
            verify(text.clearAction.enabled)
            text.clearAction.trigger()
            compare(input.text, "")
            // Said at the foot, with the way back ...
            compare(text.notice.actionText, "Undo")
            verify(text.notice.visible)
            // ... which brings the text back.
            text.notice.action()
            compare(input.text, "keep me")
            input.text = ""
            text.notice.show("")
        }

        function test_shared_text_never_overwrites_unasked() {
            var input = findByName(text, "textInput")
            // On an empty page it is simply put there ...
            text.receive("from another app")
            compare(input.text, "from another app")
            // ... over text, it waits for the person to agree.
            input.text = "mine"
            text.receive("again")
            compare(input.text, "mine")
            const ask = text.confirmReplace
            tryVerify(() => ask.opened)
            ask.reject()
            compare(input.text, "mine")
            text.receive("again")
            tryVerify(() => ask.opened)
            ask.accept()
            compare(input.text, "again")
            input.text = ""
            text.notice.show("")
        }

        function test_every_line_is_numbered() {
            var input = findByName(text, "textInput")
            input.text = "one\ntwo\n\nfour"
            compare(text.lineStarts, [0, 4, 8, 9])
            input.text = ""
        }

        function test_a_share_that_went_nowhere_points_to_copy() {
            var notice = text.notice
            text.shared("unsupported")
            verify(notice.visible)
            verify(notice.text.indexOf("Copy") >= 0)
            text.shared("handed_off")
            compare(notice.text, "Shared.")
            // Not confirmed either way: nothing claimed.
            text.shared("dismissed")
            compare(notice.text, "")
        }
    }

    TestCase {
        name: "Wording"

        function test_a_signature_s_three_facts_are_kept_apart() {
            compare(Wording.signatureFact("valid"), "Valid")
            compare(Wording.keyStatusFact("expired"), "Expired")
            compare(Wording.identityFact("not_assessed"), "Not assessed")
            // None of them claims who signed.
            verify(Wording.signatureFact("valid").indexOf("Good") < 0)
        }

        function test_a_record_says_what_happened() {
            compare(Wording.title({"operation": "encrypt", "phase": "done"}),
                    "Encrypted")
            compare(Wording.title({"operation": "decrypt", "phase": "failed"}),
                    "Not decrypted")
            compare(Wording.title({"operation": "verify", "phase": "done",
                                   "report": {"signatures": []}}),
                    "No signature found")
            compare(Wording.title({"operation": "sign", "phase": "cancelled"}),
                    "Cancelled")
        }

        // More has what the buttons do not: the plain operation when a
        // button does another, the others once turned on in Settings.
        function test_more_offers_what_the_buttons_do_not() {
            verify(!Wording.inMore("encrypt", [], "encrypt", "decrypt"))
            verify(!Wording.inMore("encrypt_sign", [], "encrypt", "decrypt"))
            verify(Wording.inMore("encrypt_sign", ["encrypt_sign"], "encrypt", "decrypt"))
            verify(!Wording.inMore("encrypt_sign", ["encrypt_sign"], "encrypt_sign", "decrypt"))
            verify(Wording.inMore("encrypt", ["encrypt_sign"], "encrypt_sign", "decrypt"))
            verify(Wording.inMore("decrypt", ["decrypt_verify"], "encrypt", "decrypt_verify"))
            // The buttons say what they do, an ampersand kept for an Action.
            compare(Wording.actionText(Wording.buttonLabel("encrypt_sign")),
                    "Encrypt && Sign")
            compare(Wording.buttonLabel("symmetric"), "Sym. Encrypt")
        }

        function test_the_last_operation_is_said_in_a_line() {
            compare(Wording.lastOperation({}), "")
            compare(Wording.lastOperation({"op": "symmetric"}),
                    "Encrypt with a passphrase")
            compare(Wording.lastOperation({"op": "sign",
                                           "signerKey": {"name": "Me"}}),
                    "Sign as Me")
            compare(Wording.lastOperation({"op": "encrypt", "recipientCount": 3,
                                           "recipientKeys": [{"name": "Alice"},
                                                             {"name": "Bob"}],
                                           "signs": true,
                                           "signerKey": {"name": "Me"}}),
                    "Encrypt to Alice, Bob +1 · signed as Me")
        }

        function test_keys_are_summed_up_shortly() {
            compare(Wording.names([{"name": "Alice"}]), "Alice")
            compare(Wording.names([{"name": "Alice"}, {"email": "b@x"},
                                   {"keyId": "C"}]), "Alice, b@x +1")
            compare(Wording.recipientsLine({"passphraseMode": true}),
                    "With a passphrase")
            compare(Wording.recipientsLine({"passphraseMode": false,
                                            "recipientCount": 0,
                                            "shownRecipientKeys": []}),
                    "Choose recipients")
            // Named from the few shown, counted from all of them.
            compare(Wording.recipientsLine({"passphraseMode": false,
                                            "recipientCount": 500,
                                            "shownRecipientKeys": [{"name": "Alice"},
                                                                   {"name": "Bob"}]}),
                    "To: Alice, Bob +498")
        }
    }

    TestCase {
        name: "PassphraseSheet"

        function test_hidden_while_nothing_is_asked() {
            verify(!passphrase.asking)
            verify(!passphrase.visible)
        }

        // From the bottom, as the other sheets: not a dialog in the middle.
        function test_it_is_a_sheet_from_the_bottom() {
            compare(passphrase.edge, Qt.BottomEdge)
            // A tap beside it is no answer.
            compare(passphrase.closePolicy, Popup.NoAutoClose)
        }
    }
}

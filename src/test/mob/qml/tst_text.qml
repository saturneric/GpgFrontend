import QtQuick
import QtTest
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

    ResultSheet {
        id: result
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
            // Encrypt and decrypt are buttons in the bar; sign and verify
            // are in its menu, which is not in the item tree until opened.
            const actions = [text.encryptAction, text.decryptAction,
                             text.signAction, text.verifyAction]
            for (const name of ["encryptButton", "decryptButton"])
                verify(!findByName(text, name).enabled, name)
            for (const action of actions) verify(!action.enabled, action.text)
            input.text = "hello"
            for (const name of ["encryptButton", "decryptButton"])
                verify(findByName(text, name).enabled, name)
            for (const action of actions) verify(action.enabled, action.text)
            input.text = ""
        }

        function test_the_bar_buttons_are_labelled_for_everyone() {
            for (const name of ["encryptButton", "decryptButton"]) {
                var button = findByName(text, name)
                verify(button.Accessible.name.length > 0, name)
                verify(button.width >= 48 && button.height >= 48, name)
            }
        }

        function test_no_result_no_status() {
            verify(!findByName(text, "textStatus").visible)
            verify(!findByName(text, "copyButton").enabled)
        }

        function test_a_result_takes_the_text_s_place_and_undo_brings_it_back() {
            var input = findByName(text, "textInput")
            input.text = "the message"
            text.applyResult("-----BEGIN PGP MESSAGE-----")
            compare(input.text, "-----BEGIN PGP MESSAGE-----")
            verify(input.canUndo)
            while (input.canUndo && input.text !== "the message") input.undo()
            compare(input.text, "the message")
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
        name: "ResultSheet"

        function test_a_refused_prompt_reads_cancelled() {
            result.report = {"status": -1, "cancelled": true}
            compare(result.title, "Cancelled")
            result.report = {"status": -1, "cancelled": false}
            compare(result.title, "Failed")
            result.report = {"status": 0}
            compare(result.title, "Done, with warnings")
        }

        function test_signature_wording() {
            compare(result.validityText("fully_valid"), "Good signature")
            compare(result.validityText("key_missing"),
                    "Signed by a key you do not have")
            compare(result.validityText("something new"),
                    "The signature could not be checked")
        }
    }

    TestCase {
        name: "PassphraseSheet"

        function test_hidden_while_nothing_is_asked() {
            verify(!passphrase.asking)
            verify(!passphrase.visible)
        }
    }
}

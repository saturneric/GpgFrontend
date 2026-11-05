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
        crypto: fake
    }

    // The engine as the page sees it, run by hand: each operation starts,
    // and ends when the test says how (finish()).
    QtObject {
        id: fake

        property bool busy: false
        property string output: ""
        property var result: ({})
        property bool sensitive: false
        property int operationId: 0
        property int finishedId: 0
        property bool replacesText: false
        property var held: ({})
        property var started: []
        property int clears: 0

        function begin(op, text) {
            started = started.concat([[op, text]])
            ++operationId
            busy = true
            return true
        }
        function encrypt(t, r, s, sym) { return begin("encrypt", t) }
        function decrypt(t, v) { return begin(v ? "decrypt_verify" : "decrypt", t) }
        function sign(t, s) { return begin("sign", t) }
        function verify(t) { return begin("verify", t) }
        function holds(id) { return false }
        function retry(id) { return false }
        function clear() {
            ++clears
            held = {}
            if (!busy) {
                output = ""
                sensitive = false
            }
        }
        function hold(id) {
            if (id !== finishedId || !replacesText) return false
            held[id] = output
            return true
        }
        function isHeld(id) { return held[id] !== undefined }
        function take(id) {
            const kept = held[id] || ""
            delete held[id]
            return kept
        }
        function release(id) { delete held[id] }

        // The operation that runs ends: @p replaces with @p out, or not.
        function finish(replaces, out, ok) {
            busy = false
            finishedId = operationId
            replacesText = replaces
            output = out
            result = ok === false ? {"status": -1, "severity": "critical",
                                     "description": "It failed."}
                                  : {"status": 1, "severity": "ok"}
            text.finished(ok !== false)
        }

        function reset() {
            busy = false; output = ""; sensitive = false; replacesText = false
            held = {}; started = []; clears = 0
        }
    }

    PassphraseSheet {
        id: passphrase
    }

    // The flow alone, with a stand-in for the key toolbox: what it runs
    // once the keys are chosen.
    QtObject {
        id: toolboxStandIn
        property var pending: null
        function openKeys(which, pending) { toolboxStandIn.pending = pending }
        function track(id) {}
    }

    QtObject {
        id: noticeStandIn
        property string said: ""
        function show(message) { said = message }
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

        function init() {
            fake.reset()
            text.editor.text = ""
            text.notice.show("")
        }

        // Type @p words as an edit (an insert, which Undo takes back).
        function type(words) {
            text.editor.insert(text.editor.length, words)
        }

        function test_nothing_to_do_without_text() {
            // Nothing there: what the page is for, without buttons.
            verify(findByName(text, "emptyText").visible)
            compare(text.unavailable, "Type or paste text first.")
            compare(text.operationMenu.unavailable, "Type or paste text first.")
            type("hello")
            compare(text.unavailable, "")
            verify(!findByName(text, "emptyText").visible)
        }

        // The Label that shows @p item's words, if its row has one.
        function wordsOf(item) {
            const walk = parent => {
                for (let i = 0; i < parent.children.length; ++i) {
                    const child = parent.children[i]
                    if (child.truncated !== undefined && child.text === item.text)
                        return child
                    const found = walk(child)
                    if (found) return found
                }
                return null
            }
            return item.contentItem ? walk(item.contentItem) : null
        }

        // The menu is as wide as its longest name needs (translations are
        // longer than English), right above the button, inside the window.
        function test_the_operation_menu_fits_its_names() {
            type("hello")
            const menu = text.operationMenu
            // A row longer than the style's 200 dp menu, as German names are.
            const again = menu.againText
            menu.againText = "Encrypt to Alice and Bob Example"
            menu.openAbove(findByName(text, "operationButton"))
            tryVerify(() => menu.opened)
            let rows = 0
            for (let i = 0; i < menu.count; ++i) {
                const item = menu.itemAt(i)
                if (!item || item.height <= 0) continue
                ++rows
                // Its words all shown: on one line where the row is wide
                // enough, wrapped where the window is narrower than they are.
                const words = wordsOf(item)
                if (words) {
                    verify(!words.truncated, item.objectName + " cut")
                    verify(words.width <= item.availableWidth + 0.5,
                           item.objectName + " past its row")
                } else {
                    verify(item.width >= item.implicitWidth - 0.5,
                           item.objectName + " cut: " + item.width + " < "
                           + item.implicitWidth)
                }
            }
            verify(rows >= 6)
            verify(menu.width <= text.width)
            verify(menu.width > 200)
            menu.close()
            tryVerify(() => !menu.visible)
            menu.againText = again
        }

        function test_one_editor_and_no_status_badges() {
            for (const gone of ["resultViews", "textKind", "textHistory",
                                "editTools", "actionDock"])
                compare(findByName(text, gone), null, gone)
            verify(findByName(text, "operationButton").visible)
            for (const name of ["undoButton", "redoButton", "copyButton"])
                verify(findByName(text, name) !== null, name)
        }

        function test_a_result_replaces_the_text_as_one_undo_step() {
            type("-----BEGIN PGP MESSAGE-----")
            text.request("decrypt")
            compare(fake.started.length, 1)
            fake.finish(true, "the secret")
            compare(text.editor.text, "the secret")
            text.editor.undo()
            compare(text.editor.text, "-----BEGIN PGP MESSAGE-----")
            text.editor.redo()
            compare(text.editor.text, "the secret")
        }

        function test_an_empty_result_is_a_result_too() {
            type("-----BEGIN PGP MESSAGE-----")
            text.request("decrypt")
            fake.finish(true, "")
            compare(text.editor.text, "")
            text.editor.undo()
            compare(text.editor.text, "-----BEGIN PGP MESSAGE-----")
        }

        function test_a_verification_or_a_failure_leaves_the_text() {
            type("-----BEGIN PGP SIGNED MESSAGE-----")
            text.request("verify")
            fake.finish(false, "")
            compare(text.editor.text, "-----BEGIN PGP SIGNED MESSAGE-----")
            text.request("decrypt")
            fake.finish(false, "", false)
            compare(text.editor.text, "-----BEGIN PGP SIGNED MESSAGE-----")
            // With no status sheet, the failure is said at the foot.
            compare(text.notice.text, "It failed.")
        }

        function test_text_changed_meanwhile_is_not_overwritten() {
            type("abc")
            text.request("decrypt")
            type("d")
            fake.finish(true, "result")
            compare(text.editor.text, "abcd")
            compare(text.notice.actionText, "Replace anyway")
            // The offer is that operation's result, put in as one step.
            text.notice.handler()
            compare(text.editor.text, "result")
            text.editor.undo()
            compare(text.editor.text, "abcd")
        }

        function test_undo_meanwhile_counts_as_a_change() {
            type("abc")
            text.request("decrypt")
            // An edit of its own (not merged with the typing), then Undo:
            // the same text again, yet it changed twice meanwhile.
            text.editor.insert(0, "x")
            text.editor.undo()
            compare(text.editor.text, "abc")
            fake.finish(true, "result")
            compare(text.editor.text, "abc")
            compare(text.notice.actionText, "Replace anyway")
        }

        function test_a_wiped_text_never_comes_back() {
            type("-----BEGIN PGP MESSAGE-----")
            fake.sensitive = true
            text.request("decrypt")
            // Cleared while it runs.
            text.clearText()
            compare(text.editor.text, "")
            verify(!text.editor.canUndo)
            fake.finish(true, "the secret")
            compare(text.editor.text, "")
            verify(fake.clears >= 2)
            compare(text.notice.actionText, "")
        }

        function test_clear_is_taken_back_by_undo() {
            type("keep me")
            text.clearText()
            compare(text.editor.text, "")
            compare(text.notice.actionText, "Undo")
            verify(text.notice.visible)
            text.notice.handler()
            compare(text.editor.text, "keep me")
        }

        function test_short_text_stays_put() {
            const flick = findByName(text, "textScroller")
            type("short")
            compare(flick.boundsBehavior, Flickable.StopAtBounds)
            verify(!flick.interactive)
            let many = ""
            for (let i = 0; i < 200; ++i) many += "line " + i + "\n"
            type(many)
            tryVerify(() => flick.interactive)
            // A result starts at its top.
            text.replaceText(many)
            compare(flick.contentY, 0)
        }

        function test_shared_text_never_overwrites_unasked() {
            text.receive("from another app")
            compare(text.editor.text, "from another app")
            text.editor.text = "mine"
            text.receive("again")
            compare(text.editor.text, "mine")
            const ask = text.confirmReplace
            tryVerify(() => ask.opened)
            ask.reject()
            compare(text.editor.text, "mine")
            text.receive("again")
            tryVerify(() => ask.opened)
            ask.accept()
            compare(text.editor.text, "again")
        }

        function test_every_line_is_numbered() {
            text.editor.text = "one\ntwo\n\nfour"
            compare(text.lineStarts, [0, 4, 8, 9])
        }

        function test_a_share_that_went_nowhere_points_to_copy() {
            var notice = text.notice
            text.shared("unsupported")
            verify(notice.visible)
            verify(notice.text.indexOf("Copy") >= 0)
            text.shared("handed_off")
            compare(notice.text, "Shared.")
            text.shared("dismissed")
            compare(notice.text, "")
        }
    }

    TestCase {
        name: "OperationFlow"

        Component {
            id: flowComponent
            OperationFlow {
                property var ran: []
                start: function(op, target) { ran = ran.concat([[op, target]]); return true }
                notice: noticeStandIn
                sheets: toolboxStandIn
            }
        }

        // A page's targets may be text: once the keys are chosen, the
        // operation runs on it, and the text is not mistaken for a reason
        // not to.
        function test_text_targets_run_once_the_keys_are_chosen() {
            const flow = createTemporaryObject(flowComponent, null, {
                "recheck": function(t) { return {"ok": true, "targets": t} }
            })
            noticeStandIn.said = ""
            flow.request("encrypt", "test")
            verify(toolboxStandIn.pending !== null)
            toolboxStandIn.pending.run()
            compare(flow.ran.length, 1)
            compare(flow.ran[0][0], "encrypt")
            compare(flow.ran[0][1], "test")
            compare(noticeStandIn.said, "")
        }

        function test_a_refused_recheck_is_said() {
            const flow = createTemporaryObject(flowComponent, null, {
                "recheck": function(t) { return {"ok": false, "message": "gone"} }
            })
            flow.request("sign", "test")
            toolboxStandIn.pending.run()
            compare(flow.ran.length, 0)
            compare(noticeStandIn.said, "gone")
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

        // An ampersand is kept for an Action, which would read it as a
        // shortcut mark.
        function test_an_ampersand_is_kept_for_an_action() {
            compare(Wording.actionText(Wording.operationLabel("encrypt_sign")),
                    "Encrypt && Sign")
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

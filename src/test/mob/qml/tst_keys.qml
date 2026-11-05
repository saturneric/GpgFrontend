// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtTest
import GpgFrontend.Mobile

Item {
    width: 400
    height: 800

    KeyPickerSheet {
        id: picker
    }

    // Keys in place of the core's (the tests have none).
    ListModel {
        id: pickerKeys

        ListElement { name: "Alice"; email: "alice@example.org"; keyId: "AAAA"
                      secretState: ""; capabilities: "ESC" }
        ListElement { name: "Bob"; email: "bob@example.org"; keyId: "BBBB"
                      secretState: ""; capabilities: "ESC" }
    }

    KeysPage {
        id: keys
        anchors.fill: parent
    }

    KeyDetailPage {
        id: detail
        visible: false
    }

    // What a user ID must be, as the real key maker says.
    KeyActionsController {
        id: identityRules
    }

    // The key maker as Quick key sees it, run by hand.
    QtObject {
        id: fakeMaker
        property bool busy: false
        property var profiles: []
        property var validities: []
        property var made: []
        function reloadProfiles() {}
        function checkIdentity(name, email, comment) {
            return identityRules.checkIdentity(name, email, comment)
        }
        function generate(name, email, profile, protect, comment) {
            made = made.concat([[name, email, profile, protect, comment]])
            busy = true
        }
    }

    GenerateSheet {
        id: maker
        actions: fakeMaker
    }

    CategorySheet {
        id: categorySheet
    }

    Component {
        id: keyRowComponent
        KeyRow {
            width: 400
            name: "Alice Example"
            email: "a.very.long.address.that.goes.on@example.invalid"
            keyId: "0123456789ABCDEF"
            usable: true
            status: "ok"
            statusText: ""
            fingerprint: "AB".repeat(20)
            secretState: "partial"
            capabilities: "ES"
            isKey: true
            members: []
        }
    }

    SubkeySheet {
        id: subkeySheet
    }

    UidSheet {
        id: uidSheet
    }

    GeneratedKeySheet {
        id: generated
    }

    Component {
        id: factsComponent
        KeyFacts {
            width: 360
        }
    }

    Component {
        id: revokeComponent
        RevokeSubkeySheet {}
    }

    Component {
        id: faceComponent
        KeyChoiceFace {
            width: 360
            name: "Alice"
            email: "alice@example.org"
            keyId: "AAAA"
            isKey: true
        }
    }

    Component {
        id: fingerprintComponent
        FingerprintView {
        }
    }

    Component {
        id: addUidComponent
        AddUidSheet {}
    }

    AddSubkeySheet {
        id: addSubkey
        validities: [{"id": "1y", "name": "1 year"}, {"id": "2y", "name": "2 years"},
                     {"id": "forever", "name": "Never"}]
    }

    TestCase {
        name: "KeysPage"
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

        // Categories: one button says which is shown and opens the list of
        // them, one under the other; no strip of chips to scroll sideways.
        function test_the_category_is_chosen_from_a_list() {
            const button = findByName(keys, "categoryButton")
            verify(button !== null)
            compare(button.text, "All keys")
            compare(findByName(keys, "categoryChips"), null)
            compare(findByName(keys, "categoryChip_new"), null)

            // Only a colour and an arrow, as narrow whatever the category is
            // called; its name is said aloud. All keys: an empty ring.
            const width = button.width
            const mark = findByName(button, "categoryMark")
            verify(Qt.colorEqual(mark.color, "transparent"))
            verify(mark.border.width > 0)
            verify(button.Accessible.name.indexOf("All keys") >= 0)
            button.category = {"id": "x", "color": "#d32f2f",
                               "name": "A category with a very long name indeed"}
            compare(button.width, width)
            verify(Qt.colorEqual(mark.color, "#d32f2f"))
            compare(mark.border.width, 0)
            verify(button.Accessible.name.indexOf("very long name") >= 0)
            button.category = {"id": "", "name": "All keys", "color": ""}

            // The search beside it takes the rest; both drawn 44 high, both
            // still a full touch target.
            const search = findByName(keys, "keySearch")
            verify(search.width > 3 * button.width)
            compare(search.background.height, 44)
            compare(button.background.height, 44)
            verify(search.height >= 48 && button.height >= 48)
        }

        // Switching databases: choosing is one sheet, the switch another,
        // never both at once; one switch at a time; the name at the top is
        // the one whose keys are listed, never the new one before it worked;
        // a switch that fails says why and lets go.
        function test_switching_a_database_is_apart_from_choosing_it() {
            const choose = keys.databaseSheet
            const progress = keys.databaseSwitch
            const before = keys.databaseName
            // The choice keeps no room for progress.
            compare(findByName(choose.contentItem, "keyDatabaseBusy"), null)

            verify(progress.start("Elsewhere", false))
            verify(progress.switching)
            compare(progress.targetName, "Elsewhere")
            // A second switch while one runs is refused.
            verify(!progress.start("Third", false))
            compare(progress.targetName, "Elsewhere")
            compare(keys.databaseName, before)
            // Not at once: a switch done quickly never flashes the sheet.
            verify(!progress.opened)
            tryVerify(() => progress.opened, progress.showAfter + 1000)
            // Asking to choose meanwhile shows the switch, not the choice.
            choose.begin()
            verify(!choose.visible)
            // It cannot be put away while it runs.
            verify(!progress.interactive)
            progress.back()
            verify(progress.opened)

            progress.switched(false, "It is gone.")
            verify(!progress.switching)
            verify(progress.opened)
            verify(progress.interactive)
            compare(findByName(progress.contentItem, "keyDatabaseSwitchProblem").text,
                    "It is gone.")
            compare(keys.databaseName, before)
            // A late answer for nothing begun here changes nothing.
            progress.switched(true, "")
            verify(!progress.awaitingKeys)
            progress.close()
            tryVerify(() => !progress.visible)
        }

        // A management page: the title is always Keys, and nothing on it
        // chooses keys for an operation.
        function test_the_keys_page_chooses_nothing_for_an_operation() {
            compare(keys.header.title, "Keys")
            compare(findByName(keys, "clearRecipients"), null)
            const before = WorkSelection.revision
            const row = createTemporaryObject(keyRowComponent, keys)
            verify(row !== null)
            verify(row.recipientToggled === undefined)
            mouseClick(row, 20, row.height / 2)
            mouseClick(row, row.width / 2, row.height / 2)
            compare(WorkSelection.revision, before)
            compare(WorkSelection.recipientCount, 0)
        }

        // GnuPG's marks, from what of the secret is here.
        function test_a_key_is_marked_as_gnupg_marks_it() {
            compare(Wording.keyTag("none"), "pub")
            compare(Wording.keyTag("full"), "sec")
            compare(Wording.keyTag("partial"), "sec#")
            compare(Wording.keyTag("card"), "sec>")
            compare(Wording.keyTag(""), "")
            compare(Wording.subkeyTag(true, false), "ssb")
            compare(Wording.subkeyTag(false, false), "sub")
            compare(Wording.subkeyTag(true, true), "ssb>")
            const row = createTemporaryObject(keyRowComponent, keys)
            compare(findByName(row, "keyTag").text, "sec#")
            // Read out once, by the row, as something to activate; its parts
            // are not read out again one by one.
            compare(row.Accessible.role, Accessible.Button)
            verify(row.Accessible.description.indexOf("Encrypt, Sign") >= 0)
            for (const part of ["keyTitle", "keyTag", "keyAddress", "keyId",
                                "keyCapabilities"])
                verify(findByName(row, part).Accessible.ignored, part + " read out")
            verify(row.Accessible.description.indexOf("0123456789ABCDEF") >= 0)
            // A key group's internal id is not read out.
            const group = createTemporaryObject(keyRowComponent, keys,
                                                {"isKey": false, "keyId": "#&CE7238BD",
                                                 "email": "", "capabilities": ""})
            verify(group.Accessible.description.indexOf("Key group") >= 0)
            verify(group.Accessible.description.indexOf("#&") < 0,
                   "id read out: " + group.Accessible.description)
            // What it can do: GnuPG's letters, apart from the mark.
            const can = findByName(row, "keyCapabilities")
            verify(can.visible)
            compare(can.Accessible.name, "Encrypt, Sign")
            // The letters as quiet text, nothing that looks pressable.
            compare(can.text, "ES")
            compare(can.font.family, Theme.mono)
            verify(can.tone === undefined, "not a Chip")
            compare(Wording.capabilityWords("CESA"),
                    "Certify, Encrypt, Sign, Authenticate")
            // Once the key ID has a line of its own, the letters go beside
            // it and leave the name its room.
            verify(!findByName(row, "keyCapabilitiesBelow").visible)
            const face = findByName(row, "keyTitle").parent.parent.parent
            face.idBelow = true
            verify(!can.visible)
            verify(findByName(row, "keyCapabilitiesBelow").visible)
            // The key ID keeps its place however long the address is.
            verify(findByName(row, "keyId").visible)
            // The list shows the short key ID; it is read out whole.
            compare(findByName(row, "keyId").text, "89ABCDEF")
            // A key group shows people, from the icons every page uses.
            compare(findByName(group, "groupMark").symbol, "group")
        }

        function test_a_picker_takes_many_keys_and_keeps_those_already_in() {
            picker.begin("Add keys", ["AAAA", "BBBB"])
            verify(picker.alreadySet["AAAA"] === true)
            compare(picker.pickedCount, 0)
            picker.toggle("CCCC")
            picker.toggle("DDDD")
            picker.toggle("CCCC")
            compare(picker.pickedCount, 1)
            let got = null
            picker.accepted.connect(ids => got = ids)
            findByName(picker.contentItem, "keyPickerAccept").clicked()
            compare(got, ["DDDD"])
            // Begun again, nothing is ticked.
            picker.begin("Add keys", [])
            compare(picker.pickedCount, 0)
            picker.close()
        }

        // The rows, as the person taps them: one tap one change, the mark
        // following what the sheet holds; keys already in ticked and not to
        // be taken out; the mark never over the key's words; rows a touch
        // target tall.
        function test_a_picker_row_ticks_what_the_sheet_holds() {
            picker.begin("Add keys", ["AAAA"])
            tryVerify(() => picker.opened)
            const list = findByName(picker.contentItem, "keyPickerList")
            list.model = pickerKeys
            waitForRendering(list)
            const fixed = findByName(list, "keyPick_AAAA")
            const free = findByName(list, "keyPick_BBBB")
            verify(fixed && free)
            // The sheet grows to its new rows before they can be tapped.
            tryVerify(() => free.mapToItem(null, 0, free.height).y
                             <= list.mapToItem(null, 0, list.height).y + 0.5, 2000,
                      "the row is in sight")
            verify(fixed.checked && !fixed.enabled)
            verify(!free.checked)
            mouseClick(free)
            compare(picker.pickedCount, 1)
            verify(free.checked)
            mouseClick(free)
            compare(picker.pickedCount, 0)
            verify(!free.checked)
            mouseClick(fixed)
            compare(picker.pickedCount, 0)
            verify(free.height >= 48)
            // The mark at the end, in the room the face leaves it.
            const mark = free.indicator.x
            const room = free.contentItem.x + free.contentItem.width
                         - free.indicator.width - free.spacing
            verify(mark >= room - 0.5, "mark at " + mark + ", the face's words to " + room)
            verify(mark + free.indicator.width <= free.width - free.rightPadding + 0.5)
            compare(free.Accessible.name, "Bob")
            picker.close()
            tryVerify(() => !picker.visible)
            list.model = undefined
        }

        // A user ID is revoked for one of the two reasons OpenPGP gives it,
        // as the desktop offers them: none, or no longer valid (4, the
        // first choice); a subkey for one of its four.
        function test_a_user_id_is_revoked_for_its_own_reasons() {
            const sheet = createTemporaryObject(revokeComponent, keys)
            let sent = -1
            sheet.accepted.connect((item, reason, text) => sent = reason)
            sheet.beginForUid({"uid": "Alice <alice@example.org>"})
            tryVerify(() => sheet.opened)
            verify(findByName(sheet.contentItem, "revokeReason_0"))
            verify(findByName(sheet.contentItem, "revokeReason_4").checked)
            compare(findByName(sheet.contentItem, "revokeReason_3"), null)
            findByName(sheet.contentItem, "revokeRun").clicked()
            compare(sent, 4)

            sheet.begin({"keyId": "0123456789ABCDEF"})
            tryVerify(() => sheet.opened)
            verify(findByName(sheet.contentItem, "revokeReason_3").checked)
            compare(findByName(sheet.contentItem, "revokeReason_4"), null)
            sheet.close()
        }

        // A choice row is checkable (TalkBack says which is chosen), and a
        // tap never leaves it with a state of its own: it follows the sheet.
        function test_a_tapped_choice_still_follows_the_sheet() {
            const sheet = createTemporaryObject(revokeComponent, keys)
            sheet.begin({"keyId": "0123456789ABCDEF"})
            tryVerify(() => sheet.opened)
            const zero = findByName(sheet.contentItem, "revokeReason_0")
            const three = findByName(sheet.contentItem, "revokeReason_3")
            verify(zero.checkable)
            mouseClick(zero)
            verify(zero.checked)
            verify(!three.checked)
            sheet.reason = 3
            verify(!zero.checked, "the tapped row kept a state of its own")
            verify(three.checked)
            sheet.close()
        }

        // A note given to a key's row after it was made shows (a picker
        // learns which keys are already in after its rows are there).
        function test_a_note_given_later_shows() {
            const face = createTemporaryObject(faceComponent, keys)
            const note = findByName(face, "keyNote")
            verify(!note.visible)
            face.note = "Already in"
            verify(note.visible, "the note shows")
            face.note = ""
            verify(!note.visible)
        }

        // Before the keys are read, the page says it is reading them, never
        // that there are none; once read, an empty database says so.
        function test_an_empty_list_says_so_only_once_read() {
            var empty = findByName(keys, "emptyKeys")
            verify(empty !== null)
            // Still loading (nothing is ever read in these tests).
            keys.loading = true
            verify(!empty.visible)
            verify(findByName(keys, "keysLoading").running)
            keys.loading = false
            tryVerify(() => empty.visible)
            compare(empty.text, "No keys yet.")
            verify(empty.detail.indexOf("+") >= 0)
            // Nothing failed: nothing to try again.
            verify(!findByName(keys, "retryKeys").visible)
        }

        function test_a_search_with_no_match_says_what_was_searched() {
            keys.loading = false
            var search = findByName(keys, "keySearch")
            search.text = "nobody"
            var empty = findByName(keys, "emptyKeys")
            verify(empty.text.indexOf("nobody") >= 0)
            search.clear()
            verify(empty.text.indexOf("nobody") < 0)
        }
    }

    TestCase {
        name: "CategoryColours"

        // Any colour, typed as HEX: six digits, kept as the desktop keeps it.
        function test_a_hex_colour_is_read_and_kept_lowercase() {
            compare(categorySheet.parseHex("#2A82DA"), "#2a82da")
            compare(categorySheet.parseHex("2a82da"), "#2a82da")
            compare(categorySheet.parseHex(" #ABCDEF "), "#abcdef")
            compare(categorySheet.parseHex("#2A82D"), "")
            compare(categorySheet.parseHex("#GGGGGG"), "")
            compare(categorySheet.parseHex("#2A82DA00"), "")
        }
    }

    TestCase {
        name: "QuickKey"
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
            fakeMaker.made = []
            fakeMaker.busy = false
        }

        // Quick key shows no form: only the passphrase is asked (by the
        // engine), once however often it is tapped.
        function test_quick_key_shows_no_form_and_runs_once() {
            maker.quick()
            maker.quick()
            compare(fakeMaker.made.length, 1)
            verify(fakeMaker.made[0][0].length > 0)
            compare(fakeMaker.made[0][2], 0)
            compare(fakeMaker.made[0][3], true)
            verify(!maker.opened)
            verify(maker.working)
        }

        // A user ID is made as the desktop makes it: a name is required, an
        // email only if wanted, a comment too; what is wrong is said under
        // the field it is in, and a name not typed yet is not yet wrong.
        function test_a_new_key_s_user_id_follows_the_desktop() {
            maker.begin()
            const name = findByName(maker.contentItem, "generateName")
            const email = findByName(maker.contentItem, "generateEmail")
            const comment = findByName(maker.contentItem, "generateComment")
            const run = findByName(maker.contentItem, "generateRun")
            const nameProblem = findByName(maker.contentItem, "generateNameProblem")
            const emailProblem = findByName(maker.contentItem, "generateEmailProblem")
            const commentProblem = findByName(maker.contentItem, "generateCommentProblem")
            verify(comment !== null)
            verify(!run.enabled)
            compare(nameProblem.text, "")
            name.text = "Alice"
            verify(run.enabled)
            email.text = "not an address"
            verify(!run.enabled)
            verify(emailProblem.text.length > 0)
            email.text = "alice@example.org"
            verify(run.enabled)
            compare(emailProblem.text, "")
            comment.text = "work <x>"
            verify(!run.enabled)
            verify(commentProblem.text.length > 0)
            comment.text = "work"
            name.text = "Alice (A)"
            verify(!run.enabled)
            verify(nameProblem.text.length > 0)
            name.text = "Alice"
            verify(run.enabled)
            maker.close()
            tryVerify(() => !maker.visible)
        }

        // Quick key gives the key the comment of its made-up user ID.
        function test_quick_key_passes_the_comment_on() {
            fakeMaker.made = []
            maker.quick()
            compare(fakeMaker.made.length, 1)
            compare(typeof fakeMaker.made[0][4], "string")
            fakeMaker.busy = false
            maker.finished("generate", false, {"reason": "cancelled"})
        }

        // Add a user ID asks what a new key asks, by the same rule.
        function test_an_added_user_id_follows_the_desktop() {
            const sheet = createTemporaryObject(addUidComponent, this)
            sheet.begin()
            const name = findByName(sheet.contentItem, "uidName")
            const email = findByName(sheet.contentItem, "uidEmail")
            const comment = findByName(sheet.contentItem, "uidComment")
            const run = findByName(sheet.contentItem, "addUidRun")
            verify(!run.enabled)
            compare(findByName(sheet.contentItem, "uidNameProblem").text, "")
            name.text = "Alice"
            verify(run.enabled)
            email.text = "alice@"
            verify(!run.enabled)
            verify(findByName(sheet.contentItem, "uidEmailProblem").text.length > 0)
            email.text = ""
            comment.text = "a)b"
            verify(!run.enabled)
            verify(findByName(sheet.contentItem, "uidCommentProblem").text.length > 0)
            comment.text = "b"
            let got = []
            sheet.accepted.connect((n, e, c) => got = [n, e, c])
            mouseClick(run)
            compare(got, ["Alice", "", "b"])
        }

        // Cancelled: nothing said, nothing left open, nothing announced.
        function test_a_cancelled_quick_key_leaves_nothing() {
            let generated = 0
            let failed = 0
            const g = () => ++generated
            const f = () => ++failed
            maker.generated.connect(g)
            maker.quickFailed.connect(f)
            maker.quick()
            fakeMaker.busy = false
            maker.finished("generate", false, {"reason": "cancelled", "error": "Cancelled."})
            compare(generated, 0)
            compare(failed, 0)
            verify(!maker.opened)
            verify(!maker.working)
            // A failure is said; success is announced once.
            maker.quick()
            fakeMaker.busy = false
            maker.finished("generate", false, {"reason": "failed", "error": "No."})
            compare(failed, 1)
            maker.quick()
            fakeMaker.busy = false
            maker.finished("generate", true, {"fingerprint": "AB"})
            compare(generated, 1)
            maker.generated.disconnect(g)
            maker.quickFailed.disconnect(f)
        }
    }

    TestCase {
        name: "KeyDetailPage"

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

        function test_a_fingerprint_is_shown_whole_in_groups_of_four() {
            compare(Theme.fingerprintGroups("0123456789ABCDEF01234567"),
                    ["0123", "4567", "89AB", "CDEF", "0123", "4567"])
            // However long it is: a v6 key's 64 hex digits are 16 groups.
            compare(Theme.fingerprintGroups("A".repeat(64)).length, 16)
            compare(Theme.fingerprintGroups("0123 4567 89"), ["0123", "4567", "89"])
            compare(Theme.fingerprintGroups(""), [])
        }

        // One's own key can be backed up, to a file, after a warning; a key
        // with no secret part here has nothing to back up.
        function test_only_an_own_key_is_backed_up() {
            const row = findByName(detail, "backupSecretKey")
            verify(row !== null)
            // Shown, so that what is visible on it is.
            detail.visible = true
            detail.details = {"fingerprint": "AB".repeat(20), "hasSecret": false}
            verify(!row.visible)
            detail.details = {"fingerprint": "AB".repeat(20), "hasSecret": true}
            verify(row.visible)
            row.clicked()
            const ask = detail.backupQuestion
            tryVerify(() => ask.opened)
            verify(ask.message.indexOf("passphrase") >= 0)
            ask.close()
            tryVerify(() => !ask.visible)
            detail.details = {}
            detail.visible = false
        }

        function test_dates_read_one_way() {
            compare(Theme.date(new Date(2026, 10, 3)), "2026-11-03")
            compare(Theme.date(new Date(NaN)), "")
            compare(Theme.date(undefined), "")
        }

        // Each part of a key says its own algorithm and what it does: the
        // signing primary key never seems to be what encrypts.
        function test_a_key_s_parts_each_say_what_they_do() {
            const facts = createTemporaryObject(factsComponent, keys, {
                "key": {"keyId": "0123456789ABCDEF", "fingerprint": "AB".repeat(20),
                        "capabilities": "CES", "algorithm": "EdDSA"},
                "parts": [{"primary": true, "algorithmDetail": "ED25519",
                           "capabilities": "CS"},
                          {"primary": false, "algorithmDetail": "CV25519",
                           "capabilities": "E"},
                          {"primary": false, "algorithmDetail": "RSA2048",
                           "capabilities": "E", "revoked": true}]})
            compare(findByName(facts, "keyCan").value, "Certify, Encrypt, Sign")
            const parts = []
            for (const child of facts.children) {
                if (child.objectName === "keyPart") parts.push([child.label, child.value])
            }
            compare(parts, [["Primary key", "ED25519 · CS"],
                            ["Subkey", "CV25519 · E"],
                            ["Subkey", "RSA2048 · E · Revoked"]])
            // Before the parts are read: the primary key alone, said as such.
            facts.parts = []
            const alone = []
            for (const child of facts.children) {
                if (child.objectName === "keyPart") alone.push([child.label, child.value])
            }
            compare(alone, [["Primary key", "EdDSA"]])
        }

        // A fingerprint in GnuPG's rows of five groups where they fit, fewer
        // in even rows where they do not; copied whole all the same.
        function test_a_fingerprint_is_laid_out_in_even_rows() {
            const view = createTemporaryObject(fingerprintComponent, keys,
                                               {"width": 380, "fingerprint": "AB12".repeat(10)})
            compare(view.groups.length, 10)
            compare(view.perRow, 5)
            view.width = 150
            // Never four: a v4 key's last row would hold two.
            verify(view.perRow === 2 || view.perRow === 1)
            view.fingerprint = "CD34".repeat(16)
            // A v6 key in two rows of eight on a phone, in a smaller type,
            // never below the label size; at the text's own size where a
            // wide screen has room.
            view.width = 380
            compare(view.perRow, 8)
            verify(view.groupSize < Theme.bodySize && view.groupSize >= Theme.labelSize)
            view.width = 700
            compare(view.perRow, 8)
            compare(view.groupSize, Theme.bodySize)
            // No room even so: the text's own size, fewer a row.
            view.width = 200
            compare(view.groupSize, Theme.bodySize)
            verify(view.perRow < 8)
            // A v4 key is never made smaller.
            view.fingerprint = "AB12".repeat(10)
            view.width = 380
            compare(view.groupSize, Theme.bodySize)
            // Whatever the width, the groups and the copy button beside
            // them stay inside it.
            const copy = findByName(view, "copyFingerprint")
            for (const w of [160, 220, 260, 300, 340, 380]) {
                for (const fpr of ["AB12".repeat(10), "CD34".repeat(16)]) {
                    view.fingerprint = fpr
                    view.width = w
                    waitForRendering(view)
                    verify(copy.mapToItem(view, copy.width, 0).x <= w + 0.5,
                           "copy button out at " + w + " (" + view.perRow + " per row)")
                }
            }
            // At the right edge, apart from the groups, whatever their count.
            view.width = 380
            waitForRendering(view)
            fuzzyCompare(copy.mapToItem(view, copy.width, 0).x, 380, 0.5)
        }

        // Copying says so for a moment, and the saying goes by itself (it
        // stayed for good before).
        function test_copying_a_fingerprint_says_so_for_a_moment() {
            const view = createTemporaryObject(fingerprintComponent, keys,
                                               {"width": 380, "fingerprint": "AB12".repeat(10)})
            const copy = findByName(view, "copyFingerprint")
            const said = copy.resources.find(o => o.objectName === "fingerprintCopied")
            verify(said, "the copied tip")
            verify(!said.visible)
            copy.clicked()
            tryVerify(() => said.visible)
            tryVerify(() => !said.visible, 3000)
        }

        function test_the_key_s_state_and_delete_apart() {
            detail.key = {"name": "Alice", "fingerprint": "AAAA".repeat(10),
                          "status": "ok", "canEncrypt": true, "usable": true,
                          "expires": new Date(NaN)}
            const flow = findByName(detail, "fingerprint")
            verify(flow !== null)
            verify(findByName(detail, "deleteKey") !== null)
            // The expiry is said once, as a fact; the state line only for
            // what must be seen at once.
            verify(!findByName(detail, "keyState").visible)
            // Key management chooses nothing for an operation.
            compare(findByName(detail, "toggleRecipient"), null)
            compare(findByName(detail, "useForSigning"), null)
        }
    }

    TestCase {
        name: "Subkeys"
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

        function subkey(primary, revoked) {
            return {"index": primary ? 0 : 1, "primary": primary, "revoked": revoked,
                    "keyId": "0123456789ABCDEF", "fingerprint": "AB".repeat(20),
                    "usage": {"encrypt": !primary, "certify": primary},
                    "expires": new Date(NaN)}
        }

        function test_a_subkey_offers_what_its_key_allows() {
            const all = {"setExpiration": true, "revokeSubkey": true, "deleteSubkey": true}
            subkeySheet.begin(subkey(false, false), all)
            tryVerify(() => subkeySheet.opened)
            compare(subkeySheet.title, "Subkey 89ABCDEF")
            verify(findByName(subkeySheet.contentItem, "subkeyExpiration").visible)
            verify(findByName(subkeySheet.contentItem, "subkeyRevoke").visible)
            verify(findByName(subkeySheet.contentItem, "subkeyDelete").visible)
            // Its expiry is a fact, not a second line at the top.
            verify(!findByName(subkeySheet.contentItem, "subkeyState").visible)
            subkeySheet.close()

            // The primary key is the whole key's: not revoked or deleted here.
            subkeySheet.begin(subkey(true, false), all)
            compare(subkeySheet.title, "Primary key")
            verify(!findByName(subkeySheet.contentItem, "subkeyRevoke").visible)
            verify(!findByName(subkeySheet.contentItem, "subkeyDelete").visible)
            subkeySheet.close()

            // Revoked, it is not revoked again nor given a new date.
            subkeySheet.begin(subkey(false, true), all)
            verify(!findByName(subkeySheet.contentItem, "subkeyRevoke").visible)
            verify(!findByName(subkeySheet.contentItem, "subkeyExpiration").visible)
            compare(findByName(subkeySheet.contentItem, "subkeyState").text, "Revoked")
            subkeySheet.close()

            // A key that is not one's own: only to look at.
            subkeySheet.begin(subkey(false, false), {})
            verify(!findByName(subkeySheet.contentItem, "subkeyExpiration").visible)
            verify(!findByName(subkeySheet.contentItem, "subkeyDelete").visible)
            subkeySheet.close()
        }

        // The new key's sheet comes where Create was: a second tap on
        // Create does not close it before it is up.
        function test_the_new_key_sheet_takes_taps_once_up() {
            generated.key = {"name": "New", "fingerprint": "AB".repeat(20),
                             "secretState": "full"}
            generated.open()
            const done = findByName(generated.contentItem, "generatedDone")
            verify(!generated.opened)
            verify(!done.enabled)
            tryVerify(() => generated.opened)
            verify(done.enabled)
            done.clicked()
            tryVerify(() => !generated.visible)
        }

        function test_a_new_subkey_starts_from_sensible_choices() {
            let got = null
            addSubkey.accepted.connect(function(purpose, validity, passphrase) {
                got = [purpose, validity, passphrase]
            })
            addSubkey.begin()
            tryVerify(() => addSubkey.opened)
            // The duration is shown, not an empty box.
            compare(findByName(addSubkey.contentItem, "subkeyValidity").currentText, "2 years")
            const run = findByName(addSubkey.contentItem, "addSubkeyRun")
            run.clicked()
            compare(got, ["encrypt", "2y", true])

            // It stays while the key is changed, and cannot be asked twice.
            verify(addSubkey.opened)
            addSubkey.busy = true
            verify(!run.enabled)
            // A wrong passphrase: said in the sheet, as it was filled in,
            // for another try.
            addSubkey.busy = false
            addSubkey.settle(false, {"reason": "bad_passphrase",
                                     "error": "Incorrect passphrase. Please try again."})
            verify(addSubkey.opened)
            const problem = findByName(addSubkey.contentItem, "addSubkeyProblem")
            verify(problem.visible)
            compare(problem.text, "Incorrect passphrase. Please try again.")
            verify(run.enabled)
            // Tried again from the same button, asked for the passphrase anew.
            compare(run.text, "Try again")
            // Another choice: the old failure no longer applies.
            addSubkey.changed()
            verify(!problem.visible)
            compare(run.text, "Add")
            addSubkey.settle(false, {"reason": "bad_passphrase",
                                     "error": "Incorrect passphrase. Please try again."})
            // Any other failure is said as it is, not as a passphrase one.
            addSubkey.settle(false, {"reason": "failed", "error": "Not supported."})
            compare(problem.text, "Not supported.")
            compare(run.text, "Add")
            // Cancelled at the passphrase: nothing went wrong.
            addSubkey.settle(false, {"reason": "cancelled", "error": "Cancelled."})
            verify(!problem.visible)
            verify(addSubkey.opened)
            // Done: it goes.
            addSubkey.settle(true, {})
            tryVerify(() => !addSubkey.opened)
        }

        function test_a_user_id_offers_what_its_key_allows() {
            const all = {"setPrimaryUid": true, "revokeUid": true, "deleteUid": true}
            const other = {"uid": "Al <al@example.org>", "name": "Al",
                           "email": "al@example.org", "primary": false}
            uidSheet.begin(other, all)
            tryVerify(() => uidSheet.opened)
            verify(findByName(uidSheet.contentItem, "uidPrimary").visible)
            verify(findByName(uidSheet.contentItem, "uidRevoke").visible)
            verify(findByName(uidSheet.contentItem, "uidDelete").visible)
            uidSheet.close()

            // The primary one is the key's name: only replaced.
            uidSheet.begin(Object.assign({}, other, {"primary": true}), all)
            verify(!findByName(uidSheet.contentItem, "uidPrimary").visible)
            verify(!findByName(uidSheet.contentItem, "uidRevoke").visible)
            verify(!findByName(uidSheet.contentItem, "uidDelete").visible)
            uidSheet.close()

            // Revoked: neither made primary nor revoked again.
            uidSheet.begin(Object.assign({}, other, {"revoked": true}), all)
            verify(!findByName(uidSheet.contentItem, "uidPrimary").visible)
            verify(!findByName(uidSheet.contentItem, "uidRevoke").visible)
            compare(findByName(uidSheet.contentItem, "uidState").text, "Revoked")
            uidSheet.close()
        }
    }
}

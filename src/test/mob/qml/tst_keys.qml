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

    KeysPage {
        id: keys
        anchors.fill: parent
    }

    KeyDetailPage {
        id: detail
        visible: false
    }

    // The key maker as Quick key sees it, run by hand.
    QtObject {
        id: fakeMaker
        property bool busy: false
        property var profiles: []
        property var validities: []
        property var made: []
        function reloadProfiles() {}
        function generate(name, email, profile, protect) {
            made = made.concat([[name, email, profile, protect]])
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
        id: fingerprintComponent
        FingerprintView {
        }
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
        }

        // Switching databases: the name at the top is the one whose keys are
        // listed, whatever the sheet is doing; a switch that fails ends at
        // once. A switch the sheet did not begin is not waited for.
        function test_the_database_name_changes_with_its_list() {
            const sheet = keys.databaseSheet
            const before = keys.databaseName
            sheet.hold("Elsewhere")
            verify(sheet.switching)
            verify(sheet.busy)
            compare(sheet.targetName, "Elsewhere")
            compare(keys.databaseName, before)

            sheet.switched(false, "It is gone.")
            verify(!sheet.switching)
            verify(!sheet.busy)

            sheet.switched(true, "")
            verify(!sheet.awaitingKeys)
            verify(!sheet.busy)
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
            // What it can do: GnuPG's letters, apart from the mark.
            const can = findByName(row, "keyCapabilities")
            verify(can.visible)
            compare(can.Accessible.name, "Encrypt, Sign")
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
            compare(findByName(row, "keyId").text, "0123456789ABCDEF")
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
            verify(view.perRow < 5)
            verify(view.perRow >= 1)
            view.fingerprint = "CD34".repeat(16)
            view.width = 380
            compare(view.perRow, 4)
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

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

    SubkeySheet {
        id: subkeySheet
    }

    UidSheet {
        id: uidSheet
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

        // Categories: the "+" chip makes one; a picker chooses many keys.
        function test_a_category_is_made_from_the_plus_chip() {
            let plus = null
            tryVerify(() => (plus = findByName(keys, "categoryChip_new")) !== null)
            compare(plus.Accessible.name, "New category")
            verify(findByName(keys, "categoryChip_edit") === null)
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

        function test_an_empty_list_says_so() {
            var empty = findByName(keys, "emptyKeys")
            verify(empty !== null)
            verify(empty.visible)
            compare(empty.text, "No keys yet.")
        }

        function test_a_search_with_no_match_says_what_was_searched() {
            var search = findByName(keys, "keySearch")
            search.text = "nobody"
            var empty = findByName(keys, "emptyKeys")
            verify(empty.text.indexOf("nobody") >= 0)
            // The way out of a search that found nothing.
            var clear = findByName(keys, "clearSearch")
            verify(clear.visible)
            clear.clicked()
            compare(search.text, "")
            verify(!clear.visible)
            verify(empty.text.indexOf("nobody") < 0)
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

        function test_the_key_s_state_and_delete_apart() {
            detail.key = {"name": "Alice", "fingerprint": "AAAA".repeat(10),
                          "status": "ok", "canEncrypt": true, "usable": true,
                          "expires": new Date(NaN)}
            const flow = findByName(detail, "fingerprint")
            verify(flow !== null)
            verify(findByName(detail, "deleteKey") !== null)
            compare(findByName(detail, "keyState").text, "Valid, does not expire")
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
            compare(findByName(subkeySheet.contentItem, "subkeyState").text, "Does not expire")
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

        function test_a_new_subkey_starts_from_sensible_choices() {
            let got = null
            addSubkey.accepted.connect(function(purpose, validity, passphrase) {
                got = [purpose, validity, passphrase]
            })
            addSubkey.begin()
            tryVerify(() => addSubkey.opened)
            // The duration is shown, not an empty box.
            compare(findByName(addSubkey.contentItem, "subkeyValidity").currentText, "2 years")
            findByName(addSubkey.contentItem, "addSubkeyRun").clicked()
            compare(got, ["encrypt", "2y", true])
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

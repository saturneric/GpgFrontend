// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtTest
import GpgFrontend.Mobile

Item {
    width: 400
    height: 800

    KeysPage {
        id: keys
        anchors.fill: parent
    }

    KeyDetailPage {
        id: detail
        visible: false
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

        function test_a_fingerprint_is_shown_in_groups_of_four() {
            compare(detail.formatFingerprint("0123456789ABCDEF01234567"),
                    "0123 4567 89AB CDEF 0123 4567")
            compare(detail.formatFingerprint(""), "")
        }

        function test_a_key_that_does_not_expire_says_never() {
            compare(detail.formatDate(new Date(NaN)), "Never")
            compare(detail.formatDate(undefined), "Never")
        }
    }
}

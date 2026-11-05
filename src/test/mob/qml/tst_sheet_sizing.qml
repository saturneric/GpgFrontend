// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtTest
import GpgFrontend.Mobile

// How tall a bottom sheet is, as the person sees it: as tall as what is in
// it while that is short, never taller than the room there is, its buttons
// always inside it. A list in a sheet asks for its rows' height and scrolls
// once there are more than fit.
Item {
    id: root

    width: 400
    height: 800

    // Content of a height the test sets.
    BottomSheet {
        id: growing

        title: "Growing"
        description: "A line under the title."

        Rectangle {
            id: block

            Layout.fillWidth: true
            implicitHeight: 100
            color: "transparent"
        }

        footer: [
            Item { Layout.fillWidth: true },
            Button { id: growingCancel; text: "Cancel"; flat: true },
            Button { id: growingOk; text: "OK"; highlighted: true }
        ]
    }

    // A list that fills the sheet and scrolls itself, as the key pickers'.
    BottomSheet {
        id: listed

        title: "Listed"
        expandable: true
        listMode: true

        ListView {
            id: rows

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: contentHeight + bottomMargin
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            bottomMargin: Theme.gapSmall
            model: 0
            delegate: ItemDelegate {
                width: ListView.view.width
                text: "Row " + index
            }
        }

        footer: [
            Item { Layout.fillWidth: true },
            Button { id: listedDone; text: "Done"; highlighted: true }
        ]
    }

    // Buttons with long words: one under the other, each inside the sheet.
    BottomSheet {
        id: wordy

        title: "Wordy"

        Label { text: "Some content" }

        footer: [
            ActionButton { id: wordyFirst; kind: "text"
                           text: "Aus einer Datei auf diesem Gerät hinzufügen" },
            ActionButton { id: wordySecond
                           text: "Schlüssel jetzt erzeugen und sofort verwenden" }
        ]
    }

    // The key toolbox, with a long list of keys put in place of the core's
    // (the tests have none).
    KeyToolbox {
        id: box
    }

    ListModel {
        id: manyKeys

        Component.onCompleted: {
            for (let i = 0; i < 200; ++i) {
                append({"name": "Key " + i, "email": "key" + i + "@example.org",
                        "keyId": ("000000000000" + i).slice(-16),
                        "fingerprint": ("0000000000000000000000000000000000000000"
                                        + i).slice(-40),
                        "secretState": "", "capabilities": "ESC",
                        "hasSecret": false, "canEncrypt": true, "canSign": false,
                        "usable": true, "status": "ok", "statusText": "",
                        "isKey": true, "members": []})
            }
        }
    }

    TestCase {
        name: "SheetSizing"
        when: windowShown

        // Where @p item ends, in the window.
        function bottomOf(item) {
            return item.mapToItem(root, 0, item.height).y
        }

        // @p item lies within @p sheet, from side to side and top to foot.
        function inside(item, sheet) {
            const at = item.mapToItem(root, 0, 0)
            return at.x >= -0.5 && at.x + item.width <= root.width + 0.5
                && at.y >= sheet.y - 0.5 && at.y + item.height <= sheet.y + sheet.height + 0.5
        }

        function settled(sheet) {
            tryVerify(() => sheet.opened)
            // A change reaches the layouts at the next polish, and only then
            // what the sheet asks for; its height then animates to it.
            waitForPolish(sheet.contentItem.Window.window)
            waitForPolish(sheet.contentItem.Window.window)
            // The animation ends on its target exactly; anything short of it
            // is its last frames (one run in about forty caught 244.001).
            tryVerify(() => sheet.height === sheet.restingHeight, 2000,
                      "height " + sheet.height + " resting " + sheet.restingHeight)
        }

        function cleanup() {
            growing.close(); listed.close(); wordy.close(); box.close()
            tryVerify(() => !growing.visible && !listed.visible && !wordy.visible
                            && !box.visible)
            block.implicitHeight = 100
            rows.model = 0
            box.keyList.model = undefined
            WorkSelection.reset()
        }

        // ---- Plain content

        function test_short_content_makes_a_short_sheet() {
            growing.open()
            settled(growing)
            compare(growing.height, growing.naturalHeight)
            verify(growing.height < root.height / 2, growing.height)
            verify(inside(growingOk, growing))
        }

        // The content grows while the sheet is up: the sheet follows, and
        // shrinks again when it shrinks.
        function test_the_sheet_follows_its_content() {
            growing.open()
            settled(growing)
            const before = growing.height
            // Layouts take a change in at the next polish, and the height
            // then animates to it.
            block.implicitHeight = 300
            tryVerify(() => Math.abs(growing.height - (before + 200)) < 1, 2000,
                      "grown to " + growing.height)
            block.implicitHeight = 50
            tryVerify(() => Math.abs(growing.height - (before - 50)) < 1, 2000,
                      "shrunk to " + growing.height)
            verify(inside(growingOk, growing))
        }

        // More than the window holds: as tall as there is room, the content
        // scrolls, and the buttons stay inside the sheet.
        function test_long_content_is_capped_and_scrolls() {
            block.implicitHeight = 2000
            growing.open()
            settled(growing)
            fuzzyCompare(growing.height, growing.fullHeight, 1)
            verify(inside(growingOk, growing))
            verify(inside(growingCancel, growing))
            // The content's column is in the scroller's content item.
            const scroller = block.parent.parent.parent
            verify(scroller.interactive, "the content scrolls")
        }

        // ---- A list in the sheet

        function test_list_sheet_height_follows_its_rows_data() {
            return [
                {"tag": "empty", "rows": 0, "half": false},
                {"tag": "one", "rows": 1, "half": false},
                {"tag": "five", "rows": 5, "half": false},
                {"tag": "thousand", "rows": 1000, "half": true}
            ]
        }

        function test_list_sheet_height_follows_its_rows(data) {
            rows.model = data.rows
            listed.open()
            settled(listed)
            if (data.half) {
                // Half the window, the rest in the list, which scrolls.
                fuzzyCompare(listed.height, listed.halfHeight, 1)
                verify(rows.contentHeight > rows.height)
            } else {
                // Its rows' height, no more: no room left empty.
                compare(listed.height, listed.naturalHeight)
                verify(listed.height < listed.halfHeight)
                verify(rows.height >= rows.contentHeight)
            }
            verify(inside(listedDone, listed))
        }

        // Rows come and go while it is up (a search narrows the list).
        function test_list_sheet_follows_a_narrowing_list() {
            rows.model = 1000
            listed.open()
            settled(listed)
            const half = listed.height
            rows.model = 2
            tryVerify(() => listed.height < half - 100, 2000, "height " + listed.height)
            settled(listed)
            compare(listed.height, listed.naturalHeight)
            verify(inside(listedDone, listed))
        }

        // ---- Buttons at the foot

        // Words longer than a row of buttons holds: the buttons go one under
        // the other, as wide as the sheet, and inside it.
        function test_long_buttons_stack_inside_the_sheet() {
            wordy.width = 360
            wordy.open()
            settled(wordy)
            verify(wordyFirst.y !== wordySecond.y, "stacked")
            verify(inside(wordyFirst, wordy))
            verify(inside(wordySecond, wordy))
            verify(wordyFirst.width <= wordy.width)
        }

        // ---- The keys for an encryption, and one key in full

        function openToolboxOnManyKeys() {
            box.begin("encrypt", {"op": "encrypt", "signs": false,
                                  "label": "Encrypt now", "run": () => {}})
            box.keyList.model = manyKeys
            settled(box)
        }

        // A long list of keys: half the window.
        function test_the_key_list_takes_half_the_window() {
            openToolboxOnManyKeys()
            verify(box.canExpand)
            fuzzyCompare(box.height, box.halfHeight, 1)
        }

        // One key in full, whose facts are short: a sheet as tall as they
        // are, not grown to all of the window because the list before it
        // was long.
        function test_a_short_key_view_is_not_expanded() {
            openToolboxOnManyKeys()
            box.infoKey = manyKeys.get(3)
            settled(box)
            compare(box.height, box.naturalHeight)
            verify(!box.expanded, "expanded for short content")
        }

        // Back to the list: the list as it was before the key was shown.
        function test_back_to_the_list_restores_its_height() {
            openToolboxOnManyKeys()
            verify(!box.expanded)
            box.infoKey = manyKeys.get(3)
            settled(box)
            box.back()
            compare(box.infoKey, null)
            settled(box)
            verify(!box.expanded)
            fuzzyCompare(box.height, box.halfHeight, 1)
        }

        // A list the person expanded is still expanded when they come back
        // to it from a key.
        function test_an_expanded_list_stays_expanded_after_a_key() {
            openToolboxOnManyKeys()
            box.expanded = true
            settled(box)
            box.infoKey = manyKeys.get(3)
            settled(box)
            compare(box.height, box.naturalHeight)
            box.back()
            settled(box)
            verify(box.expanded)
            fuzzyCompare(box.height, box.fullHeight, 1)
        }
    }
}

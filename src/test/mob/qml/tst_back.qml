// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtTest
import GpgFrontend.Mobile

// Back, one step at a time, as Android's Back asks: inside a sheet first
// (a key shown in full, a search, the step before), then the sheet, and
// only then whatever is under it. (The pages' own order, MainPage.back(),
// needs a loaded profile and is not tested here.)
Item {
    id: root

    width: 400
    height: 800

    // The sheets every working page shares.
    SheetHost {
        id: sheets

        anchors.fill: parent
    }

    // A page's own sheet, as the Keys page has several.
    BottomSheet {
        id: pageSheet
        title: "A page's sheet"
    }

    // One that asks what only the person may answer, as the passphrase does.
    BottomSheet {
        id: askingSheet
        title: "Asking"
        keepOpen: true
    }

    TestCase {
        name: "Back"
        when: windowShown

        function cleanup() {
            sheets.closeAll()
            tryVerify(() => !sheets.sheetOpen)
            WorkSelection.reset()
        }

        function openDeep() {
            const box = sheets.toolbox
            sheets.openKeys("encrypt", {"op": "encrypt", "signs": true,
                                        "label": "Encrypt and sign now",
                                        "run": () => {}})
            tryVerify(() => box.opened)
            box.step = 1
            box.search.text = "alice"
            box.infoKey = {"name": "Alice", "email": "alice@example.org",
                           "keyId": "0123456789ABCDEF", "fingerprint": ""}
            return box
        }

        // The keys for an encryption that also signs, deep inside: the
        // signing step, a search typed, one key shown in full. The system's
        // Back key undoes them one by one, then closes the sheet.
        function test_the_back_key_unwinds_the_key_sheet_step_by_step() {
            const box = openDeep()
            keyClick(Qt.Key_Back)
            compare(box.infoKey, null)
            keyClick(Qt.Key_Back)
            compare(box.search.text, "")
            keyClick(Qt.Key_Back)
            compare(box.step, 0)
            verify(box.opened)
            keyClick(Qt.Key_Back)
            tryVerify(() => !box.visible)
        }

        // The pages ask the host first (MainPage.back()): it takes one step
        // in whichever sheet is open, and says so; with none open it leaves
        // Back to the page.
        function test_the_host_takes_back_while_a_sheet_is_open() {
            openDeep()
            let steps = 0
            while (sheets.back()) {
                ++steps
                verify(steps < 10, "back never ends")
                tryVerify(() => sheets.toolbox.opened || !sheets.toolbox.visible)
            }
            compare(steps, 4)
            tryVerify(() => !sheets.sheetOpen)
            verify(!sheets.back())
        }

        // Any sheet up is known (Theme.openSheets), a page's own too: Back,
        // asked of the page (MainPage.back()), goes to the one on top, never
        // to the page under it -- even when the sheet lost the focus. A sheet
        // at work is not let go.
        function test_back_goes_to_the_sheet_on_top() {
            pageSheet.open()
            tryVerify(() => pageSheet.opened)
            const open = Theme.openSheets
            compare(open[open.length - 1], pageSheet)
            pageSheet.interactive = false
            open[open.length - 1].back()
            verify(pageSheet.opened)
            pageSheet.interactive = true
            open[open.length - 1].back()
            tryVerify(() => !pageSheet.visible)
            verify(Theme.openSheets.indexOf(pageSheet) < 0)
        }

        // Closing every sheet for the application's own reasons (a share
        // arriving, another key database) leaves one that asks the person,
        // and one at work; a plain one goes.
        function test_closing_all_leaves_a_question_open() {
            pageSheet.open()
            askingSheet.open()
            tryVerify(() => pageSheet.opened && askingSheet.opened)
            verify(Theme.closeOpenSheets())
            tryVerify(() => !pageSheet.visible)
            verify(askingSheet.opened)
            askingSheet.close()
            tryVerify(() => !askingSheet.visible)

            pageSheet.open()
            tryVerify(() => pageSheet.opened)
            pageSheet.interactive = false
            verify(Theme.closeOpenSheets())
            verify(pageSheet.opened)
            pageSheet.interactive = true
            verify(!Theme.closeOpenSheets())
            tryVerify(() => !pageSheet.visible)
        }

        // Escape, as a hardware keyboard sends it, is Back too.
        function test_escape_is_back() {
            const box = openDeep()
            keyClick(Qt.Key_Escape)
            compare(box.infoKey, null)
            verify(box.opened)
        }
    }
}

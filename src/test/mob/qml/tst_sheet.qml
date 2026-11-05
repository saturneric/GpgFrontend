import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtTest
import GpgFrontend.Mobile

// The bottom sheet every sheet is: one owner of dragging (its header), its
// height its content's within the room there is, and Back one step at a
// time, the topmost sheet first.
Item {
    id: root
    width: 400
    height: 800

    // Something on the page with the focus, as a page has before a sheet
    // opens over it.
    TextField {
        id: pageField
        objectName: "pageField"
        width: 200
    }

    BottomSheet {
        id: small
        title: "Short"
        Label { text: "One line" }
    }

    BottomSheet {
        id: tall
        title: "Tall"
        expandable: true
        listMode: true

        ListView {
            id: rows
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: contentHeight
            clip: true
            model: 200
            delegate: ItemDelegate { width: ListView.view.width; text: "Row " + index }
        }

        footer: [
            Button {
                id: headerTapProbe
                property int taps: 0
                text: "Go"
                onClicked: taps++
            }
        ]
    }

    BottomSheet {
        id: over
        title: "Over"
        Label { text: "On top" }
    }

    ImportSheet {
        id: importer
    }

    // Puts white space on the clipboard: nothing to import, whatever an
    // earlier test copied.
    TextEdit {
        id: clipboardSetter
        visible: false
        text: " \n "
    }

    TestCase {
        name: "BottomSheet"
        when: windowShown

        function cleanup() {
            over.close(); tall.close(); small.close()
            tryVerify(() => !over.visible && !tall.visible && !small.visible)
        }

        // Little in it, little of the window: not a half-window sheet.
        function test_a_short_sheet_is_as_tall_as_its_content() {
            small.open()
            tryVerify(() => small.opened)
            verify(small.height < root.height * 0.4, small.height)
            compare(small.height, small.naturalHeight)
        }

        // A long list: half the window first, nearly all of it expanded; the
        // list scrolls inside, the sheet stays where it is.
        function test_a_tall_sheet_has_two_heights_and_its_list_scrolls() {
            tall.open()
            tryVerify(() => tall.opened)
            tryCompare(tall, "height", tall.halfHeight)
            const y = tall.y
            mouseDrag(rows, rows.width / 2, rows.height - 20, 0, -200)
            tryVerify(() => rows.contentY > 0)
            compare(tall.y, y)
            tall.expanded = true
            tryCompare(tall, "height", tall.fullHeight)
            // Back collapses it first, then closes it.
            tall.back()
            verify(!tall.expanded)
            tall.back()
            tryVerify(() => !tall.visible)
        }

        // Dragging the header down far enough lets the sheet go.
        function test_dragging_the_header_down_closes_it() {
            small.open()
            tryVerify(() => small.opened)
            const top = small.contentItem.mapToItem(root, root.width / 2, 8)
            mouseDrag(root, top.x, top.y, 0, small.height * 0.8, Qt.LeftButton,
                      Qt.NoModifier, 50)
            tryVerify(() => !small.visible)
        }

        // A tap is a tap, not a drag.
        function test_a_tap_in_the_sheet_reaches_its_button() {
            tall.open()
            tryVerify(() => tall.opened)
            const before = headerTapProbe.taps
            mouseClick(headerTapProbe)
            compare(headerTapProbe.taps, before + 1)
            verify(tall.opened)
        }

        // Back goes to the topmost sheet only; the one under it stays.
        function test_back_closes_the_topmost_sheet_only() {
            tall.open()
            tryVerify(() => tall.opened)
            over.open()
            tryVerify(() => over.opened)
            keyClick(Qt.Key_Back)
            tryVerify(() => !over.visible)
            verify(tall.opened)
        }

        // One sheet closing as another opens (a menu sheet opening the
        // next): the new one keeps the focus Back comes through, though the
        // closing one hands it back to what had it before once it is gone.
        function test_a_sheet_opened_as_another_closes_keeps_the_focus() {
            pageField.forceActiveFocus()
            verify(pageField.activeFocus)
            small.open()
            tryVerify(() => small.opened)
            small.close()
            over.open()
            tryVerify(() => over.opened && !small.visible)
            wait(400)
            const focused = root.Window.activeFocusItem
            let inside = false
            for (let it = focused; it; it = it.parent)
                if (it === over.contentItem) inside = true
            verify(inside, "focus is in " + focused)
            keyClick(Qt.Key_Back)
            tryVerify(() => !over.visible)
        }

        // A busy sheet is not let go.
        function test_a_busy_sheet_stays() {
            small.interactive = false
            small.open()
            tryVerify(() => small.opened)
            small.back()
            mouseClick(root, root.width / 2, 10)
            wait(300)
            verify(small.opened)
            small.interactive = true
        }

        // Importing keys: two direct ways in, no field to paste into and no
        // Import button; an empty clipboard says so instead of doing nothing.
        function test_keys_are_imported_by_a_tap() {
            function find(item, name) {
                if (item.objectName === name) return item
                for (let i = 0; i < item.children.length; ++i) {
                    const found = find(item.children[i], name)
                    if (found) return found
                }
                return null
            }
            importer.begin()
            tryVerify(() => importer.opened)
            verify(find(importer.contentItem, "importFromFile") !== null)
            const clip = find(importer.contentItem, "importFromClipboard")
            verify(clip !== null && clip.enabled)
            compare(find(importer.contentItem, "importText"), null)
            compare(find(importer.contentItem, "importRun"), null)
            clipboardSetter.selectAll()
            clipboardSetter.copy()
            clip.clicked()
            const outcome = find(importer.contentItem, "importOutcome")
            verify(outcome.visible)
            verify(outcome.text.indexOf("clipboard") >= 0)
            importer.close()
            tryVerify(() => !importer.visible)
        }
    }
}

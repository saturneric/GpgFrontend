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

    // A form: fields and a button, as the passphrase or Add a user ID.
    BottomSheet {
        id: form
        title: "Form"
        TextField { objectName: "formField"; Layout.fillWidth: true; text: "a" }
        TextField { Layout.fillWidth: true; text: "b" }
        footer: [ Button { text: "OK" } ]
    }

    // One that must be answered: not pulled away.
    BottomSheet {
        id: kept
        title: "Kept"
        dragToClose: false
        Label { text: "Answer me" }
    }

    // A list taller than half the window, shorter than all of it.
    BottomSheet {
        id: middling
        title: "Middling"
        expandable: true
        listMode: true

        ListView {
            id: middlingRows
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: contentHeight
            clip: true
            model: 13
            delegate: ItemDelegate { width: ListView.view.width; text: "Row " + index }
        }
    }

    // Short at first, more once open all the way (the status sheet).
    BottomSheet {
        id: revealing
        title: "Revealing"
        expandable: true
        revealsMore: true
        Label { text: "Short" }
        Rectangle {
            visible: revealing.expanded
            Layout.fillWidth: true
            Layout.preferredHeight: 900
        }
    }

    BottomSheet {
        id: over
        title: "Over"
        Label { text: "On top" }
    }

    ImportSheet {
        id: importer
    }

    // Buttons of each kind: one made primary, one that becomes primary.
    Column {
        ActionButton { id: madePrimary; kind: "primary"; text: "Save as" }
        ActionButton { id: madeSecondary; kind: "secondary"; text: "Share" }
        ActionButton { id: turnsPrimary; kind: "secondary"; text: "Save as" }
    }

    // Puts white space on the clipboard: nothing to import, whatever an
    // earlier test copied.
    TextEdit {
        id: clipboardSetter
        visible: false
        text: " \n "
    }

    // A key group's name, email and comment.
    KeyGroupSheet {
        id: groupSheet
        groups: KeyGroupsController {}
    }

    // A sheet whose first item is a text field, as Rename key group.
    BottomSheet {
        id: named
        title: "Rename key group"
        TextField {
            objectName: "namedField"
            Layout.fillWidth: true
            placeholderText: "Name, for example Project team"
            text: "Team"
        }
    }

    TestCase {
        name: "BottomSheet"
        when: windowShown

        function cleanup() {
            over.close(); tall.close(); small.close()
            form.close(); kept.close(); middling.close(); revealing.close()
            tryVerify(() => !over.visible && !tall.visible && !small.visible
                      && !form.visible && !kept.visible && !middling.visible
                      && !revealing.visible)
        }

        // Pulls @p sheet's header by @p dy (down positive), in steps, and
        // holds it there; returns where it was taken.
        function pull(sheet, dy) {
            const grab = sheet.contentItem.mapToItem(root, root.width / 2, 8)
            mousePress(root, grab.x, grab.y)
            for (let i = 1; i <= 10; ++i) mouseMove(root, grab.x, grab.y + dy * i / 10, 16)
            return grab
        }

        function handleShown(sheet) {
            const handle = findChild(sheet.contentItem, "sheetHandle")
            return handle.children[0].visible
        }

        // Pulled down, a form moves down whole: its fields and its button
        // keep their size and place in it, never squeezed out; let go short
        // of closing, it goes back.
        function test_a_form_pulled_down_moves_and_keeps_its_shape() {
            form.open()
            tryVerify(() => form.opened)
            const height = form.height
            const field = findChild(form.contentItem, "formField")
            const fieldHeight = field.height
            const grab = pull(form, height * 0.3)
            compare(form.height, height)
            verify(form.offset > height * 0.2, "moved " + form.offset)
            compare(field.height, fieldHeight)
            mouseRelease(root, grab.x, grab.y + height * 0.3)
            wait(300)
            verify(form.visible, "closed when let go short")
            compare(form.offset, 0)
            compare(form.height, height)
        }

        // Let go of, a sheet goes on from the height it had under the
        // finger to where it rests, in one movement: never first back to
        // where it rested before, then on.
        function test_letting_go_never_jumps() {
            tall.open()
            tryVerify(() => tall.opened)
            tryCompare(tall, "height", tall.halfHeight)
            // Pulled a little up, short of where it would be held open: it
            // goes back to its half height from there.
            const grab = tall.contentItem.mapToItem(root, root.width / 2, 8)
            mousePress(root, grab.x, grab.y)
            for (let i = 1; i <= 10; ++i) mouseMove(root, grab.x, grab.y - 120 * i / 10, 16)
            const held = tall.height
            verify(held > tall.halfHeight + 60, "pulled to " + held)
            mouseRelease(root, grab.x, grab.y - 120)
            const heights = [tall.height]
            for (let i = 0; i < 20; ++i) {
                wait(16)
                heights.push(tall.height)
            }
            // From where it was let go of, never from elsewhere first.
            verify(Math.abs(heights[0] - held) < 1, heights[0] + " after " + held)
            // And only one way: down to where it stays, or held there.
            for (let i = 1; i < heights.length; ++i)
                verify(heights[i] <= heights[i - 1] + 0.5, "back up at step " + i + ": " + heights)
            const rest = tall.height
            verify(rest >= tall.halfHeight - 0.5)

            // Pulled most of the way: it opens all the way, again from the
            // finger's height.
            const grab2 = tall.contentItem.mapToItem(root, root.width / 2, 8)
            mousePress(root, grab2.x, grab2.y)
            const up = tall.fullHeight - tall.height - 20
            for (let i = 1; i <= 10; ++i) mouseMove(root, grab2.x, grab2.y - up * i / 10, 16)
            const pulled = tall.height
            mouseRelease(root, grab2.x, grab2.y - up)
            verify(Math.abs(tall.height - pulled) < 1, tall.height + " after " + pulled)
            tryCompare(tall, "height", tall.fullHeight)
            verify(tall.expanded)
        }

        // A sheet that must be answered stays where it is, whatever the pull.
        function test_a_sheet_that_must_be_answered_is_not_pulled_away() {
            kept.open()
            tryVerify(() => kept.opened)
            const y = kept.y
            const grab = pull(kept, kept.height * 0.9)
            compare(kept.offset, 0)
            compare(kept.y, y)
            mouseRelease(root, grab.x, grab.y + kept.height * 0.9)
            wait(300)
            verify(kept.opened)
        }

        // A list taller than half the window but not than all of it: pulled
        // to the top it stays as tall as its rows, never higher and back.
        function test_a_list_pulled_up_does_not_snap_back() {
            middling.open()
            tryVerify(() => middling.opened)
            verify(middling.canExpand, "natural " + middling.naturalHeight)
            verify(middling.naturalHeight < middling.fullHeight)
            const grab = pull(middling, -root.height)
            verify(middling.height <= middling.naturalHeight + 0.5,
                   middling.height + " over " + middling.naturalHeight)
            mouseRelease(root, grab.x, grab.y - root.height)
            wait(300)
            fuzzyCompare(middling.height, middling.naturalHeight, 0.5)
            verify(middling.expanded)
        }

        // A sheet short now but more once open (the status sheet) is
        // pulled open; its handle says it can be.
        function test_a_sheet_that_shows_more_is_pulled_open() {
            revealing.open()
            tryVerify(() => revealing.opened)
            verify(!revealing.expanded)
            verify(revealing.canExpand)
            verify(handleShown(revealing))
            const grab = pull(revealing, -200)
            mouseRelease(root, grab.x, grab.y - 200)
            tryVerify(() => revealing.expanded)
            tryCompare(revealing, "height", revealing.fullHeight)
        }

        // The handle only where pulling does more than close: not on a
        // short sheet, not on a busy one.
        function test_the_handle_is_shown_only_where_it_acts() {
            small.open()
            tryVerify(() => small.opened)
            verify(!handleShown(small))
            tall.open()
            tryVerify(() => tall.opened)
            verify(handleShown(tall))
            tall.interactive = false
            verify(!handleShown(tall))
            tall.interactive = true
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

        // Dragging the header up: the sheet follows the finger and stays
        // where it is let go of, also with the finger lifting off moving a
        // little down; dragged a little down from there it stays open too.
        function test_a_sheet_dragged_up_stays_where_it_is_let_go() {
            tall.open()
            tryVerify(() => tall.opened)
            tryCompare(tall, "height", tall.halfHeight)
            const grab = tall.contentItem.mapToItem(root, root.width / 2, 8)
            const up = (tall.fullHeight - tall.halfHeight) / 2
            mousePress(root, grab.x, grab.y)
            for (let i = 1; i <= 10; ++i) {
                mouseMove(root, grab.x, grab.y - up * i / 10, 16)
            }
            // The finger lifting off: a small move down.
            mouseMove(root, grab.x, grab.y - up + 6, 8)
            mouseRelease(root, grab.x, grab.y - up + 6)
            wait(300)
            verify(tall.visible, "closed after a drag up")
            verify(tall.height > tall.halfHeight + up * 0.6,
                   "height " + tall.height + " after a drag of " + up)
            verify(tall.height < tall.fullHeight)

            // A short drag down from there: still open.
            const grab2 = tall.contentItem.mapToItem(root, root.width / 2, 8)
            mouseDrag(root, grab2.x, grab2.y, 0, 60, Qt.LeftButton, Qt.NoModifier, 50)
            wait(300)
            verify(tall.visible, "closed after a short drag down")

            // Back brings it to its half height first, then closes it.
            tall.back()
            tryCompare(tall, "height", tall.halfHeight)
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

        // A button whose kind changes looks as one made of that kind: a
        // secondary's tint is let go of, never left behind as a pale fill
        // that reads as disabled (Save as, once a file is known temporary).
        function test_a_button_that_changes_kind_looks_like_its_kind() {
            verify(!Qt.colorEqual(madePrimary.background.color,
                                  madeSecondary.background.color))
            turnsPrimary.kind = "primary"
            verify(Qt.colorEqual(turnsPrimary.background.color,
                                 madePrimary.background.color),
                   turnsPrimary.background.color + " vs " + madePrimary.background.color)
            turnsPrimary.kind = "secondary"
            verify(Qt.colorEqual(turnsPrimary.background.color,
                                 madeSecondary.background.color))
        }

        // A text field first in a sheet keeps its floating label whole: the
        // content's clip leaves room above the field, and the title's gap
        // is no larger for it.
        function test_a_field_first_in_a_sheet_keeps_its_label() {
            named.open()
            tryVerify(() => named.opened)
            const field = findChild(named.contentItem, "namedField")
            verify(field !== null)
            let scroller = field.parent
            while (scroller && scroller.contentY === undefined) scroller = scroller.parent
            verify(scroller !== null)
            // The field's top is at least the label's rise inside the clip.
            const top = field.mapToItem(scroller, 0, 0).y
            verify(top >= 8, "field top " + top)
            verify(!scroller.interactive, "nothing to scroll")
            named.close()
            tryVerify(() => !named.visible)
        }

        // A key group takes a name, an email and a comment, as on the
        // desktop; what is wrong shows under the field it is about.
        function test_a_key_group_has_an_email_and_a_comment() {
            groupSheet.begin({"id": "", "name": "Project team",
                              "email": "team@example.org", "comment": "phones"})
            tryVerify(() => groupSheet.opened)
            const c = groupSheet.contentItem
            compare(findChild(c, "keyGroupName").text, "Project team")
            compare(findChild(c, "keyGroupEmail").text, "team@example.org")
            compare(findChild(c, "keyGroupComment").text, "phones")

            findChild(c, "keyGroupEmail").text = "not an email"
            groupSheet.save()
            verify(findChild(c, "keyGroupEmailProblem").text.length > 0)
            compare(findChild(c, "keyGroupNameProblem").text, "")

            findChild(c, "keyGroupName").text = "Tea"
            groupSheet.save()
            verify(findChild(c, "keyGroupNameProblem").text.length > 0)
            compare(findChild(c, "keyGroupEmailProblem").text, "")
            verify(groupSheet.opened)
            groupSheet.close()
            tryVerify(() => !groupSheet.visible)
        }
    }
}

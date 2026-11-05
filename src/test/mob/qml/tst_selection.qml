// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtTest
import GpgFrontend.Mobile

// What the style's own controls do when the application's state, not the
// control, decides what is chosen -- the way every choice in the sheets
// works -- and how a menu leaves out an entry. What the pages rely on when
// they use these controls as they are.
Item {
    id: root

    width: 400
    height: 800

    // The application's state: which ids are chosen.
    QtObject {
        id: chosen

        property var ids: []
        property int changes: 0

        function has(id) { return ids.indexOf(id) >= 0 }
        function toggle(id) {
            ids = has(id) ? ids.filter(i => i !== id) : ids.concat([id])
            ++changes
        }
    }

    Column {
        width: parent.width

        // Controlled: the state decides, a tap asks the state.
        CheckDelegate {
            id: controlled

            width: parent.width
            text: "Controlled"
            checkable: false
            checked: chosen.has("a")
            onClicked: chosen.toggle("a")
        }

        // Left to toggle itself as well.
        CheckDelegate {
            id: selfToggling

            width: parent.width
            text: "Self-toggling"
            checked: chosen.has("b")
            onToggled: chosen.toggle("b")
        }

        RadioDelegate {
            id: radioOne

            width: parent.width
            text: "One"
            checkable: false
            checked: chosen.ids.length > 0 && chosen.ids[0] === "one"
            onClicked: chosen.ids = ["one"]
        }

        RadioDelegate {
            id: radioTwo

            width: parent.width
            text: "Two"
            checkable: false
            checked: chosen.ids.length > 0 && chosen.ids[0] === "two"
            onClicked: chosen.ids = ["two"]
        }
    }

    // Rows of a list: plain, chosen, and one that only says something.
    Column {
        y: 300
        width: 400

        ListRow { id: plainRow; width: parent.width; text: "Plain" }
        ListRow { id: chosenRow; width: parent.width; text: "Chosen"; selected: true }
        ListRow { id: inertRow; width: parent.width; text: "Inert"; inert: true }
    }

    // The floating buttons, each kind.
    Item {
        width: 200
        height: 200
        y: 500

        Fab {
            id: addFab

            text: "Add a key"
            icon.source: Theme.icon("add")
        }

        Fab {
            id: pgpFab

            offsetX: 100
            text: "PGP operations"
            icon.source: Theme.icon("lock")
        }
    }

    // An icon in a bar, at the style's own size.
    BarButton {
        id: barButton

        y: 400
        text: "Search"
        icon.source: Theme.icon("search")
    }

    Menu {
        id: menu

        MenuItem { text: "First" }
        MenuItem { id: middle; text: "Middle" }
        MenuItem { text: "Last" }
    }

    TestCase {
        name: "Selection"
        when: windowShown

        function init() {
            chosen.ids = []
            chosen.changes = 0
        }

        // A tap changes the state once, and the control shows the state;
        // a second tap takes it back. Nothing toggles twice.
        function test_a_controlled_check_follows_the_state() {
            mouseClick(controlled)
            compare(chosen.changes, 1)
            verify(chosen.has("a"))
            verify(controlled.checked)
            mouseClick(controlled)
            compare(chosen.changes, 2)
            verify(!chosen.has("a"))
            verify(!controlled.checked)
            // The state changed elsewhere: the control still follows it.
            chosen.toggle("a")
            verify(controlled.checked)
        }

        // From the keyboard, the same.
        function test_a_controlled_check_answers_the_keyboard() {
            controlled.forceActiveFocus()
            keyClick(Qt.Key_Space)
            compare(chosen.changes, 1)
            verify(controlled.checked)
        }

        // A control that toggles itself keeps its binding to the state
        // (Qt 6.10): one change per tap, and it follows the state after.
        // Either way the state stays the one source of what is chosen.
        function test_a_self_toggling_check_still_follows_the_state() {
            mouseClick(selfToggling)
            compare(chosen.changes, 1)
            verify(selfToggling.checked)
            chosen.toggle("b")
            verify(!chosen.has("b"))
            verify(!selfToggling.checked, "stale: shows a choice the state no longer has")
        }

        // One of a few: the state keeps them exclusive, with no group.
        function test_controlled_radios_are_exclusive_by_the_state() {
            mouseClick(radioOne)
            verify(radioOne.checked && !radioTwo.checked)
            mouseClick(radioTwo)
            verify(!radioOne.checked && radioTwo.checked)
            // Tapping the chosen one again keeps it chosen.
            mouseClick(radioTwo)
            verify(radioTwo.checked)
        }

        // An entry hidden in a menu keeps its room: the menu is as tall as
        // before. Why a menu that leaves entries out (OperationMenu) gives
        // them no height as well.
        // Both floating buttons round, an icon alone, their name read out
        // and said while held.
        function test_the_floating_buttons_are_round_icons() {
            for (const fab of [addFab, pgpFab]) {
                compare(fab.width, Theme.fabSize)
                compare(fab.height, Theme.fabSize)
                compare(fab.radius, Theme.fabSize / 2)
                compare(fab.display, AbstractButton.IconOnly)
                compare(fab.Accessible.name, fab.text)
                compare(fab.ToolTip.text, fab.text)
                // One shadow, its own: the style's layer (a second, faint
                // shadow and an offscreen pass) is off.
                compare(fab.background.layer.enabled, false)
            }
        }

        // A button that cannot be used casts no shadow.
        function test_a_disabled_floating_button_casts_no_shadow() {
            const shadow = addFab.children.find(c => c.blur !== undefined)
            verify(shadow, "the shadow")
            verify(shadow.visible)
            addFab.enabled = false
            verify(!shadow.visible)
            addFab.enabled = true
        }

        // A row keeps the style's own press and focus feedback (its ripple);
        // a chosen row adds the brand's tint over it; a row that only says
        // something shows neither.
        function test_list_rows_keep_the_styles_feedback() {
            const ripple = row => row.background.children.find(c => c.pressed !== undefined)
            const tint = row => row.background.children.find(
                c => c.objectName === "listRowSelected")
            verify(ripple(plainRow), "the style's ripple")
            verify(plainRow.background.visible)
            verify(!tint(plainRow).visible)
            verify(ripple(chosenRow))
            verify(tint(chosenRow).visible)
            verify(!inertRow.background.visible)
        }

        // A touch target the platform asks for, from the style alone.
        function test_a_bar_button_is_a_touch_target() {
            compare(barButton.width, 48)
            compare(barButton.height, 48)
        }

        function test_a_hidden_menu_entry_keeps_its_room() {
            menu.open()
            tryVerify(() => menu.opened)
            const all = menu.implicitHeight
            middle.visible = false
            wait(100)
            compare(menu.implicitHeight, all)
            middle.height = 0
            tryVerify(() => menu.implicitHeight < all, 1000,
                      "menu " + menu.implicitHeight + " of " + all)
            middle.height = undefined
            middle.visible = true
            menu.close()
        }
    }
}

// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtTest
import GpgFrontend.Mobile

// Every sheet's buttons say all of what they do, on a narrow phone, in
// English and in German, at the usual text size and at twice it: no button's
// words cut short, no button past the sheet's edge.
Item {
    id: root

    // A narrow phone.
    width: 360
    height: 640

    // The sheets that come up on their own, with nothing given to them.
    // (GenerateSheet reads the key generation profiles, which need a loaded
    // profile, and cannot come up here.)
    readonly property var sheets: [
        "AddUidSheet", "DetachedVerifySheet", "GeneratedKeySheet",
        "KeyGroupSheet", "ProfileSheet"
    ]

    // The profile's PIN, asked for after a few wrong ones.
    // A page's top bar, with an action beside its title.
    AppBar {
        id: appBar

        width: root.width
        title: qsTr("Keys")

        BarButton {
            text: "Search"
            icon.source: Theme.icon("search")
        }
    }

    PinSheet {
        id: pin
    }

    // A user ID's row: its chips at the end, one of them hidden, as a
    // primary user ID that is not revoked has them.
    Column {
        width: root.width

        ListRow {
            id: chipRow
            width: parent.width
            text: "A rather long user ID name that wraps on a narrow phone"
            detail: "someone@example.org"
            Chip { id: primaryChip; text: "Primary" }
            Chip { id: revokedChip; text: "Revoked"; visible: false }
        }

        ListRow {
            id: switchRow
            width: parent.width
            text: "With a control"
            Switch {}
        }
    }

    // Forcing a held profile open, or resetting its key.
    ConfirmSheet {
        id: confirm
    }

    TestCase {
        name: "Labels"
        when: windowShown

        // Everything under @p item, depth first.
        function descendants(item, out) {
            for (let i = 0; i < item.children.length; ++i) {
                const child = item.children[i]
                out.push(child)
                descendants(child, out)
            }
            return out
        }

        // A push button with words (not a row, not an icon alone).
        function isWordButton(item) {
            return item.flat !== undefined && item.highlighted !== undefined
                && typeof item.clicked === "function" && item.text !== undefined
                && item.text.length > 0 && item.display !== AbstractButton.IconOnly
        }

        // Why @p button's words do not all show, or "" when they do.
        function cut(button, sheet) {
            const at = button.mapToItem(root, 0, 0)
            if (at.x < -0.5 || at.x + button.width > root.width + 0.5)
                return "past the edge (" + Math.round(at.x) + " + "
                       + Math.round(button.width) + ")"
            // ActionButton: its words wrap in a Label of their own.
            for (const child of descendants(button.contentItem, [button.contentItem])) {
                if (child.truncated !== undefined && child.text === button.text)
                    return child.truncated ? "cut" : ""
            }
            // The style's button: its words on one line, elided when longer
            // than the room beside the icon.
            metrics.font = button.font
            metrics.text = button.text
            const icon = button.icon.source.toString().length > 0
                         ? button.icon.width + button.spacing : 0
            return metrics.advanceWidth > button.availableWidth - icon + 0.5
                   ? "cut (" + Math.ceil(metrics.advanceWidth) + " > "
                     + Math.floor(button.availableWidth - icon) + ")"
                   : ""
        }

        TextMetrics {
            id: metrics
        }

        // A badge at a row's end keeps the row's margin, as the words at
        // its start do; two do not touch. A control brings its own inset.
        function test_a_rows_badges_keep_its_margin() {
            compare(chipRow.rightPadding, Theme.gap)
            const end = primaryChip.mapToItem(chipRow, primaryChip.width, 0).x
            fuzzyCompare(end, chipRow.width - Theme.gap, 0.5)
            revokedChip.visible = true
            const gap = () => revokedChip.mapToItem(chipRow, 0, 0).x
                              - primaryChip.mapToItem(chipRow, primaryChip.width, 0).x
            tryVerify(() => gap() >= Theme.gapSmall - 0.5, 1000, "chips " + gap() + " apart")
            fuzzyCompare(revokedChip.mapToItem(chipRow, revokedChip.width, 0).x,
                         chipRow.width - Theme.gap, 0.5)
            revokedChip.visible = false
            compare(switchRow.rightPadding, 4)
        }

        function cleanup() {
            Theme.fontScale = 1
            TestSupport.useLanguage("en_US")
        }

        function test_buttons_say_all_they_do_data() {
            const rows = []
            for (const language of ["en_US", "de_DE"]) {
                for (const scale of [1, 2]) {
                    for (const sheet of root.sheets)
                        rows.push({"tag": sheet + " " + language + " x" + scale,
                                   "sheet": sheet, "language": language,
                                   "scale": scale})
                }
            }
            return rows
        }

        function test_buttons_say_all_they_do(data) {
            TestSupport.useLanguage(data.language)
            Theme.fontScale = data.scale
            const component = Qt.createComponent(
                "qrc:/qt/qml/GpgFrontend/Mobile/qml/components/" + data.sheet + ".qml")
            compare(component.status, Component.Ready, component.errorString())
            const sheet = component.createObject(root)
            verify(sheet, "created")
            // What the window's font does on the phone (Shell).
            if (data.scale > 1) sheet.font.pixelSize = Theme.secondarySize
            sheet.open()
            tryVerify(() => sheet.opened)
            wait(50)

            const problems = []
            let seen = 0
            for (const item of descendants(sheet.contentItem, [])) {
                if (!item.visible || !isWordButton(item)) continue
                ++seen
                const why = cut(item, sheet)
                if (why.length > 0) problems.push("\"" + item.text + "\" " + why)
            }
            sheet.close()
            tryVerify(() => !sheet.visible)
            sheet.destroy()

            verify(seen > 0, "no button found")
            compare(problems.join("; "), "")
        }

        function test_a_top_bar_holds_its_title_data() {
            return [{"tag": "en x1", "language": "en_US", "scale": 1},
                    {"tag": "en x2", "language": "en_US", "scale": 2},
                    {"tag": "de x1", "language": "de_DE", "scale": 1},
                    {"tag": "de x2", "language": "de_DE", "scale": 2}]
        }

        // The title inside its bar at any text size: a bar of fixed height
        // let a large "Schlüssel" run into the search field under it. At the
        // usual size the bar keeps the style's height.
        function test_a_top_bar_holds_its_title(data) {
            TestSupport.useLanguage(data.language)
            Theme.fontScale = data.scale
            appBar.title = qsTr("Keys")
            waitForPolish(appBar)
            const title = descendants(appBar, []).find(
                i => i.text === appBar.title && i.truncated !== undefined)
            verify(title, "the title")
            // The title's row takes the change in at its next polish.
            tryVerify(() => title.mapToItem(appBar, 0, 0).y >= -0.5
                             && title.mapToItem(appBar, 0, 0).y + title.contentHeight
                                <= appBar.height + 0.5, 1000,
                      "title to " + (title.mapToItem(appBar, 0, 0).y + title.contentHeight)
                      + " in a bar of " + appBar.height)
            if (data.scale === 1)
                tryCompare(appBar, "height", appBar.implicitBackgroundHeight, 1000)
        }

        function test_the_profile_questions_say_all_they_do_data() {
            const rows = []
            for (const question of ["pin", "forceUnlock", "keyReset"]) {
                for (const language of ["en_US", "de_DE"]) {
                    for (const scale of [1, 2])
                        rows.push({"tag": question + " " + language + " x" + scale,
                                   "question": question, "language": language,
                                   "scale": scale})
                }
            }
            return rows
        }

        // The questions opening the profile asks, in their dialogs: the PIN
        // after a few wrong ones (Forgot PIN, Quit, Unlock), and forcing
        // open or resetting (the safe choice and the other).
        function test_the_profile_questions_say_all_they_do(data) {
            TestSupport.useLanguage(data.language)
            Theme.fontScale = data.scale
            const dialog = data.question === "pin" ? pin : confirm
            if (data.scale > 1) dialog.font.pixelSize = Theme.secondarySize
            if (data.question === "pin") TestSupport.askPin(false, 3)
            else TestSupport.askConfirmation(data.question === "forceUnlock")
            tryVerify(() => dialog.opened, 3000)
            wait(50)
            const problems = []
            let seen = 0
            for (const item of descendants(dialog.contentItem, [])) {
                if (!item.visible || !isWordButton(item)) continue
                ++seen
                const why = cut(item, dialog)
                if (why.length > 0) problems.push("\"" + item.text + "\" " + why)
            }
            const safe = descendants(dialog.contentItem, []).find(
                i => i.objectName === (data.question === "pin" ? "pinQuit" : "safeChoice"))
            mouseClick(safe)
            tryVerify(() => !dialog.visible)
            dialog.font.pixelSize = undefined
            compare(seen, data.question === "pin" ? 3 : 2)
            compare(problems.join("; "), "")
        }
    }
}

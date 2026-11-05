// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtTest
import GpgFrontend.Mobile

// Where a sheet's content sits across it: words in line with the title, a
// list's rows from one side of the sheet to the other, nothing past its
// edges, also with a camera cutout at the side (landscape).
Item {
    id: root

    width: 400
    height: 800

    // A form: everything kept from the sides.
    BottomSheet {
        id: form

        title: "Form"

        Label {
            id: formText

            Layout.fillWidth: true
            text: "Words"
        }
    }

    // One list of rows, edge to edge, and a line of words above it.
    BottomSheet {
        id: list

        title: "List"
        inset: 0

        Label {
            id: listText

            Layout.fillWidth: true
            Layout.leftMargin: Theme.sheetPad
            Layout.rightMargin: Theme.sheetPad
            text: "Words"
        }

        ListRow {
            id: listRow

            Layout.fillWidth: true
            leftPadding: Theme.sheetPad
            text: "A row"
        }
    }

    // A real one: how long a key stays valid.
    ValiditySheet {
        id: validity

        choices: [{"id": "1y", "name": "1 year"}, {"id": "2y", "name": "2 years"},
                  {"id": "never", "name": "Never"}]
    }

    TestCase {
        name: "SheetInset"
        when: windowShown

        function leftOf(item) {
            return item.mapToItem(root, 0, 0).x
        }

        function rightOf(item) {
            return item.mapToItem(root, item.width, 0).x
        }

        // Every row of @p sheet's content: its left and right edges.
        function rows(sheet) {
            const out = []
            const walk = item => {
                for (let i = 0; i < item.children.length; ++i) {
                    const child = item.children[i]
                    if (child.visible && child.highlighted !== undefined
                        && child.checked !== undefined && child.width > 0)
                        out.push(child)
                    walk(child)
                }
            }
            walk(sheet.body)
            return out
        }

        function settle(sheet) {
            sheet.open()
            tryVerify(() => sheet.opened)
            wait(50)
        }

        function cleanup() {
            form.close(); list.close(); validity.close()
            tryVerify(() => !form.visible && !list.visible && !validity.visible)
            Theme.safeLeft = 0
            Theme.safeRight = 0
        }

        function test_a_forms_words_are_in_line_with_its_title() {
            settle(form)
            fuzzyCompare(leftOf(formText), Theme.sheetPad, 0.5)
            fuzzyCompare(rightOf(formText), root.width - Theme.sheetPad, 0.5)
        }

        // A list's rows run from side to side; their words, and the line
        // above them, still start where the title does.
        function test_a_lists_rows_run_edge_to_edge() {
            settle(list)
            fuzzyCompare(leftOf(listRow), 0, 0.5)
            fuzzyCompare(rightOf(listRow), root.width, 0.5)
            fuzzyCompare(leftOf(listRow.contentItem), Theme.sheetPad, 0.5)
            fuzzyCompare(leftOf(listText), Theme.sheetPad, 0.5)
        }

        function test_a_real_list_sheet_is_edge_to_edge_data() {
            return [{"tag": "portrait", "side": 0},
                    {"tag": "a cutout at the side", "side": 48}]
        }

        // The validity choices, with nothing at the sides and with a cutout
        // there: from side to side of what the sheet may use, and no
        // further.
        function test_a_real_list_sheet_is_edge_to_edge(data) {
            Theme.safeLeft = data.side
            Theme.safeRight = data.side
            validity.begin("Valid for", "")
            tryVerify(() => validity.opened)
            wait(50)
            const found = rows(validity)
            verify(found.length >= 3, "rows: " + found.length)
            for (const row of found) {
                fuzzyCompare(leftOf(row), data.side, 0.5)
                fuzzyCompare(rightOf(row), root.width - data.side, 0.5)
                fuzzyCompare(leftOf(row.contentItem), data.side + Theme.sheetPad, 0.5)
            }
        }
    }
}

// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// What is chosen in a list of keys, in one row above it: the same in the
// key toolbox and in the pickers that fill a key group or a category. Always
// there and always as tall, whether none, one or many are chosen, so the
// list under it never moves as keys are ticked: a title, one line under it
// (empty, a warning or a problem), and what may be done at its end.
ItemDelegate {
    id: summary

    // Under the title: why something is held up, or empty.
    property string detail: ""
    property color detailColor: Material.secondaryTextColor
    // A mark at the start: none, "check_circle", "warning".
    property string symbol: ""
    property color symbolColor: Material.accentColor
    // At the end: an action, or a mark that it opens something.
    default property alias trailing: trailingRow.data

    Layout.fillWidth: true
    Layout.leftMargin: -Theme.sheetPad
    Layout.rightMargin: -Theme.sheetPad
    leftPadding: Theme.sheetPad
    rightPadding: 8
    topPadding: 4
    bottomPadding: 4
    // Only a summary that opens something is pressed and takes the focus.
    focusPolicy: enabled ? Qt.StrongFocus : Qt.NoFocus
    background.visible: enabled
    Accessible.description: detail

    contentItem: RowLayout {
        spacing: 8

        // Its room kept with no mark, so the words never move.
        Item {
            Layout.preferredWidth: 20
            Layout.preferredHeight: 20

            Icon {
                anchors.fill: parent
                visible: summary.symbol.length > 0
                symbol: summary.symbol
                color: summary.symbolColor
                sourceSize: Qt.size(20, 20)
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.preferredWidth: 0
            spacing: 0

            Label {
                objectName: "summaryTitle"
                Layout.fillWidth: true
                text: summary.text
                elide: Text.ElideRight
                font.pixelSize: Theme.secondarySize
                font.weight: Font.Medium
            }

            // One line, always: an empty one keeps its height.
            Label {
                objectName: "summaryDetail"
                Layout.fillWidth: true
                text: summary.detail
                maximumLineCount: 1
                elide: Text.ElideRight
                font.pixelSize: Theme.labelSize
                color: summary.detailColor
            }
        }

        RowLayout {
            id: trailingRow
            spacing: 0
        }
    }
}

// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// What is chosen in a list of keys, in one compact row above it: the same
// in the key toolbox and in the pickers that fill a key group or a category.
// Always there and always as tall, whether none, one or many are chosen, so
// the list under it never moves as keys are ticked: a mark, the title, a
// short tag when something is wrong (the whole sentence is said where there
// is room for it, and read out), and what may be done at its end.
//
// It takes a full touch target's height, drawn 40 tall, and gives back the
// rest to the gaps of the column it sits in.
ItemDelegate {
    id: summary

    // What is wrong, in a few words at the end of the row; empty for none.
    property string tag: ""
    property color tagColor: Theme.danger
    // The whole sentence, read out with the row.
    property string detail: ""
    // A mark at the start: none, "check_circle", "warning", "info".
    property string symbol: ""
    property color symbolColor: Material.accentColor
    // At the end: an action, or a mark that it opens something.
    default property alias trailing: trailingRow.data

    Layout.fillWidth: true
    Layout.leftMargin: -Theme.sheetPad
    Layout.rightMargin: -Theme.sheetPad
    // The touch target overlaps the column's gaps, not its neighbours.
    Layout.topMargin: -6
    Layout.bottomMargin: -6
    leftPadding: Theme.sheetPad
    rightPadding: 8
    topPadding: 0
    bottomPadding: 0
    topInset: 4
    bottomInset: 4
    implicitHeight: Math.max(Theme.touch, implicitContentHeight + 8)
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

        Label {
            objectName: "summaryTitle"
            Layout.fillWidth: true
            Layout.preferredWidth: 0
            text: summary.text
            elide: Text.ElideRight
            font.pixelSize: Theme.secondarySize
            font.weight: Font.Medium
        }

        Label {
            objectName: "summaryTag"
            visible: text.length > 0
            // Never more than half the row: the title keeps its room.
            Layout.maximumWidth: summary.availableWidth / 2
            text: summary.tag
            elide: Text.ElideRight
            font.pixelSize: Theme.labelSize
            font.weight: Font.Medium
            color: summary.tagColor
        }

        RowLayout {
            id: trailingRow
            spacing: 0
        }
    }
}

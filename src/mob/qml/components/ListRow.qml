// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// One row of a list, as the platform lays them out: an optional icon, the
// text and a line under it, and at the end a short value or a control. The
// text wraps rather than hides; when the value leaves the text too little
// room, it goes under the text instead.
ItemDelegate {
    id: row

    // One of the line icons in qml/icons, by name.
    property string symbol
    // A picture in its own colours in place of the symbol (an operation's
    // desktop icon), not tinted.
    property url image: ""
    property color symbolColor: Material.secondaryTextColor
    // The icon's size, and whether it stays centred beside two lines of
    // text (a file's type) rather than level with the first (a setting).
    property int symbolSize: 24
    property bool symbolCentered: false
    property string detail
    property color detailColor: Material.secondaryTextColor
    property int detailLines: 0
    // A short value at the end ("10 minutes"), if any.
    property string valueText
    // Marked as chosen (a selected row), with its tint.
    property bool selected: false
    // Only says something: not pressed, not focused, read as text.
    property bool inert: false
    // One line, shortened in the middle: a file's name keeps its extension.
    property bool singleLineTitle: false
    // A control at the end (a switch, a menu button), if any.
    default property alias trailing: trailingRow.data
    // A control at the start (a check box), if any, before the icon.
    property alias leading: leadingRow.data

    // Whether the value has room beside the text.
    readonly property bool valueBeside: valueText.length === 0
        || valueMetrics.advanceWidth <= availableWidth * 0.4

    // At the end: a control (a switch, a menu button) brings its own inset,
    // so the row keeps little room; words or marks (a chip, a chevron) keep
    // the row's own margin, aligned down the list. Counted by what shows: a
    // row whose chips are all hidden keeps its margin.
    readonly property bool trailingControl: {
        for (const child of trailingRow.visibleChildren) {
            if (typeof child.clicked === "function") return true
        }
        return false
    }
    leftPadding: leadingRow.children.length > 0 ? 4 : Theme.gap
    rightPadding: trailingControl ? 4 : Theme.gap
    topPadding: 8
    bottomPadding: 8
    implicitHeight: Math.max(detail.length > 0 ? Theme.rowHeightTwoLine
                                               : Theme.rowHeight,
                             implicitContentHeight + topPadding + bottomPadding)
    font.pixelSize: Theme.bodySize
    focusPolicy: inert ? Qt.NoFocus : Qt.StrongFocus
    Accessible.role: inert ? Accessible.StaticText : Accessible.ListItem
    Accessible.name: text
    Accessible.description: detail
    // A chosen row says so, as its tint shows it.
    Accessible.selected: row.selected

    TextMetrics {
        id: valueMetrics
        font.pixelSize: Theme.secondarySize
        text: row.valueText
    }

    // The style's own background: its ripple when pressed, its focus. None
    // for a row that only says something.
    background.visible: !inert

    // A chosen row in the brand's tint, laid over the style's background
    // (translucent), so its ripple and focus still show through.
    Rectangle {
        objectName: "listRowSelected"
        parent: row.background
        anchors.fill: parent
        visible: row.selected
        color: Theme.indicator
    }

    contentItem: RowLayout {
        spacing: Theme.gap
        opacity: row.enabled ? 1 : 0.5

        RowLayout {
            id: leadingRow
            spacing: 0
            visible: children.length > 0
        }

        Icon {
            objectName: "listRowSymbol"
            Layout.alignment: row.detail.length > 0 && !row.symbolCentered
                              ? Qt.AlignTop : Qt.AlignVCenter
            Layout.topMargin: row.detail.length > 0 && !row.symbolCentered ? 2 : 0
            // One slot for every row's icon, whatever the picture's own size.
            Layout.preferredWidth: row.symbolSize
            Layout.preferredHeight: row.symbolSize
            sourceSize: Qt.size(row.symbolSize, row.symbolSize)
            visible: row.symbol.length > 0 || row.image.toString().length > 0
            symbol: row.symbol
            source: row.image.toString().length > 0 ? row.image
                    : row.symbol.length > 0 ? Theme.icon(row.symbol) : ""
            color: row.image.toString().length > 0 ? "transparent" : row.symbolColor
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            Label {
                objectName: "listRowTitle"
                Layout.fillWidth: true
                text: row.text
                wrapMode: row.singleLineTitle ? Text.NoWrap : Text.Wrap
                elide: row.singleLineTitle ? Text.ElideMiddle : Text.ElideNone
                font.pixelSize: Theme.bodySize
            }

            Label {
                Layout.fillWidth: true
                visible: text.length > 0
                text: row.detail
                wrapMode: Text.Wrap
                maximumLineCount: row.detailLines > 0 ? row.detailLines : 1000
                elide: row.detailLines > 0 ? Text.ElideRight : Text.ElideNone
                font.pixelSize: Theme.secondarySize
                color: row.detailColor
            }

            Label {
                Layout.fillWidth: true
                visible: !row.valueBeside
                text: row.valueText
                wrapMode: Text.Wrap
                font.pixelSize: Theme.secondarySize
                color: Material.secondaryTextColor
            }
        }

        Label {
            visible: row.valueBeside && row.valueText.length > 0
            text: row.valueText
            font.pixelSize: Theme.secondarySize
            color: Material.secondaryTextColor
        }

        // (Visible by its children's number, never by their visibility:
        // hidden, it would hide them, and never show again.)
        RowLayout {
            id: trailingRow
            objectName: "listRowTrailing"
            spacing: Theme.gapSmall
            visible: children.length > 0
        }
    }
}

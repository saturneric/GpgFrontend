// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import GpgFrontend.Mobile

// A search box as the platform draws one: a rounded, softly filled field with
// a magnifier, and a cross to empty it once something is typed. Its hint is
// drawn here: Material's placeholder floats up as a label over a field of
// its own background, and on this one it collides with the pill.
TextField {
    id: field

    // What to search for, shown while nothing is typed.
    property string hint
    // How tall the pill is drawn; the field itself stays a full touch
    // target, and grows with the text size.
    property int pillHeight: Theme.touch

    // Taller only when its text needs it (large text): by the font, not by
    // the field's own content, which follows its height.
    implicitHeight: Math.max(Theme.touch, pillHeight,
                             Math.ceil(lineMetrics.height) + 2 * Theme.gapSmall)
    // The pill this much inside the touch target, and growing with it.
    topInset: Math.max(0, (Theme.touch - pillHeight) / 2)
    bottomInset: Math.max(0, (Theme.touch - pillHeight) / 2)
    leftPadding: 44
    rightPadding: clearButton.visible ? 48 : Theme.gap
    topPadding: 0
    bottomPadding: 0
    verticalAlignment: TextInput.AlignVCenter
    inputMethodHints: Qt.ImhNoPredictiveText
    font.pixelSize: Theme.secondarySize + 1
    Accessible.role: Accessible.EditableText
    Accessible.name: hint

    FontMetrics {
        id: lineMetrics
        font: field.font
    }

    background: Rectangle {
        radius: height / 2
        color: Theme.surface(field.Material.backgroundColor, field.activeFocus ? 3 : 2)
    }

    Label {
        anchors.left: parent.left
        anchors.leftMargin: field.leftPadding
        anchors.right: parent.right
        anchors.rightMargin: field.rightPadding
        anchors.verticalCenter: parent.verticalCenter
        visible: field.length === 0 && field.preeditText.length === 0
        text: field.hint
        elide: Text.ElideRight
        font: field.font
        color: field.Material.hintTextColor
    }

    Icon {
        anchors.left: parent.left
        anchors.leftMargin: 14
        anchors.verticalCenter: parent.verticalCenter
        symbol: "search"
        sourceSize: field.pillHeight < Theme.touch ? Qt.size(18, 18) : Qt.size(20, 20)
        color: Material.secondaryTextColor
    }

    BarButton {
        id: clearButton

        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.verticalCenter: parent.verticalCenter
        visible: field.text.length > 0
        text: qsTr("Clear search")
        icon.source: Theme.icon("close")
        focusPolicy: Qt.NoFocus
        onClicked: field.clear()
    }
}

// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import GpgFrontend.Mobile

// A search box as the platform draws one: a rounded, softly filled field with
// a magnifier, and a cross to empty it once something is typed.
TextField {
    id: field

    implicitHeight: Theme.touch
    leftPadding: 44
    rightPadding: clearButton.visible ? 48 : Theme.gap
    topPadding: 0
    bottomPadding: 0
    verticalAlignment: TextInput.AlignVCenter
    inputMethodHints: Qt.ImhNoPredictiveText
    font.pixelSize: Theme.secondarySize + 1
    Accessible.role: Accessible.EditableText
    Accessible.name: placeholderText

    background: Rectangle {
        radius: height / 2
        color: Theme.surface(field.Material.backgroundColor, field.activeFocus ? 3 : 2)
    }

    Icon {
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        symbol: "search"
        sourceSize: Qt.size(20, 20)
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

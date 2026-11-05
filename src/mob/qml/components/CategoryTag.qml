// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// A key category as a small tag, side by side with others in a Flow that
// wraps: its colour and name, filled while the key is in it. A tap puts the
// key in or takes it out. Its touch area is a full touch target even where
// the tag drawn is smaller.
AbstractButton {
    id: tag

    property string name
    property color tone: "transparent"

    checkable: true
    text: name
    implicitHeight: Theme.touch
    implicitWidth: row.implicitWidth + 24
    topInset: (Theme.touch - Math.max(32, row.implicitHeight + 8)) / 2
    bottomInset: topInset
    focusPolicy: Qt.NoFocus
    Accessible.role: Accessible.CheckBox
    Accessible.checked: checked
    Accessible.name: name

    background: Rectangle {
        radius: height / 2
        color: tag.checked ? Theme.alpha(tag.tone.a > 0 ? tag.tone : Material.accentColor, 0.2)
                           : "transparent"
        border.width: 1
        border.color: tag.checked ? Theme.alpha(tag.tone.a > 0 ? tag.tone
                                                               : Material.accentColor, 0.7)
                                  : Material.dividerColor
    }

    contentItem: Item {
        implicitWidth: row.implicitWidth
        implicitHeight: row.implicitHeight

        RowLayout {
            id: row

            anchors.centerIn: parent
            spacing: 6

            Rectangle {
                Layout.preferredWidth: 10
                Layout.preferredHeight: 10
                radius: 5
                color: tag.tone
            }

            Label {
                text: tag.name
                font.pixelSize: Theme.secondarySize
                font.weight: tag.checked ? Font.Medium : Font.Normal
            }
        }
    }
}

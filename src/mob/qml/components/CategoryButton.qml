// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// Which key category a list shows, and the way to choose another: its colour,
// its name and an arrow down, on a soft pill beside the search. Tapped, it
// asks to open the list of categories (CategoryListSheet).
AbstractButton {
    id: button

    // The category shown: {"id", "name", "color"}; the id is "" for all keys.
    property var category: ({"id": "", "name": qsTr("All keys"), "color": ""})
    // Its mark and the arrow only, its name said aloud: with large text the
    // search beside it needs the room.
    // Also on a narrow screen (set by whoever places it).
    property bool compact: Theme.fontScale > 1.3

    objectName: "categoryButton"
    implicitHeight: Theme.touch
    implicitWidth: Math.min(row.implicitWidth + leftPadding + rightPadding, 220)
    leftPadding: 12
    rightPadding: 8
    text: category.name
    focusPolicy: Qt.NoFocus
    Accessible.role: Accessible.ComboBox
    Accessible.name: qsTr("Category: %1. Choose another").arg(category.name)

    background: Rectangle {
        radius: height / 2
        color: Theme.surface(button.Material.backgroundColor, button.pressed ? 4 : 2)
    }

    contentItem: RowLayout {
        id: row

        spacing: 6

        Rectangle {
            visible: (button.category.color || "").length > 0
            Layout.preferredWidth: 10
            Layout.preferredHeight: 10
            radius: 5
            color: button.category.color || "transparent"
        }

        Icon {
            visible: (button.category.color || "").length === 0
            symbol: "label"
            sourceSize: Qt.size(18, 18)
            color: Material.secondaryTextColor
        }

        Label {
            Layout.fillWidth: true
            visible: !button.compact
            text: button.category.name
            elide: Text.ElideRight
            font.pixelSize: Theme.secondarySize
        }

        Icon {
            symbol: "expand_more"
            sourceSize: Qt.size(20, 20)
            color: Material.secondaryTextColor
        }
    }
}

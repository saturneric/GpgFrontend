// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// Which key category a list shows, and the way to choose another: the
// category's colour and an arrow down, on a small soft pill beside the
// search, the same width whatever the category is called. Its name is said
// aloud, not shown: the search beside it takes the room. Tapped, it asks to
// open the list of categories (CategoryListSheet), which names them all.
AbstractButton {
    id: button

    // The category shown: {"id", "name", "color"}; the id is "" for all keys,
    // which has no colour of its own and shows an empty ring.
    property var category: ({"id": "", "name": qsTr("All keys"), "color": ""})
    // How tall the pill is drawn; the button itself stays a full touch
    // target, and grows with the text size.
    property int pillHeight: Theme.touch

    readonly property bool coloured: (category.color || "").length > 0

    objectName: "categoryButton"
    implicitWidth: 56
    implicitHeight: Math.max(Theme.touch, pillHeight)
    // The pill this much inside the touch target, and growing with it.
    topInset: Math.max(0, (Theme.touch - pillHeight) / 2)
    bottomInset: Math.max(0, (Theme.touch - pillHeight) / 2)
    leftPadding: 0
    rightPadding: 0
    text: category.name
    Accessible.role: Accessible.ComboBox
    Accessible.name: qsTr("Category: %1. Choose another").arg(category.name)

    background: Rectangle {
        radius: height / 2
        color: Theme.surface(button.Material.backgroundColor,
                             button.pressed ? 4 : button.visualFocus ? 3 : 2)
    }

    contentItem: Item {
        RowLayout {
            anchors.centerIn: parent
            spacing: 4

            Rectangle {
                objectName: "categoryMark"
                Layout.preferredWidth: 12
                Layout.preferredHeight: 12
                Layout.alignment: Qt.AlignVCenter
                radius: 6
                color: button.coloured ? button.category.color : "transparent"
                border.width: button.coloured ? 0 : 1.5
                border.color: Material.hintTextColor
            }

            Icon {
                Layout.alignment: Qt.AlignVCenter
                symbol: "expand_more"
                sourceSize: Qt.size(20, 20)
                color: Material.secondaryTextColor
            }
        }
    }
}

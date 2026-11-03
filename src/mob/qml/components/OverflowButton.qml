// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import GpgFrontend.Mobile

// The three dots in a bar or at the end of a row, and the menu they open: the
// page puts its MenuItems in.
BarButton {
    id: button

    default property alias items: menu.contentData
    readonly property alias menu: menu

    text: qsTr("More options")
    icon.source: Theme.icon("more_vert")
    onClicked: menu.open()

    Menu {
        id: menu

        y: button.height
    }
}

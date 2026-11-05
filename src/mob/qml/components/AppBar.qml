// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// A page's top bar: the style's tool bar in the page's own colour, the
// title, and the page's actions as icons (BarButton).
ToolBar {
    id: bar

    property string title
    property bool canGoBack: false
    // What the leading button looks like and is called: a back arrow, or a
    // cross that ends a mode (a selection).
    property string backIcon: "arrow_back"
    property string backText: qsTr("Back")
    // Lifted off the page once what is under it has scrolled.
    property bool raised: false
    default property alias actions: actionRow.data

    signal back()

    // The page's own colour, always, under the status bar as well (the
    // window's): no change of colour and no shadow when the page scrolls,
    // only a hairline under the bar that fades in once something is under
    // it.
    Material.background: "transparent"
    Material.elevation: 0

    Rectangle {
        objectName: "appBarDivider"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: bar.Material.dividerColor
        opacity: bar.raised ? 1 : 0

        Behavior on opacity {
            NumberAnimation { duration: 150 }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: bar.canGoBack ? 4 : Theme.gap
        anchors.rightMargin: 4
        spacing: 4

        BarButton {
            objectName: "appBarBack"
            visible: bar.canGoBack
            text: bar.backText
            icon.source: Theme.icon(bar.backIcon)
            onClicked: bar.back()
        }

        Label {
            Layout.fillWidth: true
            text: bar.title
            elide: Text.ElideRight
            font.pixelSize: Theme.titleSize
            font.weight: Theme.titleWeight
            Accessible.role: Accessible.Heading
        }

        RowLayout {
            id: actionRow
            spacing: 0
        }
    }
}

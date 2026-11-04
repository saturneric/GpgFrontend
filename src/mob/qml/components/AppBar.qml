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

    // The page shows through, as on the platform's own top bars; once the
    // page has scrolled under it, it takes a tint of the page's colour and a
    // shadow. (A transparent bar's shadow fills it grey.)
    Material.background: raised && parent
                         ? Theme.surface(parent.Material.backgroundColor, 2)
                         : "transparent"
    Material.elevation: raised ? 2 : 0

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
        }

        RowLayout {
            id: actionRow
            spacing: 0
        }
    }
}

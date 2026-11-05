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
    // As tall as its title needs: the style's own height at the usual text
    // size, taller with large text, where a title of the style's height ran
    // into what is under the bar (a search field). The style's own formula,
    // with the title's row as the content (the row is laid over the bar, so
    // the bar's content item does not know its size).
    // By the title and a touch target only, never by the actions: a text
    // button that comes and goes (Select all) is taller than an icon with
    // the app's font, and the page under the bar moved with it.
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             Math.max(Theme.touch, titleLabel.implicitHeight)
                             + topPadding + bottomPadding)

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
        id: row

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
            id: titleLabel
            Layout.fillWidth: true
            text: bar.title
            elide: Text.ElideRight
            font.pixelSize: Theme.titleSize
            font.weight: Theme.titleWeight
            Accessible.role: Accessible.Heading
        }

        RowLayout {
            id: actionRow
            Layout.maximumHeight: row.height
            spacing: 0
        }
    }
}

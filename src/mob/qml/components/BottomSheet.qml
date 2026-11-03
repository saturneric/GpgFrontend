// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// A choice or a short form that slides up from the foot of the window: the
// style's drawer, with a title, its content, and buttons at the foot. It is
// never taller than 90 % of the window; taller content scrolls.
Drawer {
    id: sheet

    property string title
    default property alias content: body.data
    // The row of buttons at the foot, if any.
    property alias footer: footerRow.data

    edge: Qt.BottomEdge
    width: parent ? parent.width : 0
    // From the content and the padding, which takes in the system bar the
    // sheet runs under; not implicitHeight, which Qt works out from the height.
    height: Math.min(column.implicitHeight + topPadding + bottomPadding,
                     (parent ? parent.height : 0) * 0.9)
    modal: true
    // A popup closes on Back only while it has the focus; without it Back
    // went past the open sheet to the page under it.
    focus: true
    dragMargin: 0

    contentItem: ColumnLayout {
        id: column

        spacing: 0

        // The platform's handle, saying the sheet can be pulled away.
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 12
            Layout.bottomMargin: 8
            implicitWidth: 32
            implicitHeight: 4
            radius: 2
            color: Theme.alpha(sheet.Material.foreground, 0.4)
        }

        Label {
            Layout.fillWidth: true
            Layout.leftMargin: 24
            Layout.rightMargin: 24
            Layout.topMargin: 8
            Layout.bottomMargin: 8
            visible: text.length > 0
            text: sheet.title
            font.pixelSize: Theme.titleSize
            wrapMode: Text.Wrap
        }

        Flickable {
            id: scroller

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: body.implicitHeight
            contentHeight: body.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            interactive: contentHeight > height

            // Keeps the field being typed into in sight, also once the
            // keyboard has taken some of the sheet.
            function reveal() {
                const item = scroller.Window.activeFocusItem
                if (!item || !sheet.opened) return
                let up = item
                while (up && up !== body) up = up.parent
                if (!up) return
                const at = item.mapToItem(body, 0, 0)
                if (at.y < contentY)
                    contentY = Math.max(0, at.y - 8)
                else if (at.y + item.height > contentY + height)
                    contentY = Math.min(contentHeight - height,
                                        at.y + item.height - height + 8)
            }

            onHeightChanged: Qt.callLater(reveal)

            Connections {
                target: scroller.Window.window
                function onActiveFocusItemChanged() {
                    Qt.callLater(scroller.reveal)
                }
            }

            ColumnLayout {
                id: body
                x: 24
                width: scroller.width - 48
                spacing: 12
            }
        }

        RowLayout {
            id: footerRow

            Layout.fillWidth: true
            Layout.margins: Theme.gap
            Layout.topMargin: 8
            spacing: Theme.gapSmall
            visible: children.length > 0
        }

        // The keyboard covers the foot of the window; what is above it
        // scrolls.
        Item {
            Layout.preferredHeight: Theme.keyboard
        }
    }
}

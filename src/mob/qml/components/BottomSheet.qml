// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// A choice or a short form that slides up from the foot of the window: the
// style's drawer, with a title, its content, and buttons at the foot. It is
// never taller than 90 % of the window; taller content scrolls.
//
// An expandable sheet opens at about half the window and grows to the 90 %
// when expanded (its handle, or the sheet itself, asks). A sheet that takes
// Back step by step (stepBack) is asked first what Back undoes inside it --
// an inner page, a search -- and closes only when it says nothing is left.
Drawer {
    id: sheet

    property string title
    default property alias content: body.data
    // The row of buttons at the foot, if any.
    property alias footer: footerRow.data
    // Opens at half the window, and grows when expanded.
    property bool expandable: false
    property bool expanded: false
    // A function taking one step back inside the sheet, true when it did;
    // with none, or when it did nothing, Back collapses an expanded sheet,
    // then closes it.
    property var stepBack: null
    readonly property alias body: body

    // What Back does with the sheet open: one step, never more.
    function back() {
        // A sheet that is busy is not let go (it is then not interactive).
        if (!interactive) return
        if (stepBack && stepBack()) return
        if (expandable && expanded) {
            expanded = false
            return
        }
        close()
    }

    // From the content and the padding, which takes in the system bar the
    // sheet runs under; not implicitHeight, which Qt works out from the
    // height. Set, not bound, and a moment later: the padding follows where
    // the sheet sits, which follows its height, and a binding through both
    // loops whenever the content grows or shrinks.
    function fit() {
        // Half way only while nothing is typed: the keyboard takes its share
        // of the sheet (the spacer at its foot), and half a window less a
        // keyboard left the field typed into almost no room.
        const half = expandable && !expanded && Theme.keyboard === 0
        const room = (parent ? parent.height : 0) * (half ? 0.6 : 0.9)
        height = Math.min(column.implicitHeight + topPadding + bottomPadding, room)
    }

    edge: Qt.BottomEdge
    width: parent ? parent.width : 0
    onAboutToShow: fit()
    // Again once it sits where it opened, with the padding that goes with it.
    onOpened: Qt.callLater(fit)
    onTopPaddingChanged: Qt.callLater(fit)
    onBottomPaddingChanged: Qt.callLater(fit)
    onExpandedChanged: Qt.callLater(fit)
    onParentChanged: Qt.callLater(fit)

    // The keyboard comes or goes: the room changes with it.
    Connections {
        target: Theme
        function onKeyboardChanged() { Qt.callLater(sheet.fit) }
    }

    // A window that turns or resizes leaves a different room.
    Connections {
        target: sheet.parent
        function onHeightChanged() { Qt.callLater(sheet.fit) }
    }
    // Back is the sheet's own (back()), not the style's: it closes on Back
    // in one go.
    closePolicy: Popup.CloseOnPressOutside
    onClosed: expanded = false
    modal: true
    // A popup closes on Back only while it has the focus; without it Back
    // went past the open sheet to the page under it.
    focus: true
    dragMargin: 0

    contentItem: ColumnLayout {
        id: column

        spacing: 0
        onImplicitHeightChanged: Qt.callLater(sheet.fit)
        // Back and Escape reach the sheet from whatever has the focus in it.
        focus: true
        Keys.onBackPressed: function(event) {
            event.accepted = true
            sheet.back()
        }
        Keys.onEscapePressed: function(event) {
            event.accepted = true
            sheet.back()
        }

        // The platform's handle, saying the sheet can be pulled away; on an
        // expandable sheet a tap on it grows or shrinks the sheet.
        Item {
            Layout.fillWidth: true
            implicitHeight: 24

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                y: 12
                width: 32
                height: 4
                radius: 2
                color: Theme.alpha(sheet.Material.foreground, 0.4)
            }

            TapHandler {
                enabled: sheet.expandable
                onTapped: sheet.expanded = !sheet.expanded
            }

            Accessible.role: sheet.expandable ? Accessible.Button
                                              : Accessible.NoRole
            Accessible.name: sheet.expandable
                             ? (sheet.expanded ? qsTr("Show less")
                                               : qsTr("Show more"))
                             : ""
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

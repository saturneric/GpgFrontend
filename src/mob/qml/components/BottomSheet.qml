// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// A choice or a short form that slides up from the foot of the window: a
// title, a line saying what it is for, its content, and buttons at the
// foot, laid out the same in every sheet (Theme's sheet measures).
//
// A modal Popup, not the style's Drawer: an open Drawer either takes every
// swipe on its content (a list in it then closes the sheet) or, made
// non-interactive, no longer closes on a tap beside it or on Escape. Here
// one thing owns dragging: the handle and the title row. Dragged down far
// enough, the sheet goes; an expandable sheet is also dragged between half
// the window and nearly all of it. A list in the sheet scrolls on its own.
//
// Its height is its content's, never more than the room there is; a sheet
// with little in it is as short as that. The room is reckoned from the
// window alone (its height, its safe areas and the keyboard, all in Theme),
// never from where the sheet itself sits.
//
// A sheet that takes Back step by step (stepBack) is asked first what Back
// undoes inside it -- an inner page, a search -- and closes only when it says
// nothing is left.
Popup {
    id: sheet

    property string title
    // One line under the title saying what the sheet is for; none if empty.
    property string description
    default property alias content: body.data
    // The row of buttons at the foot, if any.
    property alias footer: footerRow.data
    // What must stay in sight above the buttons however far the content is
    // scrolled: why the last try failed (a ProblemLabel).
    property alias status: statusSlot.data
    // What sits beside the title, at its right: a progress slot, a button.
    property alias headerItems: headerRow.data
    // Opens at about half the window, and grows when expanded.
    property bool expandable: false
    property bool expanded: false
    // The content is one list that fills the sheet and scrolls itself:
    // the sheet does not scroll it a second time.
    property bool listMode: false
    // How far the content keeps from the sheet's sides. A sheet that is one
    // list of rows, edge to edge, sets 0: its rows keep their words in line
    // with the title by their own padding (Theme.sheetPad), and anything
    // else in it takes that as its margins.
    property real inset: Theme.sheetPad
    // Whether the person may close it (by Back, a tap beside it, dragging
    // it away); false while it is busy with something that must end first.
    property bool interactive: true
    // A function taking one step back inside the sheet, true when it did;
    // with none, or when it did nothing, Back collapses an expanded sheet,
    // then closes it.
    property var stepBack: null
    readonly property alias body: body
    // It comes up from the foot, as a Drawer did.
    readonly property int edge: Qt.BottomEdge

    // What Back does with the sheet open: one step, never more.
    function back() {
        // A sheet that is busy is not let go.
        if (!interactive) return
        if (stepBack && stepBack()) return
        if (expandable && (expanded || heldHeight > 0)) {
            expanded = false
            heldHeight = 0
            return
        }
        close()
    }

    // ---- Room

    readonly property real windowHeight: parent ? parent.height : 0
    // Under the content: the gesture bar, and the keyboard while it is up.
    readonly property real footInset: Theme.safeBottom + Theme.keyboard
    // The most it may take: the window below the status bar, less a little.
    readonly property real fullHeight: Math.max(0, windowHeight - Theme.safeTop - 8)
    // Half way, for an expandable sheet not expanded; not while typing, when
    // the keyboard takes its share and the field needs the rest. With large
    // text, or on a short screen, half a window holds too little to choose
    // from: all of it, then.
    readonly property real halfHeight: Theme.fontScale > 1.3 || windowHeight < 700
                                       ? fullHeight
                                       : Math.min(fullHeight, windowHeight * 0.6)
    // What its content asks for: header, content, foot, insets.
    readonly property real naturalHeight: header.implicitHeight
        + body.implicitHeight + scroller.topMargin
        + statusSlot.implicitHeight + statusSlot.Layout.topMargin
        + (footerRow.visible ? footerRow.implicitHeight + footerRow.Layout.topMargin
                               + footerRow.Layout.bottomMargin
                             : Theme.gapSmall)
        + footInset
    readonly property bool canExpand: expandable && naturalHeight > halfHeight
    // Where an expandable sheet was let go of, between its half and full
    // height: it stays there. 0 for none (half, or full when expanded).
    property real heldHeight: 0
    readonly property real restingHeight: Math.min(naturalHeight,
        !canExpand || expanded || Theme.keyboard > 0 ? fullHeight
        : heldHeight > 0 ? heldHeight : halfHeight)

    // While the header is dragged: the height it is dragged to.
    property bool dragging: false
    property real dragHeight: 0

    // How far below its place it is while it comes and goes: animated, not
    // its y, which follows its height.
    property real offset: 0

    parent: Overlay.overlay
    x: 0
    y: windowHeight - height + offset
    width: parent ? parent.width : 0
    height: dragging ? dragHeight : restingHeight
    padding: 0
    // Its surface runs to the window's sides; its content keeps clear of a
    // cutout or the navigation buttons there (landscape).
    leftPadding: Theme.safeLeft
    rightPadding: Theme.safeRight
    modal: true
    // Back is the sheet's own (back()): one step at a time.
    closePolicy: interactive ? Popup.CloseOnPressOutside : Popup.NoAutoClose
    // A popup takes Back only while it has the focus; without it Back went
    // past the open sheet to the page under it.
    focus: true
    onOpened: {
        Theme.openSheets = Theme.openSheets.filter(o => o !== sheet).concat([sheet])
        Qt.callLater(keepFocus)
    }
    onClosed: {
        Theme.openSheets = Theme.openSheets.filter(o => o !== sheet)
        expanded = false
        heldHeight = 0
        dragging = false
        offset = 0
    }
    Component.onDestruction: Theme.openSheets = Theme.openSheets.filter(o => o !== sheet)

    // Whether @p item is in a popup (any: a sheet, a menu, a dialog).
    function inPopup(item) {
        for (let it = item; it; it = it.parent) {
            if (it === Overlay.overlay) return true
        }
        return false
    }

    // The topmost sheet keeps the focus Back and Escape come through: a
    // sheet that closes as this one opens hands the focus back, when it has
    // gone, to what had it before -- the page -- and Back then never reached
    // this one. Focus that goes to another popup (a menu, a dialog over the
    // sheet) is left there.
    function keepFocus() {
        if (!opened || Theme.openSheets[Theme.openSheets.length - 1] !== sheet) return
        const window = scroller.Window.window
        const now = window ? window.activeFocusItem : null
        if (now && inPopup(now)) return
        column.forceActiveFocus()
    }

    Behavior on height {
        enabled: !sheet.dragging && sheet.opened
        NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
    }

    enter: Transition {
        NumberAnimation { property: "offset"; from: sheet.height; to: 0
                          duration: 220; easing.type: Easing.OutCubic }
    }
    exit: Transition {
        NumberAnimation { property: "offset"; from: 0; to: sheet.height
                          duration: 180; easing.type: Easing.InCubic }
    }

    background: Rectangle {
        radius: Theme.radiusLarge
        color: Theme.surface(sheet.Material.backgroundColor, 1)

        // Square at the foot: the sheet runs on to the window's edge.
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: Theme.radiusLarge
            color: parent.color
        }
    }

    contentItem: ColumnLayout {
        id: column

        spacing: 0
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

        // The handle and the title: what the sheet is dragged by.
        Item {
            id: header

            Layout.fillWidth: true
            implicitHeight: headerColumn.implicitHeight

            DragHandler {
                id: drag

                target: null
                xAxis.enabled: false
                enabled: sheet.interactive
                property real startHeight: 0
                property real startY: 0

                onActiveChanged: {
                    if (active) {
                        startHeight = sheet.height
                        startY = centroid.scenePosition.y
                        sheet.dragHeight = sheet.height
                        sheet.dragging = true
                        return
                    }
                    sheet.settle(centroid.velocity.y)
                }
                // By where the finger is in the window, not in the header:
                // the header rises with the sheet, so the finger's distance
                // from it stayed near nothing and the sheet hardly followed.
                onCentroidChanged: {
                    if (!active) return
                    const top = sheet.canExpand ? sheet.fullHeight : sheet.restingHeight
                    const moved = centroid.scenePosition.y - startY
                    sheet.dragHeight = Math.max(0, Math.min(top, startHeight - moved))
                }
            }

            ColumnLayout {
                id: headerColumn

                width: parent.width
                spacing: 0

                // The platform's handle, saying the sheet can be pulled; on
                // an expandable sheet a tap on it grows or shrinks it.
                Item {
                    Layout.fillWidth: true
                    implicitHeight: 16

                    Rectangle {
                        anchors.horizontalCenter: parent.horizontalCenter
                        y: 6
                        width: 32
                        height: 4
                        radius: 2
                        color: Theme.alpha(sheet.Material.foreground, 0.4)
                    }

                    TapHandler {
                        enabled: sheet.canExpand
                        onTapped: {
                            sheet.heldHeight = 0
                            sheet.expanded = !sheet.expanded
                        }
                    }

                    Accessible.role: sheet.canExpand ? Accessible.Button
                                                     : Accessible.NoRole
                    Accessible.name: sheet.canExpand
                                     ? (sheet.expanded ? qsTr("Show less")
                                                       : qsTr("Show more"))
                                     : ""
                }

                RowLayout {
                    id: headerRow

                    Layout.fillWidth: true
                    Layout.leftMargin: Theme.sheetPad
                    Layout.rightMargin: Theme.sheetPad - 8
                    Layout.topMargin: 4
                    // The title to the content; to its description, less.
                    // Less the room the content keeps for a field's label.
                    Layout.bottomMargin: descriptionLabel.visible
                                         ? 0 : Theme.sheetTitleGap - scroller.labelRoom
                    visible: sheet.title.length > 0 || children.length > 1
                    spacing: 8

                    Label {
                        objectName: "sheetTitle"
                        Layout.fillWidth: true
                        Layout.minimumHeight: 32
                        verticalAlignment: Text.AlignVCenter
                        text: sheet.title
                        font.pixelSize: Theme.sheetTitleSize
                        font.weight: Theme.titleWeight
                        wrapMode: Text.Wrap
                        Accessible.role: Accessible.Heading
                    }
                }

                Label {
                    id: descriptionLabel

                    objectName: "sheetDescription"
                    Layout.fillWidth: true
                    Layout.leftMargin: Theme.sheetPad
                    Layout.rightMargin: Theme.sheetPad
                    Layout.topMargin: Theme.sheetTitleGap - 8
                    Layout.bottomMargin: Theme.sheetIntroGap - scroller.labelRoom
                    visible: sheet.description.length > 0
                    text: sheet.description
                    wrapMode: Text.Wrap
                    font.pixelSize: Theme.secondarySize
                    color: sheet.Material.secondaryTextColor
                }
            }
        }

        // The content: a column that scrolls in the sheet, or, in list mode,
        // a column whose list fills what is left and scrolls itself.
        Flickable {
            id: scroller

            // Room above the content for what rises over its top: a text
            // field's label floats up out of the field, and the clip cut it
            // off on a sheet whose first item is a field ("Name, for example
            // Project team"). Taken from the gap under the title, so the
            // sheet looks as it did. A list scrolls itself and needs none.
            readonly property int labelRoom: sheet.listMode ? 0 : 8

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: body.implicitHeight + topMargin
            topMargin: labelRoom
            contentHeight: sheet.listMode ? height : body.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            interactive: !sheet.listMode && contentHeight + topMargin > height + 1
            ScrollIndicator.vertical: ScrollIndicator {
                visible: !sheet.listMode
            }

            // Keeps the field being typed into in sight, also once the
            // keyboard has taken some of the sheet.
            function reveal() {
                if (sheet.listMode) return
                const item = scroller.Window.activeFocusItem
                if (!item || !sheet.opened) return
                let up = item
                while (up && up !== body) up = up.parent
                if (!up) return
                const at = item.mapToItem(body, 0, 0)
                if (at.y < contentY)
                    contentY = Math.max(-topMargin, at.y - 8)
                else if (at.y + item.height > contentY + height)
                    contentY = Math.min(contentHeight - height,
                                        at.y + item.height - height + 8)
            }

            onHeightChanged: Qt.callLater(reveal)

            Connections {
                target: scroller.Window.window
                function onActiveFocusItemChanged() {
                    Qt.callLater(scroller.reveal)
                    if (sheet.opened) Qt.callLater(sheet.keepFocus)
                }
            }

            ColumnLayout {
                id: body

                x: sheet.inset
                width: scroller.width - 2 * sheet.inset
                height: sheet.listMode ? scroller.height : implicitHeight
                spacing: 12
            }
        }

        // Kept in sight above the buttons, whatever is scrolled.
        ColumnLayout {
            id: statusSlot

            Layout.fillWidth: true
            Layout.leftMargin: Theme.sheetPad
            Layout.rightMargin: Theme.sheetPad
            // Nothing when there is nothing to say. (Not by its visibility:
            // its children's follows its own and would never bring it back.)
            Layout.topMargin: implicitHeight > 0 ? Theme.gapSmall : 0
            spacing: 4
        }

        // The buttons side by side while they fit; one under the other when
        // they do not (large text, long words), never cut off. A button's
        // own inset adds to the gap above them, which comes to the sheets'
        // room between content and actions.
        GridLayout {
            id: footerRow

            readonly property real needed: {
                let sum = 0
                let shown = 0
                for (const child of children) {
                    if (!child.visible || child.implicitWidth <= 0) continue
                    sum += child.implicitWidth
                    ++shown
                }
                return sum + Math.max(0, shown - 1) * columnSpacing
            }

            Layout.fillWidth: true
            Layout.leftMargin: Theme.sheetPad
            Layout.rightMargin: Theme.sheetPad
            Layout.topMargin: Theme.sheetActionGap - 8
            Layout.bottomMargin: Theme.gapSmall
            // One under the other, each as wide as the sheet: a button
            // left at its own width would make the whole sheet that wide
            // (a layout keeps an item that does not fill at its width).
            // Measured against the room the sheet gives the row, not the
            // row's own width: side by side, that is as wide as the buttons
            // ask, and would never be too narrow for them.
            readonly property bool stacked:
                needed > sheet.availableWidth - Layout.leftMargin - Layout.rightMargin
            columns: stacked ? 1 : Math.max(1, children.length)
            onStackedChanged: arrange()
            onChildrenChanged: arrange()
            function arrange() {
                for (const child of children) {
                    // The buttons, not the spacers between them.
                    if (typeof child.clicked === "function") child.Layout.fillWidth = stacked
                }
            }
            columnSpacing: Theme.gapSmall
            rowSpacing: 4
            visible: children.length > 0
        }

        // The gesture bar, and the keyboard while it is up: the content
        // ends above them.
        Item {
            Layout.preferredHeight: sheet.footInset
        }
    }

    // Where the sheet goes when the header is let go: away when dragged low
    // or flung down, else to the nearest of half and full (expandable), or
    // back where it was.
    // Let go of at dragHeight, moving at @p velocity (px/s, down positive).
    // It stays where it was let go of; only a clear flick up opens it all
    // the way, and only dragging it well below its place, or flicking it
    // down from there, closes it: a finger lifting off often reports some
    // speed downwards, which used to close a sheet just pulled up.
    function settle(velocity) {
        const h = dragHeight
        dragging = false
        const low = canExpand ? halfHeight : restingHeight
        if (h < low * 0.5 || (velocity > 2500 && h < low * 0.9)) {
            close()
            return
        }
        if (!canExpand) return
        if (velocity < -2500) {
            heldHeight = 0
            expanded = true
            return
        }
        // Near the top it is open all the way (a sheet that shows more
        // when expanded, the status sheet, shows it); at or below its half
        // height it goes back there; between, it stays where it is.
        if (h >= fullHeight - 48) {
            heldHeight = 0
            expanded = true
            return
        }
        expanded = false
        heldHeight = h <= halfHeight ? 0 : h
    }
}

// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// A line over the action bar while something runs, or once something has
// ended that was not looked at: what, in a few words, and a tap away from
// its result. Nothing for an operation done before it would be read (a
// moment), and nothing at all otherwise.
Pane {
    id: line

    property SheetHost host

    readonly property var record: host ? host.lineRecord : ({})
    readonly property bool active: record.phase === "queued"
                                   || record.phase === "running"
                                   || record.phase === "waiting_passphrase"

    objectName: "statusLine"
    visible: host !== null && host.lineShown
    padding: 0
    leftPadding: Theme.gap
    rightPadding: 4

    background: Rectangle {
        color: Theme.surface(line.Material.backgroundColor, 1)
    }

    contentItem: RowLayout {
        spacing: 12

        BusyIndicator {
            visible: line.active
            running: visible
            implicitWidth: 24
            implicitHeight: 24
            padding: 0
        }

        Icon {
            visible: !line.active
            symbol: Theme.reportSymbol(line.record.report || {})
            color: Theme.reportColor(line.record.report || {},
                                     Material.secondaryTextColor)
        }

        Label {
            id: text
            Layout.fillWidth: true
            Layout.topMargin: 6
            Layout.bottomMargin: 6
            text: line.host ? line.host.lineText : ""
            elide: Text.ElideRight
            maximumLineCount: 2
            wrapMode: Text.Wrap
            font.pixelSize: Theme.secondarySize
        }

        BarButton {
            objectName: "statusLineOpen"
            text: qsTr("Show the result")
            icon.source: Theme.icon("chevron_right")
            icon.color: Material.secondaryTextColor
            rotation: -90
            onClicked: line.host.openStatus(line.record.operationId)
        }

        BarButton {
            objectName: "statusLineDismiss"
            visible: !line.active
            text: qsTr("Dismiss")
            icon.source: Theme.icon("close")
            onClicked: line.host.dismissLine()
        }
    }

    TapHandler {
        onTapped: line.host.openStatus(line.record.operationId)
    }

    Accessible.role: Accessible.StatusBar
    Accessible.name: text.text
}

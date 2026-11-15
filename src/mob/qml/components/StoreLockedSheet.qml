// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// The profile key is sealed and the device's secure storage cannot answer
// right now, most often because the device is locked. Nothing is lost, so
// the only choices are to try again or to quit: never to reset, which would
// destroy a key that unlocking the device opens.
Alert {
    id: sheet

    readonly property var request: InteractionBridge.current
    readonly property bool storeLocked: request.kind === "store_locked"
    readonly property bool deviceLocked: request.device_locked === true

    closePolicy: Popup.NoAutoClose
    visible: storeLocked
    title: deviceLocked ? qsTr("Unlock your device") : qsTr("Storage not responding")

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        // Scrolls when the window has no room for all of it, as in
        // ConfirmSheet: the choices below stay inside the dialog.
        Flickable {
            id: words

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: body.implicitHeight
            contentHeight: body.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            interactive: contentHeight > height
            ScrollIndicator.vertical: ScrollIndicator {}

            Label {
                id: body

                width: words.width
                wrapMode: Text.Wrap
                color: Material.secondaryTextColor
                text: (sheet.deviceLocked
                       ? qsTr("The key that opens this profile is in the device's secure "
                              + "storage, which can only be used while the device is "
                              + "unlocked. Unlock your device, then tap Retry.")
                       : qsTr("The key that opens this profile is in the device's secure "
                              + "storage, which is not responding right now. Tap Retry, or "
                              + "quit and try again later."))
                      + "\n\n" + qsTr("Nothing has been lost.")
            }
        }

        // At the right, side by side while they fit; the first is the
        // right one.
        Flow {
            id: actions

            Layout.fillWidth: true
            layoutDirection: Qt.RightToLeft

            ActionButton {
                objectName: "retryChoice"
                width: Math.min(implicitWidth, actions.width)
                kind: "text"
                text: qsTr("Retry")
                onClicked: InteractionBridge.answer(sheet.request.id, true)
            }

            ActionButton {
                objectName: "quitChoice"
                width: Math.min(implicitWidth, actions.width)
                kind: "text"
                text: qsTr("Quit")
                onClicked: InteractionBridge.decline(sheet.request.id)
            }
        }
    }
}

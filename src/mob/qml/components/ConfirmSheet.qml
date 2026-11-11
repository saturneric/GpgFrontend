// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// A destructive choice the profile loader cannot make alone: forcing a lock
// another process holds, or discarding the application key. The safe choice
// is the default; the destructive one has to be picked.
Alert {
    id: sheet

    readonly property var request: InteractionBridge.current
    readonly property bool forceUnlock: request.kind === "confirm_force_unlock"
    readonly property bool keyReset: request.kind === "confirm_key_reset"

    closePolicy: Popup.NoAutoClose
    visible: forceUnlock || keyReset
    title: forceUnlock ? qsTr("Profile already open") : qsTr("Reset the profile key?")

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        // The words scroll when the window has no room for all of them
        // (large German text on a small phone): the dialog stops at the
        // window's edge, and the choices below stay inside it.
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
                text: sheet.forceUnlock
                      ? qsTr("Another process (%1, process %2 on %3) has this profile open. "
                             + "Forcing it open while that process runs can damage "
                             + "your data.")
                        .arg(sheet.request.application || qsTr("unknown"))
                        .arg(sheet.request.pid)
                        .arg(sheet.request.host || qsTr("this device"))
                      : sheet.request.reason === "key_file_missing"
                      // Why, in the desktop's words for each: what to do
                      // instead differs (put the file back, unlock the
                      // keychain), not only what resetting costs.
                      ? qsTranslate("QObject", "The application key file is missing, but this "
                                    + "profile still holds settings and data "
                                    + "encrypted with it.") + "\n\n"
                        + qsTranslate("QObject", "You can quit, put the key file back from your backup, "
                                      + "and start the application again. Creating a new key "
                                      + "instead lets the application start, but everything it "
                                      + "previously encrypted becomes permanently unreadable.")
                      : sheet.request.reason === "keychain_secret_lost"
                      ? qsTranslate("QObject", "The application key is protected by a secret "
                                    + "kept in the system keychain, and that secret "
                                    + "could not be read.") + "\n\n"
                        + qsTranslate("QObject", "You can unlock the keychain and start the application "
                                      + "again. Resetting the key instead lets the application "
                                      + "start, but everything it previously encrypted becomes "
                                      + "permanently unreadable.")
                      : qsTr("The key that encrypts this profile cannot be unlocked. "
                             + "Resetting it makes everything it protected "
                             + "permanently unreadable.")
            }
        }

        // At the right, side by side while they fit, one under the other
        // when they do not (large text, German); never past the dialog's
        // edge. Laid out from the right: the first is the right one.
        Flow {
            id: actions

            Layout.fillWidth: true
            layoutDirection: Qt.RightToLeft

            ActionButton {
                objectName: "destructiveChoice"
                width: Math.min(implicitWidth, actions.width)
                kind: "text"
                text: sheet.forceUnlock ? qsTr("Force open") : qsTr("Reset key")
                onClicked: InteractionBridge.answer(sheet.request.id, true)
            }

            ActionButton {
                objectName: "safeChoice"
                width: Math.min(implicitWidth, actions.width)
                kind: "text"
                // A forgotten PIN not reset goes back to the PIN.
                text: sheet.keyReset && sheet.request.reason === "pin_forgotten"
                      ? qsTr("Back") : qsTr("Quit")
                onClicked: InteractionBridge.decline(sheet.request.id)
            }
        }
    }
}

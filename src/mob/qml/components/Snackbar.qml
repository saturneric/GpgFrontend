// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// A short word about what just happened, at the foot of the page, gone after
// a while; with an action ("Undo") when what happened can be taken back. An
// empty text hides it at once. A popup, not a tool tip: a tool tip takes no
// touch, and the action must.
Popup {
    id: bar

    property string text
    // The action offered with the text, if any, until the text changes.
    property string actionText
    // Room left under it, for a bar at the foot of the page.
    property real bottomOffset: 0

    // What the action does when it was given with show(); else action() is
    // emitted.
    property var handler: null

    // The action was taken; the bar is gone.
    signal action()

    x: parent ? (parent.width - width) / 2 : 0
    y: parent ? parent.height - height - bottomOffset - 12 : 0
    width: parent ? Math.min(parent.width - 2 * Theme.gap, 560) : implicitWidth
    modal: false
    focus: false
    closePolicy: Popup.NoAutoClose
    padding: 0
    leftPadding: Theme.gap
    rightPadding: actionButton.visible ? 4 : Theme.gap
    // The platform's snackbar is the inverse of the page.
    Material.theme: Theme.dark ? Material.Light : Material.Dark
    Material.elevation: 3

    background: Rectangle {
        radius: 4
        color: Theme.dark ? "#E6E1E5" : "#313033"
    }

    contentItem: RowLayout {
        spacing: 4

        Label {
            Layout.fillWidth: true
            Layout.topMargin: 14
            Layout.bottomMargin: 14
            wrapMode: Text.Wrap
            text: bar.text
            font.pixelSize: Theme.secondarySize
            color: Theme.dark ? "#313033" : "#F4EFF4"
        }

        Button {
            id: actionButton

            visible: bar.actionText.length > 0
            text: bar.actionText
            flat: true
            focusPolicy: Qt.NoFocus
            Material.foreground: Theme.dark ? Qt.darker(Theme.accent, 1.3)
                                            : Qt.lighter(Theme.accent, 1.5)
            onClicked: {
                // Taken before closing: a new message may replace it.
                const run = bar.handler
                bar.handler = null
                bar.close()
                if (run) run()
                else bar.action()
            }
        }
    }

    Timer {
        id: timeout
        interval: bar.actionText.length > 0 ? 6000 : 4000
        onTriggered: bar.close()
    }

    onTextChanged: {
        if (text.length > 0) {
            open()
            timeout.restart()
        } else {
            timeout.stop()
            close()
        }
    }

    // Show @p message, even when it is the one already shown -- "Copied."
    // twice in a row is two copies -- with @p actionText, if given, which
    // runs @p handler when given (else action() is emitted). An empty
    // message hides it.
    function show(message, actionText, handler) {
        bar.actionText = ""
        text = ""
        bar.handler = handler || null
        bar.actionText = actionText || ""
        text = message
    }

    // How a share went: "Shared." when a target was chosen, nothing when the
    // sheet was dismissed, @p failure otherwise.
    function showShared(outcome, failure) {
        if (outcome === "handed_off") show(qsTr("Shared."))
        else if (outcome === "dismissed") show("")
        else show(failure)
    }
}

// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import GpgFrontend.Mobile

// A passphrase or a PIN: hidden as it is typed, with an eye to show it.
TextField {
    id: field

    property bool revealed: false

    echoMode: revealed ? TextInput.Normal : TextInput.Password
    inputMethodHints: Qt.ImhSensitiveData | Qt.ImhNoPredictiveText
                      | Qt.ImhNoAutoUppercase
    rightPadding: eye.width + 4

    // Shown is for checking it, not for taking it elsewhere: no selection to
    // copy from, and no copy or cut by key either.
    selectByMouse: false
    Keys.onPressed: event => {
        if (event.matches(StandardKey.Copy) || event.matches(StandardKey.Cut))
            event.accepted = true
    }

    // Hidden again whenever it is cleared, so a shown passphrase never
    // outlives the prompt it was typed into.
    onTextChanged: if (text.length === 0) revealed = false

    ToolButton {
        id: eye

        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        text: field.revealed ? qsTr("Hide") : qsTr("Show")
        display: AbstractButton.IconOnly
        icon.source: Theme.icon(field.revealed ? "visibility_off" : "visibility")
        focusPolicy: Qt.NoFocus
        onClicked: field.revealed = !field.revealed
    }
}

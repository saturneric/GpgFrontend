// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import GpgFrontend.Mobile

// An alert that is only a question in words. The text sits in a column, as
// in the alerts with fields, which keeps the dialog's height from feeding
// back into the text's wrapping.
//
// A confirmation sets acceptText: Cancel, and the button that goes ahead.
Alert {
    id: alert

    property string message
    property string acceptText
    property string acceptObjectName

    footer: DialogButtonBox {
        visible: alert.acceptText.length > 0

        Button {
            text: qsTr("Cancel")
            flat: true
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }

        Button {
            objectName: alert.acceptObjectName
            text: alert.acceptText
            flat: true
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }

    ColumnLayout {
        anchors.fill: parent

        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: alert.message
        }
    }
}

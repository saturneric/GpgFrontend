// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

BottomSheet {
    id: sheet

    // The operation's report, as its controller gives it.
    property var report: ({})

    readonly property int status: report.status !== undefined ? report.status : -1

    function statusText(status) {
        if (status > 0) return qsTr("Done")
        if (status === 0) return qsTr("Done, with warnings")
        return qsTr("Failed")
    }

    function validityText(validity) {
        switch (validity) {
        case "fully_valid": return qsTr("Good signature")
        case "valid_not_fully_trusted":
            return qsTr("Good signature, from a key not fully trusted")
        case "valid_with_issues": return qsTr("Valid, with issues")
        case "key_missing": return qsTr("Signed by a key you do not have")
        case "key_revoked": return qsTr("Signed by a revoked key")
        case "key_expired": return qsTr("Signed by an expired key")
        case "sig_expired": return qsTr("The signature has expired")
        case "key_not_authorized":
            return qsTr("Signed by a key not allowed to sign")
        case "invalid": return qsTr("Bad signature")
        default: return qsTr("The signature could not be checked")
        }
    }

    // The colour of how it went, as the desktop shows it.
    function tone(good, warn) {
        if (good) return Theme.positive
        if (warn) return Theme.warning
        return Theme.danger
    }

    objectName: "resultSheet"
    title: report.cancelled === true ? qsTr("Cancelled") : statusText(status)

    RowLayout {
        Layout.fillWidth: true
        spacing: 12
        visible: description.text.length > 0

        Icon {
            Layout.alignment: Qt.AlignTop
            symbol: Theme.reportSymbol(sheet.report)
            color: Theme.reportColor(sheet.report, Material.secondaryTextColor)
        }

        Label {
            id: description
            objectName: "resultDescription"
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: sheet.report.description || ""
        }
    }

    Repeater {
        model: sheet.report.signatures || []

        Frame {
            required property var modelData

            Layout.fillWidth: true
            padding: 12

            ColumnLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: 6

                Chip {
                    text: sheet.validityText(modelData.validity)
                    tone: sheet.tone(modelData.verdict === "good",
                                     modelData.verdict === "warning")
                }

                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WrapAnywhere
                    text: (modelData.uid || modelData.keyId
                           || modelData.fingerprint)
                }
            }
        }
    }

    Label {
        Layout.fillWidth: true
        Layout.topMargin: 4
        visible: (sheet.report.recipients || []).length > 0
        color: Material.secondaryTextColor
        text: qsTr("Encrypted to")
    }

    Repeater {
        model: sheet.report.recipients || []

        RowLayout {
            required property var modelData

            Layout.fillWidth: true
            spacing: 12

            KeyAvatar {
                size: 32
                name: modelData.uid || modelData.keyId
                usable: modelData.keyFound
            }

            Label {
                Layout.fillWidth: true
                wrapMode: Text.WrapAnywhere
                opacity: modelData.keyFound ? 1 : 0.6
                text: modelData.uid || modelData.keyId
            }
        }
    }

    Repeater {
        model: sheet.report.errors || []

        Label {
            required property var modelData

            Layout.fillWidth: true
            wrapMode: Text.Wrap
            color: Theme.danger
            text: modelData
        }
    }

    footer: [
        Item {
            Layout.fillWidth: true
        },
        Button {
            text: qsTr("Close")
            flat: true
            onClicked: sheet.close()
        }
    ]
}

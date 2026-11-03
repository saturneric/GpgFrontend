// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

Page {
    id: page

    // The key's roles, as KeysPage passes them.
    property var key: ({})

    // While a key is being deleted or exported, the page stays: leaving it
    // would drop the answer, and with it the refresh of the key list.
    readonly property bool busy: actions.busy

    signal back()
    // The key is gone; the list wants a refresh.
    signal deleted()

    function finished(action, succeeded, detail) {
        if (!succeeded) {
            notice.show(qsTr("That did not work: %1").arg(detail.error))
            return
        }
        if (action === "export") {
            exported = detail.armored
            notice.show("")
            exportActions.open()
        } else if (action === "delete") {
            page.deleted()
        }
    }

    function shared(outcome) {
        notice.showShared(outcome,
                          qsTr("This key could not be shared. Copy it instead."))
    }

    // The public key, armored, once exported.
    property string exported: ""

    function formatFingerprint(fpr) {
        return (fpr || "").replace(/(.{4})(?!$)/g, "$1 ")
    }

    function formatDate(date) {
        return date && !isNaN(date) ? date.toLocaleDateString(Qt.locale(), Locale.ShortFormat)
                                    : qsTr("Never")
    }

    header: AppBar {
        title: qsTr("Key")
        canGoBack: !page.busy
        raised: scroller.contentItem.contentY > 0
        onBack: page.back()
    }

    ScrollView {
        id: scroller

        anchors.fill: parent
        contentWidth: availableWidth
        implicitWidth: 0

        ColumnLayout {
            width: parent.width
            spacing: 0

            // Whose key it is, and what it can do.
            ColumnLayout {
                Layout.fillWidth: true
                Layout.margins: Theme.gap
                spacing: 8

                KeyAvatar {
                    Layout.alignment: Qt.AlignHCenter
                    size: 72
                    name: page.key.name || page.key.email || ""
                    usable: page.key.usable === true
                }

                Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    font.pixelSize: 22
                    text: page.key.name || page.key.email || page.key.keyId || ""
                }

                Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WrapAnywhere
                    visible: (page.key.name || "").length > 0
                             && (page.key.email || "").length > 0
                    color: Material.secondaryTextColor
                    text: page.key.email || ""
                }

                Flow {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: 4
                    spacing: 6

                    Chip {
                        visible: (page.key.status || "ok") !== "ok"
                        text: page.key.statusText || ""
                        tone: page.key.status === "expiring_soon"
                              ? Theme.warning : Theme.danger
                    }

                    Chip {
                        visible: page.key.hasSecret === true
                        text: qsTr("Your key")
                        tone: Material.accentColor
                    }

                    Chip {
                        visible: page.key.canEncrypt === true
                        text: qsTr("Encrypt")
                        tone: Theme.positive
                    }

                    Chip {
                        visible: page.key.canSign === true
                        text: qsTr("Sign")
                        tone: Theme.positive
                    }
                }
            }

            component Field: ColumnLayout {
                property alias title: titleLabel.text
                property alias value: valueText.text
                property alias mono: valueText.font.family

                Layout.fillWidth: true
                spacing: 2

                Label {
                    id: titleLabel
                    color: Material.secondaryTextColor
                    font.pixelSize: 12
                }

                TextEdit {
                    id: valueText
                    Layout.fillWidth: true
                    readOnly: true
                    selectByMouse: true
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    color: Material.foreground
                    font.pixelSize: 16
                }
            }

            Frame {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.gap
                Layout.rightMargin: Theme.gap

                ColumnLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    spacing: 12

                    Field {
                        objectName: "fingerprint"
                        title: qsTr("Fingerprint")
                        value: page.formatFingerprint(page.key.fingerprint)
                        mono: Theme.mono
                    }
                    Field {
                        title: qsTr("Key ID")
                        value: page.key.keyId || ""
                        mono: Theme.mono
                    }
                    Field { title: qsTr("Algorithm"); value: page.key.algorithm || "" }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.gap

                        Field { title: qsTr("Created"); value: page.formatDate(page.key.created) }
                        Field { title: qsTr("Expires"); value: page.formatDate(page.key.expires) }
                    }
                }
            }

            ItemDelegate {
                objectName: "exportKey"
                Layout.fillWidth: true
                Layout.topMargin: Theme.gapSmall
                leftPadding: Theme.gap + 4
                text: qsTr("Share public key")
                enabled: !actions.busy
                icon.source: Theme.icon("share")
                onClicked: actions.exportPublic(page.key.fingerprint)
            }

            ItemDelegate {
                objectName: "deleteKey"
                Layout.fillWidth: true
                leftPadding: Theme.gap + 4
                text: qsTr("Delete key")
                enabled: !actions.busy
                icon.source: Theme.icon("delete")
                icon.color: Theme.danger
                onClicked: confirmDelete.open()
            }
        }
    }

    Snackbar {
        id: notice
        objectName: "keyNotice"
    }

    KeyActionsController {
        id: actions
    }

    OutputActions {
        id: output
    }

    Component.onCompleted: {
        actions.SignalFinished.connect(page.finished)
        output.SignalShared.connect(page.shared)
    }

    // What to do with the exported public key.
    BottomSheet {
        id: exportActions

        title: qsTr("Public key")

        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            color: Material.secondaryTextColor
            text: qsTr("Give it to the people who want to encrypt to this key "
                       + "or check its signatures. It holds nothing secret.")
        }

        ItemDelegate {
            Layout.fillWidth: true
            Layout.leftMargin: -24
            Layout.rightMargin: -24
            leftPadding: 24
            text: qsTr("Copy")
            icon.source: Theme.icon("content_copy")
            onClicked: {
                output.copy(page.exported, false)
                notice.show(qsTr("Copied."))
                exportActions.close()
            }
        }

        ItemDelegate {
            Layout.fillWidth: true
            Layout.leftMargin: -24
            Layout.rightMargin: -24
            Layout.bottomMargin: Theme.gap
            leftPadding: 24
            text: qsTr("Share")
            icon.source: Theme.icon("share")
            visible: output.canShare
            onClicked: {
                output.share(page.exported)
                exportActions.close()
            }
        }
    }

    MessageAlert {
        id: confirmDelete

        objectName: "confirmDelete"
        title: qsTr("Delete this key?")

        message: page.key.hasSecret
              ? qsTr("This is your own key. Without it you can no "
                     + "longer read messages encrypted to it or sign "
                     + "with it. This cannot be undone.")
              : qsTr("You will no longer be able to encrypt to this "
                     + "key or check its signatures, until you import "
                     + "it again.")

        acceptText: qsTr("Delete")
        acceptObjectName: "confirmDeleteRun"

        onAccepted: actions.remove(page.key.fingerprint)
    }
}

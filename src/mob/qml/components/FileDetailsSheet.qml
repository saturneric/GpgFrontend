// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// One file of the Files page, as it is: its name, its size and its SHA-256
// (worked out when the sheet opens, copied whole), whether it is a temporary
// file not saved yet, and what is done with it -- saving, sharing, opening
// it in another app, taking it off the list. The PGP operations are the PGP
// button's, on the files chosen.
BottomSheet {
    id: sheet

    property FileWorkspace workspace
    property int fileId: -1
    property var file: ({})
    // The SHA-256: empty while it is worked out; hashError when it could
    // not be.
    property string sha256: ""
    property string hashError: ""

    readonly property bool isOutput: file.kind === "output"
    readonly property bool temporary: file.origin === "made"
                                      && (file.savedName || "").length === 0
    readonly property bool hashing: sha256.length === 0 && hashError.length === 0

    signal saveRequested(int id)
    signal shareRequested(int id)
    signal openRequested(int id)
    signal removeRequested(int id)

    function begin(id) {
        sheet.fileId = id
        sheet.file = workspace.entry(id)
        sheet.sha256 = ""
        sheet.hashError = ""
        copied.visible = false
        workspace.hash(id)
        open()
    }

    // The file again whenever the workspace changes it, while it is shown.
    Connections {
        target: sheet.workspace
        enabled: sheet.opened

        function onDataChanged() {
            const file = sheet.workspace.entry(sheet.fileId)
            if (Object.keys(file).length === 0) {
                sheet.close()
                return
            }
            sheet.file = file
        }

    }

    // Its hash, whenever it comes: also before the sheet has finished coming
    // up (a small file is hashed sooner than that). The file id tells which.
    Connections {
        target: sheet.workspace

        function onHashReady(id, sha256, error) {
            if (id !== sheet.fileId) return
            sheet.sha256 = sha256
            sheet.hashError = error.length > 0 ? qsTr("Could not be read.") : ""
        }
    }

    OutputActions {
        id: output
    }

    // Gone: a hash still being worked out is no longer wanted (a large
    // file read for nothing would hold the thread the next one needs).
    onClosed: if (workspace) workspace.cancelHash()

    objectName: "fileDetails"
    title: qsTr("File")

    InfoRow {
        objectName: "fileName"
        Layout.fillWidth: true
        label: qsTr("Name")
        value: sheet.file.name || ""
    }

    InfoRow {
        objectName: "fileSize"
        Layout.fillWidth: true
        label: qsTr("Size")
        value: Theme.fileSize(sheet.file.size || 0)
    }

    // The hash: worked out, then shown whole, a tap from the clipboard.
    RowLayout {
        Layout.fillWidth: true
        spacing: 4

        InfoRow {
            objectName: "fileSha256"
            Layout.fillWidth: true
            label: qsTr("SHA-256")
            mono: sheet.sha256.length > 0
            value: sheet.sha256.length > 0 ? sheet.sha256
                 : sheet.hashError.length > 0 ? sheet.hashError
                 : qsTr("Working it out…")
            valueColor: sheet.hashError.length > 0 ? Theme.danger
                        : sheet.hashing ? Material.secondaryTextColor
                                        : Material.foreground
        }

        // Its place is kept whatever shows in it: nothing moves when the
        // hash arrives.
        Item {
            Layout.alignment: Qt.AlignTop
            implicitWidth: Theme.touch
            implicitHeight: Theme.touch

            BusyIndicator {
                anchors.fill: parent
                padding: 10
                running: sheet.hashing && sheet.opened
                visible: running
            }

            BarButton {
                objectName: "copySha256"
                anchors.fill: parent
                visible: sheet.sha256.length > 0
                text: qsTr("Copy the SHA-256")
                icon.source: Theme.icon("content_copy")
                onClicked: {
                    output.copy(sheet.sha256, false)
                    copied.visible = true
                }
            }
        }
    }

    Label {
        id: copied
        objectName: "fileCopied"
        Layout.fillWidth: true
        visible: false
        text: qsTr("Copied.")
        color: Material.secondaryTextColor
        font.pixelSize: Theme.labelSize
    }

    Label {
        Layout.fillWidth: true
        visible: sheet.file.available === false
        wrapMode: Text.Wrap
        color: Theme.danger
        font.pixelSize: Theme.secondarySize
        text: qsTr("This file can no longer be opened. It may have been "
                   + "moved or deleted, or access to it has ended.")
    }

    Label {
        objectName: "fileTemporary"
        Layout.fillWidth: true
        visible: sheet.temporary || sheet.file.origin === "shared"
        wrapMode: Text.Wrap
        color: sheet.temporary ? Theme.warning : Material.secondaryTextColor
        font.pixelSize: Theme.secondarySize
        text: sheet.temporary
              ? qsTr("Temporary, not saved. It is deleted when you remove it "
                     + "or close the app. Save it to keep it.")
              : qsTr("A copy another app shared. It is deleted when you remove "
                     + "it or close the app.")
    }

    Label {
        Layout.fillWidth: true
        visible: (sheet.file.savedName || "").length > 0
        wrapMode: Text.Wrap
        color: Material.secondaryTextColor
        font.pixelSize: Theme.secondarySize
        text: qsTr("Saved as %1.").arg(sheet.file.savedName || "")
    }

    // What is done with it: side by side while they fit, one under the other
    // on a narrow screen or with large text.
    Flow {
        objectName: "fileActions"
        Layout.fillWidth: true
        Layout.topMargin: 4
        spacing: Theme.gapSmall
        visible: sheet.isOutput

        ActionButton {
            objectName: "detailsSave"
            kind: "primary"
            text: qsTr("Save as")
            icon.source: Theme.icon("save")
            onClicked: {
                sheet.close()
                sheet.saveRequested(sheet.fileId)
            }
        }

        ActionButton {
            objectName: "detailsShare"
            kind: "secondary"
            visible: sheet.workspace && sheet.workspace.canShare
            text: qsTr("Share")
            icon.source: Theme.icon("share")
            onClicked: {
                sheet.close()
                sheet.shareRequested(sheet.fileId)
            }
        }

        ActionButton {
            objectName: "detailsOpenWith"
            kind: "secondary"
            visible: sheet.workspace && sheet.workspace.canShare
            text: qsTr("Open with…")
            icon.source: Theme.icon("visibility")
            onClicked: {
                sheet.close()
                sheet.openRequested(sheet.fileId)
            }
        }
    }

    footer: [
        ActionButton {
            objectName: "detailsRemove"
            kind: "text"
            enabled: sheet.file.phase !== "running"
            text: qsTr("Remove from list")
            icon.source: Theme.icon("delete")
            onClicked: {
                sheet.close()
                sheet.removeRequested(sheet.fileId)
            }
        },
        Item {
            Layout.fillWidth: true
        },
        ActionButton {
            kind: "text"
            text: qsTr("Close")
            onClicked: sheet.close()
        }
    ]
}

// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// One file of the Files page: what it is, whose it is -- a document of the
// person's, or a copy GpgFrontend made and keeps only for now -- how its last
// operation went, and what can be done with it.
BottomSheet {
    id: sheet

    property FileWorkspace workspace
    property int fileId: -1
    property var file: ({})

    readonly property bool isOutput: file.kind === "output"
    readonly property bool usable: file.available === true
                                   && file.phase !== "queued"
                                   && file.phase !== "running"
    readonly property bool hasReport: file.result !== undefined
                                      && Object.keys(file.result).length > 0

    signal encryptRequested(int id)
    signal decryptRequested(int id)
    signal signRequested(int id)
    signal verifyRequested(int id)
    signal saveRequested(int id)
    signal shareRequested(int id)
    signal removeRequested(int id)
    signal reportRequested(var report)

    function begin(id) {
        sheet.fileId = id
        sheet.file = workspace.entry(id)
        open()
    }

    // The file again whenever the workspace changes it, while it is shown.
    Connections {
        target: sheet.workspace
        enabled: sheet.opened

        function onDataChanged() {
            const file = sheet.workspace.entry(sheet.fileId)
            if (Object.keys(file).length === 0) sheet.close()
            else sheet.file = file
        }
    }

    objectName: "fileDetails"
    title: file.name || ""

    // An action of the sheet's: it closes, and the page is asked.
    component SheetAction: ListRow {
        Layout.fillWidth: true
        Layout.leftMargin: -24
        Layout.rightMargin: -24
        leftPadding: 24
        symbolColor: Material.foreground
        onClicked: sheet.close()
    }

    // What it is and where it came from.
    Label {
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        color: Material.secondaryTextColor
        font.pixelSize: Theme.secondarySize
        text: {
            const size = Theme.fileSize(sheet.file.size || 0)
            if (!sheet.isOutput)
                return qsTr("%1 · A file you added. Removing it from the list "
                            + "does not delete it.").arg(size)
            const made = sheet.file.operation === "encrypt"
                       ? qsTr("Encrypted from %1")
                       : sheet.file.operation === "decrypt"
                         ? qsTr("Decrypted from %1") : qsTr("Signature of %1")
            return qsTr("%1 · %2. A temporary copy GpgFrontend made: save it "
                        + "to keep it. It is deleted when you remove it from "
                        + "the list or close the app.")
                   .arg(size).arg(made.arg(sheet.file.sourceName))
        }
    }

    Label {
        Layout.fillWidth: true
        visible: sheet.file.available === false
        wrapMode: Text.Wrap
        color: Theme.danger
        text: qsTr("This file can no longer be opened. It may have been "
                   + "moved or deleted, or access to it has ended.")
    }

    // How the last operation went, with its full report a tap away.
    ListRow {
        objectName: "fileReport"
        Layout.fillWidth: true
        Layout.leftMargin: -24
        Layout.rightMargin: -24
        leftPadding: 24
        visible: sheet.hasReport
        symbol: Theme.reportSymbol(sheet.file.result || {})
        symbolColor: Theme.reportColor(sheet.file.result || {},
                                       Material.secondaryTextColor)
        text: qsTr("Last result")
        detail: {
            const r = sheet.file.result || {}
            if (r.cancelled === true) return qsTr("Cancelled")
            if (r.status === undefined) return ""
            return r.status > 0 ? qsTr("Done") : r.status === 0
                   ? qsTr("Done, with warnings") : qsTr("Failed")
        }
        onClicked: sheet.reportRequested(sheet.file.result)

        Icon {
            symbol: "chevron_right"
            color: Material.secondaryTextColor
        }
    }

    MenuSeparator {
        Layout.fillWidth: true
        Layout.leftMargin: -24
        Layout.rightMargin: -24
    }

    SheetAction {
        objectName: "detailsSave"
        visible: sheet.isOutput
        symbol: "save"
        text: qsTr("Save as")
        onClicked: sheet.saveRequested(sheet.fileId)
    }

    SheetAction {
        visible: sheet.isOutput && sheet.workspace && sheet.workspace.canShare
        symbol: "share"
        text: qsTr("Share")
        onClicked: sheet.shareRequested(sheet.fileId)
    }

    SheetAction {
        enabled: sheet.usable
        symbol: "lock"
        text: qsTr("Encrypt")
        onClicked: sheet.encryptRequested(sheet.fileId)
    }

    SheetAction {
        enabled: sheet.usable
        symbol: "lock_open_right"
        text: qsTr("Decrypt")
        onClicked: sheet.decryptRequested(sheet.fileId)
    }

    SheetAction {
        enabled: sheet.usable
        symbol: "signature"
        text: qsTr("Sign")
        onClicked: sheet.signRequested(sheet.fileId)
    }

    SheetAction {
        enabled: sheet.usable && sheet.workspace && sheet.workspace.count > 1
        symbol: "verified"
        text: qsTr("Verify")
        detail: sheet.workspace && sheet.workspace.count > 1
                ? "" : qsTr("Add the signed file and its signature first.")
        onClicked: sheet.verifyRequested(sheet.fileId)
    }

    SheetAction {
        objectName: "detailsRemove"
        Layout.bottomMargin: Theme.gapSmall
        enabled: sheet.file.phase !== "running"
        symbol: "delete"
        text: qsTr("Remove from list")
        onClicked: sheet.removeRequested(sheet.fileId)
    }
}

// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Layouts
import GpgFrontend.Mobile

// The files being worked on: those added with the system's picker, and what
// was made of them, in one list. A tap opens a file's details and what can be
// done with it; a long press starts choosing several, to act on them at once.
// GpgFrontend only sees the files added here, and removing one from the list
// never deletes the original.
Page {
    id: page

    // What the page last said in passing (a pop-up, not in the item tree).
    readonly property alias notice: notice
    readonly property alias workspace: files
    readonly property bool selecting: files.selectedCount > 0

    // The files the sheet that is open acts on.
    property var targets: []

    // One step back, as Android's Back asks: out of choosing files.
    function back() {
        if (!selecting) return false
        files.clearSelection()
        return true
    }

    function encrypt(ids) {
        page.targets = ids
        recipients.begin()
    }

    function decrypt(ids) {
        files.decrypt(ids)
        files.clearSelection()
    }

    function sign(ids) {
        page.targets = ids
        signer.begin()
    }

    function share(id) {
        const entry = files.entry(id)
        if (entry.plaintext === true) {
            plaintextNotice.target = id
            plaintextNotice.open()
        } else {
            files.share(id)
        }
    }

    function saveAs(id) {
        saveDialog.target = id
        saveDialog.currentFile = files.entry(id).name
        saveDialog.open()
    }

    function remove(ids) {
        let originals = 0
        for (const id of ids) {
            if (files.entry(id).kind === "external") ++originals
        }
        files.remove(ids)
        if (originals > 0)
            notice.show(qsTr("Removed from the list. The original files are "
                             + "not deleted."))
    }

    // "Shared." only when a target was chosen; a dismissal says nothing.
    function shared(outcome) {
        notice.showShared(outcome, qsTr("This file could not be shared. "
                                        + "Save it instead."))
    }

    // What an operation is called while it runs, and once it has.
    function runningText(operation) {
        switch (operation) {
        case "encrypt": return qsTr("Encrypting…")
        case "decrypt": return qsTr("Decrypting…")
        case "sign": return qsTr("Signing…")
        default: return qsTr("Verifying…")
        }
    }

    function madeText(operation, source) {
        switch (operation) {
        case "encrypt": return qsTr("Encrypted from %1").arg(source)
        case "decrypt": return qsTr("Decrypted from %1").arg(source)
        default: return qsTr("Signature of %1").arg(source)
        }
    }

    // What went wrong, in a line; the engine's own words are in the report.
    function failureText(operation) {
        switch (operation) {
        case "encrypt": return qsTr("Couldn't encrypt. Tap for details.")
        case "decrypt": return qsTr("Couldn't decrypt. Tap for details.")
        case "sign": return qsTr("Couldn't sign. Tap for details.")
        default: return qsTr("Couldn't verify. Tap for details.")
        }
    }

    // The line under a file's name: how large it is and where it came from,
    // or how its last operation went.
    function detailOf(file) {
        if (!file.available) return qsTr("Can no longer be opened")
        switch (file.phase) {
        case "queued": return qsTr("Waiting…")
        case "running": return page.runningText(file.operation)
        case "cancelled": return qsTr("Cancelled")
        case "failed": return page.failureText(file.operation)
        }
        const size = Theme.fileSize(file.size)
        if (file.operation === "verify" && file.phase === "done") {
            const signatures = file.result.signatures || []
            return signatures.length > 0
                   ? results.validityText(signatures[0].validity)
                   : qsTr("No signature found")
        }
        if (file.kind === "output")
            return qsTr("%1 · %2").arg(size)
                   .arg(page.madeText(file.madeBy, file.sourceName))
        return size
    }

    function detailColorOf(file) {
        if (!file.available || file.phase === "failed") return Theme.danger
        if (file.operation === "verify" && file.phase === "done") {
            const signatures = file.result.signatures || []
            if (signatures.length === 0) return Theme.warning
            return signatures[0].verdict === "good" ? Theme.positive
                 : signatures[0].verdict === "warning" ? Theme.warning
                                                       : Theme.danger
        }
        return Material.secondaryTextColor
    }

    // A file's icon: what made it, for an output; a page for a document.
    function symbolOf(file) {
        if (file.kind !== "output") return "draft"
        switch (file.madeBy) {
        case "encrypt": return "lock"
        case "decrypt": return "lock_open_right"
        case "sign": return "signature"
        default: return "draft"
        }
    }

    header: ColumnLayout {
        spacing: 0

        AppBar {
            Layout.fillWidth: true
            title: page.selecting ? qsTr("%1 selected").arg(files.selectedCount)
                                  : qsTr("Files")
            raised: list.contentY > list.originY
            canGoBack: page.selecting
            backIcon: "close"
            backText: qsTr("Done choosing")
            onBack: files.clearSelection()

            BarButton {
                objectName: "encryptSelected"
                visible: page.selecting
                text: qsTr("Encrypt")
                icon.source: Theme.icon("lock")
                onClicked: page.encrypt(files.selectedIds())
            }

            BarButton {
                objectName: "decryptSelected"
                visible: page.selecting
                text: qsTr("Decrypt")
                icon.source: Theme.icon("lock_open_right")
                onClicked: page.decrypt(files.selectedIds())
            }

            BarButton {
                objectName: "removeSelected"
                visible: page.selecting
                text: qsTr("Remove from list")
                icon.source: Theme.icon("delete")
                onClicked: page.remove(files.selectedIds())
            }

            OverflowButton {
                objectName: "filesMore"
                visible: page.selecting || files.count > 0

                MenuItem {
                    text: qsTr("Sign")
                    height: page.selecting ? implicitHeight : 0
                    visible: page.selecting
                    onTriggered: page.sign(files.selectedIds())
                }

                MenuItem {
                    text: qsTr("Select all")
                    height: page.selecting ? implicitHeight : 0
                    visible: page.selecting
                    onTriggered: files.selectAll()
                }

                MenuItem {
                    text: qsTr("Sort by name")
                    checkable: true
                    checked: files.sortKey === "name"
                    height: page.selecting ? 0 : implicitHeight
                    visible: !page.selecting
                    onTriggered: files.sortKey = "name"
                }

                MenuItem {
                    text: qsTr("Sort by size")
                    checkable: true
                    checked: files.sortKey === "size"
                    height: page.selecting ? 0 : implicitHeight
                    visible: !page.selecting
                    onTriggered: files.sortKey = "size"
                }

                MenuItem {
                    text: qsTr("Sort by recently added")
                    checkable: true
                    checked: files.sortKey === "added"
                    height: page.selecting ? 0 : implicitHeight
                    visible: !page.selecting
                    onTriggered: files.sortKey = "added"
                }

                MenuItem {
                    text: qsTr("Stop waiting operations")
                    height: files.busy && !page.selecting ? implicitHeight : 0
                    visible: files.busy && !page.selecting
                    onTriggered: files.cancelQueued()
                }

                MenuItem {
                    text: qsTr("Clear list")
                    height: page.selecting ? 0 : implicitHeight
                    visible: !page.selecting
                    enabled: !files.busy
                    onTriggered: confirmClear.open()
                }
            }
        }

        ProgressBar {
            Layout.fillWidth: true
            visible: files.busy
            indeterminate: true
        }
    }

    FileWorkspace {
        id: files
    }

    // Signals named Signal... have no on-handler in QML.
    Component.onCompleted: {
        files.SignalShared.connect(function(id, outcome) {
            page.shared(outcome)
        })
        files.SignalSaved.connect(function(id, succeeded) {
            notice.show(succeeded ? qsTr("Saved.")
                                  : qsTr("The file could not be saved."))
        })
    }

    // No name filters: Android's picker filters by MIME type, and it has none
    // that covers .asc or .gpg, so a filter greys out the very files it names.
    FileDialog {
        id: addDialog
        fileMode: FileDialog.OpenFiles
        onAccepted: files.addFiles(selectedFiles)
    }

    FileDialog {
        id: saveDialog

        property int target: -1

        fileMode: FileDialog.SaveFile
        onAccepted: files.saveAs(target, selectedFile)
    }

    ListView {
        id: list

        objectName: "fileList"
        anchors.fill: parent
        clip: true
        model: files
        // Room under the last row for the add button.
        footer: Item {
            height: 88
        }

        delegate: ListRow {
            id: row

            required property int fileId
            required property string name
            required property real size
            required property string kind
            required property bool plaintext
            required property string sourceName
            required property string operation
            required property string madeBy
            required property string phase
            required property var result
            required property bool chosen
            required property bool available

            objectName: "file_" + name
            width: ListView.view.width
            singleLineTitle: true
            text: name
            symbol: chosen ? "check_box" : page.symbolOf(row)
            symbolColor: chosen || kind === "output" ? Material.accentColor
                                                       : Material.secondaryTextColor
            detail: page.detailOf(row)
            detailColor: page.detailColorOf(row)
            detailLines: 2
            selected: chosen
            Accessible.description: detail

            onClicked: {
                if (page.selecting) files.setSelected(fileId, !chosen)
                else details.begin(fileId)
            }
            onPressAndHold: files.setSelected(fileId, true)

            BusyIndicator {
                visible: row.phase === "running"
                running: visible
                implicitWidth: Theme.touch
                implicitHeight: Theme.touch
            }

            BarButton {
                id: rowMore

                visible: row.phase !== "running" && !page.selecting
                text: qsTr("Actions for %1").arg(row.name)
                icon.source: Theme.icon("more_vert")
                onClicked: {
                    rowMenu.target = row.fileId
                    rowMenu.popup(rowMore, 0, rowMore.height)
                }
            }
        }

        EmptyState {
            objectName: "emptyFiles"
            // On the list itself, not in its content, which is empty here.
            parent: list
            anchors.centerIn: list
            width: list.width - 64
            visible: files.count === 0
            image: "folder"
            text: qsTr("No files yet")
            detail: qsTr("Add files to encrypt, decrypt, sign or verify them. "
                         + "GpgFrontend only sees the files you add here.")

            Button {
                objectName: "addFilesButton"
                text: qsTr("Add files")
                highlighted: true
                onClicked: addDialog.open()
            }
        }
    }

    Fab {
        objectName: "addFiles"
        visible: !page.selecting && files.count > 0
        text: qsTr("Add files")
        icon.source: Theme.icon("add")
        onClicked: addDialog.open()
    }

    // What can be done with one file, from the three dots on its row.
    Menu {
        id: rowMenu

        property int target: -1
        readonly property var file: target >= 0 ? files.entry(target) : ({})
        readonly property bool usable: file.available === true
                                       && file.phase !== "queued"
        readonly property bool isOutput: file.kind === "output"

        // Opened again for each row: what it knows is read afresh.
        onAboutToShow: {
            const t = target
            target = -1
            target = t
        }

        MenuItem {
            text: qsTr("Encrypt")
            enabled: rowMenu.usable
            onTriggered: page.encrypt([rowMenu.target])
        }
        MenuItem {
            text: qsTr("Decrypt")
            enabled: rowMenu.usable
            onTriggered: page.decrypt([rowMenu.target])
        }
        MenuItem {
            text: qsTr("Sign")
            enabled: rowMenu.usable
            onTriggered: page.sign([rowMenu.target])
        }
        MenuItem {
            text: qsTr("Verify")
            enabled: rowMenu.usable && files.count > 1
            onTriggered: verifier.begin(rowMenu.target)
        }

        MenuSeparator {}

        MenuItem {
            text: qsTr("Save as")
            height: rowMenu.isOutput ? implicitHeight : 0
            visible: rowMenu.isOutput
            onTriggered: page.saveAs(rowMenu.target)
        }
        MenuItem {
            text: qsTr("Share")
            height: rowMenu.isOutput && files.canShare ? implicitHeight : 0
            visible: rowMenu.isOutput && files.canShare
            onTriggered: page.share(rowMenu.target)
        }
        MenuItem {
            text: qsTr("Details")
            onTriggered: details.begin(rowMenu.target)
        }
        MenuItem {
            text: qsTr("Remove from list")
            onTriggered: page.remove([rowMenu.target])
        }
    }

    Snackbar {
        id: notice
        objectName: "fileNotice"
    }

    // Sharing plaintext leaves a copy behind for a while; say so first.
    MessageAlert {
        id: plaintextNotice

        property int target: -1

        objectName: "plaintextNotice"
        title: qsTr("Share the decrypted file?")

        message: qsTr("A copy stays in GpgFrontend's private storage for "
                   + "up to 24 hours so the receiving app can read it. "
                   + "You can remove it sooner under Temporary shared "
                   + "files in Settings.")

        acceptText: qsTr("Share")

        onAccepted: files.share(target)
    }

    MessageAlert {
        id: confirmClear

        objectName: "confirmClearFiles"
        title: qsTr("Clear the list?")
        message: qsTr("Files GpgFrontend made are deleted unless you saved "
                   + "them. The files you added are not deleted.")
        acceptText: qsTr("Clear")
        onAccepted: files.clear()
    }

    FileDetailsSheet {
        id: details

        workspace: files
        onEncryptRequested: id => page.encrypt([id])
        onDecryptRequested: id => page.decrypt([id])
        onSignRequested: id => page.sign([id])
        onVerifyRequested: id => verifier.begin(id)
        onSaveRequested: id => page.saveAs(id)
        onShareRequested: id => page.share(id)
        onRemoveRequested: id => page.remove([id])
        onReportRequested: function(report) {
            results.report = report
            results.open()
        }
    }

    VerifySheet {
        id: verifier
        workspace: files
    }

    RecipientSheet {
        id: recipients

        onConfirmed: function(chosen, signerKey) {
            files.encrypt(page.targets, chosen, signerKey)
            files.clearSelection()
        }
    }

    SignerSheet {
        id: signer

        onConfirmed: function(signerKey) {
            files.sign(page.targets, signerKey)
            files.clearSelection()
        }
    }

    ResultSheet {
        id: results
    }
}

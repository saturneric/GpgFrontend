// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// One file of the Files page, as it is: its name, its size and its SHA-256
// (worked out when the sheet opens, shown short until tapped, always copied
// whole), whether it is a temporary
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
    // The hash shown whole rather than its two ends.
    property bool hashExpanded: false

    readonly property bool isOutput: file.kind === "output"
    // A folder added as one item: no hash, encrypted as one archive.
    readonly property bool isFolder: file.isFolder === true
    readonly property bool temporary: file.origin === "made"
                                      && (file.savedName || "").length === 0
    readonly property bool hashing: sha256.length === 0 && hashError.length === 0
    // Still being copied in (a shared file): hashed once it is all here, as
    // a hash of part of it would be shown as the file's.
    property bool hashDue: false
    readonly property bool copying: (file.progress !== undefined) && file.progress >= 0

    signal saveRequested(int id)
    signal shareRequested(int id)
    signal openRequested(int id)
    signal removeRequested(int id)
    // One that can no longer be opened, picked again from where it is now.
    signal pickAgainRequested(int id)
    // How the last operation on it went, in the status sheet.
    signal resultRequested(int operationId)
    // What the row's menu offers for what it holds, offered here as well.
    signal extractRequested(int id)
    signal importKeysRequested(url file)

    // A TAR archive, offered to extract; read when the sheet opens.
    property bool isArchive: false
    // Not while it is worked on or copied in.
    readonly property bool idle: (file.activity || "").length === 0 && !copying

    // What it is (FileWorkspace.typeOf).
    property var type: ({})

    // The newest operation on it; 0 when none was run.
    property int lastOperation: 0

    function begin(id) {
        sheet.fileId = id
        sheet.file = workspace.entry(id)
        sheet.type = workspace.typeOf(id)
        sheet.sha256 = ""
        sheet.hashError = ""
        sheet.hashExpanded = false
        copied.visible = false
        sheet.lastOperation = OperationQueue.log.latestOnFile(id)
        sheet.isArchive = workspace.isArchive(id)
        sheet.hashDue = sheet.copying
        if (!sheet.hashDue && !sheet.isFolder) workspace.hash(id)
        open()
    }

    // Gone from the list: nothing here is about a file any more.
    function refresh() {
        const file = sheet.workspace.entry(sheet.fileId)
        if (Object.keys(file).length === 0) {
            sheet.close()
            return
        }
        const resized = file.size !== sheet.file.size
        sheet.file = file
        sheet.type = sheet.workspace.typeOf(sheet.fileId)
        if (sheet.isFolder) return
        sheet.lastOperation = OperationQueue.log.latestOnFile(sheet.fileId)
        // All here now, or changed since it was hashed: hashed again.
        if (!sheet.copying && (sheet.hashDue || resized)) {
            sheet.hashDue = false
            sheet.sha256 = ""
            sheet.hashError = ""
            sheet.workspace.hash(sheet.fileId)
        }
    }

    // The file again whenever the workspace changes it, while it is shown.
    Connections {
        target: sheet.workspace
        enabled: sheet.opened

        function onDataChanged() { sheet.refresh() }
        function onRowsRemoved() { sheet.refresh() }

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
    title: isFolder ? qsTranslate("Wording", "Folder") : qsTr("File")

    // Name, type, size and hash, lines of one height: the hash's monospace line
    // is a little taller, and the other two take its height.
    InfoRow {
        objectName: "fileName"
        Layout.fillWidth: true
        Layout.minimumHeight: hashRow.implicitHeight
        label: qsTr("Name")
        value: sheet.file.name || ""
    }

    // What it is: what its bytes said, else what its name suggests, and
    // saying which.
    InfoRow {
        objectName: "fileType"
        Layout.fillWidth: true
        Layout.minimumHeight: hashRow.implicitHeight
        label: qsTr("Type")
        value: Wording.fileType(sheet.type)
    }

    InfoRow {
        objectName: "fileSize"
        Layout.fillWidth: true
        Layout.minimumHeight: hashRow.implicitHeight
        label: qsTr("Size")
        value: Theme.fileSize(sheet.file.size || 0)
    }

    // How much a folder holds, all the way down.
    InfoRow {
        objectName: "folderFiles"
        Layout.fillWidth: true
        Layout.minimumHeight: hashRow.implicitHeight
        visible: sheet.isFolder
        label: qsTr("Contains")
        value: qsTr("%n file(s)", "", sheet.file.fileCount || 0)
    }

    // The hash: worked out, then shown by its two ends (a tap shows it
    // whole), a tap from the clipboard, whole. A folder has none.
    RowLayout {
        Layout.fillWidth: true
        visible: !sheet.isFolder
        spacing: 4

        InfoRow {
            id: hashRow
            objectName: "fileSha256"
            Layout.fillWidth: true
            label: qsTr("SHA-256")
            mono: sheet.sha256.length > 0
            value: sheet.sha256.length > 0
                   ? (sheet.hashExpanded ? sheet.sha256 : Theme.shortHash(sheet.sha256))
                 : sheet.hashError.length > 0 ? sheet.hashError
                 : qsTr("Working it out…")
            valueColor: sheet.hashError.length > 0 ? Theme.danger
                        : sheet.hashing ? Material.secondaryTextColor
                                        : Material.foreground

            TapHandler {
                enabled: sheet.sha256.length > 0
                onTapped: sheet.hashExpanded = !sheet.hashExpanded
            }
        }

        // Its place is kept whatever shows in it: nothing moves when the
        // hash arrives. As wide as a touch target, and no taller than the
        // line beside it: the button reaches past it, so the hash's line is
        // as tall as the name's and the size's.
        Item {
            objectName: "sha256End"
            Layout.alignment: Qt.AlignVCenter
            implicitWidth: Theme.touch
            implicitHeight: 1

            BusyIndicator {
                anchors.centerIn: parent
                width: Theme.touch
                height: Theme.touch
                padding: 10
                running: sheet.hashing && sheet.opened
                visible: running
            }

            BarButton {
                objectName: "copySha256"
                anchors.centerIn: parent
                width: Theme.touch
                height: Theme.touch
                visible: sheet.sha256.length > 0
                text: qsTr("Copy the SHA-256")
                icon.source: Theme.icon("content_copy")
                onClicked: {
                    output.copy(sheet.sha256, false)
                    copied.visible = true
                    copiedFades.restart()
                }
            }
        }
    }

    Label {
        id: copied
        objectName: "fileCopied"

        // Said for a moment, as a copy is said everywhere else.
        Timer {
            id: copiedFades
            interval: 4000
            onTriggered: copied.visible = false
        }
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

    // The way back to it, beside what says it is lost: a file picked from
    // a folder (an output made here is not picked, and is gone for good).
    ActionButton {
        objectName: "pickAgain"
        kind: "text"
        visible: sheet.file.available === false && !sheet.isOutput && !sheet.isFolder
                 && sheet.file.origin !== "shared"
        text: qsTr("Pick it again")
        onClicked: {
            const id = sheet.fileId
            sheet.close()
            sheet.pickAgainRequested(id)
        }
    }

    Label {
        objectName: "folderNote"
        Layout.fillWidth: true
        visible: sheet.isFolder
        wrapMode: Text.Wrap
        color: Material.secondaryTextColor
        font.pixelSize: Theme.secondarySize
        text: qsTr("Encrypting it packs everything in it, as it is then, into "
                   + "one archive: %1.tar.gpg. The folder itself is not changed.")
              .arg(sheet.file.name || "")
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

    // What is done with it: saving first and widest (the one that matters
    // for a temporary file), then sharing and opening as a pair of equals,
    // one under the other with large text or on a narrow screen.
    ColumnLayout {
        objectName: "fileActions"
        Layout.fillWidth: true
        Layout.topMargin: 4
        spacing: Theme.gapSmall
        visible: sheet.isOutput

        ActionButton {
            objectName: "detailsSave"
            Layout.fillWidth: true
            kind: sheet.temporary ? "primary" : "secondary"
            text: qsTr("Save as")
            icon.source: Theme.icon("save")
            onClicked: {
                sheet.close()
                sheet.saveRequested(sheet.fileId)
            }
        }

        GridLayout {
            Layout.fillWidth: true
            visible: sheet.workspace && sheet.workspace.canShare
            // By the window's width, which does not change while the sheet
            // is laid out, never by its own.
            columns: Theme.fontScale > 1.3
                     || (Window.width > 0 && Window.width < 340) ? 1 : 2
            columnSpacing: Theme.gapSmall
            rowSpacing: Theme.gapSmall

            ActionButton {
                objectName: "detailsShare"
                Layout.fillWidth: true
                Layout.preferredWidth: 0
                Layout.minimumWidth: 0
                kind: "secondary"
                text: qsTr("Share")
                icon.source: Theme.icon("share")
                onClicked: {
                    sheet.close()
                    sheet.shareRequested(sheet.fileId)
                }
            }

            ActionButton {
                objectName: "detailsOpenWith"
                Layout.fillWidth: true
                Layout.preferredWidth: 0
                Layout.minimumWidth: 0
                kind: "secondary"
                text: qsTr("Open with…")
                icon.source: Theme.icon("visibility")
                onClicked: {
                    sheet.close()
                    sheet.openRequested(sheet.fileId)
                }
            }
        }
    }

    // What it holds offers more: keys to import, an archive to extract, in
    // the row menu's words.
    ItemDelegate {
        objectName: "detailsImportKeys"
        Layout.fillWidth: true
        Layout.leftMargin: -Theme.sheetPad
        Layout.rightMargin: -Theme.sheetPad
        leftPadding: Theme.sheetPad
        visible: sheet.file.content === "key" && sheet.file.available === true
                 && sheet.idle
        text: qsTranslate("ImportSheet", "Import keys")
        icon.source: Theme.icon("key")
        onClicked: {
            const url = sheet.workspace.readableUrl(sheet.fileId)
            sheet.close()
            if (url.toString().length > 0) sheet.importKeysRequested(url)
        }
    }

    ItemDelegate {
        objectName: "detailsExtract"
        Layout.fillWidth: true
        Layout.leftMargin: -Theme.sheetPad
        Layout.rightMargin: -Theme.sheetPad
        leftPadding: Theme.sheetPad
        visible: sheet.isArchive && sheet.idle
        text: qsTranslate("FilesPage", "Extract to a folder…")
        icon.source: Theme.icon("folder")
        onClicked: {
            sheet.close()
            sheet.extractRequested(sheet.fileId)
        }
    }

    // How it went, from the file: a row that says "Not decrypted" opens
    // why, and its Retry, here.
    ItemDelegate {
        objectName: "detailsResult"
        Layout.fillWidth: true
        Layout.leftMargin: -Theme.sheetPad
        Layout.rightMargin: -Theme.sheetPad
        leftPadding: Theme.sheetPad
        visible: sheet.lastOperation > 0
        // Not a result while it runs, as the status line says it.
        text: sheet.file.phase === "running" || sheet.file.phase === "queued"
              ? qsTranslate("StatusLine", "Show progress")
              : qsTranslate("StatusLine", "Show the result")
        icon.source: Theme.icon("info")
        onClicked: {
            const id = sheet.lastOperation
            sheet.close()
            sheet.resultRequested(id)
        }
    }

    // Apart from the rest: taking it off the list.
    MenuSeparator {
        Layout.fillWidth: true
        Layout.leftMargin: -Theme.sheetPad
        Layout.rightMargin: -Theme.sheetPad
        topPadding: 0
        bottomPadding: 0
    }

    ItemDelegate {
        objectName: "detailsRemove"
        Layout.fillWidth: true
        Layout.leftMargin: -Theme.sheetPad
        Layout.rightMargin: -Theme.sheetPad
        leftPadding: Theme.sheetPad
        // As the list allows it: not while it is worked on or copied in.
        enabled: sheet.file.phase !== "running" && sheet.file.phase !== "queued"
                 && !sheet.copying
        text: qsTr("Remove from list")
        icon.source: Theme.icon("delete")
        icon.color: Theme.danger
        Material.foreground: Theme.danger
        onClicked: {
            sheet.close()
            sheet.removeRequested(sheet.fileId)
        }
    }

    // No Close of its own: Back, a tap beside it or pulling it down put it
    // away, as every sheet does.
}

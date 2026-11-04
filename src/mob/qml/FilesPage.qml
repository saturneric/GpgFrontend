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

    // The sheets the working pages share; null where none is (tests).
    property SheetHost sheets: null

    // What the page last said in passing (a pop-up, not in the item tree).
    readonly property alias notice: notice
    // A passphrase is being asked: a picker now would send the application
    // to the background, and the prompt with it.
    readonly property bool prompting: PassphraseBroker.current.info !== undefined
    readonly property string pickerBlocked: prompting
        ? qsTr("Finish or cancel the passphrase prompt first.") : ""
    // Outputs made here and not saved: gone with the session.
    readonly property int unsaved: files.unsavedCount
    readonly property FileWorkspace workspace: FileWorkspace
    readonly property bool selecting: files.selectedCount > 0
    // The files chosen, read again whenever the choice or a file changes.
    readonly property var selectedEntries: files.selectedCount >= 0
        && filesRevision >= 0 ? files.selectedIds().map(id => files.entry(id)) : []
    // Bumped on every change of the list: a file's state is read by call.
    property int filesRevision: 0

    Connections {
        target: files
        function onDataChanged() { ++page.filesRevision }
        function onRowsInserted() { ++page.filesRevision }
        function onRowsRemoved() { ++page.filesRevision }
        function onModelReset() { ++page.filesRevision }
    }

    // The files the sheet that is open acts on.
    property var targets: []
    // The search field is open in place of the title.
    property bool searching: false
    readonly property alias shown: shown

    // One step back, as Android's Back asks: out of the search (its text
    // first), then out of choosing files.
    function back() {
        if (searching) {
            if (search.text.length > 0) search.clear()
            else closeSearch()
            return true
        }
        if (!selecting) return false
        files.clearSelection()
        return true
    }

    function openSearch() {
        searching = true
        search.forceActiveFocus()
    }

    function closeSearch() {
        search.clear()
        searching = false
    }

    // Whether @p op can be done to all of @p entries, by the desktop's rule
    // for files, by their names: encrypting and signing what is not OpenPGP
    // data, decrypting what is encrypted, verifying OpenPGP data (one file
    // here, matched with its other half by the verify sheet). A file that
    // cannot be opened, or is waiting, takes no new operation.
    function allowed(op, entries) {
        if (entries.length === 0) return false
        const usable = e => e.available === true && e.phase !== "queued"
                            && e.phase !== "running"
        const related = e => (e.content || "").length > 0
        if (!entries.every(usable)) return false
        switch (op) {
        case "encrypt":
        case "encrypt_sign":
        case "symmetric":
        case "sign":
            return entries.every(e => !related(e))
        case "decrypt":
        case "decrypt_verify":
            return entries.every(e => e.content === "message")
        case "verify":
            return entries.length === 1 && related(entries[0])
        default:
            return false
        }
    }

    // Whether More offers @p op (Wording.inMore), by what Settings say.
    function inMore(op) {
        return Wording.inMore(op, prefs.extraOperations, prefs.defaultEncrypt,
                              prefs.defaultDecrypt)
    }

    // @p op on @p ids, as the Text page runs one on its text.
    function request(op, ids) { flow.request(op, ids) }
    function repeatLast(ids) { flow.repeatLast(ids) }

    // Starts @p op on @p ids: false when it could not.
    function start(op, ids) {
        let started = true
        switch (op) {
        case "encrypt":
            started = files.encrypt(ids,
                                    WorkSelection.passphraseMode ? [] : WorkSelection.recipients,
                                    WorkSelection.signsEncryption ? WorkSelection.preferredSigner : "",
                                    WorkSelection.passphraseMode)
            break
        case "decrypt":
        case "decrypt_verify":
            // Decrypt alone, as the desktop's; Decrypt & Verify checks the
            // signatures inside too.
            files.decrypt(ids, op === "decrypt_verify")
            break
        case "sign":
            files.sign(ids, WorkSelection.preferredSigner)
            break
        }
        files.clearSelection()
        if (started && sheets) sheets.trackLatest("files")
        return started
    }

    OperationFlow {
        id: flow
        start: page.start
        // Run on the files still in the list.
        recheck: function(ids) {
            const still = ids.filter(id => files.entry(id).fileId !== undefined)
            return still.length > 0
                ? still : qsTr("Not started: the files are no longer in the list.")
        }
        notice: page.notice
        sheets: page.sheets
    }

    // The system's picker, unless a prompt would be lost to it.
    function addFiles() {
        if (page.prompting) notice.show(page.pickerBlocked)
        else addDialog.open()
    }

    function encrypt(ids) { request("encrypt", ids) }
    function decrypt(ids) { request("decrypt", ids) }
    function sign(ids) { request("sign", ids) }

    function verify(signatureId, originalId) {
        files.verify(signatureId, originalId)
        if (sheets) sheets.trackLatest("files")
    }

    // ---- What the status sheet asks of this page (SheetHost.filesSurface)

    function saveOutput(id) {
        if (files.entry(id).fileId === undefined) {
            notice.show(qsTr("This file was removed from the list."))
            return
        }
        if (page.prompting) {
            notice.show(page.pickerBlocked)
            return
        }
        page.saveAs(id)
    }

    function retry(opId) {
        const why = files.retry(opId)
        if (why === "") {
            if (sheets) sheets.track(OperationQueue.log.latest("files"))
            return true
        }
        const record = OperationQueue.log.record(opId)
        if (why === "unavailable") {
            // Picked again from where it now is.
            relinkDialog.target = record.targets[0].fileId
            relinkDialog.retryId = opId
            notice.show(qsTr("“%1” can no longer be opened. Pick it again.")
                        .arg(record.targets[0].name))
            if (!page.prompting) relinkDialog.open()
        } else if (why === "missing") {
            notice.show(qsTr("Its file is no longer in the list. Add it again."))
        } else if (why === "busy") {
            notice.show(qsTr("It is already waiting or running."))
        }
        return false
    }

    function retryFailed(batchId) {
        const queued = files.retryFailed(batchId)
        if (queued > 0 && sheets) sheets.trackLatest("files")
        else if (queued === 0)
            notice.show(qsTr("None of them can run again as they are. Open "
                             + "each to see why."))
        return queued > 0
    }

    function stopWaiting() {
        files.cancelQueued()
    }

    // Bring the file @p id into view, and mark it for a moment; a filter
    // that hides it is let go of first.
    function showFile(id) {
        if (files.rowOf(id) < 0) return
        if (shown.rowOf(id) < 0) {
            closeSearch()
            shown.reset()
        }
        const row = shown.rowOf(id)
        if (row < 0) return
        list.positionViewAtIndex(row, ListView.Contain)
        list.currentIndex = row
        flash.restart()
    }

    // The newest operation on the file @p id; 0 when none.
    function latestRecordOf(id) {
        return OperationQueue.log.latestOnFile(id)
    }

    function share(id) {
        const entry = files.entry(id)
        if (entry.plaintext === true) {
            plaintextNotice.target = id
            plaintextNotice.opening = false
            plaintextNotice.open()
        } else {
            files.share(id)
        }
    }

    // Another app shows it: the same copy, kept as long, as a share.
    function openWith(id) {
        const entry = files.entry(id)
        if (entry.plaintext === true) {
            plaintextNotice.target = id
            plaintextNotice.opening = true
            plaintextNotice.open()
        } else {
            files.openWith(id)
        }
    }

    // The files made here and never saved, in the list's order.
    function unsavedIds() {
        const ids = []
        for (let row = 0; row < files.count; ++row) {
            const file = files.entry(files.idAt(row))
            if (file.origin === "made" && file.savedName.length === 0) ids.push(file.fileId)
        }
        return ids
    }

    // Save @p ids into a folder the person picks, asked only about the
    // names the folder has already.
    function saveToFolder(ids) {
        if (ids.length === 0) return
        if (page.prompting) {
            notice.show(page.pickerBlocked)
            return
        }
        folderDialog.targets = ids
        folderDialog.open()
    }

    function savedAll(saved, skipped, failed) {
        let text = qsTr("%n file(s) saved.", "", saved)
        if (skipped > 0) text += " " + qsTr("%n skipped.", "", skipped)
        if (failed > 0) text += " " + qsTr("%n could not be saved.", "", failed)
        notice.show(text)
    }

    function saveAs(id) {
        if (page.prompting) {
            notice.show(page.pickerBlocked)
            return
        }
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

    // The line under a file's name: how large it is and where it came from,
    // or how its last operation went.
    function detailOf(file) {
        if (file.progress >= 0)
            return qsTr("Copying in… %1%").arg(Math.round(file.progress * 100))
        if (!file.available) return qsTr("Can no longer be opened")
        switch (file.phase) {
        case "queued": return qsTr("Waiting…")
        case "running": return Wording.running(file.operation)
        case "cancelled": return qsTr("Cancelled")
        case "failed": return qsTr("%1. Tap for details.").arg(Wording.failed(file.operation))
        }
        const size = Theme.fileSize(file.size)
        if (file.operation === "verify" && file.phase === "done") {
            const signatures = file.result.signatures || []
            return signatures.length > 0
                   ? qsTr("Signature: %1").arg(Wording.signatureFact(signatures[0].signature))
                   : qsTr("No signature found")
        }
        if (file.origin === "made") {
            const made = file.madeBy === "encrypt" ? qsTr("Encrypted from %1")
                       : file.madeBy === "decrypt" ? qsTr("Decrypted from %1")
                                                   : qsTr("Signature of %1")
            const kept = file.savedName.length > 0
                         ? qsTr("Saved as %1").arg(file.savedName) : qsTr("Temporary")
            return qsTr("%1 · %2 · %3").arg(size).arg(made.arg(file.sourceName)).arg(kept)
        }
        const holds = Wording.fileContent(file.content)
        const from = file.origin === "shared" ? qsTr("Shared with GpgFrontend")
                                              : qsTr("Added")
        return holds.length > 0 ? qsTr("%1 · %2 · %3").arg(size).arg(holds).arg(from)
                                : qsTr("%1 · %2").arg(size).arg(from)
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
        if (file.kind !== "output") {
            switch (file.content) {
            case "message": return "lock"
            case "signature": return "signature"
            default: return "draft"
            }
        }
        switch (file.madeBy) {
        case "encrypt": return "lock"
        case "decrypt": return "lock_open_right"
        case "sign": return "signature"
        default: return "draft"
        }
    }

    header: ColumnLayout {
        spacing: 0

        // Searching: the field in place of the title, Back to leave it.
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.touch + 8
            Layout.leftMargin: 4
            Layout.rightMargin: Theme.gap
            visible: page.searching
            spacing: 4

            BarButton {
                text: qsTr("Close search")
                icon.source: Theme.icon("arrow_back")
                onClicked: page.closeSearch()
            }

            SearchField {
                id: search

                objectName: "fileSearch"
                Layout.fillWidth: true
                hint: qsTr("Search file names")
            }
        }

        AppBar {
            Layout.fillWidth: true
            visible: !page.searching
            title: page.selecting ? qsTr("%1 selected").arg(files.selectedCount)
                                  : qsTr("Files")
            raised: list.contentY > list.originY
            canGoBack: page.selecting
            backIcon: "close"
            backText: qsTr("Done choosing")
            onBack: files.clearSelection()

            Button {
                objectName: "selectAll"
                visible: files.count > 0
                         && files.selectedCount < shown.count
                flat: true
                text: qsTr("Select all")
                // Those shown: what a filter hides is not chosen unseen.
                onClicked: files.select(shown.ids())
            }

            BarButton {
                objectName: "fileSearchButton"
                visible: !page.selecting && files.count > 0
                text: qsTr("Search")
                icon.source: Theme.icon("search")
                onClicked: page.openSearch()
            }

            BarButton {
                objectName: "filesHistory"
                visible: !page.selecting && page.sheets !== null
                         && page.sheets.revision >= 0 && OperationQueue.log.rowCount() > 0
                text: qsTr("Results")
                icon.source: Theme.icon("info")
                onClicked: page.sheets.openLatest()
            }

            // Adding, here as in the empty list: the bar below is for what
            // is done to the files.
            BarButton {
                objectName: "addFilesBar"
                visible: !page.selecting && files.count > 0
                action: addAction
                icon.source: Theme.icon("add")
            }

            OverflowButton {
                objectName: "filesMenu"
                visible: !page.selecting

                MenuItem {
                    text: qsTr("Sort by name")
                    checkable: true
                    checked: files.sortKey === "name"
                    onTriggered: files.sortKey = "name"
                }
                MenuItem {
                    text: qsTr("Sort by size")
                    checkable: true
                    checked: files.sortKey === "size"
                    onTriggered: files.sortKey = "size"
                }
                MenuItem {
                    text: qsTr("Sort by recently added")
                    checkable: true
                    checked: files.sortKey === "added"
                    onTriggered: files.sortKey = "added"
                }
                MenuItem {
                    text: qsTr("Save all to a folder…")
                    enabled: page.unsaved > 0
                    onTriggered: page.saveToFolder(page.unsavedIds())
                }
                MenuSeparator {}
                MenuItem {
                    text: qsTr("Stop waiting operations")
                    enabled: files.busy
                    onTriggered: files.cancelQueued()
                }
                MenuItem {
                    objectName: "clearList"
                    text: qsTr("Clear list")
                    enabled: !files.busy && files.count > 0
                    onTriggered: confirmClear.open()
                }
            }
        }

        // What kind of file to show: by what their names say.
        ListView {
            id: chips

            objectName: "fileFilters"
            Layout.fillWidth: true
            Layout.preferredHeight: visible ? Math.max(36, Theme.secondarySize * 2.4) + 8 : 0
            // Kept while choosing: the rows do not jump under the finger.
            visible: files.count > 0
            orientation: ListView.Horizontal
            spacing: 8
            leftMargin: Theme.gap
            rightMargin: Theme.gap
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            model: [
                {"value": "all", "text": qsTr("All")},
                {"value": "encrypted", "text": qsTr("Encrypted")},
                {"value": "signatures", "text": qsTr("Signatures")},
                {"value": "made", "text": qsTr("Made here")},
                {"value": "unsaved", "text": qsTr("Not saved")}
            ]

            delegate: Button {
                required property var modelData

                objectName: "fileFilter_" + modelData.value
                anchors.verticalCenter: parent ? parent.verticalCenter : undefined
                text: modelData.text
                flat: shown.category !== modelData.value
                highlighted: shown.category === modelData.value
                checkable: true
                checked: shown.category === modelData.value
                font.pixelSize: Theme.secondarySize
                topInset: 0
                bottomInset: 0
                implicitHeight: Math.max(36, Theme.secondarySize * 2.4)
                onClicked: shown.category = modelData.value
                Accessible.name: qsTr("Show %1").arg(modelData.text)
            }
        }

        // What is made here goes with the session unless it is saved.
        Pane {
            objectName: "unsavedBanner"
            Layout.fillWidth: true
            visible: page.unsaved > 0 && !page.selecting
            padding: 8
            leftPadding: Theme.gap

            background: Rectangle {
                color: Theme.alpha(Theme.warning, 0.12)
            }

            contentItem: RowLayout {
                spacing: 12

                Icon {
                    symbol: "warning"
                    color: Theme.warning
                }

                Label {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    wrapMode: Text.Wrap
                    font.pixelSize: Theme.secondarySize
                    text: page.unsaved === 1
                          ? qsTr("1 file made here is not saved. It is deleted "
                                 + "when the app closes.")
                          : qsTr("%1 files made here are not saved. They are "
                                 + "deleted when the app closes.").arg(page.unsaved)
                }

                Button {
                    objectName: "saveAllButton"
                    flat: true
                    topInset: 0
                    bottomInset: 0
                    text: page.unsaved === 1 ? qsTr("Save") : qsTr("Save all")
                    onClicked: page.saveToFolder(page.unsavedIds())
                    Accessible.description: qsTr("Save the files made here into a folder")
                }
            }
        }
    }


    // The application's one workspace, which outlives this page.
    readonly property FileWorkspace files: FileWorkspace

    // What of it is shown: searched and narrowed.
    FileFilterModel {
        id: shown
        sourceModel: page.files
        searchText: page.searching ? search.text : ""
    }

    function fileShared(id, outcome) { page.shared(outcome) }
    function fileNotReceived(name) {
        notice.show(qsTr("“%1” could not be copied in.").arg(name))
    }
    function fileOpened(id, outcome) {
        // A dismissal says nothing; no app at all, where else to go.
        if (outcome === "failed" || outcome === "unsupported")
            notice.show(qsTr("No app here can open this file. Save it instead."))
    }
    function fileSaved(id, succeeded) {
        notice.show(succeeded ? qsTr("Saved.")
                              : qsTr("The file could not be saved."))
    }

    // Signals named Signal... have no on-handler in QML, not even in a
    // Connections. The workspace outlives the page: let go of it with the
    // page.
    Component.onCompleted: {
        files.SignalShared.connect(page.fileShared)
        files.SignalReceiveFailed.connect(page.fileNotReceived)
        files.SignalOpened.connect(page.fileOpened)
        files.SignalSavedAll.connect(page.savedAll)
        files.SignalSaved.connect(page.fileSaved)
    }
    Component.onDestruction: {
        files.SignalShared.disconnect(page.fileShared)
        files.SignalReceiveFailed.disconnect(page.fileNotReceived)
        files.SignalOpened.disconnect(page.fileOpened)
        files.SignalSavedAll.disconnect(page.savedAll)
        files.SignalSaved.disconnect(page.fileSaved)
    }

    // No name filters: Android's picker filters by MIME type, and it has none
    // that covers .asc or .gpg, so a filter greys out the very files it names.
    FileDialog {
        id: addDialog
        fileMode: FileDialog.OpenFiles
        onAccepted: files.addFiles(selectedFiles)
    }

    // Where several files go at once.
    FolderDialog {
        id: folderDialog

        property var targets: []

        onAccepted: {
            // Chosen and on its way: the choosing is over.
            files.clearSelection()
            const taken = files.takenIn(targets, selectedFolder)
            if (taken.length === 0) {
                files.saveAll(targets, selectedFolder, "keep_both")
                return
            }
            takenDialog.targets = targets
            takenDialog.folder = selectedFolder
            takenDialog.names = taken
            takenDialog.open()
        }
    }

    // Names the folder has already: what to do with them, once for all.
    MessageAlert {
        id: takenDialog

        property var targets: []
        property url folder
        property var names: []

        function answer(how) {
            close()
            files.saveAll(targets, folder, how)
        }

        objectName: "takenDialog"
        title: names.length === 1 ? qsTr("Already in the folder")
                                  : qsTr("%1 names are taken").arg(names.length)

        message: names.length === 1
                 ? qsTr("The folder has a file named “%1”.").arg(names[0] || "")
                 : qsTr("The folder has files of these names: %1").arg(names.join(", "))

        footer: DialogButtonBox {
            alignment: Qt.AlignRight

            Button {
                objectName: "takenSkip"
                flat: true
                text: qsTr("Skip")
                onClicked: takenDialog.answer("skip")
            }
            Button {
                objectName: "takenReplace"
                flat: true
                text: qsTr("Replace")
                onClicked: takenDialog.answer("replace")
            }
            Button {
                objectName: "takenKeepBoth"
                flat: true
                text: qsTr("Keep both")
                onClicked: takenDialog.answer("keep_both")
            }
        }
    }

    FileDialog {
        id: saveDialog

        property int target: -1

        fileMode: FileDialog.SaveFile
        onAccepted: files.saveAs(target, selectedFile)
    }

    // Marks the file shown from the status sheet, for a moment.
    Timer {
        id: flash
        interval: 1500
        onTriggered: list.currentIndex = -1
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

    ListView {
        id: list

        objectName: "fileList"
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        model: shown
        currentIndex: -1
        // Where in a long list one is; finding a file is the search's.
        ScrollIndicator.vertical: ScrollIndicator {}

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
            required property string origin
            required property string savedName
            required property string content
            required property real progress

            objectName: "file_" + name
            width: ListView.view.width
            singleLineTitle: true
            text: name
            symbol: page.symbolOf(row)
            symbolColor: kind === "output" ? Material.accentColor
                                           : Material.secondaryTextColor
            detail: page.detailOf(row)
            detailColor: page.detailColorOf(row)
            detailLines: 2
            selected: chosen || (flash.running && ListView.isCurrentItem)
            Accessible.description: detail

            onClicked: {
                if (page.selecting) files.setSelected(fileId, !chosen)
                else details.begin(fileId)
            }
            onPressAndHold: files.setSelected(fileId, true)

            // Always there: several files are chosen and worked on at once,
            // without a long press first.
            leading: CheckBox {
                objectName: "fileCheck_" + row.name
                checked: row.chosen
                onToggled: files.setSelected(row.fileId, checked)
                Accessible.name: qsTr("Choose %1").arg(row.name)
            }

            BusyIndicator {
                visible: row.phase === "running" || row.progress >= 0
                running: visible
                implicitWidth: Theme.touch
                implicitHeight: Theme.touch
            }

            BarButton {
                id: rowMore

                visible: row.phase !== "running" && row.progress < 0 && !page.selecting
                text: qsTr("Actions for %1").arg(row.name)
                icon.source: Theme.icon("more_vert")
                onClicked: {
                    rowMenu.target = row.fileId
                    rowMenu.popup(rowMore, 0, rowMore.height)
                }
            }
        }

        // Files there are, none of them shown.
        EmptyState {
            objectName: "noMatchingFiles"
            parent: list
            anchors.centerIn: list
            width: list.width - 64
            visible: files.count > 0 && shown.count === 0
            image: "magnifier"
            text: qsTr("No files match")
            detail: page.searching && search.text.length > 0
                    ? qsTr("No file name contains “%1”.").arg(search.text)
                    : qsTr("None of the files is of this kind.")

            Button {
                text: qsTr("Show all files")
                onClicked: {
                    page.closeSearch()
                    shown.reset()
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
                enabled: !page.prompting
                onClicked: page.addFiles()
            }
        }
    }

    StatusLine {
        Layout.fillWidth: true
        host: page.sheets
    }

    // What can be done to the files chosen, as on the Text page: the same
    // bar, each operation only when it can be done to every file chosen
    // (allowed()), as the desktop's file operations. The keys are asked for
    // when an operation needs them (request()).
    SettingsModel {
        id: prefs
    }

    Action {
        id: encryptSelected
        objectName: "encryptSelected"
        // What the button does is chosen in Settings, as on the Text page.
        text: Wording.actionText(Wording.buttonLabel(prefs.defaultEncrypt))
        icon.source: Theme.operationIcon(prefs.defaultEncrypt)
        enabled: page.allowed(prefs.defaultEncrypt, page.selectedEntries)
        onTriggered: page.request(prefs.defaultEncrypt, files.selectedIds())
    }

    Action {
        id: decryptSelected
        objectName: "decryptSelected"
        text: Wording.actionText(Wording.buttonLabel(prefs.defaultDecrypt))
        icon.source: Theme.operationIcon(prefs.defaultDecrypt)
        enabled: page.allowed(prefs.defaultDecrypt, page.selectedEntries)
        onTriggered: page.request(prefs.defaultDecrypt, files.selectedIds())
    }

    Action {
        id: signSelected
        objectName: "signSelected"
        text: qsTr("Sign")
        icon.source: Theme.operationIcon("sign")
        enabled: page.allowed("sign", page.selectedEntries)
        onTriggered: page.sign(files.selectedIds())
    }

    Action {
        id: verifySelected
        objectName: "verifySelected"
        text: qsTr("Verify")
        icon.source: Theme.operationIcon("verify")
        enabled: page.allowed("verify", page.selectedEntries)
        onTriggered: verifier.begin(files.selectedIds()[0])
    }

    OperationBar {
        id: selectionBar

        objectName: "selectionBar"
        Layout.fillWidth: true
        operations: [encryptSelected, decryptSelected, signSelected, verifySelected]
        quickText: page.selecting && page.allowed("encrypt", page.selectedEntries)
                   ? Wording.lastOperation(WorkSelection.lastOperation) : ""
        onQuickTriggered: page.repeatLast(files.selectedIds())

        // What the buttons do not, of what is turned on in Settings
        // (Wording.inMore); else not shown (a hidden menu item still takes
        // its row).
        MenuItem {
            objectName: "plainEncryptSelected"
            text: qsTr("Encrypt")
            icon.source: Theme.operationIcon("encrypt")
            icon.color: "transparent"
            height: page.inMore("encrypt") ? implicitHeight : 0
            visible: height > 0
            enabled: page.allowed("encrypt", page.selectedEntries)
            onTriggered: page.request("encrypt", files.selectedIds())
        }
        MenuItem {
            objectName: "encryptSignSelected"
            text: qsTr("Encrypt && Sign")
            icon.source: Theme.operationIcon("encrypt_sign")
            icon.color: "transparent"
            height: page.inMore("encrypt_sign") ? implicitHeight : 0
            visible: height > 0
            enabled: page.allowed("encrypt_sign", page.selectedEntries)
            onTriggered: page.request("encrypt_sign", files.selectedIds())
        }
        MenuItem {
            objectName: "symmetricSelected"
            text: qsTr("Encrypt with a passphrase")
            icon.source: Theme.operationIcon("symmetric")
            icon.color: "transparent"
            height: page.inMore("symmetric") ? implicitHeight : 0
            visible: height > 0
            enabled: page.allowed("symmetric", page.selectedEntries)
            onTriggered: page.request("symmetric", files.selectedIds())
        }
        MenuItem {
            objectName: "plainDecryptSelected"
            text: qsTr("Decrypt")
            icon.source: Theme.operationIcon("decrypt")
            icon.color: "transparent"
            height: page.inMore("decrypt") ? implicitHeight : 0
            visible: height > 0
            enabled: page.allowed("decrypt", page.selectedEntries)
            onTriggered: page.request("decrypt", files.selectedIds())
        }
        MenuItem {
            objectName: "decryptVerifySelected"
            text: qsTr("Decrypt && Verify")
            icon.source: Theme.operationIcon("decrypt_verify")
            icon.color: "transparent"
            height: page.inMore("decrypt_verify") ? implicitHeight : 0
            visible: height > 0
            enabled: page.allowed("decrypt_verify", page.selectedEntries)
            onTriggered: page.request("decrypt_verify", files.selectedIds())
        }
        MenuSeparator {
            height: ["encrypt", "encrypt_sign", "symmetric", "decrypt",
                     "decrypt_verify"].some(op => page.inMore(op))
                    ? implicitHeight : 0
            visible: height > 0
        }
        MenuItem {
            text: qsTr("Save to a folder…")
            enabled: page.selecting
            onTriggered: page.saveToFolder(files.selectedIds())
        }
        MenuItem {
            objectName: "removeSelected"
            text: qsTr("Remove from list")
            enabled: page.selecting
            onTriggered: page.remove(files.selectedIds())
        }
    }
    }

    Action {
        id: addAction
        text: qsTr("Add files")
        enabled: !page.prompting
        onTriggered: page.addFiles()
    }

    // A file that moved, picked again for a retry.
    FileDialog {
        id: relinkDialog

        property int target: -1
        property int retryId: 0

        fileMode: FileDialog.OpenFile
        onAccepted: {
            if (files.relink(target, selectedFile)) page.retry(retryId)
        }
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

        // The same operations as the bar, by the same rule (allowed()); the
        // ones turned on in Settings only then.
        MenuItem {
            text: qsTr("Encrypt")
            icon.source: Theme.operationIcon("encrypt")
            icon.color: "transparent"
            enabled: page.allowed("encrypt", [rowMenu.file])
            onTriggered: page.encrypt([rowMenu.target])
        }
        MenuItem {
            text: qsTr("Encrypt && Sign")
            icon.source: Theme.operationIcon("encrypt_sign")
            icon.color: "transparent"
            height: prefs.extraOperations.includes("encrypt_sign") ? implicitHeight : 0
            visible: height > 0
            enabled: page.allowed("encrypt_sign", [rowMenu.file])
            onTriggered: page.request("encrypt_sign", [rowMenu.target])
        }
        MenuItem {
            text: qsTr("Encrypt with a passphrase")
            icon.source: Theme.operationIcon("symmetric")
            icon.color: "transparent"
            height: prefs.extraOperations.includes("symmetric") ? implicitHeight : 0
            visible: height > 0
            enabled: page.allowed("symmetric", [rowMenu.file])
            onTriggered: page.request("symmetric", [rowMenu.target])
        }
        MenuItem {
            text: qsTr("Decrypt")
            icon.source: Theme.operationIcon("decrypt")
            icon.color: "transparent"
            enabled: page.allowed("decrypt", [rowMenu.file])
            onTriggered: page.decrypt([rowMenu.target])
        }
        MenuItem {
            text: qsTr("Decrypt && Verify")
            icon.source: Theme.operationIcon("decrypt_verify")
            icon.color: "transparent"
            height: prefs.extraOperations.includes("decrypt_verify") ? implicitHeight : 0
            visible: height > 0
            enabled: page.allowed("decrypt_verify", [rowMenu.file])
            onTriggered: page.request("decrypt_verify", [rowMenu.target])
        }
        MenuItem {
            text: qsTr("Sign")
            icon.source: Theme.operationIcon("sign")
            icon.color: "transparent"
            enabled: page.allowed("sign", [rowMenu.file])
            onTriggered: page.sign([rowMenu.target])
        }
        MenuItem {
            text: qsTr("Verify")
            icon.source: Theme.operationIcon("verify")
            icon.color: "transparent"
            enabled: page.allowed("verify", [rowMenu.file])
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
            text: qsTr("Open with…")
            height: rowMenu.isOutput && files.canShare ? implicitHeight : 0
            visible: rowMenu.isOutput && files.canShare
            onTriggered: page.openWith(rowMenu.target)
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
        // Opened in another app, not shared.
        property bool opening: false

        objectName: "plaintextNotice"
        title: opening ? qsTr("Open the decrypted file in another app?")
                       : qsTr("Share the decrypted file?")

        message: qsTr("A copy stays in GpgFrontend's private storage for "
                   + "up to 24 hours so the receiving app can read it. "
                   + "You can remove it sooner under Temporary shared "
                   + "files in Settings.")

        acceptText: opening ? qsTr("Open") : qsTr("Share")

        onAccepted: opening ? files.openWith(target) : files.share(target)
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
        onOpenRequested: id => page.openWith(id)
        onRemoveRequested: id => page.remove([id])
        onReportRequested: function(id) {
            if (page.sheets) page.sheets.openStatus(page.latestRecordOf(id))
        }
    }

    DetachedVerifySheet {
        id: verifier
        workspace: files
        onVerifyRequested: (signature, original) => page.verify(signature, original)
    }
}

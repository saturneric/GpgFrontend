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

    // Not a stop for a screen reader: a page has no name of its own, only
    // its parts do (Qt gives a Page the role of an unnamed tab).
    Accessible.ignored: true

    // The sheets the working pages share; null where none is (tests).
    property SheetHost sheets: null

    // A key file in the list is to be imported (the Keys' import sheet).
    signal importKeysRequested(url file)

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
        function onFolderAdded(id, problem) { page.folderAdded(id, problem) }
        function onExtracted(id, ok, folder, reason, count) {
            page.extracted(ok, folder, reason, count)
        }
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
    readonly property alias operationMenu: operationMenu
    readonly property alias fileDetails: details
    readonly property alias addChoice: addSheet

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
        if (!entries.every(usable)) return false
        // Verify takes one file here: the verify sheet pairs it.
        if (op === "verify" && entries.length !== 1) return false
        return files.allowsEntries(op, entries.map(e => ({"name": e.name || "",
                                                          "isFolder": e.isFolder === true})))
    }

    // Why nothing could be started on @p ids, as it is: a file that can no
    // longer be opened, one still being copied in, or one at work already.
    function refusal(ids) {
        for (const id of ids) {
            const e = files.entry(id)
            if (e.fileId === undefined)
                return qsTr("This file was removed from the list.")
            if (e.progress >= 0) return qsTr("“%1” is still being copied in.").arg(e.name)
            if (e.available !== true)
                return qsTr("“%1” can no longer be opened. Pick it again.").arg(e.name)
        }
        return qsTr("It is already waiting or running.")
    }

    // @p op on @p ids, as the Text page runs one on its text.
    function request(op, ids) { flow.request(op, ids) }

    // Starts @p op on @p ids: false when it could not.
    function start(op, ids) {
        let started = true
        switch (op) {
        case "encrypt":
            started = files.encrypt(ids,
                                    WorkSelection.passphraseMode ? [] : WorkSelection.operationRecipients(),
                                    WorkSelection.signsEncryption ? WorkSelection.operationSigner() : "",
                                    WorkSelection.passphraseMode)
            break
        case "decrypt":
        case "decrypt_verify":
            // Decrypt alone, as the desktop's; Decrypt & Verify checks the
            // signatures inside too.
            started = files.decrypt(ids, op === "decrypt_verify")
            break
        case "sign":
            started = files.sign(ids, WorkSelection.operationSigner())
            break
        }
        // Nothing queued (each is waiting or running already): said, and
        // the choice kept, never an earlier operation shown as this one.
        if (!started) {
            notice.show(Wording.startRefusal(op) || page.refusal(ids))
            return false
        }
        files.clearSelection()
        if (sheets) sheets.trackLatest("files")
        return true
    }

    OperationFlow {
        id: flow
        start: page.start
        // Run on the files still in the list.
        recheck: function(ids) {
            const still = ids.filter(id => files.entry(id).fileId !== undefined)
            return still.length > 0
                ? {"ok": true, "targets": still}
                : {"ok": false,
                   "message": qsTr("Not started: the files are no longer in the list.")}
        }
        notice: page.notice
        sheets: page.sheets
        ask: function(question, goOn) { encryptQuestion.ask(question, goOn) }
    }

    EncryptQuestionAlert {
        id: encryptQuestion
    }

    // Files or a folder, then the system's picker for it, unless a prompt
    // would be lost to it.
    function addFiles() {
        if (page.prompting) notice.show(page.pickerBlocked)
        else addSheet.open()
    }

    // A folder added as one item, or why it was not.
    function folderAdded(id, problem) {
        if (problem === "unreadable")
            notice.show(qsTr("This folder could not be read."))
        else if (problem === "too_large")
            notice.show(qsTr("This folder holds too many files or folders to "
                             + "encrypt as one."))
        else if (id >= 0)
            page.showFile(id)
    }

    // An archive extracted into a folder, or why nothing was.
    function extracted(ok, folder, reason, count) {
        if (ok)
            notice.show(qsTr("%n file(s) extracted into “%1”.", "", count).arg(folder))
        else if (reason.length > 0)
            notice.show(qsTr("Nothing was extracted. %1").arg(reason))
        else
            notice.show(qsTr("Extracting was stopped. Nothing was kept."))
    }

    // Extract the archive @p id into a folder the person picks.
    function extract(id) {
        if (page.prompting) {
            notice.show(page.pickerBlocked)
            return
        }
        extractDialog.archiveId = id
        extractDialog.open()
    }

    function encrypt(ids) { request("encrypt", ids) }
    function decrypt(ids) { request("decrypt", ids) }
    function sign(ids) { request("sign", ids) }

    function verify(signatureId, originalId) {
        if (!files.verify(signatureId, originalId)) {
            notice.show(page.refusal(originalId > 0 ? [signatureId, originalId]
                                                    : [signatureId]))
            return
        }
        // Started: the choosing is over, as for every operation.
        files.clearSelection()
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

    // The file @p id, which can no longer be opened, picked again.
    function pickAgain(id) {
        if (page.prompting) {
            notice.show(page.pickerBlocked)
            return
        }
        relinkDialog.target = id
        relinkDialog.retryId = 0
        relinkDialog.open()
    }

    function retry(opId) {
        const why = files.retry(opId)
        if (why === "") {
            if (sheets) sheets.track(OperationQueue.log.latest("files"))
            return true
        }
        const record = OperationQueue.log.record(opId)
        if (why === "unavailable") {
            // Picked again from where it now is: the one that cannot be
            // opened, which for a detached signature may be its data file.
            const target = record.targets.find(t => files.entry(t.fileId).available === false)
                           || record.targets[0]
            relinkDialog.target = target.fileId
            relinkDialog.retryId = opId
            notice.show(qsTr("“%1” can no longer be opened. Pick it again.")
                        .arg(target.name))
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

    // What waits of the batch @p batchId; of every batch with none.
    function stopWaiting(batchId) {
        files.cancelQueued(batchId || 0)
    }

    // Bring the file @p id into view, and mark it for a moment; a filter
    // that hides it is let go of first.
    function showFile(id) {
        if (files.rowOf(id) < 0) {
            notice.show(qsTr("This file was removed from the list."))
            return
        }
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
        const chosen = files.selectedIds()
        folderDialog.fromSelection = chosen.length === ids.length
                                     && ids.every(id => chosen.includes(id))
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

    // Takes @p ids off the list. One being worked on or copied in stays (the
    // list would not let it go); a file made here and not saved is asked
    // about first, as it is deleted with it.
    function remove(ids, confirmed) {
        const removable = ids.filter(id => {
            const e = files.entry(id)
            return e.fileId !== undefined && e.phase !== "running"
                   && e.phase !== "queued" && !(e.progress >= 0)
        })
        if (removable.length === 0) {
            notice.show(page.refusal(ids))
            return
        }
        if (!confirmed && removable.some(id => {
            const e = files.entry(id)
            return e.origin === "made" && (e.savedName || "").length === 0
        })) {
            confirmRemove.targets = removable
            confirmRemove.open()
            return
        }
        // A copy another app shared is GpgFrontend's own: it goes; only a
        // file picked from the device stays where it is.
        let originals = 0
        for (const id of removable) {
            const e = files.entry(id)
            if (e.kind === "external" && e.origin !== "shared") ++originals
        }
        files.remove(removable)
        if (originals > 0)
            notice.show(qsTr("Removed from the list. The original files are "
                             + "not deleted."))
    }

    // "Shared." only when a target was chosen; a dismissal says nothing.
    function shared(outcome) {
        notice.showShared(outcome, qsTr("This file could not be shared. "
                                        + "Save it instead."))
    }

    // The line under a file's name: what is happening to it, or how large
    // it is and, for a file made here, whether it is saved yet. Nothing on
    // where it came from: the name says what it is.
    function detailOf(file) {
        if (file.progress >= 0) {
            const percent = Math.round(file.progress * 100)
            if (file.activity === "packing")
                return qsTr("Packing the folder… %1%").arg(percent)
            if (file.activity === "extracting")
                return qsTr("Extracting… %1%").arg(percent)
            return qsTr("Copying in… %1%").arg(percent)
        }
        if (!file.available) return qsTr("Can no longer be opened")
        switch (file.phase) {
        case "queued": return qsTr("Waiting…")
        case "running": return Wording.running(file.operation)
        case "cancelled": return qsTr("Cancelled")
        case "failed":
            return page.failureText(file)
        }
        const size = Theme.fileSize(file.size)
        if (file.isFolder)
            return qsTr("Folder · %n file(s) · %1", "", file.fileCount).arg(size)
        // A verification, or a decryption that checked signatures too: the
        // worst of them, as the line's colour (detailColorOf()).
        const signatures = file.phase === "done" ? (file.result.signatures || []) : []
        if (file.phase === "done" && (file.operation === "verify" || signatures.length > 0)) {
            return signatures.length > 0
                   ? qsTr("%1 · Signature: %2").arg(size)
                         .arg(Wording.signatureSummary(Wording.worstSignature(signatures)))
                   : qsTr("%1 · No signature found").arg(size)
        }
        if (file.origin === "made" && file.savedName.length === 0)
            return qsTr("%1 · Temporary, not saved").arg(size)
        return size
    }

    // A failed file's line: what did not happen, and why when the result
    // says (by its kind, never by matching the engine's words).
    function failureText(file) {
        // By its kind; else the app's own words for it (a file that could
        // not be read), as the status sheet says them.
        const result = file.result || {}
        const why = Wording.failureReason(result.reason || "")
                    || (result.reason ? "" : (result.description || ""))
        return why.length > 0 ? qsTr("%1 · %2").arg(Wording.failed(file.operation)).arg(why)
                              : Wording.failed(file.operation)
    }

    function detailColorOf(file) {
        if (!file.available || file.phase === "failed") return Theme.danger
        const signatures = file.phase === "done" ? (file.result.signatures || []) : []
        if (file.phase === "done" && (file.operation === "verify" || signatures.length > 0)) {
            if (signatures.length === 0) return Theme.warning
            // Every signature, each by its fact and its grade.
            return Theme.signaturesTone(signatures)
        }
        if (file.origin === "made" && file.savedName.length === 0) return Theme.warning
        return Material.secondaryTextColor
    }

    // A file's icon, by what it holds (its name, confirmed by its first
    // bytes), whether it was added or made here.
    function symbolOf(file) {
        if (file.isFolder) return "folder"
        switch (file.content) {
        case "message": return "lock"
        case "signature": return "signature"
        case "key": return "key"
        default: return "draft"
        }
    }

    // Why @p op cannot be done to the files chosen; empty when it can.
    function reasonFor(op) {
        const entries = page.selectedEntries
        if (page.allowed(op, entries)) return ""
        const usable = e => e.available === true && e.phase !== "queued"
                            && e.phase !== "running"
        if (!entries.every(usable))
            return qsTr("A chosen file is busy or can no longer be opened.")
        switch (op) {
        case "decrypt":
        case "decrypt_verify":
            return qsTr("Only for encrypted files.")
        case "verify":
            return qsTr("For one signature or signed file at a time.")
        default:
            return qsTr("Not for files that are OpenPGP data already.")
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

            // While choosing: what is done with the chosen files apart from
            // operations (those are the PGP button's).
            BarButton {
                objectName: "saveSelected"
                visible: page.selecting
                text: qsTr("Save to a folder")
                icon.source: Theme.icon("save")
                onClicked: page.saveToFolder(files.selectedIds())
            }

            BarButton {
                objectName: "removeSelected"
                visible: page.selecting
                text: qsTr("Remove from list")
                icon.source: Theme.icon("delete")
                onClicked: page.remove(files.selectedIds())
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
                MenuItem {
                    objectName: "recentOperations"
                    text: qsTranslate("HistorySheet", "Recent operations")
                    enabled: page.sheets !== null
                    onTriggered: page.sheets.openHistory()
                }
                MenuSeparator {}
                MenuItem {
                    text: qsTranslate("StatusSheet", "Stop waiting files")
                    enabled: files.busy
                    onTriggered: files.cancelQueued()
                }
                MenuItem {
                    objectName: "clearList"
                    text: qsTr("Clear list")
                    enabled: !files.busy && files.count > 0
                    // Asked only when it would delete something not saved,
                    // as Remove does.
                    onTriggered: {
                        if (page.unsaved > 0) {
                            confirmClear.open()
                        } else {
                            files.clear()
                            notice.show(qsTr("Removed from the list. The original "
                                             + "files are not deleted."))
                        }
                    }
                }
            }
        }

        // What is made here goes with the session unless it is saved.
        Pane {
            objectName: "unsavedBanner"
            Layout.fillWidth: true
            // Also while choosing: the list under it never moves for that.
            visible: page.unsaved > 0
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
                    // As it happens: removed from the list, or when the
                    // app closes (or, after a crash, when it next starts).
                    // A plural: languages count more forms than two.
                    text: qsTr("%n temporary file(s) not saved yet. Deleted "
                               + "when removed from the list or when the app "
                               + "closes.", "", page.unsaved)
                }

                Button {
                    objectName: "saveAllButton"
                    flat: true
                    topInset: 0
                    bottomInset: 0
                    text: page.unsaved === 1 ? qsTr("Save") : qsTr("Save all")
                    // One file is saved as one is everywhere else (a place
                    // and a name); several, into a folder.
                    onClicked: {
                        const ids = page.unsavedIds()
                        if (ids.length === 1) page.saveAs(ids[0])
                        else page.saveToFolder(ids)
                    }
                    Accessible.description: qsTr("Save the temporary files into a folder")
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
        // What a search hides is not chosen unseen: the count is not shown
        // while searching, and an operation would act on files out of sight.
        onSearchTextChanged: Qt.callLater(page.keepChosenInSight)
    }

    function keepChosenInSight() {
        for (const id of files.selectedIds()) {
            if (shown.rowOf(id) < 0) files.setSelected(id, false)
        }
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
    OpenDialog {
        id: addDialog
        multiple: true
        onAccepted: files.addFiles(selectedFiles)
    }

    // A folder, added as one item.
    FolderDialog {
        id: addFolderDialog
        onAccepted: files.addFolder(selectedFolder)
    }

    // Where an archive is extracted: a new folder in the one picked.
    FolderDialog {
        id: extractDialog

        property int archiveId: -1

        onAccepted: {
            if (!files.extract(archiveId, selectedFolder))
                notice.show(qsTr("It cannot be extracted now. Try again once "
                                 + "the current one is done."))
        }
    }

    // Files one by one, or all of a folder's.
    BottomSheet {
        id: addSheet

        objectName: "addSheet"
        title: qsTranslate("AddUidSheet", "Add")
        inset: 0

        ListRow {
            objectName: "addPickFiles"
            Layout.fillWidth: true
            leftPadding: Theme.sheetPad
            symbol: "note_add"
            symbolColor: Material.accentColor
            text: qsTr("Files")
            detail: qsTr("Choose one or more files.")
            onClicked: {
                addSheet.close()
                if (page.prompting) notice.show(page.pickerBlocked)
                else addDialog.open()
            }
        }

        ListRow {
            objectName: "addPickFolder"
            Layout.fillWidth: true
            Layout.bottomMargin: Theme.gapSmall
            leftPadding: Theme.sheetPad
            symbol: "folder"
            symbolColor: Material.accentColor
            text: qsTr("A folder")
            detail: qsTr("Added as one item, with everything in it. Encrypting "
                         + "it makes one .tar.gpg archive.")
            onClicked: {
                addSheet.close()
                if (page.prompting) notice.show(page.pickerBlocked)
                else addFolderDialog.open()
            }
        }
    }

    // Where several files go at once.
    FolderDialog {
        id: folderDialog

        property var targets: []
        // Asked for the files chosen: the choosing is over once it goes.
        property bool fromSelection: false

        onAccepted: {
            const taken = files.takenIn(targets, selectedFolder)
            if (taken.length === 0) {
                if (fromSelection) files.clearSelection()
                files.saveAll(targets, selectedFolder, "keep_both")
                return
            }
            // The choosing is over once they are saved: Cancel at the
            // question keeps it, for another folder.
            takenDialog.fromSelection = fromSelection
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
        property bool fromSelection: false

        function answer(how) {
            close()
            if (fromSelection) files.clearSelection()
            files.saveAll(targets, folder, how)
        }

        objectName: "takenDialog"
        // Several: a plural, for the languages with more than two forms.
        title: names.length === 1 ? qsTr("Already in the folder")
                                  : qsTr("%n name(s) taken", "", names.length)

        message: names.length === 1
                 ? qsTr("The folder has a file named “%1”.").arg(names[0] || "")
                 : qsTr("The folder has files of these names: %1")
                       .arg(names.slice(0, 5).join(", ") + (names.length > 5 ? ", …" : ""))

        footer: DialogButtonBox {
            alignment: Qt.AlignRight

            // Nothing saved, said by a button, not only by Back.
            Button {
                objectName: "takenCancel"
                flat: true
                text: qsTranslate("MessageAlert", "Cancel")
                onClicked: takenDialog.close()
            }
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

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: list

                objectName: "fileList"
                anchors.fill: parent
                clip: true
                model: shown
                currentIndex: -1
                // The last row scrolls clear of the buttons at the foot.
                bottomMargin: Theme.fabBand
                // Where in a long list one is; finding a file is the search's.
                ScrollIndicator.vertical: ScrollIndicator {}

                delegate: ListRow {
                    id: row

                    required property int fileId
                    required property string name
                    required property real size
                    required property string kind
                    required property bool plaintext
                    required property string operation
                    required property string phase
                    required property var result
                    required property bool chosen
                    required property bool available
                    required property string origin
                    required property string savedName
                    required property string content
                    required property real progress
                    required property bool isFolder
                    required property int fileCount
                    required property string activity

                    readonly property bool working: phase === "running" || progress >= 0

                    objectName: "file_" + name
                    width: ListView.view.width
                    singleLineTitle: true
                    text: name
                    symbol: page.symbolOf(row)
                    symbolColor: Material.secondaryTextColor
                    symbolSize: 28
                    symbolCentered: true
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

                    // Always there: several files are chosen and worked on at
                    // once, without a long press first.
                    leading: CheckBox {
                        objectName: "fileCheck_" + row.name
                        checked: row.chosen
                        onToggled: files.setSelected(row.fileId, checked)
                        Accessible.name: qsTr("Choose %1").arg(row.name)
                    }

                    // One slot at the end, always as wide: the actions, or
                    // the progress while it works. Choosing files changes
                    // neither the width of the text nor the row's height.
                    Item {
                        objectName: "fileRowEnd"
                        implicitWidth: Theme.touch
                        implicitHeight: Theme.touch

                        BusyIndicator {
                            anchors.fill: parent
                            visible: row.working
                            running: visible
                        }

                        BarButton {
                            id: rowMore

                            objectName: "fileRowMore"
                            anchors.fill: parent
                            opacity: row.working || page.selecting ? 0 : 1
                            enabled: !row.working && !page.selecting
                            // Hidden, not there for TalkBack either.
                            Accessible.ignored: row.working || page.selecting
                            text: qsTr("Actions for %1").arg(row.name)
                            icon.source: Theme.icon("more_vert")
                            onClicked: {
                                rowMenu.target = row.fileId
                                rowMenu.popup(rowMore, 0, rowMore.height)
                            }
                        }
                    }
                }

                // Files there are, none of them found.
                EmptyState {
                    objectName: "noMatchingFiles"
                    parent: list
                    anchors.centerIn: list
                    width: list.width - 64
                    visible: files.count > 0 && shown.count === 0
                    symbol: "search"
                    text: qsTr("No files match")
                    detail: qsTr("No file name contains “%1”.").arg(search.text)
                }

                // Nothing yet: what the page is for. Adding is the + button's.
                EmptyState {
                    objectName: "emptyFiles"
                    // On the list itself, not in its content, empty here.
                    parent: list
                    anchors.centerIn: list
                    width: list.width - 64
                    visible: files.count === 0
                    symbol: "folder"
                    text: qsTr("No files yet")
                    detail: qsTr("Use + to add files to encrypt, decrypt, sign or "
                                 + "verify. GpgFrontend only sees the files you add "
                                 + "here.")
                }
            }

            // The PGP operations in the corner, the page's main action, once
            // there are files; adding files beside it on the left, or in the
            // corner itself while there are none.
            Fab {
                id: addButton

                objectName: "addFilesButton"
                offsetX: operationButton.visible
                         ? operationButton.width + Theme.gapSmall : 0
                text: qsTr("Add files")
                icon.source: Theme.icon("add")
                enabled: !page.prompting
                onClicked: page.addFiles()
            }

            Fab {
                id: operationButton

                objectName: "operationButton"
                visible: files.count > 0
                text: qsTr("PGP operations")
                icon.source: Theme.icon("lock")
                onClicked: operationMenu.openAbove(operationButton)
            }
        }

        StatusLine {
            Layout.fillWidth: true
            host: page.sheets
        }
    }

    // The operations, on the files chosen, each only when it can be done to
    // every one of them (allowed()), as the desktop's file operations; the
    // keys are asked for when an operation needs them (request()). Never on
    // files nobody chose: Select all is one tap away.
    SettingsModel {
        id: prefs
    }

    OperationMenu {
        id: operationMenu

        unavailable: !page.selecting ? qsTr("Select files first, or use Select all.")
                                     : ""
        reasonFor: op => page.reasonFor(op)
        symmetric: prefs.extraOperations.includes("symmetric")
        onRequested: function(op) {
            if (op === "verify") verifier.begin(files.selectedIds()[0])
            else page.request(op, files.selectedIds())
        }
    }

    // A file that moved, picked again for a retry.
    OpenDialog {
        id: relinkDialog

        property int target: -1
        property int retryId: 0

        onAccepted: {
            if (files.relink(target, selectedFile)) {
                if (retryId > 0) page.retry(retryId)
            }
            else if (files.entry(target).fileId === undefined)
                notice.show(qsTr("Its file is no longer in the list. Add it again."))
            else notice.show(qsTr("It is already waiting or running."))
        }
    }

    // What is done with one file, from the three dots on its row: managing
    // it. The PGP operations are the PGP button's.
    FitMenu {
        id: rowMenu

        property int target: -1
        readonly property var file: target >= 0 ? files.entry(target) : ({})
        readonly property bool isOutput: file.kind === "output"
        // A TAR archive (a folder decrypted, say), offered to extract;
        // never extracted by itself.
        property bool isArchive: false

        // Opened again for each row: what it knows is read afresh.
        onAboutToShow: {
            const t = target
            target = -1
            target = t
            isArchive = files.isArchive(t) && (file.activity || "").length === 0
        }

        // A key file -- shared from another app, say, which the system's
        // picker cannot reach -- imported from here.
        MenuItem {
            objectName: "rowImportKeys"
            text: qsTranslate("ImportSheet", "Import keys")
            readonly property bool offered: rowMenu.file.content === "key"
                                            && rowMenu.file.available === true
                                            && (rowMenu.file.activity || "").length === 0
            height: offered ? implicitHeight : 0
            visible: offered
            onTriggered: {
                const url = files.readableUrl(rowMenu.target)
                if (url.toString().length > 0) page.importKeysRequested(url)
            }
        }

        MenuItem {
            objectName: "rowExtract"
            text: qsTr("Extract to a folder…")
            height: rowMenu.isArchive ? implicitHeight : 0
            visible: rowMenu.isArchive
            onTriggered: page.extract(rowMenu.target)
        }

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
        // Above the buttons at the foot, never over them.
        bottomOffset: Theme.fabBand - 12
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
                   + "up to an hour so the receiving app can read it. "
                   + "You can remove it sooner under Temporary shared "
                   + "files in Settings.")

        acceptText: opening ? qsTr("Open") : qsTr("Share")

        onAccepted: opening ? files.openWith(target) : files.share(target)
    }

    // Removing a file made here and not saved deletes it.
    MessageAlert {
        id: confirmRemove

        property var targets: []

        objectName: "confirmRemoveFiles"
        title: qsTr("Remove from the list?")
        message: qsTr("Files GpgFrontend made are deleted unless you saved "
                   + "them. The files you added are not deleted.")
        acceptText: qsTr("Remove from list")
        destructive: true
        onAccepted: page.remove(targets, true)
    }

    MessageAlert {
        id: confirmClear

        objectName: "confirmClearFiles"
        title: qsTr("Clear the list?")
        message: qsTr("Files GpgFrontend made are deleted unless you saved "
                   + "them. The files you added are not deleted.")
        acceptText: qsTr("Clear")
        destructive: true
        onAccepted: files.clear()
    }

    FileDetailsSheet {
        id: details

        workspace: files
        onSaveRequested: id => page.saveAs(id)
        onShareRequested: id => page.share(id)
        onOpenRequested: id => page.openWith(id)
        onRemoveRequested: id => page.remove([id])
        onPickAgainRequested: id => page.pickAgain(id)
        onExtractRequested: id => page.extract(id)
        onImportKeysRequested: url => page.importKeysRequested(url)
        onResultRequested: id => { if (page.sheets) page.sheets.openStatus(id, true) }
    }

    DetachedVerifySheet {
        id: verifier
        workspace: files
        onVerifyRequested: (signature, original) => page.verify(signature, original)
    }
}

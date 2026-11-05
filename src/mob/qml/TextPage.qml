// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// A text editor that encrypts, decrypts, signs and verifies what it holds.
//
// One editor. The operations are in the menu of the page's operation button;
// the keys an operation needs are asked for when it runs (the key toolbox).
// When an operation makes text -- a message, a signed text, what a message
// said -- that text takes the place of the text it was made from, as one
// step Undo takes back and Redo brings again; a verification, a failure or
// a cancellation leaves the text as it is.
//
// A result belongs to the operation that made it, the page's session and the
// text it read (docRevision): text changed meanwhile is never overwritten
// unasked, and text wiped (decrypted text cleared) never comes back.
Page {
    id: page

    // The sheets the working pages share; null where none is (tests).
    property SheetHost sheets: null

    // What the page last said in passing (a pop-up, not in the item tree).
    readonly property alias notice: notice
    readonly property alias editor: editor
    readonly property alias operationMenu: operationMenu
    // Asked before shared text takes the place of text there (a pop-up).
    readonly property alias confirmReplace: confirmReplace

    // Bumped by every change of the text, however it came: typing, paste,
    // Undo, Redo, Clear, shared text, a result, a wipe.
    property int docRevision: 0
    // Renewed when decrypted text is wiped: what was asked before belongs to
    // text that is gone.
    property int sessionId: 1
    // The operation this page started and waits for:
    // {id, session, revision}; null when none.
    property var pendingRun: null
    // A result kept back because the text changed meanwhile, offered with
    // "Replace anyway"; 0 when none.
    property int offeredId: 0
    // The text set by the page, not typed: not something the person did.
    property bool programmatic: false
    // Line numbers, on request.
    property bool lineNumbers: false

    // What the text on the page is: an armored block is shown in monospace
    // and wraps anywhere.
    readonly property string kind: ContentKind.textKind(editor.text)
    readonly property bool armored: kind !== "plain" && kind !== "empty"
    readonly property bool isKey: kind === "public_key" || kind === "private_key"

    // How much of the editor the on-screen keyboard covers. The window does
    // not make room for the keyboard (it is not one of its safe areas), so
    // the editor's foot can scroll above it instead. Only room: the
    // navigation and everything else stay as they are, whatever the input
    // method says (it has been seen to say "shown" after the keyboard went).
    readonly property real keyboardCover: {
        const cover = Theme.keyboard
        if (cover <= 0 || !Window.window) return 0
        const foot = page.mapToItem(null, 0, page.height).y
        return Math.max(0, cover - (Window.height - foot))
    }

    // Where each line starts in the text, for the line numbers. Past this
    // many lines the numbers are left out: laying them out costs more than
    // they help.
    readonly property int maxNumberedLines: 2000
    readonly property var lineStarts: {
        const starts = [0]
        const text = editor.text
        for (let i = text.indexOf("\n"); i >= 0 && starts.length <= maxNumberedLines;
             i = text.indexOf("\n", i + 1)) {
            starts.push(i + 1)
        }
        return starts
    }

    signal importRequested(string text)
    signal openFilesRequested()

    // Nothing to step back from inside the page: Back goes on.
    function back() {
        return false
    }

    // ---- What the status sheet asks of this page (SheetHost.textSurface)

    function holds(id) {
        return crypto.holds(id)
    }

    // "input" or "output" when the text on the page is operation @p id's.
    function relation(id) {
        return OperationQueue.log.textRelation(id, editor.text)
    }

    // Operation @p id again, on the text it read. Its result takes the
    // place of the text only while the text on the page is still that one.
    function retry(id) {
        const onPage = relation(id) === "input"
        if (!crypto.retry(id)) return false
        track(onPage ? docRevision : -1)
        return true
    }

    // ---- Running operations

    // @p text in place of the text, as one step Undo takes back.
    function replaceText(text) {
        programmatic = true
        history.replaceAll(editor.textDocument, text)
        programmatic = false
        editor.cursorPosition = 0
        flick.contentY = 0
    }

    // Everything gone, Undo included: decrypted text is not brought back.
    // An operation still running keeps its record: it belongs to the old
    // session, and what it makes is dropped when it ends.
    function wipe() {
        ++sessionId
        dropOffer()
        programmatic = true
        editor.text = ""
        programmatic = false
        history.forget(editor.textDocument)
        crypto.clear()
    }

    // The operation just started is the page's to wait for; its result
    // replaces the text only while the text is still at @p revision.
    function track(revision) {
        pendingRun = {"id": crypto.operationId, "session": sessionId,
                      "revision": revision}
        if (sheets) sheets.track(crypto.operationId)
    }

    // Starts @p op on @p target ({text, revision}): false when it could not.
    function start(op, target) {
        const text = target.text
        let started = false
        switch (op) {
        case "encrypt":
            started = crypto.encrypt(text,
                                     WorkSelection.passphraseMode ? [] : WorkSelection.recipients,
                                     WorkSelection.signsEncryption ? WorkSelection.preferredSigner : "",
                                     WorkSelection.passphraseMode)
            break
        case "decrypt":
        case "decrypt_verify":
            // Decrypt alone, as the desktop's; Decrypt & Verify checks the
            // signatures inside too.
            started = crypto.decrypt(text, op === "decrypt_verify")
            break
        case "sign":
            started = crypto.sign(text, WorkSelection.preferredSigner)
            break
        default:
            started = crypto.verify(text)
        }
        if (started) track(target.revision)
        return started
    }

    // What an operation runs on: the text as it is now.
    function target() {
        return {"text": editor.text, "revision": docRevision}
    }

    // @p op on the text, as the Files page runs one on files.
    function request(op) { flow.request(op, target()) }
    function repeatLast() { flow.repeatLast(target()) }

    OperationFlow {
        id: flow
        start: page.start
        // Run on the very text it was asked for, or not at all.
        recheck: function(target) {
            return target.revision === page.docRevision
                   ? {"ok": true, "targets": target}
                   : {"ok": false, "message": qsTr("Not started: the text changed.")}
        }
        notice: page.notice
        sheets: page.sheets
    }

    function finished(succeeded) {
        const id = crypto.finishedId
        const run = pendingRun !== null && pendingRun.id === id ? pendingRun : null
        if (run) pendingRun = null
        // Asked for text that has since been wiped: what it made goes too.
        if (run && run.session !== sessionId) {
            crypto.clear()
            return
        }
        // Not one this page asked for and waits for: nothing to put in.
        if (!run) return
        if (!crypto.replacesText) {
            if (sheets === null && !succeeded) {
                // No status sheet to say it: a word at the foot.
                const result = crypto.result
                notice.show(result.cancelled === true ? qsTr("Cancelled.")
                            : (result.description || qsTr("It did not work.")))
            }
            return
        }
        if (run.revision === docRevision) {
            replaceText(crypto.output)
            return
        }
        offer(id)
    }

    // The text changed while operation @p id ran: its result is kept and
    // offered, never put in unasked.
    function offer(id) {
        dropOffer()
        if (!crypto.hold(id)) return
        offeredId = id
        const session = sessionId
        notice.show(qsTr("The text changed while it was being worked on, so "
                         + "the result was not put in."),
                    qsTr("Replace anyway"),
                    function() {
                        const text = crypto.take(id)
                        if (page.offeredId === id) page.offeredId = 0
                        if (session === page.sessionId) page.replaceText(text)
                    })
    }

    function dropOffer() {
        if (offeredId === 0) return
        crypto.release(offeredId)
        offeredId = 0
    }

    // Text another application shared: in place of an empty page, or, over
    // text there is, once the person agrees.
    function receive(text) {
        if (text.length === 0) return
        if (editor.text.trim().length === 0) {
            placeShared(text)
            return
        }
        confirmReplace.pending = text
        confirmReplace.open()
    }

    function placeShared(text) {
        // Decrypted text goes as Clear wipes it, not into the undo history.
        if (crypto.sensitive) wipe()
        replaceText(text)
        notice.show(qsTr("Shared text added."))
    }

    // "Shared." only when a target was chosen; a dismissal says nothing.
    function shared(outcome) {
        notice.showShared(outcome, qsTr("This text could not be shared. "
                                 + "Copy it instead."))
    }

    // Clears the text as one edit, which Undo -- offered right here -- takes
    // back. Decrypted text is wiped instead, once it is confirmed: bringing
    // it back would bring back what was released. The keys stay chosen.
    function clearText() {
        if (crypto.sensitive) {
            wipe()
            notice.show(qsTr("Decrypted text cleared."))
            return
        }
        replaceText("")
        notice.show(qsTr("Text cleared."), qsTr("Undo"), function() { editor.undo() })
    }

    function copyText() {
        const sensitive = crypto.sensitive
        actions.copy(editor.text, sensitive)
        notice.show(sensitive ? qsTr("Copied. It is cleared from the "
                                     + "clipboard in a minute.")
                              : qsTr("Copied."))
    }

    // Why an operation cannot run on the text; empty when it can.
    readonly property string unavailable: editor.length === 0
        ? qsTr("Type or paste text first.")
        : crypto.busy ? qsTr("An operation is still running.") : ""

    // The operations besides the four, offered once turned on in Settings.
    SettingsModel {
        id: prefs
    }

    EditorHistory {
        id: history
    }

    // What runs the operations; a test puts a stand-in here.
    property var crypto: engine

    TextCryptoController {
        id: engine
    }

    OutputActions {
        id: actions
    }

    // Signals named Signal... have no on-handler in QML. A stand-in for
    // the engine is told by its test when it finishes.
    Component.onCompleted: {
        engine.SignalFinished.connect(page.finished)
        actions.SignalShared.connect(page.shared)
    }

    header: AppBar {
        title: qsTr("Text")
        raised: flick.contentY > 0

        // Kept out of the focus chain: a tap must not take the keyboard
        // away from the text.
        BarButton {
            objectName: "undoButton"
            text: qsTr("Undo")
            icon.source: Theme.icon("undo")
            focusPolicy: Qt.NoFocus
            enabled: editor.canUndo
            onClicked: editor.undo()
        }

        BarButton {
            objectName: "redoButton"
            text: qsTr("Redo")
            icon.source: Theme.icon("redo")
            focusPolicy: Qt.NoFocus
            enabled: editor.canRedo
            onClicked: editor.redo()
        }

        BarButton {
            objectName: "copyButton"
            text: qsTr("Copy")
            icon.source: Theme.icon("content_copy")
            focusPolicy: Qt.NoFocus
            enabled: editor.length > 0
            onClicked: page.copyText()
        }

        OverflowButton {
            objectName: "textMore"
            focusPolicy: Qt.NoFocus

            MenuItem {
                objectName: "pasteButton"
                text: qsTr("Paste")
                enabled: editor.canPaste
                onTriggered: editor.paste()
            }
            MenuItem {
                objectName: "shareButton"
                text: qsTr("Share")
                enabled: editor.length > 0 && actions.canShare
                height: actions.canShare ? implicitHeight : 0
                visible: actions.canShare
                onTriggered: actions.share(editor.text)
            }
            MenuItem {
                objectName: "importButton"
                text: qsTr("Import key")
                height: page.isKey ? implicitHeight : 0
                visible: page.isKey
                onTriggered: page.importRequested(editor.text)
            }
            MenuItem {
                objectName: "lineNumbers"
                text: qsTr("Line numbers")
                checkable: true
                checked: page.lineNumbers
                onTriggered: page.lineNumbers = !page.lineNumbers
            }
            MenuSeparator {}
            MenuItem {
                objectName: "clearButton"
                text: qsTr("Clear")
                enabled: editor.length > 0
                onTriggered: crypto.sensitive ? confirmClear.open() : page.clearText()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // The text, from its top left. It scrolls only when it is longer
            // than the view, and never past its ends: short text stays put.
            Flickable {
                id: flick

                objectName: "textScroller"
                anchors.fill: parent
                // The window does not move for the keyboard (adjustNothing):
                // the view ends where the keyboard begins, so the line typed
                // on is kept in sight above it.
                anchors.bottomMargin: page.keyboardCover
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                interactive: contentHeight > height + 1
                ScrollIndicator.vertical: ScrollIndicator {}

                TextArea.flickable: TextArea {
                    id: editor

                    objectName: "textInput"
                    // Room for the line numbers, then a little air.
                    leftPadding: gutter.width + (gutter.visible ? 12 : Theme.gap)
                    rightPadding: Theme.gap
                    topPadding: 12
                    // The last line clears the operation button.
                    bottomPadding: Theme.fabBand
                    // Armor is one long run of letters; prose wraps at words.
                    wrapMode: page.armored ? TextEdit.WrapAnywhere : TextEdit.Wrap
                    textFormat: TextEdit.PlainText
                    inputMethodHints: Qt.ImhNoPredictiveText
                    selectByMouse: true
                    font.family: page.armored ? Theme.mono : Qt.application.font.family
                    font.pixelSize: page.armored ? Theme.secondarySize : Theme.bodySize
                    background: null
                    Accessible.name: qsTr("Text")

                    onTextChanged: {
                        ++page.docRevision
                        // Typing: whoever waited for a result has gone on.
                        if (!page.programmatic && page.sheets) page.sheets.distract()
                    }

                    // The hint while there is nothing: Material's own
                    // placeholder is not drawn once the field has no
                    // background.
                    Label {
                        x: editor.leftPadding
                        y: editor.topPadding
                        visible: editor.length === 0 && editor.preeditText.length === 0
                                 && editor.activeFocus
                        text: qsTr("Type or paste text")
                        font: editor.font
                        color: editor.Material.hintTextColor
                    }

                    // The line numbers, each level with the first row of its
                    // line however it wraps.
                    Rectangle {
                        id: gutter

                        visible: page.lineNumbers
                                 && page.lineStarts.length <= page.maxNumberedLines
                        // As wide as the largest number, and a margin each side.
                        width: visible ? String(page.lineStarts.length).length
                                         * numberMetrics.averageCharacterWidth + 12
                                       : 0
                        height: Math.max(editor.height, editor.contentHeight
                                         + editor.topPadding + editor.bottomPadding)
                        // A faint strip of its own instead of a rule.
                        color: Theme.surface(page.Material.backgroundColor, 1)

                        FontMetrics {
                            id: numberMetrics
                            font: editor.font
                        }

                        Repeater {
                            model: gutter.visible ? page.lineStarts.length : 0

                            Label {
                                required property int index

                                x: 0
                                width: gutter.width - 6
                                // Again when the text reflows.
                                y: {
                                    editor.contentHeight
                                    editor.width
                                    return editor.positionToRectangle(
                                                page.lineStarts[index]).y
                                }
                                horizontalAlignment: Text.AlignRight
                                text: index + 1
                                color: Material.secondaryTextColor
                                font: editor.font
                            }
                        }
                    }
                }
            }

            // Nothing here yet: what the page is for. The editor under it
            // takes the tap.
            EmptyState {
                objectName: "emptyText"
                anchors.centerIn: parent
                width: parent.width - 64
                visible: editor.length === 0 && !editor.activeFocus
                text: qsTr("Nothing to work on yet")
                detail: qsTr("Type or paste a message here. The PGP button "
                             + "encrypts, decrypts, signs or verifies it.")
            }

            Fab {
                id: operationButton

                objectName: "operationButton"
                variant: "operation"
                text: qsTr("PGP")
                icon.source: Theme.operationIcon("encrypt")
                Accessible.name: qsTr("PGP operations")
                onClicked: operationMenu.openAbove(operationButton)
            }
        }

        StatusLine {
            Layout.fillWidth: true
            host: page.sheets
            visible: page.sheets !== null && page.sheets.lineShown
        }
    }

    OperationMenu {
        id: operationMenu

        unavailable: page.unavailable
        symmetric: prefs.extraOperations.includes("symmetric")
        againText: page.unavailable.length === 0
                   ? Wording.lastOperation(WorkSelection.lastOperation) : ""
        onRequested: op => page.request(op)
        onAgainRequested: page.repeatLast()
    }

    Snackbar {
        id: notice
        objectName: "textNotice"
        bottomOffset: Theme.fabBand
        // A result offered and not taken is let go once the offer is gone
        // (a taken one has already been).
        onClosed: Qt.callLater(function() {
            if (!notice.opened) page.dropOffer()
        })
    }

    MessageAlert {
        id: confirmReplace

        // Kept only while the question is open.
        property string pending: ""

        objectName: "confirmReplace"
        title: qsTr("Replace the text?")
        message: qsTr("Another app shared text with GpgFrontend. Put it in "
                   + "place of the text here? The keys you chose stay chosen.")
        acceptText: qsTr("Replace")
        onAccepted: {
            page.placeShared(pending)
            pending = ""
        }
        onRejected: pending = ""
    }

    MessageAlert {
        id: confirmClear

        objectName: "confirmClear"
        title: qsTr("Clear the decrypted text?")
        message: qsTr("It cannot be brought back with Undo. Decrypt the "
                   + "message again to see it. The keys you chose stay chosen.")
        acceptText: qsTr("Clear")
        onAccepted: page.clearText()
    }
}

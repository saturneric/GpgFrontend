// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// A text editor that encrypts, decrypts, signs and verifies what it holds.
//
// What the text is (plain, an encrypted message, signed text, a key) is said
// beside the title, and the bar at the foot offers what suits it, labelled;
// every other operation is in the bar's menu. The keys come from the
// session's choice (WorkSelection): an action that lacks them opens the
// toolbox instead of guessing.
//
// After an operation the page shows its result, read-only, and the text it
// was made from a tap away (Original | Result). Editing either makes it the
// text to work on; the operation keeps its record (the status sheet).
// The editor's own tools come up with the keyboard, and go with it.
Page {
    id: page

    // The sheets the working pages share; null where none is (tests).
    property SheetHost sheets: null

    // What the page last said in passing (a pop-up, not in the item tree).
    readonly property alias notice: notice
    readonly property alias editor: editor
    readonly property alias views: views
    // Asked before shared text takes the place of text there (a pop-up).
    readonly property alias confirmReplace: confirmReplace

    // After an operation: the text it read and the one it made, shown in
    // turn and not editable, until the person edits one of them.
    property bool resultState: false
    property string resultInput: ""
    property string resultOutput: ""
    readonly property bool showingInput: resultState && views.currentIndex === 0
    // The text set by the page, not typed: not an edit.
    property bool programmatic: false
    // Line numbers, on request.
    property bool lineNumbers: false

    // What the text on the page is.
    readonly property string kind: ContentKind.textKind(editor.text)
    readonly property bool armored: kind !== "plain" && kind !== "empty"
    readonly property bool keyboardUp: Theme.keyboard > 0 && editor.activeFocus

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

    // One step back, as Android's Back asks: out of a result into the text.
    function back() {
        if (!resultState) return false
        leaveResult()
        return true
    }

    // ---- What the status sheet asks of this page (SheetHost.textSurface)

    function holds(id) {
        return crypto.holds(id)
    }

    // "input" or "output" when the text on the page is operation @p id's.
    function relation(id) {
        return OperationQueue.log.textRelation(id, editor.text)
    }

    function retry(id) {
        if (!crypto.retry(id)) return false
        if (sheets) sheets.track(crypto.operationId)
        return true
    }

    // ---- Running operations

    // Set the editor's text as the page does, not as an edit.
    function setText(text) {
        programmatic = true
        editor.text = text
        programmatic = false
        editor.cursorPosition = 0
    }

    // The text an operation runs on: what is shown.
    function workingText() {
        return editor.text
    }

    // Starts @p op on @p text: false when it could not.
    function start(op, text) {
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
        if (started && sheets) sheets.track(crypto.operationId)
        return started
    }

    // Whether More offers @p op (Wording.inMore), by what Settings say.
    function inMore(op) {
        return Wording.inMore(op, prefs.extraOperations, prefs.defaultEncrypt,
                              prefs.defaultDecrypt)
    }

    // @p op on the text shown, as the Files page runs one on files.
    function request(op) { flow.request(op, workingText()) }
    function repeatLast() { flow.repeatLast(workingText()) }

    OperationFlow {
        id: flow
        start: page.start
        // Run on the very text it was asked for, or not at all.
        recheck: function(text) {
            return page.workingText() === text ? text
                                               : qsTr("Not started: the text changed.")
        }
        notice: page.notice
        sheets: page.sheets
    }

    function finished(succeeded) {
        notice.show("")
        if (succeeded && crypto.output.length > 0) {
            resultInput = workingText()
            resultOutput = crypto.output
            resultState = true
            views.currentIndex = 1
            setText(resultOutput)
        } else if (!succeeded && sheets === null) {
            // No status sheet to say it: a word at the foot.
            const result = crypto.result
            notice.show(result.cancelled === true ? qsTr("Cancelled.")
                        : (result.description || qsTr("It did not work.")))
        }
    }

    // The text shown becomes the one to work on; the record stays.
    function leaveResult() {
        if (!resultState) return
        resultState = false
        // Its own undo history starts here: nothing before it comes back.
        setText(editor.text)
        resultInput = ""
        resultOutput = ""
    }

    // Text another application shared: in place of an empty page, or, over
    // text there is, once the person agrees.
    function receive(text) {
        if (text.length === 0) return
        if (workingText().trim().length === 0 && !resultState) {
            placeShared(text)
            return
        }
        confirmReplace.pending = text
        confirmReplace.open()
    }

    function placeShared(text) {
        if (resultState) leaveResult()
        // Decrypted text goes as Clear wipes it, not into the undo history.
        if (crypto.sensitive) crypto.clear()
        setText(text)
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
        const wipe = crypto.sensitive
        if (resultState) leaveResult()
        // Setting the text, unlike removing it, also drops the undo history.
        if (wipe) setText("")
        else editor.remove(0, editor.length)
        crypto.clear()
        if (wipe) notice.show(qsTr("Decrypted text cleared."))
        else notice.show(qsTr("Text cleared."), qsTr("Undo"))
    }

    readonly property alias encryptAction: encryptAction
    readonly property alias decryptAction: decryptAction
    readonly property alias signAction: signAction
    readonly property alias verifyAction: verifyAction
    readonly property alias shareAction: shareAction
    readonly property alias clearAction: clearAction
    readonly property alias copyAction: copyAction
    readonly property alias importAction: importAction

    // The action the bar offers first, for what the text is.
    readonly property Action primaryAction: {
        if (resultState) return copyAction
        switch (kind) {
        case "message": return decryptAction
        case "signed": return verifyAction
        case "public_key":
        case "private_key": return importAction
        default: return encryptAction
        }
    }

    signal importRequested(string text)
    signal openFilesRequested()

    Action {
        id: encryptAction
        objectName: "encryptButton"
        // What the button does is chosen in Settings, as the desktop's
        // toolbar default: Encrypt, Encrypt & Sign or Sym. Encrypt.
        icon.source: Theme.operationIcon(prefs.defaultEncrypt)
        text: Wording.actionText(Wording.buttonLabel(prefs.defaultEncrypt))
        enabled: editor.length > 0 && !crypto.busy
        onTriggered: page.request(prefs.defaultEncrypt)
    }

    // Encrypt alone, in More while the button does another.
    Action {
        id: plainEncryptAction
        icon.source: Theme.operationIcon("encrypt")
        icon.color: "transparent"
        text: qsTr("Encrypt")
        enabled: editor.length > 0 && !crypto.busy
        onTriggered: page.request("encrypt")
    }

    Action {
        id: plainDecryptAction
        icon.source: Theme.operationIcon("decrypt")
        icon.color: "transparent"
        text: qsTr("Decrypt")
        enabled: editor.length > 0 && !crypto.busy
        onTriggered: page.request("decrypt")
    }

    // The operations besides the four, offered once turned on in Settings,
    // as the desktop's toolbar offers them.
    SettingsModel {
        id: prefs
    }

    Action {
        id: encryptSignAction
        objectName: "encryptSignButton"
        icon.source: Theme.operationIcon("encrypt_sign")
        icon.color: "transparent"
        text: qsTr("Encrypt && Sign")
        enabled: editor.length > 0 && !crypto.busy
        onTriggered: page.request("encrypt_sign")
    }

    Action {
        id: decryptVerifyAction
        objectName: "decryptVerifyButton"
        icon.source: Theme.operationIcon("decrypt_verify")
        icon.color: "transparent"
        text: qsTr("Decrypt && Verify")
        enabled: editor.length > 0 && !crypto.busy
        onTriggered: page.request("decrypt_verify")
    }

    // To a passphrase rather than to keys: no keys asked for.
    Action {
        id: symmetricAction
        icon.source: Theme.operationIcon("symmetric")
        icon.color: "transparent"
        text: qsTr("Encrypt with a passphrase")
        enabled: editor.length > 0 && !crypto.busy
        onTriggered: page.request("symmetric")
    }

    Action {
        id: decryptAction
        objectName: "decryptButton"
        // Decrypt or Decrypt & Verify, as chosen in Settings.
        icon.source: Theme.operationIcon(prefs.defaultDecrypt)
        text: Wording.actionText(Wording.buttonLabel(prefs.defaultDecrypt))
        enabled: editor.length > 0 && !crypto.busy
        onTriggered: page.request(prefs.defaultDecrypt)
    }

    Action {
        id: signAction
        objectName: "signButton"
        icon.source: Theme.operationIcon("sign")
        text: qsTr("Sign")
        enabled: editor.length > 0 && !crypto.busy
        onTriggered: page.request("sign")
    }

    Action {
        id: verifyAction
        objectName: "verifyButton"
        icon.source: Theme.operationIcon("verify")
        text: qsTr("Verify")
        enabled: editor.length > 0 && !crypto.busy
        onTriggered: page.request("verify")
    }

    Action {
        id: importAction
        text: qsTr("Import key")
        enabled: editor.length > 0 && !crypto.busy
        onTriggered: page.importRequested(editor.text)
    }

    Action {
        id: copyAction
        text: qsTr("Copy")
        enabled: editor.length > 0
        onTriggered: {
            const sensitive = crypto.sensitive && !page.showingInput
            actions.copy(editor.selectedText.length > 0 ? editor.selectedText
                                                        : editor.text, sensitive)
            notice.show(sensitive ? qsTr("Copied. It is cleared from the "
                                         + "clipboard in a minute.")
                                  : qsTr("Copied."))
        }
    }

    Action {
        id: shareAction
        text: qsTr("Share")
        enabled: editor.length > 0 && !crypto.busy
        onTriggered: actions.share(editor.text)
    }

    Action {
        id: clearAction
        text: qsTr("Clear")
        enabled: editor.length > 0 && !crypto.busy
        onTriggered: crypto.sensitive ? confirmClear.open() : page.clearText()
    }

    header: ColumnLayout {
        spacing: 0

        AppBar {
            Layout.fillWidth: true
            title: qsTr("Text")
            raised: scroller.contentItem.contentY > 0

            Chip {
                objectName: "textKind"
                visible: page.kind !== "empty"
                text: Wording.textKind(page.kind)
                tone: page.armored ? Material.accentColor : Material.secondaryTextColor
                Layout.rightMargin: 4
            }

            BarButton {
                objectName: "textHistory"
                visible: page.sheets !== null
                         && page.sheets.revision >= 0 && OperationQueue.log.rowCount() > 0
                text: qsTr("Results")
                icon.source: Theme.icon("info")
                onClicked: page.sheets.openLatest()
            }
        }

        // The result and what it was made from, in turn.
        RowLayout {
            objectName: "resultViews"
            Layout.fillWidth: true
            Layout.leftMargin: Theme.gapSmall
            Layout.rightMargin: 4
            visible: page.resultState
            spacing: 4

            TabBar {
                id: views
                Layout.fillWidth: true
                Material.background: "transparent"
                currentIndex: 1
                onCurrentIndexChanged: {
                    if (!page.resultState) return
                    page.setText(currentIndex === 0 ? page.resultInput
                                                    : page.resultOutput)
                }

                TabButton {
                    objectName: "viewOriginal"
                    text: qsTr("Original")
                }
                TabButton {
                    objectName: "viewResult"
                    text: qsTr("Result")
                }
            }

            BarButton {
                objectName: "editResult"
                text: qsTr("Edit this text")
                icon.source: Theme.icon("draft")
                onClicked: {
                    page.leaveResult()
                    editor.forceActiveFocus()
                }
            }
        }
    }

    TextCryptoController {
        id: crypto
    }

    OutputActions {
        id: actions
    }

    // Signals named Signal... have no on-handler in QML.
    Component.onCompleted: {
        crypto.SignalFinished.connect(page.finished)
        actions.SignalShared.connect(page.shared)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ScrollView {
            id: scroller

            Layout.fillWidth: true
            Layout.fillHeight: true
            // Sized by the page, never the other way round: Material's Page
            // takes its implicit width from its content.
            implicitWidth: 0

            TextArea {
                id: editor

                objectName: "textInput"
                // Room for the line numbers, then a little air.
                leftPadding: gutter.width + (gutter.visible ? 12 : Theme.gap)
                rightPadding: Theme.gap
                topPadding: 12
                bottomPadding: Theme.gap
                // Armor is one long run of letters; prose wraps at words.
                wrapMode: page.armored ? TextEdit.WrapAnywhere : TextEdit.Wrap
                inputMethodHints: Qt.ImhNoPredictiveText
                selectByMouse: true
                readOnly: crypto.busy || page.resultState
                font.family: page.armored ? Theme.mono : Qt.application.font.family
                font.pixelSize: page.armored ? Theme.secondarySize : Theme.bodySize
                background: null
                Accessible.name: qsTr("Text")
                Accessible.description: page.resultState
                                        ? qsTr("A result. Choose Edit this text to change it.")
                                        : Wording.textKind(page.kind)

                onTextChanged: {
                    // Typing: whoever waited for a result has gone on.
                    if (!page.programmatic && page.sheets) page.sheets.distract()
                }

                // The hint while there is nothing: Material's own placeholder
                // is not drawn once the field has no background.
                Label {
                    id: hint

                    x: editor.leftPadding
                    y: editor.topPadding
                    visible: editor.length === 0 && editor.preeditText.length === 0
                             && editor.activeFocus
                    text: qsTr("Type or paste text")
                    font: editor.font
                    color: editor.Material.hintTextColor
                }

                // The line numbers, each level with the first row of its line
                // however it wraps.
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

        StatusLine {
            Layout.fillWidth: true
            host: page.sheets
            visible: page.sheets !== null && page.sheets.lineShown && !page.keyboardUp
        }

        // While typing: the editor's own tools, on the keyboard.
        Pane {
            id: tools

            objectName: "editTools"
            Layout.fillWidth: true
            visible: page.keyboardUp
            padding: 0
            leftPadding: 4
            rightPadding: 4

            background: Rectangle {
                color: Theme.surface(page.Material.backgroundColor, 3)
            }

            contentItem: RowLayout {
                spacing: 0

                component Tool: BarButton {
                    // Keeps the keyboard up while a tool is used.
                    focusPolicy: Qt.NoFocus
                }

                Tool {
                    objectName: "undoButton"
                    text: qsTr("Undo")
                    icon.source: Theme.icon("undo")
                    enabled: editor.canUndo && !editor.readOnly
                    onClicked: editor.undo()
                }

                Tool {
                    objectName: "redoButton"
                    text: qsTr("Redo")
                    icon.source: Theme.icon("redo")
                    enabled: editor.canRedo && !editor.readOnly
                    onClicked: editor.redo()
                }

                Item {
                    Layout.fillWidth: true
                }

                Tool {
                    objectName: "pasteButton"
                    text: qsTr("Paste")
                    icon.source: Theme.icon("content_paste")
                    enabled: editor.canPaste && !editor.readOnly
                    onClicked: editor.paste()
                }

                Tool {
                    objectName: "selectAllButton"
                    text: qsTr("Select all")
                    icon.source: Theme.icon("select_all")
                    enabled: editor.length > 0
                    onClicked: editor.selectAll()
                }

                Tool {
                    objectName: "copyButton"
                    text: qsTr("Copy")
                    icon.source: Theme.icon("content_copy")
                    enabled: editor.length > 0
                    onClicked: copyAction.trigger()
                }

                Tool {
                    objectName: "doneTyping"
                    text: qsTr("Done typing")
                    icon.source: Theme.icon("check_box")
                    onClicked: {
                        Qt.inputMethod.hide()
                        editor.focus = false
                    }
                }
            }
        }

        // What can be done to the text; the keys are asked for when an
        // operation needs them (request()).
        OperationBar {
            id: dock

            objectName: "actionDock"
            Layout.fillWidth: true
            visible: !page.keyboardUp
            operations: [encryptAction, decryptAction, signAction, verifyAction]
            suggested: page.primaryAction
            quickText: editor.length > 0 && !crypto.busy && !page.resultState
                       ? Wording.lastOperation(WorkSelection.lastOperation) : ""
            onQuickTriggered: page.repeatLast()

            // What the buttons do not, of what is turned on in Settings
            // (Wording.inMore); else not shown (a hidden menu item still
            // takes its row).
            MenuItem {
                objectName: "plainEncryptItem"
                // The desktop's colours: a menu item tints its action's icon.
                icon.color: "transparent"
                action: plainEncryptAction
                height: page.inMore("encrypt") ? implicitHeight : 0
                visible: height > 0
            }
            MenuItem {
                objectName: "encryptSignItem"
                // The desktop's colours: a menu item tints its action's icon.
                icon.color: "transparent"
                action: encryptSignAction
                height: page.inMore("encrypt_sign") ? implicitHeight : 0
                visible: height > 0
            }
            MenuItem {
                objectName: "symmetricButton"
                // The desktop's colours: a menu item tints its action's icon.
                icon.color: "transparent"
                action: symmetricAction
                height: page.inMore("symmetric") ? implicitHeight : 0
                visible: height > 0
            }
            MenuItem {
                objectName: "plainDecryptItem"
                // The desktop's colours: a menu item tints its action's icon.
                icon.color: "transparent"
                action: plainDecryptAction
                height: page.inMore("decrypt") ? implicitHeight : 0
                visible: height > 0
            }
            MenuItem {
                objectName: "decryptVerifyItem"
                // The desktop's colours: a menu item tints its action's icon.
                icon.color: "transparent"
                action: decryptVerifyAction
                height: page.inMore("decrypt_verify") ? implicitHeight : 0
                visible: height > 0
            }
            MenuItem {
                action: importAction
                // A menu item that is not shown still takes its row.
                height: page.kind === "public_key" || page.kind === "private_key"
                        ? implicitHeight : 0
                visible: height > 0
            }
            // Only between something and something.
            MenuSeparator {
                height: ["encrypt", "encrypt_sign", "symmetric", "decrypt",
                         "decrypt_verify"].some(op => page.inMore(op))
                        || page.kind === "public_key" || page.kind === "private_key"
                        ? implicitHeight : 0
                visible: height > 0
            }
            MenuItem {
                objectName: "shareButton"
                action: shareAction
                height: actions.canShare ? implicitHeight : 0
                visible: actions.canShare
            }
            MenuItem {
                action: copyAction
            }
            MenuItem {
                objectName: "clearButton"
                action: clearAction
            }
            MenuItem {
                objectName: "lineNumbers"
                text: qsTr("Line numbers")
                checkable: true
                checked: page.lineNumbers
                onTriggered: page.lineNumbers = !page.lineNumbers
            }
        }
    }

    // Nothing here yet: the ways to start.
    EmptyState {
        objectName: "emptyText"
        anchors.centerIn: parent
        width: parent.width - 64
        visible: editor.length === 0 && !editor.activeFocus && !page.resultState
        text: qsTr("Nothing to work on yet")
        detail: qsTr("Paste a message to decrypt or verify, or type one to "
                     + "encrypt or sign.")

        Button {
            objectName: "emptyPaste"
            text: qsTr("Paste")
            highlighted: true
            enabled: editor.canPaste
            onClicked: editor.paste()
        }

        Button {
            objectName: "emptyType"
            text: qsTr("Type")
            flat: true
            onClicked: editor.forceActiveFocus()
        }

        Button {
            objectName: "emptyFile"
            text: qsTr("Open a file…")
            flat: true
            onClicked: page.openFilesRequested()
        }
    }

    Snackbar {
        id: notice
        objectName: "textNotice"
        bottomOffset: dock.visible ? dock.height : tools.height
        // The one action offered here: Undo, after Clear.
        onAction: editor.undo()
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

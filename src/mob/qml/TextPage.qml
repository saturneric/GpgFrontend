// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// A text editor that encrypts, decrypts, signs and verifies what it holds, as
// the desktop's editor does: the result takes the text's place (Undo brings
// the text back), and a banner under it says how it went. Encrypt and decrypt
// are beside the title; sign, verify, share and clear in its menu; the
// editor's own actions at the thumb, Clear taken back by Undo.
Page {
    id: page

    // What the page last said in passing (a pop-up, not in the item tree).
    readonly property alias notice: notice

    // What the last operation was, for the wording around its result.
    property string operation: ""
    // The last operation's report, shown in the banner until dismissed.
    property var report: ({})
    readonly property bool reported: report.status !== undefined
                                     || report.cancelled === true

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

    // The result in the text's place, as one edit that Undo takes back.
    function applyResult(text) {
        editor.remove(0, editor.length)
        editor.insert(0, text)
        editor.cursorPosition = 0
    }

    function finished(succeeded) {
        notice.show("")
        page.report = crypto.result
        if (succeeded && crypto.output.length > 0) page.applyResult(crypto.output)
    }

    // "Shared." only when a target was chosen; a dismissal says nothing.
    function shared(outcome) {
        notice.showShared(outcome, qsTr("This text could not be shared. "
                                 + "Copy it instead."))
    }

    // What happened, in a word.
    function reportTitle() {
        if (report.cancelled === true) return qsTr("Cancelled")
        if (report.status < 0) {
            switch (page.operation) {
            case "encrypt": return qsTr("Not encrypted")
            case "decrypt": return qsTr("Not decrypted")
            case "sign": return qsTr("Not signed")
            default: return qsTr("Not verified")
            }
        }
        switch (page.operation) {
        case "encrypt": return qsTr("Encrypted")
        case "decrypt": return qsTr("Decrypted")
        case "sign": return qsTr("Signed")
        default: return qsTr("Verified")
        }
    }

    // A line under it: the first signature, or what went wrong.
    function reportDetail() {
        const signatures = report.signatures || []
        if (signatures.length > 0) {
            const first = signatures[0]
            const who = first.uid || first.keyId || first.fingerprint || ""
            const text = results.validityText(first.validity)
            return who.length > 0 ? qsTr("%1: %2").arg(text).arg(who) : text
        }
        return report.description || ""
    }

    // The four operations, for the bar's buttons and its menu alike.
    readonly property alias encryptAction: encryptAction
    readonly property alias decryptAction: decryptAction
    readonly property alias signAction: signAction
    readonly property alias verifyAction: verifyAction

    Action {
        id: encryptAction
        text: qsTr("Encrypt")
        icon.source: Theme.icon("lock")
        enabled: editor.length > 0 && !crypto.busy
        onTriggered: recipients.begin()
    }

    Action {
        id: decryptAction
        text: qsTr("Decrypt")
        icon.source: Theme.icon("lock_open_right")
        enabled: editor.length > 0 && !crypto.busy
        onTriggered: {
            page.operation = "decrypt"
            crypto.decrypt(editor.text)
        }
    }

    Action {
        id: signAction
        text: qsTr("Sign")
        icon.source: Theme.icon("signature")
        enabled: editor.length > 0 && !crypto.busy
        onTriggered: signer.begin()
    }

    Action {
        id: verifyAction
        text: qsTr("Verify")
        icon.source: Theme.icon("verified")
        enabled: editor.length > 0 && !crypto.busy
        onTriggered: {
            page.operation = "verify"
            crypto.verify(editor.text)
        }
    }

    readonly property alias shareAction: shareAction
    readonly property alias clearAction: clearAction

    Action {
        id: shareAction
        text: qsTr("Share")
        icon.source: Theme.icon("share")
        enabled: editor.length > 0 && !crypto.busy
        onTriggered: actions.share(editor.text)
    }

    Action {
        id: clearAction
        text: qsTr("Clear")
        icon.source: Theme.icon("delete_sweep")
        enabled: editor.length > 0 && !crypto.busy
        onTriggered: crypto.sensitive ? confirmClear.open() : page.clearText()
    }

    // Clears the text as one edit, which Undo -- in the bar, or offered right
    // here -- takes back. Decrypted text is wiped instead, once it is
    // confirmed: bringing it back would bring back what was released.
    function clearText() {
        const wipe = crypto.sensitive
        // Setting the text, unlike removing it, also drops the undo history.
        if (wipe) editor.text = ""
        else editor.remove(0, editor.length)
        crypto.clear()
        page.report = {}
        if (wipe) notice.show(qsTr("Decrypted text cleared."))
        else notice.show(qsTr("Text cleared."), qsTr("Undo"))
    }

    header: ColumnLayout {
        spacing: 0

        // Encrypt and decrypt at hand beside the title; signing and
        // verifying in the menu.
        AppBar {
            Layout.fillWidth: true
            title: qsTr("Text")
            raised: scroller.contentItem.contentY > 0

            BarButton {
                objectName: "encryptButton"
                action: encryptAction
            }

            BarButton {
                objectName: "decryptButton"
                action: decryptAction
            }

            OverflowButton {
                id: more

                objectName: "textMore"

                MenuItem {
                    objectName: "signButton"
                    action: signAction
                }

                MenuItem {
                    objectName: "verifyButton"
                    action: verifyAction
                }

                MenuSeparator {}

                MenuItem {
                    objectName: "shareButton"
                    action: shareAction
                    // A menu item that is not shown still takes its row.
                    height: actions.canShare ? implicitHeight : 0
                    visible: actions.canShare
                }

                MenuItem {
                    objectName: "clearButton"
                    action: clearAction
                }
            }
        }

        ProgressBar {
            Layout.fillWidth: true
            visible: crypto.busy
            indeterminate: true
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
                leftPadding: gutter.width + 12
                rightPadding: Theme.gap
                topPadding: 12
                bottomPadding: Theme.gap
                wrapMode: TextEdit.WrapAnywhere
                inputMethodHints: Qt.ImhNoPredictiveText
                selectByMouse: true
                readOnly: crypto.busy
                font.family: Theme.mono
                font.pixelSize: Theme.secondarySize
                background: null
                Accessible.name: qsTr("Text")
                Accessible.description: hint.text

                // The hint while there is nothing: Material's own placeholder
                // is not drawn once the field has no background.
                Label {
                    id: hint

                    x: editor.leftPadding
                    y: editor.topPadding
                    visible: editor.length === 0 && editor.preeditText.length === 0
                    text: qsTr("Type or paste text")
                    font: editor.font
                    color: editor.Material.hintTextColor
                }

                // The line numbers, each level with the first row of its line
                // however it wraps.
                Rectangle {
                    id: gutter

                    visible: page.lineStarts.length <= page.maxNumberedLines
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

        // How the last operation went, until it is dismissed or the text is
        // cleared.
        Pane {
            objectName: "textStatus"
            Layout.fillWidth: true
            Layout.leftMargin: Theme.gapSmall
            Layout.rightMargin: Theme.gapSmall
            Layout.bottomMargin: Theme.gapSmall
            visible: page.reported && !crypto.busy
            topPadding: 4
            bottomPadding: 4
            leftPadding: Theme.gap
            rightPadding: 4

            background: Rectangle {
                radius: 12
                color: Theme.surface(page.Material.backgroundColor, 2)
            }

            contentItem: RowLayout {
                spacing: 12

                Icon {
                    symbol: Theme.reportSymbol(page.report)
                    color: Theme.reportColor(page.report,
                                             Material.secondaryTextColor)
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 6
                    Layout.bottomMargin: 6
                    spacing: 0

                    Label {
                        Layout.fillWidth: true
                        text: page.reportTitle()
                        font.weight: Font.DemiBold
                        color: Theme.reportColor(page.report,
                                                 Material.secondaryTextColor)
                    }

                    Label {
                        Layout.fillWidth: true
                        visible: text.length > 0
                        text: page.reportDetail()
                        elide: Text.ElideRight
                        maximumLineCount: 2
                        wrapMode: Text.Wrap
                        color: Material.secondaryTextColor
                    }
                }

                BarButton {
                    objectName: "resultDetails"
                    text: qsTr("Details")
                    icon.source: Theme.icon("chevron_right")
                    onClicked: {
                        results.report = page.report
                        results.open()
                    }
                }

                BarButton {
                    text: qsTr("Dismiss")
                    icon.source: Theme.icon("close")
                    onClicked: page.report = {}
                }
            }
        }

        // The editor's own actions, at the thumb, on the keyboard while it
        // is up: what to do with the text here, on a tinted strip of the
        // editor's that tells it apart from the sections' bar under it.
        Pane {
            id: tools

            Layout.fillWidth: true
            padding: 0
            leftPadding: 4
            rightPadding: 4

            // A shade deeper than the sections' bar under it, so the two
            // read as two bars.
            background: Rectangle {
                color: Theme.surface(page.Material.backgroundColor, 3)
            }

            contentItem: RowLayout {
                spacing: 0

                component Tool: BarButton {
                    // Keeps the keyboard up while a tool is used.
                    focusPolicy: Qt.NoFocus
                }

                // Taking back and redoing edits ...
                Tool {
                    objectName: "undoButton"
                    text: qsTr("Undo")
                    icon.source: Theme.icon("undo")
                    enabled: editor.canUndo && !crypto.busy
                    onClicked: editor.undo()
                }

                Tool {
                    objectName: "redoButton"
                    text: qsTr("Redo")
                    icon.source: Theme.icon("redo")
                    enabled: editor.canRedo && !crypto.busy
                    onClicked: editor.redo()
                }

                Item {
                    Layout.fillWidth: true
                }

                // ... and moving text in and out.
                Tool {
                    objectName: "pasteButton"
                    text: qsTr("Paste")
                    icon.source: Theme.icon("content_paste")
                    enabled: editor.canPaste && !crypto.busy
                    onClicked: editor.paste()
                }

                Tool {
                    objectName: "selectAllButton"
                    text: qsTr("Select all")
                    icon.source: Theme.icon("select_all")
                    enabled: editor.length > 0
                    onClicked: {
                        editor.forceActiveFocus()
                        editor.selectAll()
                    }
                }

                Tool {
                    objectName: "copyButton"
                    text: qsTr("Copy")
                    icon.source: Theme.icon("content_copy")
                    enabled: editor.length > 0
                    onClicked: {
                        actions.copy(editor.selectedText.length > 0
                                     ? editor.selectedText : editor.text,
                                     crypto.sensitive)
                        notice.show(crypto.sensitive
                                ? qsTr("Copied. It is cleared from the "
                                       + "clipboard in a minute.")
                                : qsTr("Copied."))
                    }
                }
            }
        }
    }

    Snackbar {
        id: notice
        objectName: "textNotice"
        bottomOffset: tools.height
        // The one action offered here: Undo, after Clear.
        onAction: editor.undo()
    }

    MessageAlert {
        id: confirmClear

        objectName: "confirmClear"
        title: qsTr("Clear the decrypted text?")
        message: qsTr("It cannot be brought back with Undo. Decrypt the "
                   + "message again to see it.")
        acceptText: qsTr("Clear")
        onAccepted: page.clearText()
    }

    RecipientSheet {
        id: recipients

        onConfirmed: function(chosen, signerKey) {
            page.operation = "encrypt"
            crypto.encrypt(editor.text, chosen, signerKey)
        }
    }

    SignerSheet {
        id: signer

        onConfirmed: function(signerKey) {
            page.operation = "sign"
            crypto.sign(editor.text, signerKey)
        }
    }

    ResultSheet {
        id: results
    }
}

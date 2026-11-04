// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Layouts
import GpgFrontend.Mobile

BottomSheet {
    id: sheet

    // Open with @p text (a key block from the Text page), or empty.
    function begin(text) {
        block.text = text || ""
        outcome.text = ""
        open()
    }

    objectName: "importSheet"
    title: qsTr("Import keys")
    closePolicy: importer.busy ? Popup.NoAutoClose
                               : Popup.CloseOnEscape | Popup.CloseOnPressOutside
    interactive: !importer.busy

    // A secret key block is as sensitive as it gets: gone when the sheet
    // closes, and when the application goes to the background.
    onClosed: block.text = ""

    function finished(imported) {
        const r = importer.result
        if (r.considered === undefined) {
            outcome.text = qsTr("No key could be read from this text.")
        } else if (imported) {
            outcome.text = qsTr("Imported %1 key(s).").arg(
                        r.imported + r.secretImported)
            KeyListController.refresh()
            block.text = ""
        } else if (r.unchanged > 0) {
            outcome.text = qsTr("You already have these keys.")
        } else {
            outcome.text = qsTr("No key was imported.")
        }
    }

    KeyImportController {
        id: importer
    }

    // Going to the background forgets whatever was pasted.
    function wipe() {
        if (!importer.busy) block.text = ""
    }

    // The bridge outlives the sheet: let go of it with the sheet.
    Component.onCompleted: {
        importer.SignalFinished.connect(sheet.finished)
        InteractionBridge.SignalWipeSecretInputs.connect(sheet.wipe)
    }
    Component.onDestruction: InteractionBridge.SignalWipeSecretInputs.disconnect(sheet.wipe)

    // No name filters: Android's picker filters by MIME type, and it has none
    // that covers .asc, so a filter greys out the very files it names. What a
    // file holds is checked when it is read.
    FileDialog {
        id: keyFile
        fileMode: FileDialog.OpenFile
        onAccepted: importer.importFile(selectedFile)
    }

    ListRow {
        objectName: "importFromFile"
        Layout.fillWidth: true
        Layout.leftMargin: -24
        Layout.rightMargin: -24
        leftPadding: 24
        enabled: !importer.busy
        symbol: "note_add"
        symbolColor: Material.accentColor
        text: qsTr("From a file")
        detail: qsTr("A .asc or .gpg file with one or more keys.")
        onClicked: keyFile.open()
    }

    ListRow {
        Layout.fillWidth: true
        Layout.leftMargin: -24
        Layout.rightMargin: -24
        leftPadding: 24
        enabled: !importer.busy && block.canPaste
        symbol: "content_paste"
        symbolColor: Material.accentColor
        text: qsTr("From the clipboard")
        detail: qsTr("Paste a key block copied from somewhere else.")
        onClicked: {
            block.selectAll()
            block.paste()
        }
    }

    TextArea {
        id: block
        objectName: "importText"
        Layout.fillWidth: true
        Layout.minimumHeight: 120
        Layout.maximumHeight: 240
        wrapMode: TextEdit.WrapAnywhere
        inputMethodHints: Qt.ImhSensitiveData | Qt.ImhNoPredictiveText
        placeholderText: qsTr("-----BEGIN PGP PUBLIC KEY BLOCK-----")
        font.family: Theme.mono
        font.pixelSize: Theme.labelSize
    }

    Label {
        id: outcome
        objectName: "importOutcome"
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        visible: text.length > 0
    }

    BusyIndicator {
        Layout.alignment: Qt.AlignHCenter
        running: importer.busy
        visible: running
    }

    footer: [
        Item {
            Layout.fillWidth: true
        },
        Button {
            text: qsTr("Close")
            flat: true
            enabled: !importer.busy
            onClicked: sheet.close()
        },
        Button {
            objectName: "importRun"
            text: qsTr("Import")
            highlighted: true
            enabled: block.text.trim().length > 0 && !importer.busy
            onClicked: importer.importText(block.text)
        }
    ]
}

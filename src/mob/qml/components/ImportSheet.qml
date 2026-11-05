// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Layouts
import GpgFrontend.Mobile

// Importing keys: from a file, or from what was copied, each a tap that
// imports at once -- no text to paste into a field first. The clipboard is
// read only on that tap, and its text never comes to this page. A key block
// the Text page hands over is imported as the sheet opens.
BottomSheet {
    id: sheet

    // Open; with @p text (a key block from the Text page), import it at once.
    function begin(text) {
        outcome.text = ""
        open()
        if ((text || "").trim().length > 0) importer.importText(text)
    }

    objectName: "importSheet"
    title: qsTr("Import keys")
    closePolicy: importer.busy ? Popup.NoAutoClose
                               : Popup.CloseOnEscape | Popup.CloseOnPressOutside
    interactive: !importer.busy

    function finished(imported) {
        const r = importer.result
        if (r.considered === undefined) {
            outcome.text = qsTr("No key could be read from this.")
        } else if (imported) {
            // Keys counted once, a secret key with its public part; what a
            // key already here gained is said apart.
            const said = []
            if (r.imported > 0) said.push(qsTr("Imported %1 key(s).").arg(r.imported))
            if (r.updated > 0) said.push(qsTr("Updated %1 key(s).").arg(r.updated))
            outcome.text = said.join(" ")
            KeyListController.refresh()
        } else if (r.unchanged > 0) {
            outcome.text = qsTr("You already have these keys.")
        } else {
            outcome.text = qsTr("No key was imported.")
        }
    }

    function importClipboard() {
        outcome.text = ""
        if (!importer.importClipboard())
            outcome.text = qsTr("The clipboard has no text to import. Copy a "
                                + "key first.")
    }

    KeyImportController {
        id: importer
    }

    // Signals named Signal... have no on-handler in QML.
    Component.onCompleted: importer.SignalFinished.connect(sheet.finished)

    // No name filters: Android's picker filters by MIME type, and it has none
    // that covers .asc, so a filter greys out the very files it names. What a
    // file holds is checked when it is read.
    FileDialog {
        id: keyFile
        fileMode: FileDialog.OpenFile
        onAccepted: {
            outcome.text = ""
            importer.importFile(selectedFile)
        }
    }

    ListRow {
        objectName: "importFromFile"
        Layout.fillWidth: true
        Layout.leftMargin: -Theme.sheetPad
        Layout.rightMargin: -Theme.sheetPad
        leftPadding: Theme.sheetPad
        enabled: !importer.busy
        symbol: "note_add"
        symbolColor: Material.accentColor
        text: qsTr("From a file")
        detail: qsTr("A .asc or .gpg file with one or more keys.")
        onClicked: keyFile.open()
    }

    ListRow {
        objectName: "importFromClipboard"
        Layout.fillWidth: true
        Layout.leftMargin: -Theme.sheetPad
        Layout.rightMargin: -Theme.sheetPad
        Layout.topMargin: -Theme.gapSmall
        leftPadding: Theme.sheetPad
        enabled: !importer.busy
        symbol: "content_paste"
        symbolColor: Material.accentColor
        text: qsTr("From the clipboard")
        detail: qsTr("A key block you copied from somewhere else.")
        onClicked: sheet.importClipboard()
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
        }
    ]
}

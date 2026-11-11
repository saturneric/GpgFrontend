// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Dialogs
import GpgFrontend.Mobile

// Picking files to read: one, or several when `multiple`. Used as a
// FileDialog is: open(), then accepted with selectedFile / selectedFiles.
//
// On Android the system's picker is asked directly (DocumentPicker). Qt
// Quick's FileDialog checks that each picked file exists, knows only the
// URLs of the device's own storage, and so drops a file picked from a
// cloud folder without a word -- accepted, with nothing selected.
// Elsewhere FileDialog itself is shown.
Item {
    id: dialog

    property bool multiple: false
    property list<url> selectedFiles: []
    readonly property url selectedFile: selectedFiles.length > 0 ? selectedFiles[0] : ""

    signal accepted()
    signal rejected()

    function open() {
        if (!systemPicker.pick(multiple)) fallback.open()
    }

    function took(files) {
        if (files.length === 0) {
            dialog.rejected()
            return
        }
        dialog.selectedFiles = files
        dialog.accepted()
    }

    visible: false

    DocumentPicker {
        id: systemPicker
        onPicked: files => dialog.took(files)
    }

    FileDialog {
        id: fallback
        fileMode: dialog.multiple ? FileDialog.OpenFiles : FileDialog.OpenFile
        onAccepted: dialog.took(selectedFiles)
        onRejected: dialog.rejected()
    }
}

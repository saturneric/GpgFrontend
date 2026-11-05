// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Layouts
import GpgFrontend.Mobile

// Checking a detached signature: which file is the signature and which the
// original are two fields, filled in for the person where it is known -- a
// signature made here knows its file -- and only suggested where a name
// merely matches. Either can be changed, swapped, or picked from the device.
BottomSheet {
    id: sheet

    property FileWorkspace workspace
    property int signatureId: -1
    property int originalId: -1
    // How each field was filled: "linked" (known), "suggested" (by name),
    // "" (by the person, or not at all).
    property string signatureHow: ""
    property string originalHow: ""
    // The field a picked file goes into.
    property string picking: ""

    // Start from the file @p id, put where what is known of it says.
    function begin(id) {
        const file = workspace.entry(id)
        signatureId = -1
        originalId = -1
        signatureHow = ""
        originalHow = ""
        // The desktop's rule for what a name is and what its other half
        // is called.
        const match = workspace.verifyMatch(file.name || "")
        const looksSigned = match.role === "signature"
        if (file.madeBy === "sign") {
            signatureId = id
            if (workspace.entry(file.sourceId).fileId !== undefined) {
                originalId = file.sourceId
                originalHow = "linked"
            }
        } else if (looksSigned) {
            signatureId = id
            const found = findFirst(match.partners)
            if (found >= 0) {
                originalId = found
                originalHow = "suggested"
            }
        } else {
            originalId = id
            const made = findSignatureOf(id)
            if (made >= 0) {
                signatureId = made
                signatureHow = "linked"
            } else {
                const found = findFirst(match.role === "data" ? match.partners
                                                    : [file.name + ".sig", file.name + ".asc"])
                if (found >= 0) {
                    signatureId = found
                    signatureHow = "suggested"
                }
            }
        }
        open()
    }

    function findByName(name) {
        return workspace.idNamed(name)
    }

    // The first of @p names in the list; -1 when none is.
    function findFirst(names) {
        for (let i = 0; i < names.length; ++i) {
            const id = findByName(names[i])
            if (id >= 0) return id
        }
        return -1
    }

    // The signature made here of the file @p id, if one is still listed.
    function findSignatureOf(id) {
        return workspace.signatureOf(id)
    }

    function swap() {
        const id = signatureId
        signatureId = originalId
        originalId = id
        const how = signatureHow
        signatureHow = originalHow
        originalHow = how
    }

    objectName: "detachedVerifySheet"
    title: qsTr("Verify a signature")

    component FileField: ColumnLayout {
        id: field

        property string label
        property int fileId: -1
        property string how: ""
        property int otherId: -1

        signal chosen(int id)
        signal pickRequested()

        Layout.fillWidth: true
        spacing: 4

        Label {
            text: field.label
            color: Material.secondaryTextColor
            font.pixelSize: Theme.labelSize
        }

        ChoiceBox {
            Layout.fillWidth: true
            model: sheet.workspace
            textRole: "name"
            valueRole: "fileId"
            displayText: field.fileId >= 0
                         ? (sheet.workspace.entry(field.fileId).name || "")
                         : qsTr("Choose a file")
            currentIndex: indexOfValue(field.fileId)
            Accessible.name: qsTr("%1: %2").arg(field.label).arg(displayText)
            onActivated: field.chosen(currentValue)
        }

        Label {
            Layout.fillWidth: true
            visible: text.length > 0
            wrapMode: Text.Wrap
            font.pixelSize: Theme.labelSize
            color: field.how === "suggested" ? Theme.warning
                                             : Material.secondaryTextColor
            text: field.how === "linked" ? qsTr("Known: the signature was made here of this file.")
                : field.how === "suggested" ? qsTr("Suggested by its name. Check it is the right one.")
                : field.fileId >= 0 && field.fileId === field.otherId
                  ? qsTr("The same file cannot be both.") : ""
        }

        // As wide as its words, and narrower when the sheet is: its words
        // then wrap (large text, German), never past the sheet's edge.
        ActionButton {
            Layout.fillWidth: true
            Layout.maximumWidth: implicitWidth
            kind: "text"
            text: qsTr("Add from device…")
            onClicked: field.pickRequested()
        }
    }

    FileField {
        objectName: "verifySignatureField"
        label: qsTr("Signature file")
        fileId: sheet.signatureId
        how: sheet.signatureHow
        otherId: sheet.originalId
        onChosen: id => {
            sheet.signatureId = id
            sheet.signatureHow = ""
        }
        onPickRequested: {
            sheet.picking = "signature"
            picker.open()
        }
    }

    ActionButton {
        objectName: "verifySwap"
        Layout.alignment: Qt.AlignHCenter
        Layout.fillWidth: true
        Layout.maximumWidth: implicitWidth
        kind: "text"
        text: qsTr("Swap the two")
        enabled: sheet.signatureId >= 0 || sheet.originalId >= 0
        onClicked: sheet.swap()
    }

    FileField {
        objectName: "verifyOriginalField"
        label: qsTr("Original file")
        fileId: sheet.originalId
        how: sheet.originalHow
        otherId: sheet.signatureId
        onChosen: id => {
            sheet.originalId = id
            sheet.originalHow = ""
        }
        onPickRequested: {
            sheet.picking = "original"
            picker.open()
        }
    }

    FileDialog {
        id: picker
        fileMode: FileDialog.OpenFile
        onAccepted: {
            const ids = sheet.workspace.addFiles([selectedFile])
            if (ids.length === 0) return
            const id = ids[0]
            if (sheet.picking === "signature") {
                sheet.signatureId = id
                sheet.signatureHow = ""
            } else {
                sheet.originalId = id
                sheet.originalHow = ""
            }
        }
    }

    signal verifyRequested(int signatureId, int originalId)

    footer: [
        Item {
            Layout.fillWidth: true
        },
        Button {
            text: qsTr("Cancel")
            flat: true
            onClicked: sheet.close()
        },
        Button {
            objectName: "verifyRun"
            text: qsTr("Verify")
            highlighted: true
            enabled: sheet.signatureId >= 0 && sheet.originalId >= 0
                     && sheet.signatureId !== sheet.originalId
            onClicked: {
                sheet.close()
                sheet.verifyRequested(sheet.signatureId, sheet.originalId)
            }
        }
    ]
}

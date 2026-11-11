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
// Where the other half is certain (known, or the one name that matches), the
// sheet is not shown at all: begin() verifies the pair at once; a file that
// holds its signature inline is verified alone.
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

    // Which file is the signature and which the original, starting from the
    // file @p id (FileWorkspace.pairFor: the rule, in C++, beside the
    // desktop's naming rule it follows).
    function pair(id) {
        return workspace.pairFor(id)
    }

    // There to be read now: not gone, not waiting or being worked on.
    function usable(id) {
        const e = workspace.entry(id)
        return e.available === true && e.phase !== "queued"
               && e.phase !== "running" && !(e.progress >= 0)
    }

    // Start from the file @p id: verified at once when its other half is
    // certain, else the two fields, filled in as far as is known.
    function begin(id) {
        acted = false
        const p = pair(id)
        if (p.unique || p.inline) {
            // Inline: originalId is -1, the file is verified alone.
            verifyRequested(p.signatureId, p.originalId)
            return
        }
        signatureId = p.signatureId
        originalId = p.originalId
        signatureHow = p.signatureHow
        originalHow = p.originalHow
        open()
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

    // A field: what it is, the file in it with a way to pick one from the
    // device right beside it, and a word under it only when there is
    // something to say.
    component FileField: ColumnLayout {
        id: field

        property string label
        // Said for the button that picks a file from the device.
        property string browseText
        property int fileId: -1
        property string how: ""
        property int otherId: -1

        signal chosen(int id)
        signal pickRequested()

        Layout.fillWidth: true
        spacing: 2

        Label {
            Layout.fillWidth: true
            text: field.label
            elide: Text.ElideRight
            color: Material.secondaryTextColor
            font.pixelSize: Theme.labelSize
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 0

            ChoiceBox {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
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

            BarButton {
                objectName: field.objectName + "Browse"
                text: field.browseText
                icon.source: Theme.icon("folder")
                onClicked: field.pickRequested()
            }
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
    }

    // The two fields close together, the swap a quiet word between them.
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 0

        FileField {
            objectName: "verifySignatureField"
            label: qsTr("Signature file")
            browseText: qsTr("Browse device for the signature file")
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

        Button {
            objectName: "verifySwap"
            Layout.alignment: Qt.AlignRight
            Layout.maximumWidth: parent.width
            flat: true
            topInset: 0
            bottomInset: 0
            implicitHeight: Theme.touch - 8
            font.pixelSize: Theme.labelSize
            text: qsTr("Swap files")
            enabled: sheet.signatureId >= 0 || sheet.originalId >= 0
            onClicked: sheet.swap()
        }

        FileField {
            objectName: "verifyOriginalField"
            label: qsTr("Original file")
            browseText: qsTr("Browse device for the original file")
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
    }

    OpenDialog {
        id: picker
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
        ActionButton {
            kind: "text"
            text: qsTr("Cancel")
            onClicked: sheet.close()
        },
        ActionButton {
            objectName: "verifyRun"
            text: qsTr("Verify")
            enabled: sheet.signatureId >= 0 && sheet.originalId >= 0
                     && sheet.signatureId !== sheet.originalId
            onClicked: {
                // A second tap while it slides away: already done.
                if (sheet.acted) return
                sheet.acted = true
                sheet.close()
                sheet.verifyRequested(sheet.signatureId, sheet.originalId)
            }
        }
    ]
}

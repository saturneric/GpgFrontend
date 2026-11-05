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
    // file @p id: what is known of it (a signature made here knows its
    // file), else what the names suggest. unique: exactly one partner, known
    // or the only name that matches, and both files there to be read --
    // never a judgement on the signature, which only verifying makes.
    function pair(id) {
        const file = workspace.entry(id)
        const result = {"signatureId": -1, "originalId": -1,
                        "signatureHow": "", "originalHow": "", "unique": false,
                        "inline": false}
        // The desktop's rule for what a name is and what its other half
        // is called.
        const match = workspace.verifyMatch(file.name || "")
        // The data and its signature in one file, verified alone: what the
        // desktop takes for one by its name (.gpg, .pgp); and an .asc with
        // no data file beside it whose first bytes are a message signed in
        // the clear (where the desktop asks, and is told to leave the data
        // file empty).
        const asInline = () => {
            result.signatureId = id
            result.inline = true
            result.unique = usable(id)
            return result
        }
        if (file.madeBy !== "sign" && match.role === "combined") return asInline()
        let found = []
        if (file.madeBy === "sign") {
            result.signatureId = id
            if (workspace.entry(file.sourceId).fileId !== undefined) {
                result.originalId = file.sourceId
                result.originalHow = "linked"
            }
        } else if (match.role === "signature") {
            // A message in its first bytes (signed in the clear, or
            // encrypted) is verified alone, whatever file is named like
            // the data it would be a signature of.
            if (file.content === "message") return asInline()
            result.signatureId = id
            found = findAll(match.partners)
            if (found.length > 0) {
                result.originalId = found[0]
                result.originalHow = "suggested"
            }
        } else {
            result.originalId = id
            const made = findSignatureOf(id)
            if (made >= 0) {
                result.signatureId = made
                result.signatureHow = "linked"
            } else {
                found = findAll(match.role === "data" ? match.partners
                                    : [file.name + ".sig", file.name + ".asc"])
                if (found.length > 0) {
                    result.signatureId = found[0]
                    result.signatureHow = "suggested"
                }
            }
        }
        const linked = result.signatureHow === "linked"
                       || result.originalHow === "linked"
        result.unique = result.signatureId >= 0 && result.originalId >= 0
                        && result.signatureId !== result.originalId
                        && (linked || found.length === 1)
                        && usable(result.signatureId) && usable(result.originalId)
        return result
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

    // The ids of those of @p names that are in the list, in their order;
    // two files of one name (the second listed as "name (2).ext") both, so
    // which is meant is never taken as known.
    function findAll(names) {
        const ids = []
        for (let i = 0; i < names.length; ++i) {
            for (const id of workspace.idsNamed(names[i])) {
                if (!ids.includes(id)) ids.push(id)
            }
        }
        return ids
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

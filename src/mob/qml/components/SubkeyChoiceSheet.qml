// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// Which subkey a key should use, asked as the desktop asks it when a chosen
// key has more than one that could encrypt or sign (WorkSelection's
// subkeyChoices): one of them, or the engine's own choice. Cancel, Back or
// pulling it away start nothing.
BottomSheet {
    id: sheet

    // {key, name, role, subkeys: [{fingerprint, keyId, algorithm, created,
    // expires, primary}]}
    property var choice: ({})
    // The subkey chosen; empty for the engine's choice.
    property string chosen: ""
    // Called with the subkey chosen ("" for the engine's) when the person
    // goes on; dropped otherwise.
    property var goOn: null

    function ask(c, onGoOn) {
        choice = c
        chosen = ""
        goOn = onGoOn
        acted = false
        open()
    }

    onClosed: goOn = null

    objectName: "subkeyChoiceSheet"
    title: qsTr("Which subkey?")

    Label {
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        text: sheet.choice.role === "sign"
              ? qsTr("%1 has more than one subkey that can sign. Which one "
                     + "should sign this?").arg(sheet.choice.name || "")
              : qsTr("%1 has more than one subkey that can encrypt. Which one "
                     + "should this be encrypted to?").arg(sheet.choice.name || "")
    }

    // One list, edge to edge, without the sheet's spacing between its rows.
    ColumnLayout {
        Layout.fillWidth: true
        Layout.leftMargin: -Theme.sheetPad
        Layout.rightMargin: -Theme.sheetPad
        spacing: 0

        RadioDelegate {
            objectName: "subkeyChoice_any"
            Layout.fillWidth: true
            leftPadding: Theme.sheetPad
            rightPadding: Theme.sheetPad - 8
            text: qsTr("Let GpgFrontend choose")
            checkable: true
            autoExclusive: false
            checked: sheet.chosen === ""
            onToggled: checked = Qt.binding(() => sheet.chosen === "")
            onClicked: sheet.chosen = ""
        }

        Repeater {
            model: sheet.choice.subkeys || []

            RadioDelegate {
                id: row

                required property var modelData

                objectName: "subkeyChoice_" + modelData.keyId
                Layout.fillWidth: true
                leftPadding: Theme.sheetPad
                rightPadding: Theme.sheetPad - 8
                checkable: true
                autoExclusive: false
                checked: sheet.chosen === modelData.fingerprint
                onToggled: checked = Qt.binding(() => sheet.chosen === row.modelData.fingerprint)
                onClicked: sheet.chosen = modelData.fingerprint
                text: [modelData.primary ? qsTranslate("KeyFacts", "Primary key")
                                         : qsTranslate("KeyFacts", "Subkey"),
                       (modelData.keyId || "").slice(-16)].join(" ")

                contentItem: ColumnLayout {
                    spacing: 2

                    Label {
                        Layout.fillWidth: true
                        Layout.rightMargin: 48
                        text: row.text
                        font.family: Theme.mono
                        elide: Text.ElideRight
                    }
                    Label {
                        Layout.fillWidth: true
                        Layout.rightMargin: 48
                        wrapMode: Text.Wrap
                        color: Material.secondaryTextColor
                        font.pixelSize: Theme.secondarySize
                        text: [row.modelData.algorithm || "",
                               qsTr("made %1").arg(Theme.date(row.modelData.created)),
                               Theme.date(row.modelData.expires).length > 0
                               ? qsTranslate("Wording", "Valid until %1")
                                 .arg(Theme.date(row.modelData.expires))
                               : qsTranslate("Wording", "Does not expire")]
                              .filter(t => t.length > 0).join(" · ")
                    }
                }
            }
        }
    }

    footer: [
        Item {
            Layout.fillWidth: true
        },
        ActionButton {
            kind: "text"
            objectName: "subkeyChoiceCancel"
            text: qsTr("Cancel")
            onClicked: sheet.close()
        },
        ActionButton {
            objectName: "subkeyChoiceGo"
            text: qsTr("Continue")
            onClicked: {
                if (sheet.acted) return
                sheet.acted = true
                const next = sheet.goOn
                const chosen = sheet.chosen
                sheet.goOn = null
                sheet.close()
                if (next) next(chosen)
            }
        }
    ]
}

// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// One key in a list of keys to choose from, the same in every such list:
// the key toolbox (recipients, the signer) and the pickers that fill a key
// group or a category. The key's face, its mark at the end showing whether
// it is chosen; what is chosen is the list's own (chosen), a tap asks it
// (chooseRequested) and never changes the row by itself.
ItemDelegate {
    id: row

    required property string name
    required property string email
    required property string keyId
    required property string secretState
    required property string capabilities
    // A key group in the toolbox's list (its rows give these from the
    // model); a picker's are keys.
    property bool isKey: true
    property bool usable: true
    property var members: []

    // Whether it is chosen, as the list holds it.
    property bool chosen: false
    // One of a kind (the signer): a round mark; else a check box.
    property bool exclusive: false
    // Why it cannot be chosen, or that it is in already: said under it.
    property string note: ""
    property bool problem: false

    signal chooseRequested()

    width: ListView.view ? ListView.view.width : implicitWidth
    leftPadding: Theme.sheetPad
    rightPadding: 4
    topPadding: 6
    bottomPadding: 6
    // Checkable, so TalkBack says whether it is chosen; a tap gives the row
    // back its binding at once.
    checkable: true
    checked: chosen
    onToggled: checked = Qt.binding(() => row.chosen)
    onClicked: chooseRequested()
    Accessible.role: exclusive ? Accessible.RadioButton : Accessible.CheckBox
    Accessible.checkable: true
    Accessible.checked: chosen
    Accessible.name: Wording.keyTitle(name, email, keyId)

    contentItem: KeyChoiceFace {
        idBelow: true
        opacity: row.enabled ? 1 : 0.6
        name: row.name
        email: row.email
        keyId: row.keyId
        isKey: row.isKey
        usable: row.usable
        members: row.members
        secretState: row.secretState
        capabilities: row.capabilities
        note: row.note
        problem: row.problem

        // One mark at the end, showing the choice; a tap on it is a tap on
        // the row (never a second toggle).
        CheckBox {
            objectName: "keyChoiceMark"
            visible: !row.exclusive
            enabled: row.enabled
            checkable: false
            checked: row.chosen
            focusPolicy: Qt.NoFocus
            onClicked: row.chooseRequested()
            Accessible.ignored: true
        }

        RadioButton {
            visible: row.exclusive
            enabled: row.enabled
            checkable: false
            checked: row.chosen
            focusPolicy: Qt.NoFocus
            onClicked: row.chooseRequested()
            Accessible.ignored: true
        }
    }
}

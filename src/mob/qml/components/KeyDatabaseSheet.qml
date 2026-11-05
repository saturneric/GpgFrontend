// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// Which key database the app works in, and a new one by name, as the App
// Store build offers them. Moving, renaming and removing databases are the
// desktop's.
//
// Only the choice: once a database is chosen (or named) this sheet goes and
// the switch is the switching sheet's (KeyDatabaseSwitchSheet), so the two
// never show at once and this one keeps no room for a progress bar. The
// database marked here is the one in use, which changes only once a switch
// has worked.
BottomSheet {
    id: sheet

    property KeyDatabasesController databases
    // What runs a switch once one is chosen (KeyDatabaseSwitchSheet), placed
    // beside this sheet by the page: declared inside it, a closed sheet, it
    // would have no window to come up in.
    property KeyDatabaseSwitchSheet switcher
    // Naming a new database rather than choosing one.
    property bool naming: false
    // The core is busy with the databases (opening one at startup): nothing
    // to choose until it is done.
    readonly property bool busy: databases !== null && databases.busy

    function begin() {
        // A switch still running is shown, not a second one begun.
        if (switcher && switcher.switching) {
            switcher.open()
            return
        }
        naming = false
        problem.text = ""
        open()
    }

    function beginNew() {
        name.text = databases.suggestName()
        problem.text = ""
        naming = true
        name.forceActiveFocus()
        name.selectAll()
    }

    // The database @p id, called @p label, chosen: this sheet goes, the
    // switch runs. The one in use, chosen again, changes nothing.
    function choose(id, label, current) {
        if (current || !switcher) return
        close()
        switcher.switchTo(id, label)
    }

    function create() {
        const why = databases.nameProblem(name.text)
        problem.text = why
        if (why.length > 0 || !switcher) return
        const label = name.text.trim()
        close()
        switcher.create(label)
    }

    objectName: "keyDatabaseSheet"
    title: naming ? qsTr("New key database") : qsTr("Key database")
    description: naming
                 ? qsTr("Kept in your profile, so it travels with a profile "
                        + "you export. Just choose a name.")
                 : qsTr("Each key database has its own keys. The one you choose "
                        + "is used from now on.")
    stepBack: function() {
        if (!sheet.naming) return false
        sheet.naming = false
        problem.text = ""
        return true
    }

    // One list, edge to edge, without the sheet's spacing between its
    // choices: a row a touch target tall, its name beside the mark.
    ColumnLayout {
        Layout.fillWidth: true
        Layout.leftMargin: -Theme.sheetPad
        Layout.rightMargin: -Theme.sheetPad
        visible: !sheet.naming
        spacing: 0

        Repeater {
            model: sheet.databases ? sheet.databases.databases : []

            RadioDelegate {
                required property var modelData
                objectName: "keyDatabase_" + modelData.name
                Layout.fillWidth: true
                leftPadding: Theme.sheetPad
                rightPadding: Theme.sheetPad - 8
                text: modelData.name
                // The one in use, until another has worked: checkable, so
                // TalkBack says which, but a tap that ticks the row gives it
                // back its binding at once; the switch is asked for.
                checkable: true
                autoExclusive: false
                checked: modelData.current
                onToggled: checked = Qt.binding(() => modelData.current)
                enabled: !sheet.busy
                onClicked: sheet.choose(modelData.id, modelData.name, modelData.current)
            }
        }
    }

    TextField {
        id: name
        objectName: "keyDatabaseName"
        Layout.fillWidth: true
        visible: sheet.naming
        enabled: !sheet.busy
        placeholderText: qsTr("Name, for example Work")
        maximumLength: 64
        inputMethodHints: Qt.ImhNoPredictiveText
        onTextEdited: problem.text = ""
        onAccepted: sheet.create()
    }

    ProblemLabel {
        id: problem
        objectName: "keyDatabaseProblem"
    }


    footer: [
        ActionButton {
            objectName: "keyDatabaseNew"
            kind: "secondary"
            visible: !sheet.naming
            enabled: !sheet.busy
            text: qsTr("New key database")
            icon.source: Theme.icon("add")
            onClicked: sheet.beginNew()
        },
        Item {
            Layout.fillWidth: true
        },
        ActionButton {
            kind: "text"
            text: sheet.naming ? qsTr("Cancel") : qsTr("Close")
            enabled: !sheet.busy
            onClicked: sheet.naming ? sheet.stepBack() : sheet.close()
        },
        ActionButton {
            objectName: "keyDatabaseCreate"
            visible: sheet.naming
            text: qsTr("Create")
            enabled: !sheet.busy && name.text.trim().length > 0
            onClicked: sheet.create()
        }
    ]
}

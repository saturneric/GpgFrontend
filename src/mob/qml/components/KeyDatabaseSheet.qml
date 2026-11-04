// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// Which key database the app works in, and a new one by name, as the App
// Store build offers them. Moving, renaming and removing databases are the
// desktop's.
BottomSheet {
    id: sheet

    property KeyDatabasesController databases
    // Naming a new database rather than choosing one.
    property bool naming: false
    readonly property bool busy: databases !== null && databases.busy

    // Said once the sheet is closed: "Now using ...".
    signal finished(string message)

    function begin() {
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

    function create() {
        const why = databases.nameProblem(name.text)
        problem.text = why
        if (why.length === 0) databases.create(name.text)
    }

    objectName: "keyDatabaseSheet"
    title: naming ? qsTr("New key database") : qsTr("Key database")
    stepBack: function() {
        if (sheet.busy) return true
        if (!sheet.naming) return false
        sheet.naming = false
        problem.text = ""
        return true
    }

    function switched(succeeded, error) {
        if (!opened) return
        if (succeeded) {
            close()
            finished(qsTr("Now using %1.").arg(databases.currentName))
        } else {
            problem.text = error
        }
    }

    function created(succeeded, error) {
        if (!opened) return
        if (succeeded) {
            close()
            finished(qsTr("%1 is made, and in use.").arg(databases.currentName))
        } else {
            problem.text = error
        }
    }

    // Signals named Signal... have no handler property in QML. Once per
    // controller: an initial value raises no change.
    property QtObject listening: null
    function listen() {
        if (!databases || listening === databases) return
        databases.SignalSwitched.connect(sheet.switched)
        databases.SignalCreated.connect(sheet.created)
        listening = databases
    }
    onDatabasesChanged: listen()
    Component.onCompleted: listen()

    Label {
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        color: Material.secondaryTextColor
        text: sheet.naming
              ? qsTr("Kept in your profile, so it travels with a profile "
                     + "you export. Just choose a name.")
              : qsTr("Each key database has its own keys. The one you choose "
                     + "is used from now on.")
    }

    // One list, without the sheet's spacing between its choices.
    ColumnLayout {
        Layout.fillWidth: true
        visible: !sheet.naming
        spacing: 0

        Repeater {
            model: sheet.databases ? sheet.databases.databases : []

            RadioButton {
                required property var modelData
                objectName: "keyDatabase_" + modelData.name
                Layout.fillWidth: true
                text: modelData.name
                checked: modelData.current
                enabled: !sheet.busy
                onClicked: {
                    problem.text = ""
                    sheet.databases.switchTo(modelData.id)
                }
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

    RowLayout {
        Layout.fillWidth: true
        visible: sheet.busy
        spacing: Theme.gap

        BusyIndicator {
            running: sheet.busy
            Layout.preferredWidth: 32
            Layout.preferredHeight: 32
        }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: qsTr("Opening the key databases…")
        }
    }

    footer: [
        Button {
            objectName: "keyDatabaseNew"
            visible: !sheet.naming
            enabled: !sheet.busy
            text: qsTr("New key database")
            flat: true
            onClicked: sheet.beginNew()
        },
        Item {
            Layout.fillWidth: true
        },
        Button {
            text: sheet.naming ? qsTr("Cancel") : qsTr("Close")
            flat: true
            enabled: !sheet.busy
            onClicked: sheet.naming ? sheet.stepBack() : sheet.close()
        },
        Button {
            objectName: "keyDatabaseCreate"
            visible: sheet.naming
            text: qsTr("Create")
            highlighted: true
            enabled: !sheet.busy && name.text.trim().length > 0
            onClicked: sheet.create()
        }
    ]
}

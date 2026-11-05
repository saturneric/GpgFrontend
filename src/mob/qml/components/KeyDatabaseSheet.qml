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
    // Switched: waiting for the new database's keys before closing.
    property bool awaitingKeys: false
    // The database being switched to or made, by name, for the progress line.
    property string targetName: ""
    // From the tap until the new database's keys are read, or it failed.
    property bool switching: false
    readonly property bool busy: (databases !== null && databases.busy) || awaitingKeys
                                 || switching

    function hold(target) {
        targetName = target
        switching = true
    }

    // The new database is in use and its keys are shown: the page behind
    // says so by its name and its list, nothing more is needed.
    signal finished()

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
        if (why.length === 0) {
            hold(name.text.trim())
            databases.create(name.text)
        }
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

    // Only a switch this sheet began: the core may switch on its own.
    function switched(succeeded, error) {
        if (!switching) return
        if (succeeded) {
            awaitKeys()
            return
        }
        switching = false
        problem.text = error
    }

    function created(succeeded, error) {
        switched(succeeded, error)
    }

    // The database is in use; the sheet stays, busy, until its keys are
    // read, so the list behind it never shows the old database's keys or
    // an empty "No keys yet" meanwhile.
    function awaitKeys() {
        awaitingKeys = true
        keysSettled()
    }

    function keysSettled() {
        if (!awaitingKeys || KeyListController.refreshing || !KeyListController.loaded)
            return
        awaitingKeys = false
        switching = false
        if (KeyListController.error.length > 0) {
            problem.text = qsTr("%1 is in use, but its keys could not be read: %2")
                           .arg(targetName).arg(KeyListController.error)
            return
        }
        close()
        finished()
    }

    // The key list's state, followed through a property of the sheet's own:
    // its change signal (Signal...) has no handler in QML.
    readonly property bool keysPending: KeyListController.refreshing
                                        || !KeyListController.loaded
    onKeysPendingChanged: Qt.callLater(sheet.keysSettled)

    interactive: !busy
    onClosed: awaitingKeys = false


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

    // The one progress shown while switching, under the title, always
    // taking its room: it comes and goes without moving anything.
    ColumnLayout {
        objectName: "keyDatabaseBusy"
        Layout.fillWidth: true
        spacing: 2
        opacity: sheet.busy ? 1 : 0
        Accessible.role: Accessible.ProgressBar
        Accessible.name: progressText.text

        ProgressBar {
            Layout.fillWidth: true
            indeterminate: sheet.busy
        }

        Label {
            id: progressText
            Layout.fillWidth: true
            elide: Text.ElideRight
            font.pixelSize: Theme.secondarySize
            color: Material.secondaryTextColor
            text: sheet.naming ? qsTr("Making %1…").arg(sheet.targetName)
                               : qsTr("Switching to %1…").arg(sheet.targetName)
        }
    }

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
                    sheet.hold(modelData.name)
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

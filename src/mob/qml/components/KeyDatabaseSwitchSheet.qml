// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// A key database being switched to (or made), from the choice until its
// keys are read: the one thing the person sees meanwhile, apart from the
// choosing sheet (KeyDatabaseSheet), which has gone by then.
//
// It owns the switch: one at a time, a second asked for while one runs is
// refused. It comes up only when the switch takes a moment -- one that ends
// at once never flashes it -- and while it runs it cannot be put away (Back
// and a tap beside it do nothing): the database is never half switched
// behind it. The page behind names the new database only once its keys are
// listed (KeysPage.databaseName), never before. A switch that fails says why
// here, and the database in use stays the one before.
BottomSheet {
    id: sheet

    property var databases: null
    // The database being switched to or made, by name.
    property string targetName: ""
    // Making a new database rather than switching to one.
    property bool creating: false
    // From the choice until the new database's keys are read, or it failed.
    property bool switching: false
    // Switched: waiting for the new database's keys.
    property bool awaitingKeys: false
    // Why it failed; empty while it runs and after it worked.
    property string problem: ""
    // How long a switch may take before this sheet comes up for it.
    property int showAfter: 300

    // The new database is in use and its keys are listed.
    signal finished()

    // Switch to the database @p id, called @p name; false when another
    // switch is still running (nothing is asked of the core then).
    function switchTo(id, name) {
        if (!start(name, false)) return false
        databases.switchTo(id)
        return true
    }

    // Make a database called @p name and switch to it; false while another
    // switch runs.
    function create(name) {
        if (!start(name, true)) return false
        databases.create(name)
        return true
    }

    function start(name, making) {
        if (switching || !databases) return false
        targetName = name
        creating = making
        problem = ""
        awaitingKeys = false
        switching = true
        reveal.restart()
        return true
    }

    // The core's answer (Signal... has no handler in QML: connected below).
    // Only a switch begun here: the core may switch on its own.
    function switched(succeeded, error) {
        if (!switching || awaitingKeys) return
        if (!succeeded) {
            fail(error)
            return
        }
        awaitingKeys = true
        keysSettled()
    }

    function fail(text) {
        reveal.stop()
        switching = false
        awaitingKeys = false
        problem = text
        open()
    }

    function keysSettled() {
        if (!awaitingKeys || KeyListController.refreshing || !KeyListController.loaded)
            return
        if (KeyListController.error.length > 0) {
            fail(qsTr("%1 is in use, but its keys could not be read: %2")
                 .arg(targetName).arg(KeyListController.error))
            return
        }
        reveal.stop()
        awaitingKeys = false
        switching = false
        close()
        finished()
    }

    // The key list's state, followed through a property of the sheet's own:
    // its change signal (Signal...) has no handler in QML.
    readonly property bool keysPending: KeyListController.refreshing
                                        || !KeyListController.loaded
    onKeysPendingChanged: Qt.callLater(sheet.keysSettled)

    // Comes up only for a switch that takes a moment: no flash for one that
    // is done at once. Not a wait: the switch runs meanwhile.
    Timer {
        id: reveal
        interval: sheet.showAfter
        onTriggered: if (sheet.switching) sheet.open()
    }

    property QtObject listening: null
    function listen() {
        if (!databases || listening === databases) return
        databases.SignalSwitched.connect(sheet.switched)
        databases.SignalCreated.connect(sheet.switched)
        listening = databases
    }
    onDatabasesChanged: listen()
    Component.onCompleted: listen()

    objectName: "keyDatabaseSwitch"
    title: problem.length > 0 ? (creating ? qsTr("Key database not made")
                                          : qsTr("Key database not switched"))
         : creating ? qsTr("Making key database") : qsTr("Switching key database")
    interactive: !switching

    // Where it goes, and that it is on its way.
    Label {
        objectName: "keyDatabaseTarget"
        Layout.fillWidth: true
        visible: sheet.problem.length === 0
        text: sheet.targetName
        elide: Text.ElideRight
        font.pixelSize: Theme.bodySize
    }

    ProgressBar {
        objectName: "keyDatabaseProgress"
        Layout.fillWidth: true
        visible: sheet.problem.length === 0
        // Nothing to count: the core says only when it is done.
        indeterminate: sheet.switching
        Accessible.name: sheet.creating ? qsTr("Making %1…").arg(sheet.targetName)
                                        : qsTr("Switching to %1…").arg(sheet.targetName)
    }

    ProblemLabel {
        objectName: "keyDatabaseSwitchProblem"
        text: sheet.problem
    }

    // No Close of its own: Back, a tap beside it or pulling it down put it
    // away, as every sheet does.
}

// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// A passphrase the core asks for while an operation runs, from the bottom
// as the other sheets come. Cancelling ends the operation as cancelled, and so
// does Back, pulling the sheet away, or going to the background.
BottomSheet {
    id: sheet

    readonly property var request: PassphraseBroker.current
    readonly property bool asking: request.info !== undefined
    // A passphrase being set, typed twice. Not askForNew: rPGP also sets that
    // for symmetric decryption, to keep it out of the passphrase cache.
    readonly property bool choosing: request.confirm === true

    // Answered once: a second tap on OK while the sheet goes (or before the
    // next request is shown) would answer the next request with nothing.
    property bool answered: false

    function clear() {
        passphrase.text = ""
        repeat.text = ""
    }

    function submit() {
        if (answered || !asking || !ok.enabled) return
        answered = true
        const text = passphrase.text
        clear()
        PassphraseBroker.submit(text)
    }

    objectName: "passphraseSheet"
    // A tap beside it is not an answer: only Cancel, Back or pulling it away.
    closePolicy: Popup.NoAutoClose
    // Only the person cancels it: closing it to show a share would cancel
    // the operation that asks.
    keepOpen: true
    // Turned down by Back or Cancel only, never by a pull that slipped.
    dragToClose: false
    title: choosing ? qsTr("Choose a passphrase") : qsTr("Enter the passphrase")

    onAskingChanged: asking ? open() : close()
    // A request after another: answer it afresh. One still waiting when
    // the sheet was closed on the last (Back cancels only that one) brings
    // the sheet back up for it.
    // (Signals named Signal... have no on-handler, not even in Connections:
    // connected below.)
    function currentChanged() {
        answered = false
        if (asking && !visible) Qt.callLater(reopen)
    }
    function reopen() {
        if (asking && !visible) open()
    }
    Component.onCompleted: {
        PassphraseBroker.SignalWipeInput.connect(sheet.clear)
        PassphraseBroker.SignalCurrentChanged.connect(sheet.currentChanged)
        if (asking) open()
    }
    // The broker outlives the sheet: let go of it with the sheet.
    Component.onDestruction: {
        PassphraseBroker.SignalWipeInput.disconnect(sheet.clear)
        PassphraseBroker.SignalCurrentChanged.disconnect(sheet.currentChanged)
    }

    onAboutToShow: clear()
    // Ready to type into, keyboard up.
    onOpened: passphrase.forceActiveFocus()
    // Closed while still asked: the person turned it down.
    onClosed: {
        clear()
        if (asking && !answered) PassphraseBroker.cancel()
    }

    Label {
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        visible: text.length > 0
        text: sheet.request.key ? sheet.request.key : ""
        font.weight: Font.DemiBold
    }

    Label {
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        color: sheet.request.retry ? Theme.danger : Material.secondaryTextColor
        text: sheet.request.retry
              ? qsTr("That passphrase was wrong. Try again.")
              : sheet.choosing
                ? (sheet.request.key
                   ? qsTr("The passphrase protects this secret key.")
                   : qsTr("Keep it safe: it is needed to use what it "
                          + "protects, and anyone who has it can."))
                : sheet.request.key
                  ? qsTr("The passphrase unlocks this secret key.")
                  : qsTr("The passphrase this message was encrypted "
                         + "with.")
    }

    // Leaving the app declines it (nothing secret is left waiting): said
    // here, so a trip to a password manager is not a surprise.
    Label {
        objectName: "passphraseLeaving"
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        color: Material.secondaryTextColor
        font.pixelSize: Theme.labelSize
        text: qsTr("Leaving GpgFrontend now cancels this. Copy the passphrase "
                   + "first, or type it.")
    }

    SecretField {
        id: passphrase
        objectName: "passphraseField"
        Layout.fillWidth: true
        placeholderText: qsTr("Passphrase")
        // Typed twice: on to the second field; else the keyboard's action
        // answers, as OK does.
        EnterKey.type: sheet.choosing ? Qt.EnterKeyNext : Qt.EnterKeyDone
        onAccepted: {
            if (sheet.choosing) repeat.forceActiveFocus()
            else if (text.length > 0) sheet.submit()
        }
    }

    SecretField {
        id: repeat
        objectName: "passphraseRepeat"
        Layout.fillWidth: true
        visible: sheet.choosing
        placeholderText: qsTr("Repeat the passphrase")
        EnterKey.type: Qt.EnterKeyDone
        onAccepted: sheet.submit()
    }

    // Why OK cannot be pressed yet, once the repeat is typed: said, as the
    // PIN sheet says it, not only shown by a button that does nothing.
    ProblemLabel {
        objectName: "passphraseMismatch"
        text: sheet.choosing && repeat.text.length > 0
              ? PassphraseBroker.entryProblem(passphrase.text, repeat.text, true) : ""
    }

    footer: [
        Item {
            Layout.fillWidth: true
        },
        ActionButton {
            kind: "text"
            objectName: "passphraseCancel"
            text: qsTr("Cancel")
            enabled: !sheet.answered
            onClicked: {
                sheet.answered = true
                sheet.clear()
                PassphraseBroker.cancel()
            }
        },
        ActionButton {
            id: ok
            objectName: "passphraseOk"
            text: qsTr("OK")
            // The desktop's rule: only a new passphrase is checked.
            // Nothing typed is no answer (and a second tap, the fields
            // cleared, never answers the next request with nothing).
            enabled: !sheet.answered && passphrase.text.length > 0
                     && PassphraseBroker.entryProblem(passphrase.text, repeat.text,
                                                      sheet.choosing).length === 0
            onClicked: sheet.submit()
        }
    ]
}

// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

BottomSheet {
    id: sheet

    // The new key's fingerprint: it was made.
    signal generated(string fingerprint)
    // Quick key did not make one, and why (never for a cancellation).
    signal quickFailed(string error)

    // Quick key runs: the form is not shown, the passphrase alone is asked.
    property bool quickRunning: false
    readonly property bool working: actions.busy || quickRunning
    // What is wrong with the user ID typed, by the desktop's rule: a name,
    // an email only if wanted, no mail delimiters in the name or comment.
    readonly property var identity: actions.checkIdentity(name.text, email.text,
                                                          comment.text)

    function begin() {
        name.text = ""
        email.text = ""
        comment.text = ""
        // Settings may have changed the profiles since.
        actions.reloadProfiles()
        profile.currentIndex = 0
        protect.checked = true
        // As it was last time, here or on the desktop.
        expert.checked = profileStore.expertMode
        withSubkey.checked = true
        problem.text = ""
        if (expert.checked) fromProfile()
        open()
    }

    // A made-up name and address, from the pattern in the key generation
    // settings: for a key nobody needs to recognise.
    function fillAnonymously() {
        const uid = profileStore.anonymousUid()
        name.text = uid.name
        email.text = uid.email
        comment.text = uid.comment
    }

    // Quick Generate, as the desktop's: an anonymous key from the default
    // profile, at once; only its passphrase is asked for.
    function quick() {
        // Once: a second tap while it runs makes no second key.
        if (working) return
        actions.reloadProfiles()
        const uid = profileStore.anonymousUid()
        quickRunning = true
        actions.generate(uid.name, uid.email, 0, true, uid.comment)
    }

    // An encryption subkey of the primary key's family, as the desktop
    // suggests it, whenever the primary key's algorithm changes.
    function suggestSubkey() {
        const ref = profileStore.suggestSubkey(primaryChooser.algo, "encrypt")
        if (ref.id !== undefined) subkeyChooser.choose(ref)
    }

    // Expert Mode starts from the profile that was chosen: its algorithms
    // and lifetimes, each now open to change.
    function fromProfile() {
        const offered = actions.profiles[profile.currentIndex]
        const all = profileStore.profiles
        // By the name it is stored under: the one shown is a translation,
        // and a hidden profile may be shown under the same.
        const p = offered ? all.find(q => q.storedName === offered.storedName) : undefined
        if (!p || p.available !== true) {
            suggestSubkey()
            return
        }
        primaryChooser.choose(p.primary)
        if ((p.primarySub || {}).id) primaryChooser.pair = p.primarySub
        primaryChooser.validity = p.validity
        withSubkey.checked = p.hasSubkey === true
        if (p.hasSubkey === true) {
            subkeyChooser.choose(p.subkey)
            if ((p.subkeySub || {}).id) subkeyChooser.pair = p.subkeySub
            subkeyChooser.validity = p.subkeyValidity
        } else {
            suggestSubkey()
        }
    }

    function create() {
        problem.text = ""
        if (!expert.checked) {
            actions.generate(name.text, email.text, profile.currentIndex,
                             protect.checked, comment.text)
            return
        }
        actions.generateCustom(name.text, email.text, primaryChooser.choices,
                               withSubkey.checked ? subkeyChooser.choices : ({}),
                               protect.checked, comment.text)
    }

    function finished(action, succeeded, detail) {
        if (action !== "generate") return
        const wasQuick = quickRunning
        quickRunning = false
        if (succeeded) {
            sheet.close()
            sheet.generated(detail.fingerprint)
            return
        }
        // Stopped by the person: nothing made, nothing to say, no form left.
        if (detail.reason === "cancelled") return
        if (wasQuick) sheet.quickFailed(detail.error)
        else problem.text = qsTr("The key could not be generated: %1").arg(detail.error)
    }

    objectName: "generateSheet"
    title: qsTr("Create a key")
    description: qsTr("Others see this name and email when they encrypt to "
                      + "you. Only the name is required.")
    interactive: !actions.busy

    // What makes the key; a test puts a stand-in here.
    property var actions: engine

    KeyActionsController {
        id: engine
    }

    KeyGenProfilesController {
        id: profileStore
    }

    // A stand-in is told by its test when it finishes.
    Component.onCompleted: engine.SignalFinished.connect(sheet.finished)

    TextField {
        id: name
        objectName: "generateName"
        Layout.fillWidth: true
        placeholderText: qsTr("Name")
    }

    // As the desktop: a name not typed yet is not yet wrong.
    ProblemLabel {
        objectName: "generateNameProblem"
        text: name.text.length > 0 ? sheet.identity.name : ""
    }

    TextField {
        id: email
        objectName: "generateEmail"
        Layout.fillWidth: true
        placeholderText: qsTr("Email (optional)")
        inputMethodHints: Qt.ImhEmailCharactersOnly | Qt.ImhNoAutoUppercase
    }

    ProblemLabel {
        objectName: "generateEmailProblem"
        text: sheet.identity.email
    }

    TextField {
        id: comment
        objectName: "generateComment"
        Layout.fillWidth: true
        placeholderText: qsTr("Comment (optional)")
    }

    ProblemLabel {
        objectName: "generateCommentProblem"
        text: sheet.identity.comment
    }

    Button {
        objectName: "generateAnonymous"
        flat: true
        Layout.leftMargin: -8
        text: qsTr("Fill in anonymously")
        icon.source: Theme.icon("redo")
        onClicked: sheet.fillAnonymously()
        Accessible.description: qsTr("Makes up a name and an address that "
                                     + "reach nobody.")
    }


    // Where every key starts: a profile. In Expert Mode its choices are
    // filled in below, to be changed; another profile fills them in again.
    ChoiceBox {
        id: profile
        objectName: "generateProfile"
        Layout.fillWidth: true
        model: actions.profiles
        textRole: "name"
        onActivated: if (expert.checked) sheet.fromProfile()
        Accessible.name: qsTr("Profile")
    }

    Label {
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        color: Material.secondaryTextColor
        font.pixelSize: Theme.labelSize
        text: profile.currentIndex >= 0 && actions.profiles.length > 0
              ? actions.profiles[profile.currentIndex].description : ""
    }

    // Every choice the profile made, open to change, as the desktop's wizard
    // shows them in Expert Mode.
    SettingSwitch {
        id: expert
        objectName: "generateExpert"
        text: qsTr("Expert mode")
        onToggled: {
            profileStore.expertMode = checked
            if (checked) sheet.fromProfile()
        }
    }

    SectionHeader {
        Layout.fillWidth: true
        Layout.leftMargin: -Theme.gap
        visible: expert.checked
        topPadding: 8
        text: qsTr("Primary key")
    }

    KeyAlgoChooser {
        id: primaryChooser
        objectName: "primaryChooser"
        Layout.fillWidth: true
        visible: expert.checked
        profiles: profileStore
        algos: expert.checked ? profileStore.primaryAlgos : []
        validities: actions.validities
    }

    SettingSwitch {
        id: withSubkey
        objectName: "generateWithSubkey"
        visible: expert.checked
        text: qsTr("Add a subkey")
        checked: true
    }

    KeyAlgoChooser {
        id: subkeyChooser
        objectName: "subkeyChooser"
        Layout.fillWidth: true
        visible: expert.checked && withSubkey.checked
        profiles: profileStore
        algos: expert.checked ? profileStore.subkeyAlgos : []
        subkey: true
        validities: actions.validities
    }

    SettingSwitch {
        id: protect
        text: qsTr("Protect it with a passphrase")
        checked: true
    }

    ProblemLabel {
        id: problem
    }

    BusyIndicator {
        Layout.alignment: Qt.AlignHCenter
        running: actions.busy
        visible: running
    }

    footer: [
        Item {
            Layout.fillWidth: true
        },
        ActionButton {
            kind: "text"
            text: qsTr("Cancel")
            enabled: !actions.busy
            onClicked: sheet.close()
        },
        ActionButton {
            objectName: "generateRun"
            text: qsTr("Create")
            enabled: !actions.busy && sheet.identity.ok === true
                     && (!expert.checked || (primaryChooser.complete
                         && (!withSubkey.checked || subkeyChooser.complete)))
            onClicked: sheet.create()
        }
    ]
}

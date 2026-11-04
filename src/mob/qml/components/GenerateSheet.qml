// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

BottomSheet {
    id: sheet

    // The new key's fingerprint, or a failure.
    signal generated(string fingerprint)

    function begin() {
        name.text = ""
        email.text = ""
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
    }

    // Quick Generate, as the desktop's: an anonymous key from the default
    // profile, at once; only its passphrase is asked for.
    function quick() {
        begin()
        expert.checked = false
        profile.currentIndex = 0
        fillAnonymously()
        create()
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
        const p = offered ? all.find(q => q.name === offered.name) : undefined
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
            actions.generate(name.text, email.text, profile.currentIndex, protect.checked)
            return
        }
        actions.generateCustom(name.text, email.text, primaryChooser.choices,
                               withSubkey.checked ? subkeyChooser.choices : ({}),
                               protect.checked)
    }

    function finished(action, succeeded, detail) {
        if (action !== "generate") return
        if (succeeded) {
            sheet.close()
            sheet.generated(detail.fingerprint)
        } else {
            problem.text = qsTr("The key could not be generated: %1")
                           .arg(detail.error)
        }
    }

    objectName: "generateSheet"
    title: qsTr("Create a key")
    closePolicy: actions.busy ? Popup.NoAutoClose
                              : Popup.CloseOnEscape | Popup.CloseOnPressOutside
    interactive: !actions.busy

    KeyActionsController {
        id: actions
    }

    KeyGenProfilesController {
        id: profileStore
    }

    Component.onCompleted: actions.SignalFinished.connect(sheet.finished)

    Label {
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        color: Material.secondaryTextColor
        text: qsTr("Others see this name and email when they encrypt to you.")
    }

    TextField {
        id: name
        objectName: "generateName"
        Layout.fillWidth: true
        placeholderText: qsTr("Name")
    }

    TextField {
        id: email
        objectName: "generateEmail"
        Layout.fillWidth: true
        placeholderText: qsTr("Email")
        inputMethodHints: Qt.ImhEmailCharactersOnly | Qt.ImhNoAutoUppercase
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
    ComboBox {
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
    Switch {
        id: expert
        objectName: "generateExpert"
        Layout.fillWidth: true
        Layout.leftMargin: -8
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

    Switch {
        id: withSubkey
        objectName: "generateWithSubkey"
        Layout.fillWidth: true
        Layout.leftMargin: -8
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

    Switch {
        id: protect
        Layout.fillWidth: true
        Layout.leftMargin: -8
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
        Button {
            text: qsTr("Cancel")
            flat: true
            enabled: !actions.busy
            onClicked: sheet.close()
        },
        Button {
            objectName: "generateRun"
            text: qsTr("Create")
            highlighted: true
            enabled: !actions.busy && name.text.trim().length > 0
                     && /^[^@\s]+@[^@\s]+$/.test(email.text.trim())
                     && (!expert.checked || (primaryChooser.complete
                         && (!withSubkey.checked || subkeyChooser.complete)))
            onClicked: sheet.create()
        }
    ]
}

// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// One key generation profile, as the desktop's profile editor shows it: its
// name and what it is for, the primary key's algorithm and lifetime, and the
// subkey's, if it has one.
Page {
    id: page

    // Where it is in the list, -1 for a new one, and what it holds now
    // (KeyGenProfilesController.profiles' fields).
    property int index: -1
    property var profile: ({})

    signal back()
    signal saved()

    function save() {
        const edited = {
            "storedName": name.text,
            "description": description.text,
            "primary": primaryChooser.algo,
            "primarySub": primaryChooser.pair,
            "validity": primaryChooser.validity,
            "hasSubkey": withSubkey.checked,
            "subkey": subkeyChooser.algo,
            "subkeySub": subkeyChooser.pair,
            "subkeyValidity": subkeyChooser.validity,
            "hidden": page.profile.hidden === true
        }
        const why = store.save(page.index, edited)
        problem.text = why
        if (why.length === 0) page.saved()
    }

    function suggestSubkey() {
        const ref = store.suggestSubkey(primaryChooser.algo, "encrypt")
        if (ref.id !== undefined) subkeyChooser.choose(ref)
    }

    Component.onCompleted: {
        if (page.index < 0) {
            withSubkey.checked = true
            suggestSubkey()
            return
        }
        name.text = profile.storedName || ""
        description.text = profile.description || ""
        primaryChooser.choose(profile.primary)
        if ((profile.primarySub || {}).id) primaryChooser.pair = profile.primarySub
        primaryChooser.validity = profile.validity || "2y"
        withSubkey.checked = profile.hasSubkey === true
        if (profile.hasSubkey === true) {
            subkeyChooser.choose(profile.subkey)
            if ((profile.subkeySub || {}).id) subkeyChooser.pair = profile.subkeySub
            subkeyChooser.validity = profile.subkeyValidity || "2y"
        } else {
            suggestSubkey()
        }
    }

    header: AppBar {
        title: page.index < 0 ? qsTr("New profile") : qsTr("Edit profile")
        canGoBack: true
        raised: scroller.contentItem.contentY > 0
        onBack: page.back()

        Button {
            objectName: "saveProfile"
            flat: true
            text: qsTr("Save")
            enabled: name.text.trim().length > 0 && primaryChooser.complete
                     && (!withSubkey.checked || subkeyChooser.complete)
            onClicked: page.save()
        }
    }

    KeyGenProfilesController {
        id: store
    }

    KeyActionsController {
        id: actions
    }

    ScrollView {
        id: scroller

        anchors.fill: parent
        // Under the bar, which takes the page's colour, nothing shows.
        clip: true
        contentWidth: availableWidth
        implicitWidth: 0

        ColumnLayout {
            width: scroller.availableWidth
            spacing: 8

            TextField {
                id: name
                objectName: "profileName"
                Layout.fillWidth: true
                Layout.leftMargin: Theme.gap
                Layout.rightMargin: Theme.gap
                Layout.topMargin: Theme.gap
                placeholderText: qsTr("Name")
            }

            // A sentence or two: it wraps rather than scrolls.
            TextArea {
                id: description
                objectName: "profileDescription"
                Layout.fillWidth: true
                Layout.leftMargin: Theme.gap
                Layout.rightMargin: Theme.gap
                wrapMode: TextEdit.Wrap
                placeholderText: qsTr("What it is for (optional)")
            }

            ProblemLabel {
                id: problem
                Layout.leftMargin: Theme.gap
                Layout.rightMargin: Theme.gap
            }

            SectionHeader {
                Layout.fillWidth: true
                text: qsTr("Primary key")
            }

            KeyAlgoChooser {
                id: primaryChooser
                objectName: "profilePrimary"
                Layout.fillWidth: true
                Layout.leftMargin: Theme.gap
                Layout.rightMargin: Theme.gap
                profiles: store
                algos: store.primaryAlgos
                validities: actions.validities
                // A profile keeps the algorithms, not what they are used for.
                showUsage: false
                onAlgoChanged: if (!withSubkey.checked) page.suggestSubkey()
            }

            SectionHeader {
                Layout.fillWidth: true
                text: qsTr("Subkey")
            }

            Switch {
                id: withSubkey
                objectName: "profileWithSubkey"
                Layout.fillWidth: true
                Layout.leftMargin: Theme.gap - 8
                text: qsTr("Add a subkey")
            }

            KeyAlgoChooser {
                id: subkeyChooser
                objectName: "profileSubkey"
                Layout.fillWidth: true
                Layout.leftMargin: Theme.gap
                Layout.rightMargin: Theme.gap
                Layout.bottomMargin: Theme.gap
                visible: withSubkey.checked
                profiles: store
                algos: store.subkeyAlgos
                subkey: true
                validities: actions.validities
                showUsage: false
            }
        }
    }
}

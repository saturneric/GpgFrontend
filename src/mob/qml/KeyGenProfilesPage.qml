// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// The key generation profiles, as the desktop's Settings keep them, and
// shared with it: which algorithms a new key is made of, under a name. One of
// them is what Create a key starts from. A profile this device cannot make is
// kept, and said to be unavailable.
Page {
    id: page

    signal back()
    // Edit the profile at @p index, or a new one when it is -1.
    signal editRequested(int index, var profile)

    StackView.onActivated: store.reload()

    header: AppBar {
        title: qsTr("Key generation profiles")
        canGoBack: true
        raised: list.contentY > list.originY
        onBack: page.back()

        BarButton {
            objectName: "addProfile"
            text: qsTr("New profile")
            icon.source: Theme.icon("add")
            onClicked: page.editRequested(-1, ({}))
        }

        OverflowButton {
            objectName: "profilesMore"

            MenuItem {
                objectName: "restoreProfiles"
                text: qsTr("Restore built-in profiles")
                onTriggered: {
                    store.restoreDefaults()
                    notice.show(qsTr("Built-in profiles restored. Your own are kept."))
                }
            }
        }
    }

    KeyGenProfilesController {
        id: store
    }

    ListView {
        id: list

        objectName: "profileList"
        anchors.fill: parent
        clip: true
        model: store.profiles

        header: Label {
            width: ListView.view.width
            padding: Theme.gap
            wrapMode: Text.Wrap
            color: Material.secondaryTextColor
            text: qsTr("Create a key offers these. They are the same on the "
                       + "desktop with this profile.")
        }

        delegate: ListRow {
            id: row

            required property var modelData

            objectName: "profile_" + modelData.index
            width: ListView.view.width
            text: modelData.name
            detail: [modelData.available ? modelData.algorithms
                                         : qsTr("Cannot be made on this device"),
                     modelData.description]
                    .filter(t => t && t.length > 0).join("\n")
            detailColor: modelData.available ? Material.secondaryTextColor : Theme.warning
            detailLines: 3
            onClicked: page.editRequested(modelData.index, modelData)

            Chip {
                visible: row.modelData.isDefault === true
                text: qsTr("Default")
                tone: Material.accentColor
            }

            Chip {
                visible: row.modelData.hidden === true
                text: qsTr("Hidden")
            }

            BarButton {
                text: qsTr("Actions for %1").arg(row.modelData.name)
                icon.source: Theme.icon("more_vert")
                onClicked: {
                    rowMenu.profile = row.modelData
                    rowMenu.popup(this, 0, height)
                }
            }
        }
    }

    Menu {
        id: rowMenu

        property var profile: ({})

        MenuItem {
            text: qsTr("Edit")
            onTriggered: page.editRequested(rowMenu.profile.index, rowMenu.profile)
        }
        MenuItem {
            objectName: "moveUp"
            text: qsTr("Move up")
            enabled: rowMenu.profile.index > 0
            onTriggered: store.move(rowMenu.profile.index, rowMenu.profile.index - 1)
        }
        MenuItem {
            objectName: "moveDown"
            text: qsTr("Move down")
            enabled: rowMenu.profile.index < store.profiles.length - 1
            onTriggered: store.move(rowMenu.profile.index, rowMenu.profile.index + 1)
        }
        MenuItem {
            objectName: "makeDefault"
            text: qsTr("Use by default")
            enabled: rowMenu.profile.isDefault !== true && rowMenu.profile.available === true
            onTriggered: store.setDefault(rowMenu.profile.index)
        }
        MenuItem {
            text: rowMenu.profile.hidden === true ? qsTr("Show in Create a key")
                                                  : qsTr("Hide from Create a key")
            onTriggered: {
                const changed = Object.assign({}, rowMenu.profile)
                changed.hidden = !(rowMenu.profile.hidden === true)
                const why = store.save(rowMenu.profile.index, changed)
                if (why.length > 0) notice.show(why)
            }
        }
        MenuSeparator {}
        MenuItem {
            objectName: "deleteProfile"
            text: qsTr("Delete")
            onTriggered: {
                confirmDelete.profile = rowMenu.profile
                confirmDelete.open()
            }
        }
    }

    MessageAlert {
        id: confirmDelete

        property var profile: ({})

        title: qsTr("Delete “%1”?").arg(profile.name || "")
        message: qsTr("Keys made with it are not touched. Built-in profiles "
                   + "can be restored later.")
        acceptText: qsTr("Delete")
        onAccepted: store.remove(profile.index)
    }

    Snackbar {
        id: notice
    }
}

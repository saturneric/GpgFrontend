// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Layouts
import GpgFrontend.Mobile

// The profiles on this phone: which one is open, opening another (the app
// starts again), a new one, and a profile to a file and back. Renaming and
// deleting profiles are the desktop's.
Page {
    id: page

    signal back()

    // What the page last said in passing (a pop-up, not in the item tree).
    readonly property alias notice: done
    readonly property alias sheet: profileSheet

    // The export's passphrase, from the sheet until the picker answers.
    property string exportSecret: ""

    header: AppBar {
        title: qsTr("Profiles")
        canGoBack: true
        raised: scroller.contentItem.contentY > 0
        onBack: page.back()
    }

    ProfilesController {
        id: profileStore
    }

    // Signals named Signal... have no handler property in QML.
    Component.onCompleted: {
        profileStore.SignalFailed.connect(function(error) { done.show(error) })
        profileStore.SignalExported.connect(function(ok, error) {
            done.show(ok ? qsTr("The profile is exported. Keep the "
                                + "passphrase safe.")
                         : error)
        })
        profileStore.SignalPassphraseNeeded.connect(function(error) {
            profileSheet.begin("import", error)
        })
        profileStore.SignalImported.connect(function(ok, error, id, name) {
            if (!ok) {
                done.show(error)
                return
            }
            confirmOpen.profileId = id
            confirmOpen.profileName = name
            confirmOpen.title = qsTr("“%1” is imported").arg(name)
            confirmOpen.open()
        })
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
            spacing: 0

            SectionHeader {
                Layout.fillWidth: true
                topPadding: 8
                text: qsTr("On this phone")
            }

            Repeater {
                model: profileStore.profiles

                ListRow {
                    id: profileRow

                    required property var modelData

                    objectName: "profile_" + modelData.id
                    Layout.fillWidth: true
                    text: modelData.name
                    detail: modelData.current ? qsTr("In use") : ""
                    selected: modelData.current
                    enabled: !profileStore.busy
                    onClicked: {
                        if (modelData.current) return
                        confirmOpen.profileId = modelData.id
                        confirmOpen.profileName = modelData.name
                        confirmOpen.title = qsTr("Open “%1”?").arg(modelData.name)
                        confirmOpen.open()
                    }

                    Icon {
                        visible: profileRow.modelData.current
                        symbol: "check_circle"
                        color: Material.accentColor
                    }
                }
            }

            ListRow {
                objectName: "newProfile"
                Layout.fillWidth: true
                symbol: "add"
                text: qsTr("New profile")
                enabled: !profileStore.busy
                onClicked: profileSheet.begin("create")
            }

            SectionHeader {
                Layout.fillWidth: true
                text: qsTr("Backup")
            }

            ListRow {
                objectName: "exportProfile"
                Layout.fillWidth: true
                symbol: "save"
                text: qsTr("Export this profile")
                detail: qsTr("One file with its keys and settings, sealed with "
                             + "a passphrase. The desktop opens it too.")
                enabled: !profileStore.busy
                onClicked: profileSheet.begin("export")
            }

            ListRow {
                objectName: "importProfile"
                Layout.fillWidth: true
                symbol: "note_add"
                text: qsTr("Import a profile")
                detail: qsTr("From a profile file (.gfp), made here or on the "
                             + "desktop. It is added beside this one.")
                enabled: !profileStore.busy
                onClicked: importDialog.open()
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.margins: Theme.gap
                visible: profileStore.busy
                spacing: Theme.gap

                BusyIndicator {
                    running: profileStore.busy
                    Layout.preferredWidth: 32
                    Layout.preferredHeight: 32
                }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    text: qsTr("Working on the profile file…")
                }
            }
        }
    }

    ProfileSheet {
        id: profileSheet
        profiles: profileStore

        onAccepted: function(value) {
            if (mode === "create") {
                const id = profileStore.create(value)
                if (id.length === 0) return
                confirmOpen.profileId = id
                confirmOpen.profileName = value.trim()
                confirmOpen.title = qsTr("“%1” is ready").arg(value.trim())
                confirmOpen.open()
            } else if (mode === "export") {
                page.exportSecret = value
                exportDialog.currentFile = profileStore.exportFileName()
                exportDialog.open()
            } else {
                profileStore.importWith(value)
            }
        }
        onDismissed: if (mode === "import") profileStore.cancelImport()
    }

    FileDialog {
        id: exportDialog
        fileMode: FileDialog.SaveFile
        onAccepted: {
            profileStore.exportTo(selectedFile, page.exportSecret)
            page.exportSecret = ""
        }
        onRejected: page.exportSecret = ""
    }

    FileDialog {
        id: importDialog
        fileMode: FileDialog.OpenFile
        onAccepted: profileStore.importFrom(selectedFile)
    }

    MessageAlert {
        id: confirmOpen

        property string profileId
        property string profileName

        message: qsTr("GpgFrontend starts again and opens “%1”. It opens "
                      + "this profile until you choose another.")
                 .arg(profileName)
        acceptText: qsTr("Open")
        acceptObjectName: "openProfile"
        onAccepted: profileStore.switchTo(profileId)
    }

    Snackbar {
        id: done
    }
}

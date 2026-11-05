// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// The settings a phone offers, in groups as the platform's own settings lay
// them out: each row its name and, at its end, the value in effect; what a
// row does rather than holds says so under its name. Settings the build pins
// are not offered at all.
Page {
    id: page

    // Not a stop for a screen reader: a page has no name of its own, only
    // its parts do (Qt gives a Page the role of an unnamed tab).
    Accessible.ignored: true

    // What the page last said in passing (a pop-up, not in the item tree).
    readonly property alias notice: done

    // The About page was asked for.
    signal aboutRequested()
    // What the desktop's About dialog tells besides: the build, the rPGP
    // engine, the translators.
    signal buildInfoRequested()
    signal engineInfoRequested()
    signal translatorsRequested()
    signal profilesRequested()
    // The key categories, to manage.
    signal categoriesRequested()
    // The profiles on this phone (not the key generation ones above).
    signal profileManagerRequested()
    // Another key database is in use: what is open of the last one goes.
    signal keyDatabaseSwitched()

    header: AppBar {
        title: qsTr("Settings")
        raised: scroller.contentItem.contentY > 0
    }

    SettingsModel {
        id: settings
    }

    KeyDatabasesController {
        id: keyDatabases
    }

    ProfilesController {
        id: phoneProfiles
    }

    // The language in use, by its name.
    readonly property string languageName: {
        for (const l of settings.languages) {
            if (l.code === settings.language) return l.name
        }
        return settings.languages.length > 0 ? settings.languages[0].name : ""
    }

    // The count changes behind the model's back as files are shared.
    onVisibleChanged: if (visible) settings.SignalSharedFilesChanged()

    function sharedFilesText() {
        const count = settings.sharedFileCount
        if (count === 0) return qsTr("None")
        const files = qsTr("%n file(s)", "", count)
        return qsTr("%1, %2").arg(files).arg(Theme.fileSize(settings.sharedFileSize))
    }

    // Stores @p value, and says so when it was not taken: the row then still
    // shows the value in effect, which the model says again.
    function store(row, value) {
        if (!settings.setValue(row, value))
            done.show(qsTr("That value could not be set."))
    }

    // Settings of the old operation buttons: the PGP button's menu now
    // offers every operation, so they decide nothing here. Still kept (a
    // profile carries them), never shown.
    readonly property var retired: ["mobile/show_encrypt_sign",
                                    "mobile/show_decrypt_verify",
                                    "mobile/default_encrypt_operation",
                                    "mobile/default_decrypt_operation"]

    // The model's rows of one section, each its name and its value.
    component SettingRows: Repeater {
        id: rows

        property string section

        model: settings

        ListRow {
            id: row

            required property int index
            required property string key
            required property string title
            required property string description
            required property string type
            required property var value
            required property string unit
            required property var choices
            required property bool custom
            required property string section

            // One of named values: there only while there is a choice.
            readonly property bool named: type === "string" && (choices || []).length > 0

            objectName: "setting_" + index
            Layout.fillWidth: true
            visible: section === rows.section && (!named || choices.length > 1)
                     && !page.retired.includes(key)
            text: title
            // A switch says what it does; a value speaks for itself.
            detail: type === "bool" ? description : ""
            valueText: type === "int" || named ? editor.describe(row.value, unit) : ""
            onClicked: {
                if (type === "bool") {
                    page.store(index, !row.value)
                } else if (type === "int" || named) {
                    editor.begin(index, title, unit, choices, custom, row.value)
                }
            }

            Switch {
                visible: row.type === "bool"
                checked: row.type === "bool" && row.value === true
                // Toggling replaces the binding; a refused change puts it
                // back, so the switch shows what is in effect.
                onToggled: {
                    if (!settings.setValue(row.index, checked)) {
                        checked = Qt.binding(() => row.type === "bool"
                                                   && row.value === true)
                        done.show(qsTr("That value could not be set."))
                    }
                }
            }
        }
    }

    ScrollView {
        id: scroller

        anchors.fill: parent
        // Under the bar, which takes the page's colour, nothing shows.
        clip: true
        contentWidth: availableWidth
        // Sized by the page, never the other way round: Material's Page takes
        // its implicit width from its content.
        implicitWidth: 0

        ColumnLayout {
            objectName: "settingsList"
            width: scroller.availableWidth
            spacing: 0

            SectionHeader {
                Layout.fillWidth: true
                topPadding: 8
                text: qsTr("General")
            }

            ListRow {
                objectName: "language"
                Layout.fillWidth: true
                text: qsTr("Language")
                valueText: page.languageName
                onClicked: languageSheet.open()
            }

            SectionHeader {
                Layout.fillWidth: true
                text: qsTr("Security")
            }

            SettingRows {
                section: "passphrases"
            }

            ListRow {
                objectName: "forgetPassphrases"
                Layout.fillWidth: true
                text: qsTr("Clear cached passphrases")
                // The cache times above hold only while GpgFrontend is in
                // front: leaving it clears them as well (AppLifecycle).
                detail: qsTr("The next operation asks for them again. They are "
                             + "also cleared whenever you leave GpgFrontend.")
                onClicked: {
                    settings.forgetPassphrases()
                    done.show(qsTr("Cached passphrases are cleared."))
                }
            }

            SectionHeader {
                Layout.fillWidth: true
                text: qsTr("Operations")
            }

            SettingRows {
                section: "operations"
            }

            SectionHeader {
                Layout.fillWidth: true
                text: qsTr("Keys")
            }

            SettingRows {
                section: "keys"
            }

            ListRow {
                objectName: "keyDatabase"
                Layout.fillWidth: true
                text: qsTr("Key database")
                detail: qsTr("Choose where your keys are kept, or start a new one.")
                valueText: keyDatabases.currentName
                onClicked: databaseSheet.begin()
            }

            ListRow {
                objectName: "keyCategories"
                Layout.fillWidth: true
                text: qsTr("Key categories")
                detail: qsTr("Your own groups of keys, shared with the desktop.")
                onClicked: page.categoriesRequested()

                Icon {
                    symbol: "chevron_right"
                    color: Material.secondaryTextColor
                }
            }

            ListRow {
                objectName: "keyGenProfiles"
                Layout.fillWidth: true
                text: qsTr("Key generation profiles")
                detail: qsTr("What Create a key offers, shared with the desktop.")
                onClicked: page.profilesRequested()

                Icon {
                    symbol: "chevron_right"
                    color: Material.secondaryTextColor
                }
            }

            SectionHeader {
                Layout.fillWidth: true
                text: qsTr("Storage")
            }

            ListRow {
                objectName: "clearSharedFiles"
                Layout.fillWidth: true
                text: qsTr("Temporary shared files")
                detail: settings.sharedFileCount === 0
                        ? qsTr("Kept for up to a day, so the app you share "
                               + "with can open them.")
                        : qsTr("Tap to remove them now. Apps you shared them "
                               + "with can no longer open them.")
                valueText: page.sharedFilesText()
                onClicked: {
                    if (settings.sharedFileCount > 0) confirmClear.open()
                    else done.show(qsTr("No shared files are kept."))
                }
            }

            SectionHeader {
                Layout.fillWidth: true
                text: qsTr("Profile")
            }

            ListRow {
                objectName: "profileManager"
                Layout.fillWidth: true
                text: qsTr("Profiles")
                detail: qsTr("Open another, start a new one, or keep a backup.")
                valueText: phoneProfiles.currentName
                onClicked: page.profileManagerRequested()
            }

            SectionHeader {
                Layout.fillWidth: true
                text: qsTr("About")
            }

            ListRow {
                objectName: "about"
                Layout.fillWidth: true
                text: qsTr("GpgFrontend")
                valueText: Qt.application.version
                onClicked: page.aboutRequested()
            }

            ListRow {
                objectName: "buildInfo"
                Layout.fillWidth: true
                text: qsTr("Build information")
                detail: qsTr("What this build is made of, and when.")
                onClicked: page.buildInfoRequested()
            }

            ListRow {
                objectName: "engineInfo"
                Layout.fillWidth: true
                text: qsTr("rPGP engine")
                detail: qsTr("The OpenPGP engine that does the work here.")
                onClicked: page.engineInfoRequested()
            }

            ListRow {
                objectName: "translators"
                Layout.fillWidth: true
                Layout.bottomMargin: Theme.gap
                text: qsTr("Translators")
                detail: qsTr("Who brought GpgFrontend to your language.")
                onClicked: page.translatorsRequested()
            }
        }
    }

    // The languages, one under the other, the one in use ticked; a tap
    // anywhere on a row chooses it, and every page is in it at once.
    BottomSheet {
        id: languageSheet

        objectName: "languageSheet"
        inset: 0
        title: qsTr("Language")

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            Repeater {
                model: settings.languages

                RadioDelegate {
                    required property var modelData

                    objectName: "language_" + (modelData.code || "system")
                    Layout.fillWidth: true
                    leftPadding: Theme.sheetPad
                    text: modelData.name
                    font.pixelSize: Theme.bodySize
                    // What is in use, always: checkable, so TalkBack says
                    // which is chosen, but a tap that ticks the row gives it
                    // back its binding at once.
                    checkable: true
                    autoExclusive: false
                    checked: settings.language === modelData.code
                    onToggled: checked = Qt.binding(() => settings.language === modelData.code)
                    onClicked: {
                        languageSheet.close()
                        settings.language = modelData.code
                    }
                }
            }
        }
    }

    SettingSheet {
        id: editor

        onChosen: function(row, value) {
            page.store(row, value)
        }
    }

    KeyDatabaseSheet {
        id: databaseSheet
        databases: keyDatabases
        switcher: databaseSwitch
    }

    KeyDatabaseSwitchSheet {
        id: databaseSwitch
        databases: keyDatabases
        onFinished: page.keyDatabaseSwitched()
    }

    MessageAlert {
        id: confirmClear
        title: qsTr("Remove shared files?")

        message: qsTr("Apps you shared files with can no longer open them.")

        acceptText: qsTr("Remove")

        onAccepted: {
            settings.clearSharedFiles()
            done.show(qsTr("Shared files are removed, and other apps "
                             + "can no longer open them."))
        }
    }

    Snackbar {
        id: done
    }
}

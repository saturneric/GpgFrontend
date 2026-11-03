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

    // What the page last said in passing (a pop-up, not in the item tree).
    readonly property alias notice: done

    // The About page was asked for.
    signal aboutRequested()

    header: AppBar {
        title: qsTr("Settings")
        raised: scroller.contentItem.contentY > 0
    }

    SettingsModel {
        id: settings
    }

    // Stores @p value, and says so when it was not taken: the row then still
    // shows the value in effect, which the model says again.
    function store(row, value) {
        if (!settings.setValue(row, value))
            done.show(qsTr("That value could not be set."))
    }

    // The model's rows of one section, each its name and its value.
    component SettingRows: Repeater {
        id: rows

        property string section

        model: settings

        ListRow {
            id: row

            required property int index
            required property string title
            required property string description
            required property string type
            required property var value
            required property string unit
            required property var choices
            required property bool custom
            required property string section

            objectName: "setting_" + index
            Layout.fillWidth: true
            visible: section === rows.section
            text: title
            valueText: type === "int" ? editor.describe(row.value, unit) : ""
            onClicked: {
                if (type === "bool") {
                    page.store(index, !row.value)
                } else if (type === "int") {
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
                text: qsTr("Security")
            }

            SettingRows {
                section: "passphrases"
            }

            ListRow {
                objectName: "forgetPassphrases"
                Layout.fillWidth: true
                text: qsTr("Clear cached passphrases")
                detail: qsTr("The next operation asks for them again.")
                onClicked: {
                    settings.forgetPassphrases()
                    done.show(qsTr("Cached passphrases are cleared."))
                }
            }

            SectionHeader {
                Layout.fillWidth: true
                text: qsTr("Keys")
            }

            SettingRows {
                section: "keys"
            }

            SectionHeader {
                Layout.fillWidth: true
                text: qsTr("About")
            }

            ListRow {
                objectName: "about"
                Layout.fillWidth: true
                Layout.bottomMargin: Theme.gap
                text: qsTr("GpgFrontend")
                valueText: Qt.application.version
                onClicked: page.aboutRequested()
            }
        }
    }

    SettingSheet {
        id: editor

        onChosen: function(row, value) {
            page.store(row, value)
        }
    }

    Snackbar {
        id: done
    }
}

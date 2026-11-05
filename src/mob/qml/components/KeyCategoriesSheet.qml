// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// The categories one key is in, from a long press on it: ticked in or out at
// once, and a new one made with the key already in it.
BottomSheet {
    id: sheet

    property KeyCategoriesController categories
    // The key: {keyId, name}.
    property var key: ({})
    // A category made from here takes the key in at once.
    property bool addToNew: false

    function begin(k) {
        key = k
        addToNew = false
        open()
    }

    function categoryAdded(id) {
        if (addToNew) categories.setMember(id, key.keyId, true)
        addToNew = false
    }

    // Signals named Signal... have no handler property in QML. Once per
    // controller: an initial value raises no change.
    property QtObject listening: null
    function listen() {
        if (!categories || listening === categories) return
        categories.SignalAdded.connect(sheet.categoryAdded)
        listening = categories
    }
    onCategoriesChanged: listen()
    Component.onCompleted: listen()

    objectName: "keyCategoriesSheet"
    inset: 0
    title: qsTr("Categories of %1").arg(key.name || "")

    Label {
        Layout.fillWidth: true
        Layout.leftMargin: Theme.sheetPad
        Layout.rightMargin: Theme.sheetPad
        visible: sheet.categories !== null && sheet.categories.categories.length === 0
        wrapMode: Text.Wrap
        color: Material.secondaryTextColor
        text: qsTr("No categories yet. Make one, and this key is put in it.")
    }

    // One list, without the sheet's spacing between its rows.
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 0

        Repeater {
            model: sheet.categories ? sheet.categories.categories : []

            CheckDelegate {
                id: choice

                required property var modelData
                // Read again whenever a membership changes.
                readonly property bool member: sheet.categories.revision >= 0
                    && sheet.categories.categoriesOf(sheet.key.keyId || "")
                                     .indexOf(modelData.id) >= 0

                objectName: "keyCategoryChoice_" + modelData.id
                Layout.fillWidth: true
                leftPadding: Theme.sheetPad
                text: modelData.name
                checked: member
                onToggled: sheet.categories.setMember(modelData.id,
                                                      sheet.key.keyId, checked)

                indicator: CheckBox {
                    x: choice.width - width - choice.rightPadding
                    anchors.verticalCenter: parent.verticalCenter
                    checked: choice.checked
                    // The row's toggle() would flip it without saying so:
                    // stored here, as a tap on the row stores it.
                    onToggled: sheet.categories.setMember(choice.modelData.id,
                                                          sheet.key.keyId,
                                                          checked)
                }

                contentItem: RowLayout {
                    spacing: 12

                    Rectangle {
                        implicitWidth: 12
                        implicitHeight: 12
                        radius: 6
                        color: choice.modelData.color
                    }

                    Label {
                        Layout.fillWidth: true
                        Layout.rightMargin: 48
                        text: choice.text
                        elide: Text.ElideRight
                        font.pixelSize: Theme.bodySize
                    }
                }
            }
        }

        ListRow {
            objectName: "keyCategoryNew"
            Layout.fillWidth: true
            leftPadding: Theme.sheetPad
            symbol: "add"
            symbolColor: Material.foreground
            text: qsTr("New category")
            onClicked: {
                sheet.addToNew = true
                editor.begin(null)
            }
        }
    }

    footer: [
        Item {
            Layout.fillWidth: true
        },
        Button {
            text: qsTr("Done")
            flat: true
            onClicked: sheet.close()
        }
    ]

    CategorySheet {
        id: editor
        categories: sheet.categories
        onClosed: Qt.callLater(() => sheet.addToNew = false)
    }
}

// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Layouts
import GpgFrontend.Mobile

// A key category's name and colour, new or to change, as the desktop's
// category dialog asks for them: the colour from the same swatches, or any
// other, typed as HEX or picked in Qt's own colour dialog. Kept as
// "#rrggbb", as the desktop keeps it.
BottomSheet {
    id: sheet

    property KeyCategoriesController categories
    // The category being changed; empty for a new one.
    property string categoryId: ""
    property string color: ""

    // A colour as typed: "#" and six hex digits (the "#" may be left out).
    function parseHex(text) {
        const t = text.trim()
        const m = /^#?([0-9a-fA-F]{6})$/.exec(t)
        return m ? "#" + m[1].toLowerCase() : ""
    }

    function begin(category) {
        categoryId = category ? category.id : ""
        name.text = category ? category.name : ""
        color = category ? category.color : categories.suggestColor()
        hex.text = color
        problem.text = ""
        open()
    }

    // The one thing to type: ready for it once the sheet is up (asked
    // before, the focus went back to the page as another sheet closed).
    onOpened: name.forceActiveFocus()

    function save() {
        const why = categoryId.length > 0
                  ? categories.edit(categoryId, name.text, color)
                  : categories.add(name.text, color)
        problem.text = why
        if (why.length === 0) close()
    }

    objectName: "categorySheet"
    title: categoryId.length > 0 ? qsTr("Edit category") : qsTr("New category")
    description: qsTr("A key can be in several categories. The desktop shows "
                       + "the same ones.")

    // A small label over each part: what the field is for stays said once
    // something is typed, without the style's tall floating label.
    component PartLabel: Label {
        Layout.fillWidth: true
        font.pixelSize: Theme.labelSize
        color: Material.secondaryTextColor
    }

    // A one-line field, a touch target high.
    component CompactField: TextField {
        implicitHeight: Theme.touch
        topPadding: 8
        bottomPadding: 8
        verticalAlignment: TextInput.AlignVCenter
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 2

        PartLabel {
            text: qsTr("Name")
        }

        CompactField {
            id: name
            objectName: "categoryName"
            Layout.fillWidth: true
            maximumLength: 64
            Accessible.name: qsTr("Name, for example Work")
        }

        ProblemLabel {
            id: problem
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 2

        PartLabel {
            text: qsTr("Colour")
        }

        // The ready-made colours, close together; the chosen one ticked.
        Flow {
            objectName: "categoryColors"
            Layout.fillWidth: true
            spacing: 0

            Repeater {
                model: sheet.categories ? sheet.categories.presetColors : []

                AbstractButton {
                    id: swatch

                    required property var modelData
                    readonly property bool chosen:
                        sheet.color.toLowerCase() === modelData.color.toLowerCase()

                    implicitWidth: 44
                    implicitHeight: 44
                    checkable: true
                    checked: chosen
                    onClicked: {
                        sheet.color = modelData.color
                        hex.text = modelData.color
                    }
                    Accessible.name: modelData.name
                    Accessible.role: Accessible.RadioButton
                    Accessible.checked: chosen

                    contentItem: Item {
                        Rectangle {
                            anchors.centerIn: parent
                            width: 30
                            height: 30
                            radius: 15
                            color: swatch.modelData.color

                            Icon {
                                anchors.centerIn: parent
                                visible: swatch.chosen
                                symbol: "check_circle"
                                sourceSize: Qt.size(20, 20)
                                color: "white"
                            }
                        }
                    }
                }
            }
        }

        // Any other colour: seen, typed as HEX, or picked; the picker goes
        // under the field when there is no room beside it.
        GridLayout {
            objectName: "categoryCustomColor"
            Layout.fillWidth: true
            Layout.topMargin: 4
            columns: Theme.fontScale > 1.3 || sheet.width < 360 ? 2 : 3
            columnSpacing: Theme.gapSmall
            rowSpacing: 2

            Rectangle {
                objectName: "categoryColorPreview"
                Layout.preferredWidth: 30
                Layout.preferredHeight: 30
                Layout.leftMargin: 7
                Layout.rightMargin: 7
                radius: 15
                color: sheet.color.length > 0 ? sheet.color : "transparent"
                border.width: 1
                border.color: Material.dividerColor
                Accessible.role: Accessible.Graphic
                Accessible.name: qsTr("Colour %1").arg(sheet.color)
            }

            CompactField {
                id: hex

                objectName: "categoryHex"
                Layout.fillWidth: true
                font.family: Theme.mono
                inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
                maximumLength: 7
                validator: RegularExpressionValidator {
                    regularExpression: /#?[0-9a-fA-F]{0,6}/
                }
                onTextEdited: {
                    const parsed = sheet.parseHex(text)
                    if (parsed.length > 0) sheet.color = parsed
                }
                Accessible.name: qsTr("Colour as HEX, for example #2A82DA")
            }

            ActionButton {
                objectName: "categoryPickColor"
                Layout.columnSpan: parent.columns === 2 ? 2 : 1
                Layout.alignment: Qt.AlignRight
                kind: "text"
                text: qsTr("Pick…")
                onClicked: {
                    picker.selectedColor = sheet.color
                    picker.open()
                }
            }
        }
    }

    Label {
        objectName: "categoryHexProblem"
        Layout.fillWidth: true
        visible: hex.text.length > 0 && sheet.parseHex(hex.text).length === 0
        text: qsTr("Six hex digits, as #2A82DA.")
        color: Theme.danger
        font.pixelSize: Theme.labelSize
    }

    // Qt's own colour dialog: on Android, Qt Quick's (there is no system
    // one), in the application's style.
    ColorDialog {
        id: picker
        title: qsTr("Category colour")
        onAccepted: {
            const picked = selectedColor.toString().slice(0, 7).toLowerCase()
            sheet.color = picked
            hex.text = picked
        }
    }

    footer: [
        Item {
            Layout.fillWidth: true
        },
        ActionButton {
            kind: "text"
            text: qsTr("Cancel")
            onClicked: sheet.close()
        },
        ActionButton {
            objectName: "categorySave"
            text: sheet.categoryId.length > 0 ? qsTr("Save") : qsTr("Create")
            enabled: name.text.trim().length > 0
                     && sheet.parseHex(sheet.color).length > 0
            onClicked: sheet.save()
        }
    ]
}

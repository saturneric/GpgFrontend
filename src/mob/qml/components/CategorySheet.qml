// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// A key category's name and colour, new or to change, as the desktop's
// category dialog asks for them; the colour from the same swatches.
BottomSheet {
    id: sheet

    property KeyCategoriesController categories
    // The category being changed; empty for a new one.
    property string categoryId: ""
    property string color: ""

    function begin(category) {
        categoryId = category ? category.id : ""
        name.text = category ? category.name : ""
        color = category ? category.color : categories.suggestColor()
        problem.text = ""
        open()
        // The one thing to type: ready for it.
        name.forceActiveFocus()
    }

    function save() {
        const why = categoryId.length > 0
                  ? categories.edit(categoryId, name.text, color)
                  : categories.add(name.text, color)
        problem.text = why
        if (why.length === 0) close()
    }

    objectName: "categorySheet"
    title: categoryId.length > 0 ? qsTr("Edit category") : qsTr("New category")

    Label {
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        color: Material.secondaryTextColor
        text: qsTr("Sort keys into your own groups. A key can be in several. "
                   + "The desktop shows the same categories.")
    }

    TextField {
        id: name
        objectName: "categoryName"
        Layout.fillWidth: true
        placeholderText: qsTr("Name, for example Work")
        maximumLength: 64
    }

    ProblemLabel {
        id: problem
    }

    Flow {
        objectName: "categoryColors"
        Layout.fillWidth: true
        spacing: 6

        Repeater {
            model: sheet.categories ? sheet.categories.presetColors : []

            AbstractButton {
                id: swatch

                required property var modelData
                readonly property bool chosen:
                    sheet.color.toLowerCase() === modelData.color.toLowerCase()

                implicitWidth: Theme.touch
                implicitHeight: Theme.touch
                checkable: true
                checked: chosen
                onClicked: sheet.color = modelData.color
                Accessible.name: modelData.name
                Accessible.role: Accessible.RadioButton

                contentItem: Item {
                    Rectangle {
                        anchors.centerIn: parent
                        width: 30
                        height: 30
                        radius: 15
                        color: swatch.modelData.color
                        border.width: swatch.chosen ? 3 : 0
                        border.color: Material.foreground
                    }
                }
            }
        }
    }

    footer: [
        Item {
            Layout.fillWidth: true
        },
        Button {
            text: qsTr("Cancel")
            flat: true
            onClicked: sheet.close()
        },
        Button {
            objectName: "categorySave"
            text: sheet.categoryId.length > 0 ? qsTr("Save") : qsTr("Create")
            highlighted: true
            enabled: name.text.trim().length > 0
            onClicked: sheet.save()
        }
    ]
}

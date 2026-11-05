// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// The key categories to choose from, one under the other: All keys, the
// person's own, then the ready-made ones; the one shown is ticked. At the
// foot, apart from them, making a new category and managing them -- never
// confused with adding a key. One of the person's own also opens, to see and
// change who is in it.
BottomSheet {
    id: sheet

    // [{"id", "name", "color"}] to offer, after All keys.
    property var entries: []
    // The id of the category shown.
    property string current: ""
    // Offer New category and Manage categories.
    property bool manageable: true
    // Ids of the person's own categories, which open.
    property var ownIds: []

    signal chosen(var category)
    signal newRequested()
    signal manageRequested()
    signal openRequested(var category)

    readonly property var all: [{"id": "", "name": qsTr("All keys"), "color": ""}]
                               .concat(entries)

    objectName: "categoryListSheet"
    title: qsTr("Show keys in")

    // One list, its rows one under the other with no gap between them, as
    // a list is; the sheet's own spacing is for separate parts.
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 0

        Repeater {
            model: sheet.all

            RadioDelegate {
                id: row

                required property var modelData
                readonly property bool own: sheet.ownIds.includes(modelData.id)

                objectName: "category_" + (modelData.id || "all")
                Layout.fillWidth: true
                Layout.leftMargin: -Theme.gap
                Layout.rightMargin: -Theme.gap
                leftPadding: Theme.gap
                text: modelData.name
                checked: sheet.current === modelData.id
                onClicked: {
                    sheet.close()
                    sheet.chosen(modelData)
                }

                contentItem: RowLayout {
                    spacing: 12

                    Rectangle {
                        Layout.preferredWidth: 12
                        Layout.preferredHeight: 12
                        radius: 6
                        color: row.modelData.color || "transparent"
                        border.width: (row.modelData.color || "").length > 0 ? 0 : 1
                        border.color: Material.dividerColor
                    }

                    Label {
                        Layout.fillWidth: true
                        text: row.text
                        wrapMode: Text.Wrap
                        font.pixelSize: Theme.bodySize
                    }

                    // One's own category: who is in it.
                    BarButton {
                        visible: row.own && sheet.manageable
                        text: qsTr("Keys in %1").arg(row.modelData.name)
                        icon.source: Theme.icon("chevron_right")
                        onClicked: {
                            sheet.close()
                            sheet.openRequested(row.modelData)
                        }
                    }

                    // The radio button's room, as the style's own label keeps it.
                    Item {
                        Layout.preferredWidth: row.indicator.width + row.spacing
                    }
                }
            }
        }

        MenuSeparator {
            Layout.fillWidth: true
            visible: sheet.manageable
        }

        ItemDelegate {
            objectName: "newCategory"
            Layout.fillWidth: true
            Layout.leftMargin: -Theme.gap
            Layout.rightMargin: -Theme.gap
            leftPadding: Theme.gap
            visible: sheet.manageable
            text: qsTr("New category")
            icon.source: Theme.icon("label")
            icon.color: Material.accentColor
            onClicked: {
                sheet.close()
                sheet.newRequested()
            }
        }

        ItemDelegate {
            objectName: "manageCategories"
            Layout.fillWidth: true
            Layout.leftMargin: -Theme.gap
            Layout.rightMargin: -Theme.gap
            leftPadding: Theme.gap
            visible: sheet.manageable
            text: qsTr("Manage categories")
            icon.source: Theme.icon("settings")
            icon.color: Material.secondaryTextColor
            onClicked: {
                sheet.close()
                sheet.manageRequested()
            }
        }
    }
}

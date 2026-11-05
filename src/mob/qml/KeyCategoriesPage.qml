// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// The key categories of this key database, as the desktop's key list keeps
// them: made, renamed, recoloured, put in order and deleted here. Which keys
// are in one is chosen on each key's page.
Page {
    id: page

    // Not a stop for a screen reader: a page has no name of its own, only
    // its parts do (Qt gives a Page the role of an unnamed tab).
    Accessible.ignored: true

    signal back()
    // A category, to see and manage its keys.
    signal categoryOpened(var category)

    header: AppBar {
        title: qsTr("Key categories")
        canGoBack: true
        raised: list.contentY > list.originY
        onBack: page.back()

        BarButton {
            objectName: "addCategory"
            text: qsTr("New category")
            icon.source: Theme.icon("add")
            onClicked: sheet.begin(null)
        }
    }

    KeyCategoriesController {
        id: store
    }

    ListView {
        id: list

        objectName: "categoryList"
        anchors.fill: parent
        clip: true
        model: store.categories

        delegate: ListRow {
            id: row

            required property var modelData
            required property int index

            objectName: "category_" + index
            width: ListView.view.width
            text: modelData.name
            detail: qsTr("%n key(s)", "", modelData.count)
            onClicked: page.categoryOpened(modelData)

            Rectangle {
                Layout.alignment: Qt.AlignVCenter
                width: 16
                height: 16
                radius: 8
                color: row.modelData.color
            }

            BarButton {
                text: qsTr("Actions for %1").arg(row.modelData.name)
                icon.source: Theme.icon("more_vert")
                onClicked: {
                    rowMenu.category = row.modelData
                    rowMenu.index = row.index
                    rowMenu.popup(this, 0, height)
                }
            }
        }

        EmptyState {
            parent: list
            anchors.centerIn: list
            width: list.width - 64
            visible: list.count === 0
            symbol: "label"
            text: qsTr("No categories yet")
            detail: qsTr("Sort keys into your own groups, such as Work or "
                         + "Family, and show one group at a time.")

            Button {
                text: qsTr("New category")
                highlighted: true
                onClicked: sheet.begin(null)
            }
        }
    }

    FitMenu {
        id: rowMenu

        property var category: ({})
        property int index: -1

        MenuItem {
            text: qsTr("Edit")
            onTriggered: sheet.begin(rowMenu.category)
        }
        MenuItem {
            text: qsTr("Move up")
            enabled: rowMenu.index > 0
            onTriggered: store.move(rowMenu.category.id, -1)
        }
        MenuItem {
            text: qsTr("Move down")
            enabled: rowMenu.index < list.count - 1
            onTriggered: store.move(rowMenu.category.id, 1)
        }
        MenuSeparator {}
        MenuItem {
            objectName: "deleteCategory"
            text: qsTr("Delete")
            onTriggered: {
                confirmDelete.category = rowMenu.category
                confirmDelete.open()
            }
        }
    }

    CategorySheet {
        id: sheet
        categories: store
    }

    MessageAlert {
        id: confirmDelete

        property var category: ({})

        title: qsTr("Delete “%1”?").arg(category.name || "")
        message: qsTr("Only the category goes: its keys stay, in the key list "
                   + "and in their other categories.")
        acceptText: qsTr("Delete")
        onAccepted: store.remove(category.id)
    }
}

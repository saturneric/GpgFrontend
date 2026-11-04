// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

Page {
    id: page

    // The chosen row's roles, as an object.
    signal keyChosen(var key)
    signal importRequested()
    signal generateRequested()
    // An anonymous key from the default profile, at once.
    signal quickGenerateRequested()
    // A category of the person's own, to manage (long press on its chip).
    signal categoryOpened(var category)
    // A key group to open (by its id); @p pick: ask for its keys at once.
    signal groupOpened(string id, bool pick)

    // The key category shown; empty for every key.
    property string category: ""

    KeyCategoriesController {
        id: categoryStore
    }

    // A category's members changed, or it went. (Signals named Signal... have
    // no on-handler in QML.)
    function categoriesChanged() {
        if (page.category.length > 0
                && !categoryStore.categories.concat(categoryStore.builtins)
                                         .some(c => c.id === page.category))
            page.category = ""
        filtered.refreshCategory()
    }

    Component.onCompleted: categoryStore.SignalChanged.connect(page.categoriesChanged)

    KeyDatabasesController {
        id: keyDatabases
    }

    // A long press on a key: the categories it is in.
    KeyCategoriesSheet {
        id: keyCategories
        categories: categoryStore
    }

    // The "+" chip: a new category.
    CategorySheet {
        id: newCategory
        categories: categoryStore
    }

    KeyDatabaseSheet {
        id: databaseSheet
        databases: keyDatabases
        onFinished: function(message) { notice.show(message) }
    }

    header: AppBar {
        title: WorkSelection.recipientCount > 0
               ? qsTr("%1 to encrypt to").arg(WorkSelection.recipientCount)
               : qsTr("Keys")
        raised: list.contentY > list.originY

        // The key database the list shows; tapped, the others and a new
        // one. Always there: recipients stay chosen for the session, and
        // hiding it while they were left no way to switch.
        Button {
            objectName: "keyDatabaseButton"
            flat: true
            text: keyDatabases.currentName
            icon.source: Theme.icon("folder")
            display: WorkSelection.recipientCount > 0 ? AbstractButton.IconOnly
                                                       : AbstractButton.TextBesideIcon
            Layout.maximumWidth: page.width * 0.5
            onClicked: databaseSheet.begin()
            Accessible.name: qsTr("Key database: %1. Choose another")
                             .arg(keyDatabases.currentName)
        }

        Button {
            objectName: "clearRecipients"
            visible: WorkSelection.recipientCount > 0
            flat: true
            text: qsTr("Clear")
            onClicked: WorkSelection.clearRecipients()
            Accessible.name: qsTr("Clear the keys to encrypt to")
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        SearchField {
            id: search

            objectName: "keySearch"
            Layout.fillWidth: true
            Layout.leftMargin: Theme.gap
            Layout.rightMargin: Theme.gap
            Layout.bottomMargin: Theme.gapSmall
            hint: qsTr("Search name, email or key ID")
        }

        // The categories, as the desktop's key list tabs them: all keys, or
        // one group of them; a long press on one of the person's own opens
        // it. "+" at the end of the row, always in reach, makes one.
        RowLayout {
            Layout.fillWidth: true
            spacing: 0

            ListView {
                id: categoryChips

                objectName: "categoryChips"
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(36, Theme.secondarySize * 2.4) + 8
                orientation: ListView.Horizontal
                spacing: 8
                leftMargin: Theme.gap
                rightMargin: 8
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                model: [{"id": "", "name": qsTr("All keys"), "color": ""}]
                // The person's own first, in reach without scrolling the row;
                // the ready-made ones after them.
                       .concat(categoryStore.categories)
                       .concat(categoryStore.builtins)

                delegate: Button {
                    id: chip

                    required property var modelData
                    // One of the person's own, which a long press opens.
                    readonly property bool custom: modelData.id.length > 0
                        && !categoryStore.builtins.some(b => b.id === modelData.id)

                    objectName: "categoryChip_" + modelData.id
                    anchors.verticalCenter: parent ? parent.verticalCenter : undefined
                    text: modelData.name
                    flat: page.category !== modelData.id
                    highlighted: page.category === modelData.id
                    checkable: true
                    checked: page.category === modelData.id
                    font.pixelSize: Theme.secondarySize
                    topInset: 0
                    bottomInset: 0
                    implicitHeight: Math.max(36, Theme.secondarySize * 2.4)
                    leftPadding: modelData.color ? 28 : 16
                    onClicked: page.category = modelData.id
                    onPressAndHold: if (custom) page.categoryOpened(modelData)
                    Accessible.name: qsTr("Show %1").arg(modelData.name)
                    Accessible.description: custom ? qsTr("Long-press to manage it") : ""

                    Rectangle {
                        visible: chip.modelData.color.length > 0
                        anchors.left: parent.left
                        anchors.leftMargin: 12
                        anchors.verticalCenter: parent.verticalCenter
                        width: 10
                        height: 10
                        radius: 5
                        color: chip.modelData.color
                    }
                }
            }

            BarButton {
                objectName: "categoryChip_new"
                Layout.rightMargin: 4
                text: qsTr("New category")
                icon.source: Theme.icon("add")
                onClicked: newCategory.begin(null)
            }
        }

        ListView {
            id: list

            // Pulled down far enough to refresh when let go.
            readonly property bool pulled: verticalOvershoot < -72

            objectName: "keyList"
            // Where in a long list one is; finding a key is the search's.
            ScrollIndicator.vertical: ScrollIndicator {}
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            // Room under the last row for the add button.
            footer: Item {
                height: 88
            }
            model: KeyFilterModel {
                id: filtered
                sourceModel: KeyListController.keys
                searchText: search.text
                category: page.category
                withGroups: true
            }
            delegate: KeyRow {
                required property var model

                width: ListView.view.width
                // A key group has no categories: a long press opens it, as
                // a tap does.
                onCategoriesRequested: {
                    if (!model.isKey) {
                        page.groupOpened(model.fingerprint, false)
                        return
                    }
                    keyCategories.begin({
                        "keyId": model.keyId, "name": primaryText
                    })
                }
                onRecipientToggled: {
                    if (!model.canEncrypt || !model.usable) {
                        notice.show(qsTr("%1 cannot be encrypted to.").arg(primaryText))
                        return
                    }
                    WorkSelection.toggleRecipient(fingerprint)
                }
                onClicked: if (!model.isKey) page.groupOpened(model.fingerprint, false)
                           else page.keyChosen({
                    "name": model.name, "email": model.email,
                    "comment": model.comment, "keyId": model.keyId,
                    "fingerprint": model.fingerprint,
                    "hasSecret": model.hasSecret,
                    "canEncrypt": model.canEncrypt, "canSign": model.canSign,
                    "usable": model.usable, "expired": model.expired,
                    "revoked": model.revoked, "status": model.status,
                    "statusText": model.statusText, "created": model.created,
                    "expires": model.expires, "algorithm": model.algorithm
                })
            }

            onDragEnded: if (pulled && !KeyListController.refreshing)
                             KeyListController.refresh()

            // What a pull does, before it is let go.
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                y: Math.max(8, -list.verticalOvershoot / 2 - height / 2)
                visible: list.verticalOvershoot < -16 && !KeyListController.refreshing
                text: list.pulled ? qsTr("Release to refresh")
                                  : qsTr("Pull to refresh")
                color: Material.secondaryTextColor
            }

            EmptyState {
                objectName: "emptyKeys"
                // On the list itself, not in its content, which is only as
                // tall as what it holds -- nothing, here.
                parent: list
                anchors.centerIn: list
                width: list.width - 64
                visible: filtered.count === 0 && !KeyListController.refreshing
                image: search.text.length > 0 ? "magnifier" : "key_generate"
                text: search.text.length > 0
                      ? qsTr("No key matches “%1”.").arg(search.text)
                      : page.category.length > 0 ? qsTr("No keys in this category.")
                      : qsTr("No keys yet.")
                detail: search.text.length > 0 ? ""
                        : page.category.length > 0
                        ? (categoryStore.builtins.some(c => c.id === page.category)
                           ? "" : qsTr("Add keys to it on each key's page."))
                        : qsTr("Create a key of your own, or import the keys "
                               + "of the people you write to.")

                Button {
                    objectName: "clearSearch"
                    visible: search.text.length > 0
                    text: qsTr("Clear search")
                    flat: true
                    onClicked: search.clear()
                }

                Button {
                    visible: search.text.length === 0 && page.category.length === 0
                    text: qsTr("Import")
                    flat: true
                    onClicked: page.importRequested()
                }

                Button {
                    visible: search.text.length === 0 && page.category.length === 0
                    text: qsTr("Create a key")
                    highlighted: true
                    onClicked: page.generateRequested()
                }
            }

            BusyIndicator {
                anchors.horizontalCenter: parent.horizontalCenter
                y: filtered.count === 0 ? (parent.height - height) / 2 : 8
                running: KeyListController.refreshing
            }
        }
    }

    Snackbar {
        id: notice
        objectName: "keysNotice"
    }

    KeyGroupsController {
        id: keyGroups
    }

    KeyGroupSheet {
        id: groupNamer
        groups: keyGroups
        onCreated: id => page.groupOpened(id, true)
    }

    Fab {
        objectName: "addKey"
        text: qsTr("Add a key")
        icon.source: Theme.icon("add")
        onClicked: addSheet.open()
    }

    BottomSheet {
        id: addSheet

        title: qsTr("Add a key")

        ListRow {
            objectName: "generateKey"
            Layout.fillWidth: true
            Layout.leftMargin: -24
            Layout.rightMargin: -24
            leftPadding: 24
            symbol: "key"
            symbolColor: Material.accentColor
            text: qsTr("Create a key")
            detail: qsTr("Create a key for signing and receiving encrypted "
                         + "content.")
            onClicked: {
                addSheet.close()
                page.generateRequested()
            }
        }

        ListRow {
            objectName: "quickGenerateKey"
            Layout.fillWidth: true
            Layout.leftMargin: -24
            Layout.rightMargin: -24
            leftPadding: 24
            symbol: "redo"
            symbolColor: Material.accentColor
            text: qsTr("Quick key")
            detail: qsTr("An anonymous key from the default profile, at once. "
                         + "Only its passphrase is asked for.")
            onClicked: {
                addSheet.close()
                page.quickGenerateRequested()
            }
        }

        ListRow {
            objectName: "newKeyGroup"
            Layout.fillWidth: true
            Layout.leftMargin: -24
            Layout.rightMargin: -24
            leftPadding: 24
            image: "qrc:/icons/key-group.png"
            text: qsTr("New key group")
            detail: qsTr("Encrypt to several keys at once, as one recipient.")
            onClicked: {
                addSheet.close()
                groupNamer.begin(null)
            }
        }

        ListRow {
            objectName: "importKeys"
            Layout.fillWidth: true
            Layout.leftMargin: -24
            Layout.rightMargin: -24
            Layout.bottomMargin: Theme.gapSmall
            leftPadding: 24
            symbol: "note_add"
            symbolColor: Material.accentColor
            text: qsTr("Import a key")
            detail: qsTr("Import from a file or the clipboard.")
            onClicked: {
                addSheet.close()
                page.importRequested()
            }
        }
    }
}

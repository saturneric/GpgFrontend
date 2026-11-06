// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

Page {
    id: page

    // Not a stop for a screen reader: a page has no name of its own, only
    // its parts do (Qt gives a Page the role of an unnamed tab).
    Accessible.ignored: true

    // The chosen row's roles, as an object.
    signal keyChosen(var key)
    signal importRequested()
    signal generateRequested()
    // An anonymous key from the default profile, at once.
    signal quickGenerateRequested()
    // The categories, to add, edit, order and remove.
    signal categoriesManageRequested()
    // A key group to open (by its id); @p pick: ask for its keys at once.
    signal groupOpened(string id, bool pick)
    // Another key database is in use: what is open of the last one goes.
    signal keyDatabaseSwitched()

    // The key category shown; empty for every key.
    property string category: ""
    // A quick key is being made.
    property bool creatingKey: false
    // The name of the database whose keys are listed: while another is
    // being switched to, the one before, so the name and the list change
    // together, in the same frame.
    readonly property string databaseName: {
        const listed = KeyListController.loadedDatabase
        for (const database of keyDatabases.databases || []) {
            if (database.id === listed) return database.name
        }
        return keyDatabases.currentName
    }
    readonly property alias notice: notice
    readonly property alias databaseSheet: databaseSheet
    readonly property alias databaseSwitch: databaseSwitch

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

    // The key databases: the application's one controller (MainPage), so
    // a switch begun on one page is seen as under way on the other; its own
    // where the page stands alone.
    property KeyDatabasesController keyDatabases: KeyDatabasesController {}

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
        switcher: databaseSwitch
    }

    // A switch under way, apart from choosing one.
    KeyDatabaseSwitchSheet {
        id: databaseSwitch
        databases: keyDatabases
        onFinished: page.keyDatabaseSwitched()
    }

    // The categories to show, in the order the list offers them: the
    // person's own, then the ready-made ones.
    readonly property var categoryEntries: categoryStore.categories.concat(categoryStore.builtins)
    // The search field (KeyFinder's).
    readonly property alias search: finder.field
    // The key list is still being read (a database opening, the first
    // refresh): not empty yet, only not known.
    property bool loading: keyDatabases.busy || !KeyListController.loaded
                                    || (KeyListController.refreshing && filtered.count === 0)

    header: AppBar {
        title: qsTr("Keys")
        raised: list.contentY > list.originY

        // The key database the list shows; tapped, the others and a new one.
        Button {
            objectName: "keyDatabaseButton"
            flat: true
            text: page.databaseName
            icon.source: Theme.icon("database")
            Layout.maximumWidth: page.width * 0.5
            onClicked: databaseSheet.begin()
            Accessible.name: qsTr("Key database: %1. Choose another")
                             .arg(page.databaseName)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Finding a key: by what it says, and by its category.
        KeyFinder {
            id: finder

            Layout.fillWidth: true
            Layout.leftMargin: Theme.gap
            Layout.rightMargin: Theme.gap
            Layout.bottomMargin: Theme.gapSmall
            searchName: "keySearch"
            entries: page.categoryEntries
            category: page.category
            manageable: true
            onCategoryChosen: id => page.category = id
            onNewRequested: newCategory.begin(null)
            onManageRequested: page.categoriesManageRequested()
        }

        ListView {
            id: list

            // Pulled down far enough to refresh when let go.
            readonly property bool pulled: verticalOvershoot < -72
            // The room above the first row while the keys are read again,
            // the progress shown in it: opened by the pull (or by a refresh
            // begun elsewhere while the list is at its top), closed when
            // the reading is done. Held as the list's top margin, so the
            // rows sit under it, never under the progress.
            readonly property real refreshRoomFull: 64
            property real refreshRoom: 0
            topMargin: refreshRoom
            // What is shown above the first row: the pull, then the room.
            readonly property real revealed: Math.max(0, originY - contentY)
            // (A key being made shows there too: it ends in a refresh.)
            readonly property bool roomWanted: (KeyListController.refreshing
                                                || page.creatingKey)
                                               && filtered.count > 0
            onRoomWantedChanged: {
                if (roomWanted && refreshRoom >= refreshRoomFull) return
                // A refresh begun elsewhere opens the room only for a list
                // at its top; one scrolled away is left where it is.
                if (roomWanted && !atYBeginning) return
                roomAnimation.to = roomWanted ? refreshRoomFull : 0
                roomAnimation.restart()
            }
            // The rows follow the room as it opens or closes, while the list
            // is at its top and not in the finger's hands.
            property real roomBefore: 0
            onRefreshRoomChanged: {
                const was = roomBefore
                roomBefore = refreshRoom
                if (dragging || flicking) return
                if (contentY <= originY - was + 1) contentY = originY - refreshRoom
            }

            NumberAnimation {
                id: roomAnimation
                target: list
                property: "refreshRoom"
                duration: 220
                easing.type: Easing.OutCubic
            }

            objectName: "keyList"
            // Where in a long list one is; finding a key is the search's.
            ScrollIndicator.vertical: ScrollIndicator {}
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            // The last row scrolls clear of the add button.
            bottomMargin: Theme.fabBand
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
                // A long press: which categories the key, or the key group,
                // is in (a group by its id, as the desktop keeps it).
                onCategoriesRequested: keyCategories.begin({
                    "keyId": model.keyId, "name": primaryText
                })
                onClicked: if (!model.isKey) page.groupOpened(model.fingerprint, false)
                           else page.keyChosen({
                    "name": model.name, "email": model.email,
                    "comment": model.comment, "keyId": model.keyId,
                    "fingerprint": model.fingerprint,
                    "hasSecret": model.hasSecret,
                    "secretState": model.secretState,
                    "capabilities": model.capabilities,
                    "canEncrypt": model.canEncrypt, "canSign": model.canSign,
                    "usable": model.usable, "expired": model.expired,
                    "revoked": model.revoked, "status": model.status,
                    "statusText": model.statusText, "created": model.created,
                    "expires": model.expires, "algorithm": model.algorithm
                })
            }

            // Let go of pulled far enough: the room is held open at once
            // (the list comes back to it, not past it), and the keys are
            // read again, once: a pull while they are read starts nothing.
            onDragEnded: {
                if (!pulled || KeyListController.refreshing) return
                roomAnimation.stop()
                roomBefore = refreshRoomFull
                refreshRoom = refreshRoomFull
                KeyListController.refresh()
            }

            // What a pull does, before it is let go: in the room the pull
            // opens, in its middle.
            Label {
                objectName: "keysPullHint"
                parent: list
                anchors.horizontalCenter: parent.horizontalCenter
                y: (list.revealed - height) / 2
                // Only under the finger: not while the room closes again.
                visible: list.dragging && list.refreshRoom === 0
                         && list.revealed > height + 8 && !KeyListController.refreshing
                text: list.pulled ? qsTr("Release to refresh")
                                  : qsTr("Pull to refresh")
                color: Material.secondaryTextColor
            }

            // The keys being read again: in the room above the first row,
            // in its middle, following it as it opens and closes. Made only
            // while there is room for it: a BusyIndicator left in the scene
            // kept the window drawing frame after frame with nothing moving.
            Loader {
                id: refreshProgress

                objectName: "keysRefreshing"
                parent: list
                anchors.horizontalCenter: parent.horizontalCenter
                width: 40
                height: 40
                y: (list.revealed - height) / 2
                active: list.revealed > 8
                opacity: Math.min(1, list.revealed / list.refreshRoomFull)

                sourceComponent: BusyIndicator {
                    running: list.roomWanted
                    Accessible.name: qsTr("Refreshing")
                }
            }

            // Nothing to show: said as it is -- nothing matches, the
            // category is empty, the database holds no keys, or they could
            // not be read. Never while they are still being read.
            EmptyState {
                id: emptyKeys

                objectName: "emptyKeys"
                // On the list itself, not in its content, which is only as
                // tall as what it holds -- nothing, here.
                parent: list
                anchors.centerIn: list
                width: list.width - 64
                visible: filtered.count === 0 && !page.loading
                readonly property bool failed: KeyListController.error.length > 0
                                               && search.text.length === 0
                                               && page.category.length === 0
                symbol: search.text.length > 0 ? "search"
                        : failed ? "" : "key"
                text: search.text.length > 0
                      ? qsTr("No key matches “%1”.").arg(search.text)
                      : page.category.length > 0 ? qsTr("No keys in this category.")
                      : failed ? qsTr("The keys could not be read.")
                      : qsTr("No keys yet.")
                detail: search.text.length > 0 ? ""
                        : page.category.length > 0
                        ? (categoryStore.builtins.some(c => c.id === page.category)
                           ? "" : qsTr("Add keys to it from the category's page."))
                        : failed ? KeyListController.error
                        : qsTr("Use + to create a key of your own or import "
                               + "the keys of the people you write to.")

                Button {
                    objectName: "retryKeys"
                    visible: emptyKeys.failed
                    text: qsTr("Try again")
                    flat: true
                    onClicked: KeyListController.refresh()
                }
            }

            // With no rows yet: in the middle of the empty list. (With rows,
            // the progress is the refresh room's, above them.)
            BusyIndicator {
                objectName: "keysLoading"
                parent: list
                anchors.centerIn: parent
                // Never a second one beside the switching sheet's, nor
                // just before it comes up.
                running: filtered.count === 0
                         && (page.loading || KeyListController.refreshing || page.creatingKey)
                         && !page.databaseSwitch.opened && !page.databaseSwitch.switching
            }
        }
    }

    Snackbar {
        id: notice
        objectName: "keysNotice"
        // Above the add button, never over it.
        bottomOffset: Theme.fabBand - 12
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
        inset: 0

        ListRow {
            objectName: "generateKey"
            Layout.fillWidth: true
            leftPadding: Theme.sheetPad
            symbol: "key"
            symbolColor: Material.accentColor
            text: qsTr("Create a key")
            detail: qsTr("Create a key for signing and receiving encrypted "
                         + "content.")
            // One key made at a time: its sheet could not be closed.
            enabled: !page.creatingKey
            onClicked: {
                addSheet.close()
                page.generateRequested()
            }
        }

        ListRow {
            objectName: "quickGenerateKey"
            Layout.fillWidth: true
            leftPadding: Theme.sheetPad
            symbol: "redo"
            symbolColor: Material.accentColor
            text: qsTr("Quick key")
            enabled: !page.creatingKey
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
            leftPadding: Theme.sheetPad
            symbol: "group"
            symbolColor: Material.accentColor
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
            Layout.bottomMargin: Theme.gapSmall
            leftPadding: Theme.sheetPad
            symbol: "note_add"
            symbolColor: Material.accentColor
            text: qsTranslate("ImportSheet", "Import keys")
            detail: qsTr("Import from a file or the clipboard.")
            onClicked: {
                addSheet.close()
                page.importRequested()
            }
        }
    }
}

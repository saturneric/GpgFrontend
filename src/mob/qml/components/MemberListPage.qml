// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// What a key group's page and a category's page share: a bar with a menu, the
// keys in it (a row taken out with its own button), an empty state, a button
// that adds many keys at once with the key picker, a delete question and a
// line about what was done. The page using it gives the words, the rows and
// what each answer does.
Page {
    id: page

    // Not a stop for a screen reader: a page has no name of its own, only
    // its parts do (Qt gives a Page the role of an unnamed tab).
    Accessible.ignored: true

    // The bar's menu (its title is the page's own).
    property alias menuItems: overflow.items
    property string menuName: ""
    // The list: its rows and what is above them.
    property alias model: list.model
    property alias delegate: list.delegate
    property alias listHeader: list.header
    property string listName: ""
    readonly property alias count: list.count
    // Shown while it holds nothing.
    property string emptyName: ""
    property string emptyText: ""
    property string emptyDetail: ""
    // The add button.
    property string addName: ""
    property string addText: ""
    // The delete question.
    property string deleteTitle: ""
    property string deleteMessage: ""

    signal back()
    // A member opened: its key page, or a key group's own page.
    signal keyChosen(var key)
    signal groupOpened(string id)
    // The add button was pressed.
    signal addRequested()
    // Keys chosen in the picker.
    signal keysPicked(var keyIds)
    // Delete was confirmed.
    signal deleteAccepted()

    readonly property alias notice: done
    readonly property alias picker: picker
    readonly property alias confirmDelete: confirmDelete

    // A row tapped: a group opens its page, a key its own.
    function openMember(isGroup, id, fingerprint) {
        if (isGroup) page.groupOpened(id)
        else page.keyChosen(KeyListController.keys.keyOf(fingerprint))
    }

    // Taken out with one tap, so put back with one: said, with Undo when it
    // went, or that it did not.
    function tookOut(done_, doneText, failedText, undo) {
        if (done_) done.show(doneText, qsTranslate("TextPage", "Undo"), undo)
        else done.show(failedText)
    }

    header: AppBar {
        title: page.title
        canGoBack: true
        raised: list.contentY > list.originY
        onBack: page.back()

        OverflowButton {
            id: overflow
            objectName: page.menuName
        }
    }

    ListView {
        id: list

        objectName: page.listName
        anchors.fill: parent
        clip: true
        // The last row scrolls clear of the add button.
        bottomMargin: Theme.fabBand
        ScrollIndicator.vertical: ScrollIndicator {}

        EmptyState {
            objectName: page.emptyName
            parent: list
            anchors.centerIn: list
            width: list.width - 64
            visible: list.count === 0
            symbol: "key"
            text: page.emptyText
            detail: page.emptyDetail
        }
    }

    Fab {
        objectName: page.addName
        text: page.addText
        icon.source: Theme.icon("add")
        onClicked: page.addRequested()
    }

    KeyPickerSheet {
        id: picker
        onAccepted: function(keyIds) { page.keysPicked(keyIds) }
    }

    MessageAlert {
        id: confirmDelete

        title: page.deleteTitle
        message: page.deleteMessage
        destructive: true
        onAccepted: page.deleteAccepted()
    }

    Snackbar {
        id: done
        // Above the add button, never over it.
        bottomOffset: Theme.fabBand - 12
    }
}

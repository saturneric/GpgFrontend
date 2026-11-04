// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// One key category: the keys in it, taken out one by one, and many put in at
// once with a picker like the key toolbox. Renamed, recoloured or deleted
// from the menu.
Page {
    id: page

    // The category: {id, name, color}.
    property var category: ({})

    signal back()
    // A key of it was tapped: its page.
    signal keyChosen(var key)

    // What the page last said in passing (a pop-up, not in the item tree).
    readonly property alias notice: done
    readonly property alias picker: picker

    // Its name and colour as they are now, after an edit here.
    readonly property var current: {
        const all = store.categories
        for (const c of all) if (c.id === category.id) return c
        return category
    }

    // A membership changed here or elsewhere: the list follows. (Signals
    // named Signal... have no on-handler in QML.)
    Component.onCompleted: store.SignalChanged.connect(members.refreshCategory)

    function addKeys() {
        picker.begin(qsTr("Add keys to %1").arg(current.name),
                     store.keyIdsOf(category.id))
    }

    header: AppBar {
        title: page.current.name || ""
        canGoBack: true
        raised: list.contentY > list.originY
        onBack: page.back()

        BarButton {
            objectName: "categoryAddKeys"
            text: qsTr("Add keys")
            icon.source: Theme.icon("add")
            onClicked: page.addKeys()
        }

        OverflowButton {
            objectName: "categoryMenu"

            MenuItem {
                text: qsTr("Edit")
                onTriggered: editor.begin(page.current)
            }
            MenuItem {
                objectName: "categoryDelete"
                text: qsTr("Delete")
                onTriggered: confirmDelete.open()
            }
        }
    }

    KeyCategoriesController {
        id: store
    }

    ListView {
        id: list

        objectName: "categoryKeys"
        anchors.fill: parent
        clip: true
        ScrollIndicator.vertical: ScrollIndicator {}
        model: KeyFilterModel {
            id: members
            sourceModel: KeyListController.keys
            category: page.category.id || ""
        }

        delegate: KeyRow {
            id: member

            required property var model

            width: ListView.view.width
            onClicked: page.keyChosen({
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

            BarButton {
                objectName: "categoryTakeOut_" + member.keyId
                text: qsTr("Take %1 out").arg(member.primaryText)
                icon.source: Theme.icon("close")
                onClicked: {
                    store.setMember(page.category.id, member.keyId, false)
                    done.show(qsTr("%1 is out of %2.").arg(member.primaryText)
                              .arg(page.current.name))
                }
            }
        }

        EmptyState {
            objectName: "categoryEmpty"
            parent: list
            anchors.centerIn: list
            width: list.width - 64
            visible: list.count === 0
            image: "key_generate"
            text: qsTr("No keys in it yet")
            detail: qsTr("Add keys here, many at once, or long-press a key "
                         + "in the key list.")

            Button {
                text: qsTr("Add keys")
                highlighted: true
                onClicked: page.addKeys()
            }
        }
    }

    KeyPickerSheet {
        id: picker
        actionText: qsTr("Add")
        onAccepted: function(keyIds) {
            const added = store.setMembers(page.category.id, keyIds, true)
            done.show(qsTr("%n key(s) added.", "", added))
        }
    }

    CategorySheet {
        id: editor
        categories: store
    }

    MessageAlert {
        id: confirmDelete

        title: qsTr("Delete “%1”?").arg(page.current.name || "")
        message: qsTr("Only the category goes: its keys stay, in the key list "
                      + "and in their other categories.")
        acceptText: qsTr("Delete")
        onAccepted: {
            store.remove(page.category.id)
            page.back()
        }
    }

    Snackbar {
        id: done
    }
}

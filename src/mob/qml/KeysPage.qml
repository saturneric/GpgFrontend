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

    header: AppBar {
        title: qsTr("Keys")
        raised: list.contentY > list.originY
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
            placeholderText: qsTr("Search name, email or key ID")
        }

        ListView {
            id: list

            // Pulled down far enough to refresh when let go.
            readonly property bool pulled: verticalOvershoot < -72

            objectName: "keyList"
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
            }
            delegate: KeyRow {
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
                      : qsTr("No keys yet.")
                detail: search.text.length > 0
                        ? ""
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
                    visible: search.text.length === 0
                    text: qsTr("Import")
                    flat: true
                    onClicked: page.importRequested()
                }

                Button {
                    visible: search.text.length === 0
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

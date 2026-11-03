// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

ListView {
    id: list

    property bool multiple: true
    property bool onlySecret: false
    property bool onlyEncryptCapable: false
    property bool onlySignCapable: false
    property var chosen: []

    signal toggled(string fingerprint, bool on)

    function isChosen(fpr) {
        return chosen.indexOf(fpr) >= 0
    }

    function choose(fpr, on) {
        let next = multiple ? chosen.filter(f => f !== fpr) : []
        if (on) next.push(fpr)
        chosen = next
        toggled(fpr, on)
    }

    clip: true
    // Sized to its rows inside a sheet, which scrolls instead.
    implicitHeight: Math.max(contentHeight, 96)
    interactive: false
    model: KeyFilterModel {
        id: filtered
        sourceModel: KeyListController.keys
        onlySecret: list.onlySecret
        onlyEncryptCapable: list.onlyEncryptCapable
        onlySignCapable: list.onlySignCapable
    }

    delegate: ItemDelegate {
        id: choice

        required property string name
        required property string email
        required property string keyId
        required property string fingerprint
        required property bool hasSecret
        required property bool usable

        width: ListView.view.width
        enabled: usable
        leftPadding: 0
        rightPadding: 0
        onClicked: list.choose(fingerprint, !list.isChosen(fingerprint))

        contentItem: RowLayout {
            spacing: Theme.gap

            KeyAvatar {
                name: choice.name.length > 0 ? choice.name : choice.email
                usable: choice.usable
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Label {
                    Layout.fillWidth: true
                    text: choice.name.length > 0 ? choice.name : choice.email
                    elide: Text.ElideRight
                    font.pixelSize: 16
                }

                Label {
                    Layout.fillWidth: true
                    text: choice.email.length > 0 && choice.name.length > 0
                          ? choice.email + " · " + choice.keyId : choice.keyId
                    elide: Text.ElideRight
                    color: Material.secondaryTextColor
                }
            }

            Icon {
                visible: choice.hasSecret
                symbol: "key"
                color: Material.secondaryTextColor
                Accessible.role: Accessible.Graphic
                Accessible.name: qsTr("Your key")
            }

            CheckBox {
                visible: list.multiple
                checked: list.isChosen(choice.fingerprint)
                onToggled: list.choose(choice.fingerprint, checked)
            }

            RadioButton {
                visible: !list.multiple
                checked: list.isChosen(choice.fingerprint)
                onToggled: list.choose(choice.fingerprint, checked)
            }
        }
    }

    Label {
        // On the list itself, not in its content, which is empty here.
        parent: list
        anchors.centerIn: list
        width: list.width - 32
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        color: Material.secondaryTextColor
        visible: filtered.count === 0
        text: list.onlySecret ? qsTr("You have no key of your own yet.")
                              : qsTr("No key to encrypt to yet.")
    }
}

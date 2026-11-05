// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// The PGP operations of a page, opened from its operation button: Encrypt,
// Decrypt, Sign, Verify, Encrypt & Sign, Decrypt & Verify (and Encrypt with
// a passphrase once turned on in Settings), and first, when there is one,
// the last operation again with its keys.
//
// What cannot be done is said, never left to a silent tap: when nothing can
// be done yet (no text, no files chosen) a line at the top says what is
// missing; an operation that does not apply to what is chosen stays listed,
// greyed, with its reason under its name.
FitMenu {
    id: menu

    // Why none of the operations can be used now; empty when they can.
    property string unavailable: ""
    // The reason @p op cannot be used on what is chosen, or "" when it can.
    property var reasonFor: function(op) { return "" }
    // Offer Encrypt with a passphrase.
    property bool symmetric: false
    // The last operation, said in a line ("Encrypt to Alice"); empty for none.
    property string againText: ""

    signal requested(string op)
    signal againRequested()

    readonly property var operations: ["encrypt", "decrypt", "sign", "verify",
                                       "encrypt_sign", "decrypt_verify"]
                                      .concat(symmetric ? ["symmetric"] : [])

    objectName: "operationMenu"
    // Kept inside the window, below the status bar and above the gesture
    // bar: a long menu on a small screen scrolls, it is not cut off.
    topMargin: Theme.safeTop + 8
    bottomMargin: Theme.safeBottom + 8
    leftMargin: Theme.gap
    rightMargin: Theme.gap
    // As wide as its names need (FitMenu), just above the button it belongs
    // to, the right edges in line; worked out from the menu's real size,
    // whatever rows it has, and kept inside the window by its margins.
    x: parent ? parent.width - width : 0
    y: -height - Theme.gapSmall

    function openAbove(button) {
        parent = button
        open()
    }
    modal: true
    dim: false

    // What is missing before anything can be done. A fixed measure: the
    // reason wraps in it, the menu does not stretch to its whole length on
    // one line (a Label's own implicit width is that whole line).
    Item {
        implicitWidth: 240
        // A hidden item still takes its row in a menu: no height when unused.
        implicitHeight: menu.unavailable.length > 0 ? unavailableLabel.implicitHeight : 0
        height: implicitHeight
        visible: height > 0

        Label {
            id: unavailableLabel

            objectName: "operationsUnavailable"
            width: parent.width
            padding: 16
            bottomPadding: 12
            wrapMode: Text.Wrap
            text: menu.unavailable
            color: Material.secondaryTextColor
            font.pixelSize: Theme.secondarySize
        }
    }

    MenuItem {
        objectName: "operationAgain"
        height: menu.againText.length > 0 && menu.unavailable.length === 0
                ? implicitHeight : 0
        visible: height > 0
        text: qsTr("Again: %1").arg(menu.againText)
        icon.source: Theme.icon("redo")
        onTriggered: menu.againRequested()
    }

    MenuSeparator {
        height: menu.againText.length > 0 && menu.unavailable.length === 0
                ? implicitHeight : 0
        visible: height > 0
    }

    // Made as the list of operations changes, after the three items above
    // (Qt's way of filling a menu from a model).
    Instantiator {
        model: menu.operations
        onObjectAdded: (index, object) => menu.insertItem(index + 3, object)
        onObjectRemoved: (index, object) => menu.removeItem(object)

        delegate: MenuItem {
            id: item

            required property string modelData

            readonly property string reason: menu.unavailable.length > 0
                                             ? menu.unavailable
                                             : menu.reasonFor(modelData)

            objectName: "operation_" + modelData
            text: Wording.operationLabel(modelData)
            enabled: reason.length === 0
            Accessible.name: text
            Accessible.description: reason
            onTriggered: menu.requested(modelData)

            // The desktop's coloured icon, the name, and under it, while it
            // cannot be used, why: in the item, read without a tip.
            contentItem: RowLayout {
                spacing: 16
                opacity: item.enabled ? 1 : 0.5

                IconImage {
                    Layout.alignment: Qt.AlignVCenter
                    source: Theme.operationIcon(item.modelData)
                    sourceSize: Qt.size(24, 24)
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0

                    // Wraps when even the window's width is too little
                    // (large text, long names): never cut short.
                    Label {
                        Layout.fillWidth: true
                        text: item.text
                        wrapMode: Text.Wrap
                        font: item.font
                    }

                    Label {
                        Layout.fillWidth: true
                        Layout.maximumWidth: 220
                        // The common reason is said once, at the top.
                        visible: item.reason.length > 0 && menu.unavailable.length === 0
                        text: item.reason
                        wrapMode: Text.Wrap
                        color: Material.secondaryTextColor
                        font.pixelSize: Theme.labelSize
                    }
                }
            }
        }
    }
}

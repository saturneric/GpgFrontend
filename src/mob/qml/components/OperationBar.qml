// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// What can be done, and nothing else: one button per operation, each its
// icon over its name, and More as one more of them. The keys an operation
// needs are asked for when it runs, in the key toolbox, not shown here.
//
// A toolbar of the page, not a second navigation: on the editor's own
// surface under a hairline, its buttons all alike (none marked as chosen),
// while the navigation below keeps its tinted bar and its pill.
Pane {
    id: bar

    // The operations, as Actions with an icon each, in order.
    property var operations: []
    // The one that fits best (the text is a message: Decrypt); told to
    // accessibility services, not drawn as chosen.
    property Action suggested: null
    // The last operation, to do again in one tap (the quick path); empty
    // hides it.
    property string quickText: ""
    signal quickTriggered()

    // MenuItems for More.
    default property alias items: more.items
    readonly property alias menu: more.menu

    objectName: "operationBar"
    padding: 0
    topPadding: 1
    bottomPadding: 0

    background: Rectangle {
        color: bar.Material.backgroundColor

        Rectangle {
            width: parent.width
            height: 1
            color: bar.Material.dividerColor
        }
    }

    contentItem: ColumnLayout {
        spacing: 0

        // Again: the same operation, with the same keys, at once. A small
        // outlined chip, a step below the operations; its row is still a
        // full touch target.
        AbstractButton {
            id: quick

            objectName: "quickPath"
            Layout.fillWidth: true
            visible: bar.quickText.length > 0
            implicitHeight: 48
            text: qsTr("Again: %1").arg(bar.quickText)
            onClicked: bar.quickTriggered()
            Accessible.role: Accessible.Button
            Accessible.name: text

            contentItem: Item {
                Rectangle {
                    x: Theme.gap
                    anchors.verticalCenter: parent.verticalCenter
                    width: Math.min(chip.implicitWidth + 20,
                                    parent.width - 2 * Theme.gap)
                    height: 32
                    radius: 8
                    color: quick.pressed ? Theme.alpha(Material.foreground, 0.08)
                                         : "transparent"
                    border.width: 1
                    border.color: bar.Material.dividerColor

                    RowLayout {
                        id: chip

                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 12
                        spacing: 6

                        Icon {
                            symbol: "redo"
                            sourceSize: Qt.size(18, 18)
                            color: Material.accentColor
                        }

                        Label {
                            Layout.fillWidth: true
                            text: quick.text
                            elide: Text.ElideRight
                            font.pixelSize: Theme.labelSize
                            font.weight: Font.Medium
                        }
                    }
                }
            }

            background: Item {}
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 0

            Repeater {
                model: bar.operations

                ToolButton {
                    id: operation

                    required property var modelData

                    objectName: modelData.objectName
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    action: modelData
                    display: AbstractButton.TextUnderIcon
                    topPadding: 6
                    bottomPadding: 6
                    spacing: 2
                    font.pixelSize: Theme.labelSize
                    font.weight: Font.Medium
                    // The desktop's coloured icons, as they are: not tinted
                    // to the text; faded when they cannot be used.
                    icon.color: "transparent"
                    icon.width: 24
                    icon.height: 24
                    opacity: enabled ? 1 : 0.38
                    // Its name as said, without the doubled ampersand an
                    // Action's text needs.
                    Accessible.name: text.replace(/&&/g, "&")
                    Accessible.description: modelData === bar.suggested
                                            ? qsTr("Suggested for this content") : ""

                    // Its icon over its name, the name made smaller to fit
                    // its column rather than cut ("Encrypt & Sign", and
                    // longer words in other languages).
                    contentItem: Column {
                        spacing: operation.spacing

                        IconImage {
                            anchors.horizontalCenter: parent.horizontalCenter
                            source: operation.icon.source
                            sourceSize: Qt.size(operation.icon.width,
                                                operation.icon.height)
                            width: operation.icon.width
                            height: operation.icon.height
                        }

                        Label {
                            width: parent.width
                            horizontalAlignment: Text.AlignHCenter
                            // An Action's text doubles its ampersand.
                            text: operation.text.replace(/&&/g, "&")
                            font: operation.font
                            fontSizeMode: Text.HorizontalFit
                            minimumPixelSize: Math.round(Theme.labelSize * 0.75)
                            elide: Text.ElideRight
                            color: operation.Material.foreground
                        }
                    }
                }
            }

            // More, the fifth of them: the same size, spacing and type.
            OverflowButton {
                id: more

                objectName: "operationMore"
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                display: AbstractButton.TextUnderIcon
                text: qsTr("More")
                topPadding: 6
                bottomPadding: 6
                spacing: 2
                font.pixelSize: Theme.labelSize
                font.weight: Font.Medium
                icon.width: 24
                icon.height: 24
                Accessible.name: qsTr("More options")
                // The tip would only say again what is written.
                ToolTip.visible: false
            }
        }
    }
}

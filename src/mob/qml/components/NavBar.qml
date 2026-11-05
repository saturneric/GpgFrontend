// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// The bar at the foot of the window that moves between the sections, as the
// platform's navigation bar: each section an icon over its label, the chosen
// one on a rounded pill, on a surface a shade off the page's.
Pane {
    id: bar

    // [{"text": ..., "icon": ...}] in order.
    property var sections: []
    property int currentIndex: 0

    // One size for every label: as large as the system's text asks, as far
    // as the narrowest fit allows. With large text a long name
    // ("Einstellungen") makes them all a little smaller, never below the
    // usual size, rather than being cut short or set apart from the rest.
    readonly property int labelSize: {
        const room = bar.width / Math.max(1, sections.length) - 4
        let size = Theme.labelSize
        for (const section of sections) {
            const needed = labelMetrics.advanceWidth(section.text)
            if (needed > room) size = Math.min(size, Math.floor(Theme.labelSize * room / needed))
        }
        return Math.max(12, size)
    }

    FontMetrics {
        id: labelMetrics
        font.pixelSize: Theme.labelSize
        font.weight: Font.DemiBold
    }

    padding: 0
    topPadding: 12
    bottomPadding: 16
    // Painted here, not through Material.background: that one feeds
    // backgroundColor, which a tint of it would read back in a loop.
    // It also runs on under the system's gesture bar, which the window
    // leaves to the page: nothing else is drawn there.
    background: Rectangle {
        width: bar.width
        height: bar.height + 200
        color: Theme.surface(bar.Material.backgroundColor, 1)
    }

    contentItem: RowLayout {
        spacing: 0

        Repeater {
            model: bar.sections

            AbstractButton {
                id: section

                required property var modelData
                required property int index

                readonly property bool current: bar.currentIndex === index

                Layout.fillWidth: true
                Layout.preferredWidth: 1
                implicitHeight: Math.max(Theme.touch, column.implicitHeight)
                text: modelData.text
                focusPolicy: Qt.NoFocus
                Accessible.role: Accessible.PageTab
                Accessible.name: text
                Accessible.checked: current
                onClicked: bar.currentIndex = index

                contentItem: ColumnLayout {
                    id: column

                    spacing: 4

                    // The pill, and the press ripple inside it.
                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        implicitWidth: 64
                        implicitHeight: 32
                        radius: height / 2
                        color: section.current ? Theme.indicator
                               : section.pressed ? Theme.alpha(Material.foreground, 0.08)
                                                 : "transparent"

                        Behavior on color {
                            ColorAnimation { duration: 150 }
                        }

                        Icon {
                            anchors.centerIn: parent
                            symbol: section.modelData.icon
                            color: section.current ? Material.foreground
                                                   : Material.secondaryTextColor
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideRight
                        text: section.text
                        font.pixelSize: bar.labelSize
                        font.weight: section.current ? Font.DemiBold : Font.Medium
                        color: section.current ? Material.foreground
                                               : Material.secondaryTextColor
                    }
                }
            }
        }
    }
}

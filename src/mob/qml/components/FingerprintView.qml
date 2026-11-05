// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// A whole fingerprint, in groups of four, in even rows: five groups to a
// row for a v4 key (its two halves, as `gpg --fingerprint` splits it), four
// for a v6 key's sixteen; fewer only where the width or the text size
// leaves no room -- never a row cut off or scrolled sideways. A button
// copies it whole, without the spaces.
ColumnLayout {
    id: view

    property string fingerprint
    property string title: qsTr("Fingerprint")

    readonly property var groups: Theme.fingerprintGroups(fingerprint)
    // How many groups a row takes: the most of these that fits beside the
    // copy button, tried in order -- never three, which leaves one group
    // alone on a v4 key's last row.
    readonly property int perRow: {
        const n = groups.length
        const room = view.width - copyButton.implicitWidth - groupRow.spacing
        const fits = k => k * groupWidth.width + (k - 1) * grid.columnSpacing <= room
        const tried = n % 5 === 0 ? [5, 4, 2, 1] : [4, 2, 1]
        for (const k of tried) {
            if (k <= Math.max(1, n) && fits(k)) return k
        }
        return 1
    }

    spacing: 2

    OutputActions {
        id: output
    }

    Label {
        Layout.fillWidth: true
        text: view.title
        color: Material.secondaryTextColor
        font.pixelSize: Theme.labelSize
    }

    TextMetrics {
        id: groupWidth
        font.family: Theme.mono
        font.pixelSize: Theme.bodySize
        text: "0000"
    }

    // The groups, and beside them the button that copies them: no row of
    // its own for the button.
    RowLayout {
        id: groupRow

        Layout.fillWidth: true
        spacing: Theme.gapSmall

        GridLayout {
            id: grid

            objectName: "fingerprint"
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            columns: view.perRow
            columnSpacing: Math.round(Theme.bodySize * 0.6)
            rowSpacing: 2
            Accessible.role: Accessible.StaticText
            Accessible.name: qsTr("Fingerprint %1").arg(view.groups.join(" "))

            Repeater {
                model: view.groups

                Label {
                    required property string modelData
                    Layout.alignment: Qt.AlignLeft
                    text: modelData
                    font.family: Theme.mono
                    font.pixelSize: Theme.bodySize
                }
            }
        }

        BarButton {
            id: copyButton

            objectName: "copyFingerprint"
            Layout.alignment: Qt.AlignVCenter
            text: qsTr("Copy the fingerprint")
            icon.source: Theme.icon("content_copy")
            onClicked: {
                output.copy(view.fingerprint.replace(/\s+/g, ""), false)
                copiedTip.open()
            }

            // Said for a moment, then gone by itself.
            ToolTip {
                id: copiedTip

                objectName: "fingerprintCopied"
                text: qsTr("Copied.")
                timeout: 1500
            }
        }
    }
}

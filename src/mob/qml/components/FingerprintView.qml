// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// A whole fingerprint, in groups of four, in even rows: five groups to a
// row for a v4 key (its two halves, as `gpg --fingerprint` splits it), eight
// for a v6 key's sixteen -- two rows on a phone too, in a smaller type, never
// smaller than identifiers are shown elsewhere; fewer only where the width or
// the text size leaves no room even so -- never a row cut off or scrolled
// sideways. A button copies it whole, without the spaces.
ColumnLayout {
    id: view

    property string fingerprint
    property string title: qsTr("Fingerprint")

    readonly property var groups: Theme.fingerprintGroups(fingerprint)
    // The room beside the copy button, and a gap each side of the spacer
    // before it.
    readonly property real room: view.width - copyButton.implicitWidth
                                 - 2 * groupRow.spacing
    // How wide k groups are at the pixel size s: the type is monospaced, so
    // a group's width grows with the size (measured as a label is laid out,
    // a pixel more for each column's rounding); the gap between groups is
    // 0.6 of the size.
    function rowWidth(k, s) {
        const group = Math.ceil(groupWidth.implicitWidth * s / Theme.bodySize) + 1
        return k * group + (k - 1) * Math.round(s * 0.6)
    }
    // The groups' size: the text's own, unless a v6 key's eight a row need
    // smaller -- then the largest that fits, down to the label size; below
    // that, the text's own again, and fewer a row.
    readonly property int groupSize: {
        if (groups.length < 16 || groups.length % 8 !== 0) return Theme.bodySize
        for (let s = Theme.bodySize; s >= Theme.labelSize; --s) {
            if (rowWidth(8, s) <= room) return s
        }
        return Theme.bodySize
    }
    // How many groups a row takes: the most of these that fits, tried in
    // order, each a divisor of the whole so every row is as long as the
    // others -- a v4 key's ten in two rows of five (four would leave two
    // alone), a v6 key's sixteen in two of eight, else four of four.
    readonly property int perRow: {
        const n = groups.length
        const tried = n % 5 === 0 ? [5, 2, 1] : n % 8 === 0 ? [8, 4, 2, 1] : [4, 2, 1]
        for (const k of tried) {
            if (k <= Math.max(1, n) && rowWidth(k, groupSize) <= room) return k
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

    // A group as the grid lays it out, at the text's own size: measured,
    // never shown.
    Label {
        id: groupWidth
        visible: false
        Layout.preferredWidth: 0
        Layout.preferredHeight: 0
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
            columnSpacing: Math.round(view.groupSize * 0.6)
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
                    font.pixelSize: view.groupSize
                }
            }
        }

        // The button at the right edge, apart from the groups: a layout
        // whose children do not fill is no wider than they are.
        Item {
            Layout.fillWidth: true
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

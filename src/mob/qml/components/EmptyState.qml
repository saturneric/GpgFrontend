// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// What a page shows while it has nothing in it: one of the desktop's icons, a
// line saying so, and what to do about it.
ColumnLayout {
    id: empty

    // One of the desktop's icons in gf_res, e.g. "key_generate".
    property string image
    property alias text: titleLabel.text
    property alias detail: detailLabel.text
    default property alias actions: actionRow.data

    spacing: 12

    Image {
        Layout.alignment: Qt.AlignHCenter
        visible: empty.image.length > 0
        source: empty.image.length > 0 ? "qrc:/icons/" + empty.image + ".png" : ""
        sourceSize: Qt.size(64, 64)
        Layout.preferredWidth: 64
        Layout.preferredHeight: 64
        Layout.bottomMargin: 4
    }

    Label {
        id: titleLabel
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        font.pixelSize: 18
    }

    Label {
        id: detailLabel
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        visible: text.length > 0
        color: Material.secondaryTextColor
    }

    RowLayout {
        id: actionRow
        Layout.alignment: Qt.AlignHCenter
        Layout.topMargin: 4
        spacing: Theme.gapSmall
        visible: children.length > 0
    }
}

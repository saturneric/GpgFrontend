// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// What a page shows while it has nothing in it: one of the app's line icons
// on a soft round tint of the accent, a line saying so, and what to do about
// it. Drawn from the vector icons, so it is sharp at any density (the
// desktop's bitmaps, scaled up, were soft).
ColumnLayout {
    id: empty

    // One of the app's line icons (qml/icons), e.g. "key"; "" for none.
    property string symbol
    property alias text: titleLabel.text
    property alias detail: detailLabel.text
    default property alias actions: actionRow.data

    spacing: 12

    Rectangle {
        objectName: "emptySymbol"
        Layout.alignment: Qt.AlignHCenter
        Layout.bottomMargin: 4
        visible: empty.symbol.length > 0
        implicitWidth: 80
        implicitHeight: 80
        radius: 40
        color: Theme.alpha(Material.accentColor, 0.12)
        Accessible.ignored: true

        Icon {
            anchors.centerIn: parent
            symbol: empty.symbol
            sourceSize: Qt.size(40, 40)
            color: Material.accentColor
        }
    }

    Label {
        id: titleLabel
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        font.pixelSize: Theme.headlineSize
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

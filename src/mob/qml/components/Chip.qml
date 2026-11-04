// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import GpgFrontend.Mobile

// A short status word, as the GpgFrontend desktop paints it (PaintChip): the
// colour's text on a faint fill of it, inside a border of it. One of the few
// marks of the desktop kept on the phone.
Label {
    property color tone: Material.secondaryTextColor

    leftPadding: 6
    rightPadding: 6
    topPadding: 1
    bottomPadding: 1
    color: tone
    font.pixelSize: Theme.labelSize

    background: Rectangle {
        radius: 4
        color: Theme.alpha(tone, 38 / 255)
        border.width: 1
        border.color: Theme.alpha(tone, 110 / 255)
    }
}

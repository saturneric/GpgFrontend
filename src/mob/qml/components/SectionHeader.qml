// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import GpgFrontend.Mobile

// The title over a group of rows, in the accent, as the platform's settings
// lists head theirs.
Label {
    leftPadding: Theme.gap
    rightPadding: Theme.gap
    topPadding: 24
    bottomPadding: 8
    wrapMode: Text.Wrap
    color: Material.accentColor
    font.pixelSize: Theme.secondarySize
    font.weight: Font.DemiBold
    Accessible.role: Accessible.Heading
}

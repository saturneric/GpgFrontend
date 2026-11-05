// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import GpgFrontend.Mobile

// A key's face in a list: the owner's initials on a circle of the brand
// colour, faded for a key that cannot be used.
Rectangle {
    id: avatar

    property string name
    property bool usable: true
    property int size: 40

    function initials(text) {
        const words = (text || "").trim().split(/[\s@._-]+/).filter(w => w.length > 0)
        if (words.length === 0) return "?"
        if (words.length === 1) return words[0].charAt(0).toUpperCase()
        return (words[0].charAt(0) + words[1].charAt(0)).toUpperCase()
    }

    implicitWidth: size
    implicitHeight: size
    radius: size / 2
    color: Theme.alpha(Material.accentColor, usable ? 0.18 : 0.08)

    Label {
        anchors.centerIn: parent
        // Only a picture of the name, which is said beside it: not read out.
        Accessible.ignored: true
        text: avatar.initials(avatar.name)
        color: Material.accentColor
        opacity: avatar.usable ? 1 : 0.6
        font.pixelSize: avatar.size * 0.4
        font.weight: Font.DemiBold
    }
}

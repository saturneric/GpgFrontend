// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// A button by what it does, the same everywhere; the style draws its pressed
// and disabled looks:
//  - "primary": the action a sheet or form is for, filled;
//  - "secondary": another action that matters, on a tonal surface (New key
//    database, Share as text);
//  - "text": a quiet one (Cancel, Close);
//  - "destructive": deleting or revoking, filled in the danger colour.
// Given less room than its words take (a sheet's buttons one under the
// other, large text, a long translation), its words wrap: never cut off.
Button {
    id: button

    property string kind: "primary"

    highlighted: kind === "primary" || kind === "destructive"
    flat: kind === "text"
    Material.accent: kind === "destructive" ? Theme.danger : Theme.accent
    Material.elevation: 0

    // The style's icon and words, in the style's colours, the words free to
    // wrap.
    contentItem: RowLayout {
        spacing: button.spacing

        IconImage {
            visible: button.icon.source.toString().length > 0
            source: button.icon.source
            sourceSize: Qt.size(button.icon.width, button.icon.height)
            color: button.icon.color
        }

        Label {
            Layout.fillWidth: true
            text: button.text
            font: button.font
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            color: !button.enabled ? button.Material.hintTextColor
                 : button.flat && button.highlighted ? button.Material.accentColor
                 : button.highlighted ? button.Material.primaryHighlightedTextColor
                 : button.Material.foreground
        }
    }

    // A secondary's tint of what it sits on (not of itself, which reads
    // back), and the accent for its text. Any other kind resets both
    // (undefined), so a button whose kind changes looks as one made of that
    // kind: a Binding put the tint back as an explicit colour when it let go,
    // and a primary drew it as its fill, pale, reading as disabled.
    Material.background: kind === "secondary"
        ? Theme.surface(parent ? parent.Material.backgroundColor : "white", 4)
        : undefined
    Material.foreground: kind === "secondary" ? Theme.accent : undefined
}

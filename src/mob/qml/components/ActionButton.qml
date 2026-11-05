// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import GpgFrontend.Mobile

// A button by what it does, the same everywhere; the style draws its pressed
// and disabled looks:
//  - "primary": the action a sheet or form is for, filled;
//  - "secondary": another action that matters, on a tonal surface (New key
//    database, Share as text);
//  - "text": a quiet one (Cancel, Close);
//  - "destructive": deleting or revoking, filled in the danger colour.
Button {
    id: button

    property string kind: "primary"

    highlighted: kind === "primary" || kind === "destructive"
    flat: kind === "text"
    Material.accent: kind === "destructive" ? Theme.danger : Theme.accent
    Material.elevation: 0

    // A tint of what it sits on (not of itself, which reads back), and the
    // accent for its text.
    Binding {
        target: button.Material
        property: "background"
        when: button.kind === "secondary"
        value: Theme.surface(button.parent ? button.parent.Material.backgroundColor
                                           : "white", 4)
    }
    Binding {
        target: button.Material
        property: "foreground"
        when: button.kind === "secondary"
        value: Theme.accent
    }
}

// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import GpgFrontend.Mobile

// A page's floating action, at the bottom right as the platform has it. Two
// kinds, never confused:
//  - "add" (the default): adding something to the page -- files, keys,
//    members -- a rounded square filled with the accent and a plus, the same
//    on every page;
//  - "operation": the PGP operations, an extended button with the desktop's
//    lock icon and its name on a tonal surface, so it reads as something
//    other than adding.
// It anchors itself to its parent's foot at the right; offsetX moves it left
// to make room for another one beside it.
RoundButton {
    id: fab

    property string variant: "add"
    property real offsetX: 0
    property bool held: false

    readonly property bool extended: variant === "operation"

    anchors.right: parent ? parent.right : undefined
    anchors.bottom: parent ? parent.bottom : undefined
    anchors.rightMargin: Theme.fabMargin + offsetX
    anchors.bottomMargin: Theme.fabMargin
    implicitWidth: extended ? Math.max(Theme.fabSize, implicitContentWidth + 44)
                            : Theme.fabSize
    implicitHeight: Theme.fabSize
    radius: Theme.radiusLarge
    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0
    leftPadding: extended ? 20 : 0
    rightPadding: extended ? 24 : 0
    spacing: 10
    highlighted: !extended
    display: extended ? AbstractButton.TextBesideIcon : AbstractButton.IconOnly
    font.pixelSize: Theme.bodySize
    font.weight: Font.Medium
    icon.width: Theme.fabIcon
    icon.height: Theme.fabIcon
    // The desktop's coloured operation icon stays as it is.
    icon.color: extended ? "transparent" : Material.primaryHighlightedTextColor
    // A tint of what it floats over, not of itself (which would read its
    // own colour back).
    Material.background: extended
                         ? Theme.surface(parent ? parent.Material.backgroundColor
                                                : Material.backgroundColor, 5)
                         : Material.accentColor
    // The extended one's name in the accent; the filled one's is the
    // style's own text on the accent.
    Binding {
        target: fab.Material
        property: "foreground"
        when: fab.extended
        value: Theme.accent
    }
    Material.elevation: 4
    Accessible.name: text

    onPressAndHold: held = true
    onReleased: held = false
    onCanceled: held = false

    // An icon-only button says its name while held; the extended one shows
    // it already.
    ToolTip.visible: held && !extended
    ToolTip.text: text
}

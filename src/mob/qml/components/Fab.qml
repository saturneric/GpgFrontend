// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Effects
import GpgFrontend.Mobile

// A page's floating action, at the bottom right as the platform has it: a
// round button filled with the accent and a line icon (a plus to add, the
// lock for the PGP operations), the same on every page; its name said while
// held and read out. It anchors itself to its parent's foot at the right;
// offsetX moves it left to make room for another one beside it.
RoundButton {
    id: fab

    property real offsetX: 0
    property bool held: false

    anchors.right: parent ? parent.right : undefined
    anchors.bottom: parent ? parent.bottom : undefined
    anchors.rightMargin: Theme.fabMargin + offsetX
    anchors.bottomMargin: Theme.fabMargin
    implicitWidth: Theme.fabSize
    implicitHeight: Theme.fabSize
    radius: Theme.fabSize / 2
    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0
    padding: 0
    highlighted: true
    display: AbstractButton.IconOnly
    icon.width: Theme.fabIcon
    icon.height: Theme.fabIcon
    icon.color: Material.primaryHighlightedTextColor
    Material.background: Material.accentColor
    // Floating over the page: one soft shadow under it, deeper while
    // pressed (the style's ripple, focus and press stay the button's own).
    // Not the style's elevation: that shadow all but vanishes on a dark
    // page, and the two together would double it. Only here: buttons in
    // sheets stay flat.
    Material.elevation: 0
    // The style still draws its background through a layer with an
    // elevation effect at elevation 0: a faint second shadow under this one,
    // and an offscreen pass for every frame it is in.
    background.layer.enabled: false
    Accessible.name: text

    RectangularShadow {
        anchors.fill: parent
        z: -2
        radius: fab.radius
        offset.y: fab.down ? Theme.fabShadowOffsetPressed : Theme.fabShadowOffset
        blur: fab.down ? Theme.fabShadowBlurPressed : Theme.fabShadowBlur
        color: Theme.fabShadow
        visible: fab.enabled && fab.visible

        Behavior on offset.y {
            NumberAnimation { duration: 120 }
        }
    }

    onPressAndHold: held = true
    onReleased: held = false
    onCanceled: held = false

    // Its name while held: it shows only its icon.
    ToolTip.visible: held
    ToolTip.text: text
}
